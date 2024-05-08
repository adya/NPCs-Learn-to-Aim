#include "Hooks.h"
#include "CombatProjectileAimController.h"
#include <numbers>
#include <algorithm>

namespace NLA
{
	static std::default_random_engine rnd;

	namespace Calculations
	{
		// TODO: Add option to customize these values
		static const float maxDistance = 3072;   // I didn't find the actual setting that controls at which max range NPCs can shoot. But this is roughly twice the distance that I observed in-game :)
		static const float maxTargetSize = 500;  // For reference: dragon ~ 700-1000 (depending on whether they spread their wings); giant ~ 180; human ~ 90; rabbit - 30

		// C = sqrt(w/d^2)/2
		float ShotComplexity(float distance, float targetSize) {
			return std::sqrt(targetSize) / (2 * targetSize);
		}

		// c = 0.8/log(skill); skill >= 10
		float SkillFactor(float skill) {
			return 0.8f / std::log10((std::max)(skill, 10.0f));
		}

		// c2 = 0.5 * e^(2 * sqrt(c)) - 1 + c
		float SkillConsistency(float skillFactor) {
			return 0.5f * std::exp(2 * std::sqrt(skillFactor)) - 1 + skillFactor;
		}

		// normalize distance in range [10;100]
		// d = min + (distance - Dmin) * (max - min) / (Dmax - Dmin)
		float NormalizedDistance(float distance, float maxDistance = Calculations::maxDistance) {
			return 10 + 90 * (std::clamp(distance, 0.0f, maxDistance) / maxDistance);
		}

		// normalize target size in range [10;100]
		// w = min + (size - Smin) * (max - min) / (Smax - Smin)
		float NormalizedTargetSize(float targetSize, float maxSize = maxTargetSize) {
			return 10 + 90 * (std::clamp(targetSize, 0.0f, maxSize) / maxSize);
		}

		// s = 0.5 * v * c
		float Deviation(float skillFactor, float variance) {
			return 0.5 * skillFactor * variance;
		}

		// m = v * c^2 * log(C)
		float Mean(float skillFactor, float shotComplexity, float variance) {
			return variance * std::pow(skillFactor, 2) * std::log10(shotComplexity);
		}

		RE::NiPoint3 RandomOffset(float skill, float distance, float targetSize, float varianceOption) {
			auto skillFactor = SkillFactor(skill);
			auto skillConsistency = SkillConsistency(skillFactor);

			auto variance = (std::max)(varianceOption, 0.1f);  // formula requires variance to be > 0

			auto normalizedDistance = NormalizedDistance(distance, maxDistance);
			auto normalizedTargetSize = NormalizedTargetSize(targetSize, maxTargetSize);
			auto shotComplexity = ShotComplexity(normalizedDistance, normalizedTargetSize);

			auto deviation = Deviation(skillFactor, variance);
			auto mean = Mean(skillFactor, shotComplexity, variance);

			std::uniform_real_distribution  combiner;
			std::normal_distribution<float> distrLeft(-mean, variance);
			std::normal_distribution<float> distrRight(mean, variance);

			return {
				skillConsistency * (combiner(rnd) > 0.5 ? distrLeft(rnd) : distrRight(rnd)),
				skillConsistency * (combiner(rnd) > 0.5 ? distrLeft(rnd) : distrRight(rnd)),
				skillConsistency * (combiner(rnd) > 0.5 ? distrLeft(rnd) : distrRight(rnd))
			};
		}
	};

	namespace Settings
	{
		static float fCombatAimProjectileRandomOffset() {
			auto settings = RE::GameSettingCollection::GetSingleton();
			if (auto setting = settings->GetSetting("fCombatAimProjectileRandomOffset")) {
				return setting->data.f;
			}
			return 16.0f;
		}

		static float fCombatRangedAimVariance() {
			auto settings = RE::GameSettingCollection::GetSingleton();
			if (auto setting = settings->GetSetting("fCombatRangedAimVariance")) {
				return setting->data.f;
			}
			return 0.9f;
		}

		static float fCombatAimProjectileGroundMinRadius() {
			auto settings = RE::GameSettingCollection::GetSingleton();
			if (auto setting = settings->GetSetting("fCombatAimProjectileGroundMinRadius")) {
				return setting->data.f;
			}
			return 128.0f;
		}
	}

	namespace Combat
	{
		RE::NiPoint3* GetTargetPoint(RE::NiPoint3* result, RE::TESObjectREFR* target, std::uint32_t bodyPart) {
			using func_t = decltype(&GetTargetPoint);
			REL::Relocation<func_t> func{ RELOCATION_ID(46021, 47282) };
			return func(result, target, bodyPart);
		}

		struct CalculateAim
		{
			static void PickAnotherBodyPart(RE::CombatProjectileAimController* controller, RE::Actor* target) {
				std::uniform_int_distribution<std::uint32_t> dist(0, 4);
				std::uniform_real_distribution<float>        offsetRND(0, 1);

				// Prevent randomizing when the shot is locked to aim for the ground (this is logic from original func that we're hooking)
				// This is the case, for example, for fireballs, which NPC aim at ground level due to AoE effect.
				if (Settings::fCombatAimProjectileGroundMinRadius() < controller->groundRadius && controller->unkB0 == -(std::numeric_limits<float>::max)()) {
					return;
				}
				std::uint32_t bodyPart = dist(rnd);

				if (bodyPart) {
					RE::NiPoint3 point;
					RE::NiPoint3 targetPoint = controller->targetBodyPartOffset + target->data.location;

					GetTargetPoint(&point, target, bodyPart);
					RE::NiPoint3 offset = point - targetPoint;
					auto         factor = offsetRND(rnd) * Settings::fCombatRangedAimVariance();
					controller->aimOffset.x += offset.x * factor;
					controller->aimOffset.y += offset.y * factor;
					controller->aimOffset.z += offset.z * factor;
				}
			}

			static void thunk(RE::CombatProjectileAimController* controller, RE::Actor* target) {
#ifndef NDEBUG
				logger::info("{:*^50}", "Calculating Projectile Aim");
#endif
				RE::Actor* attacker = controller->combatController->attackerHandle.get().get();
				if (!attacker) { // without an attacker we can't know the skill level.
					func(controller, target);
					return;
				}
					
#ifndef NDEBUG
				logger::info("{} is shooting at {}", attacker->GetDisplayFullName(), target->GetDisplayFullName());
#endif
				// TODO: Add option bForcedAimForAll to bypass this
				if (!attacker->HasKeywordByEditorID("ActorTypeNPC") && !attacker->GetRace()->HasKeywordString("ActorTypeNPC")) {
#ifndef NDEBUG
					logger::info("{} is not an NPC that can learn", attacker->GetDisplayFullName());
#endif
					func(controller, target);
					return;
				}

				RE::ActorValue skillToUse = RE::ActorValue::kNone;

				if (controller->projectile->IsArrow()) { // TODO: Add option bEnableArcheryAim
					skillToUse = RE::ActorValue::kArchery;
				} else { // TODO: Add option bEnableMagicAim
					if (auto caster = controller->mcaster) {
						if (auto spell = caster->currentSpell) {
							if (auto effect = spell->GetCostliestEffectItem()) {
								if (auto base = effect->baseEffect) {
									skillToUse = base->GetMagickSkill();
								}
							}
						}
					}
				}

				// Often when NPC is a magic caster the currentSpell only gets set when NPC starts charging/casting it, 
				// in such cases we can't determine the skill to use and should fallback to default logic.
				// Frankly, calls to aim in such cases are meaningless anyways, so we're not missing much.
				if (skillToUse == RE::ActorValue::kNone) {
					func(controller, target);
					return;
				}

				// Reset aimVariance to disable original randomization logic.
				controller->aimVariance = 0;
				func(controller, target);

				// This emulates the original randomization of changing body parts.
				PickAnotherBodyPart(controller, target);

				float aimOffset = Settings::fCombatAimProjectileRandomOffset();
				float aimVariance = Settings::fCombatRangedAimVariance();

				// Check real distances and dragon size to set sensible maximums.
				float distance = controller->attackerLocation.GetDistance(controller->aimPoint);
				float skill = attacker->GetActorValue(skillToUse);
				float width = 0;

				if (target->IsPlayerRef() && !RE::PlayerCharacter::GetSingleton()->playerFlags.isInThirdPersonMode) {
					width = target->IsSneaking() ? 89.3 : 93.5;  // these are values for default player size in 3rd person mode. Since 1st person always returns 1 in bounds, we manually set these.
				} else if (auto bounds = target->Get3D()) {
					width = bounds->worldBound.radius;
				}

				auto offsetFractions = Calculations::RandomOffset(skill, distance, width, aimVariance);

				controller->aimOffset.x += aimOffset * offsetFractions.x;
				controller->aimOffset.y += aimOffset * offsetFractions.y;
				controller->aimOffset.z += aimOffset * offsetFractions.z;
#ifndef NDEBUG
				logger::info("\tfCombatAimProjectileRandomOffset: {:.4f}", aimOffset);
				logger::info("\tfCombatRangedAimVariance: {:.4f}", aimVariance);
				logger::info("\tWidth of {}: {:.4f} (normalized: {:.4f}", target->GetDisplayFullName(), width, Calculations::NormalizedTargetSize(width));
				logger::info("\tDistance to {}: {:.4f} (normalized: {:.4f})", target->GetDisplayFullName(), distance, Calculations::NormalizedDistance(distance));
				logger::info("\t{} Skill of {}: {:.4f}", skillToUse, attacker->GetDisplayFullName(), attacker->GetActorValue(RE::ActorValue::kArchery));
				logger::info("\taimOffset: {}", controller->aimOffset);
#endif
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		// TODO: This effectively disables usage of fBowNPCSpreadAngle. Perhaps we could use it later.
		/// Disable random offset for arrows.
		struct WeapFireAmmoRangomizeArrowDirection
		{
			static float thunk(float min, float max) {
				return 0;
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		inline void Install() {
			const REL::Relocation<std::uintptr_t> update{ RELOCATION_ID(43162, 44384) };
			const REL::Relocation<std::uintptr_t> weaponFire{ RELOCATION_ID(17693, 18102) };

			stl::write_thunk_call<CalculateAim>(update.address() + OFFSET(0x103, 0x97));

			stl::write_thunk_call<WeapFireAmmoRangomizeArrowDirection>(weaponFire.address() + OFFSET(0xCB0, 0xCD5));

			logger::info("Installed Aiming hooks");
		}
	}

	void Install() {
		logger::info("{:*^30}", "HOOKS");
		Combat::Install();
	}
}
