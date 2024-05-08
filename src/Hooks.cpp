#include "Hooks.h"
#include "Calculations.h"
#include "CombatProjectileAimController.h"
#include "Options.h"
#include <algorithm>
#include <numbers>

namespace NLA
{
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
				using namespace Calculations;

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
				if (!attacker) {  // without an attacker we can't know the skill level.
					func(controller, target);
					return;
				}

#ifndef NDEBUG
				logger::info("{} is shooting at {}", *attacker, *target);
#endif
				if (attacker->HasKeywordByEditorID("NLA_Ignored")) {
#ifndef NDEBUG
					logger::info("{} is ignored", *attacker);
#endif
					func(controller, target);
					return;
				}

				if (!Options::General::forceAimForAll && !attacker->HasKeywordByEditorID("ActorTypeNPC") && !attacker->GetRace()->HasKeywordString("ActorTypeNPC")) {
#ifndef NDEBUG
					logger::info("{} is not an NPC that can learn", *attacker);
#endif
					func(controller, target);
					return;
				}

				RE::ActorValue skillToUse = RE::ActorValue::kNone;

				if (controller->projectile->IsArrow()) {
					if (Options::General::archeryAiming)
						skillToUse = RE::ActorValue::kArchery;
				} else {
					if (auto caster = controller->mcaster) {
						if (auto spell = caster->currentSpell) {
							if (auto effect = spell->GetCostliestEffectItem()) {
								if (auto base = effect->baseEffect) {
									if (Options::General::magicAiming)
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
				logger::info("\tWidth of {}: {:.4f} (normalized: {:.4f})", *target, width, Calculations::NormalizedTargetSize(width));
				logger::info("\tDistance to {}: {:.4f} (normalized: {:.4f})", *target, distance, Calculations::NormalizedDistance(distance));
				logger::info("\t{} Skill of {}: {:.4f}", skillToUse, *attacker, attacker->GetActorValue(RE::ActorValue::kArchery));
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
				return Options::General::archeryAiming ? 0 : func(min, max);
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
