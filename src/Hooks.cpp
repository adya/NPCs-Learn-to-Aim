#include "Hooks.h"
#include "CombatProjectileAimController.h"
#include "Utils.h"
#include "render/DrawHandler.h"
#include <numbers>

namespace NLA
{
	static std::default_random_engine rnd;

	namespace Calculations
	{

		static const float maxDistance = 3072;   // I didn't find the actual setting that controls at which max range NPCs can shoot. But this is roughly twice the distance that I observed in-game :)
		static const float maxTargetSize = 500;  // For reference: dragon ~ 700-1000 (depending ); giant ~ 180; human ~ 90; rabbit - 30

		// C = sqrt(w/d^2)/2
		float ShotComplexity(float distance, float targetSize) {
			return std::sqrt(targetSize) / (2 * targetSize);
		}

		// c = 1/log(skill); skill >= 10
		float SkillFactor(float skill) {
			return 0.8f / std::log10(std::max(skill, 10.0f));
		}

		// TODO: Figure out the formula that also takes into account shot complexity (so that at closer range even bad archers could show some accuracy.
		// c2 = 0.4 * e^(2*c)
		float SkillConsistency(float skillFactor) {
			return 0.5f * std::exp(2 * std::sqrt(skillFactor)) - 1 + skillFactor;
		}

		// normalize distance in range [10;100]
		// min + (distance - Dmin) * (max - min) / (Dmax - Dmin)
		float NormalizedDistance(float distance, float maxDistance = Calculations::maxDistance) {
			return 10 + 90 * (std::clamp(distance, 0.0f, maxDistance) / maxDistance);
		}

		// normalize distance in range [10;100]
		// min + (size - Smin) * (max - min) / (Smax - Smin)
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

		RE::NiPoint3 RandomOffset(float skill, float distance, float targetSize, float variance) {
			auto skillFactor = SkillFactor(skill);
			auto skillConsistency = SkillConsistency(skillFactor);

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

	namespace UpdateHooks
	{
		struct Nullsub
		{
			static void thunk() {
				func();
				DrawHandler::GetSingleton()->Update(*g_deltaTime);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		inline void Install() {
			const REL::Relocation<uintptr_t> update{ RELOCATION_ID(35565, 36564) };  // 5B2FF0, 5D9F50, main update
			stl::write_thunk_call<Nullsub>(update.address() + OFFSET(0x748, 0xC26));
		}
	}

	namespace Options
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
		struct Color
		{
			using color = glm::vec4;

			static constexpr color red = { 1, 0, 0, 1 };
			static constexpr color blue = { 0, 0, 1, 1 };
			static constexpr color green = { 0, 1, 0, 1 };
			static constexpr color yellow = { 1, 1, 0, 1 };
			static constexpr color purple = { 0.5, 0, 0.5, 1 };
			static constexpr color cyan = { 0, 1, 1, 1 };
			static constexpr color orange = { 1, 0.5, 0, 1 };
			static constexpr color magenta = { 1, 0, 1, 1 };
			static constexpr color lime = { 0.75, 1, 0, 1 };
			static constexpr color teal = { 0, 0.5, 0.5, 1 };

			static constexpr color pale_red = { 1, 0.5, 0.5, 1 };
			static constexpr color pale_blue = { 0.7, 0.7, 1, 1 };
			static constexpr color pale_green = { 0.5, 1, 0.5, 1 };
			static constexpr color pale_yellow = { 1, 1, 0.5, 1 };
			static constexpr color pale_purple = { 0.75, 0.5, 0.75, 1 };
			static constexpr color pale_cyan = { 0.5, 1, 1, 1 };
			static constexpr color pale_orange = { 1, 0.75, 0.5, 1 };
			static constexpr color pale_magenta = { 1, 0.5, 1, 1 };
			static constexpr color pale_lime = { 0.85, 1, 0.5, 1 };
			static constexpr color pale_teal = { 0.5, 0.75, 0.75, 1 };

			// Green reserved for player
			static constexpr std::array<color, 9> colors = { red, blue, /*green,*/ yellow, purple, cyan, orange, magenta, lime };

			static constexpr std::array<color, 9> pale_colors = { pale_red, pale_blue, /*pale_green,*/ pale_yellow, pale_purple, pale_cyan, pale_orange, pale_magenta, pale_lime };

			static color GetRandomColor(bool pale = false) {
				return pale ? pale_colors[std::uniform_int_distribution<std::uint32_t>{ 0, pale_colors.size() - 1 }(rnd)] : colors[std::uniform_int_distribution<std::uint32_t>{ 0, colors.size() - 1 }(rnd)];
			}
			static color GetRandomColor(const RE::TESObjectREFR* obj, bool pale = false) {
				if (obj) {
					if (obj == RE::PlayerCharacter::GetSingleton()) {
						return pale ? Color::pale_green : Color::green;
					}

					return pale ? pale_colors[obj->formID % colors.size()] : colors[obj->formID % colors.size()];
				}

				return pale ? pale_red : red;
			}
		};

		static void Mark(const RE::NiPoint3& point, const Color::color& color) {
			DrawHandler::DrawDebugPoint(point, 0.5, color);
		}
		static void Mark2(const RE::NiPoint3& point, const Color::color& color) {
			DrawHandler::DrawDebugSphere(point, 10, 0.5, color);
		}

		static void Mark(const RE::NiBound& bound, const Color::color& color) {
			DrawHandler::DrawDebugSphere(bound.center, bound.radius, 0.2, color);
		}

		static void Mark(const RE::NiPoint3& from, const RE::NiPoint3& to, const Color::color& color) {
			DrawHandler::DrawDebugLine(from, to, 0.2, color);
		}

		static void Mark(const RE::NiPoint3& point, const RE::TESObjectREFR* owner, bool pale = false) {
			Mark(point, Color::GetRandomColor(owner, pale));
		}

		static void Mark2(const RE::NiPoint3& point, const RE::TESObjectREFR* owner, bool pale = false) {
			Mark2(point, Color::GetRandomColor(owner, pale));
		}

		static void Mark(const RE::NiBound& bound, const RE::TESObjectREFR* owner, bool pale = false) {
			Mark(bound, Color::GetRandomColor(owner, pale));
		}

		static void Mark(const RE::NiPoint3& from, const RE::NiPoint3& to, const RE::TESObjectREFR* owner, bool pale = false) {
			Mark(from, to, Color::GetRandomColor(owner, pale));
		}

		RE::NiPoint3* GetTargetPoint(RE::NiPoint3* result, RE::TESObjectREFR* target, std::uint32_t bodyPart) {
			using func_t = decltype(&GetTargetPoint);
			REL::Relocation<func_t> func{ RELOCATION_ID(0, 47282) };
			return func(result, target, bodyPart);
		}

		struct CalculateAim
		{
			static void PickAnotherBodyPart(RE::CombatProjectileAimController* controller, RE::Actor* target) {
				std::uniform_int_distribution<std::uint32_t> dist(0, 4);
				std::uniform_real_distribution<float>        offsetRND(0, 1);

				// Prevent randomizing when the shot is locked to aim for the ground (this is logic from original func that we're hooking)
				if (Options::fCombatAimProjectileGroundMinRadius() < controller->groundRadius && controller->unkB0 == -std::numeric_limits<float>::max()) {
					return;
				}
				std::uint32_t bodyPart = dist(rnd);

				if (bodyPart) {
					RE::NiPoint3 point;
					RE::NiPoint3 targetPoint = controller->targetBodyPartOffset + target->data.location;

					GetTargetPoint(&point, target, bodyPart);
					RE::NiPoint3 offset = point - targetPoint;
					auto         factor = offsetRND(rnd) * Options::fCombatRangedAimVariance();
					controller->aimOffset.x += offset.x * factor;
					controller->aimOffset.y += offset.y * factor;
					controller->aimOffset.z += offset.z * factor;

					//Mark2(point, Color::red);
				}
			}

			static void thunk(RE::CombatProjectileAimController* controller, RE::Actor* target) {
				DrawHandler::GetSingleton()->OnPostLoad();  // workaround to see debug rendering when coc-ing from main menu
				logger::info("{:*^50}", "Calculating Projectile Aim");

				RE::Actor* attacker = nullptr;
				if (attacker = controller->combatController->attackerHandle.get().get()) {
					logger::info("{} is shooting at {}", attacker->GetName(), target->GetName());
				} else {
					func(controller, target);
					return;
				}

				if (!attacker->HasKeywordByEditorID("ActorTypeNPC") && !attacker->GetRace()->HasKeywordString("ActorTypeNPC")) {
					logger::info("{} is not an NPC that can learn", attacker->GetName());
					func(controller, target);
					return;
				}

				RE::ActorValue skillToUse = RE::ActorValue::kNone;

				if (controller->projectile->IsArrow()) {
					skillToUse = RE::ActorValue::kArchery;
				} else {
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

				if (skillToUse == RE::ActorValue::kNone) {
					func(controller, target);
					return;
				}

				// Reset aimVariance to disable original randomization logic.
				controller->aimVariance = 0;
				func(controller, target);

				// This emulates the original randomization of changing body parts.
				PickAnotherBodyPart(controller, target);

				float aimOffset = Options::fCombatAimProjectileRandomOffset();
				float aimVariance = Options::fCombatRangedAimVariance();

				// Check real distances and dragon size to set sensible maximums.
				float distance = controller->attackerLocation.GetDistance(controller->aimPoint);
				float skill = attacker->GetActorValue(skillToUse);
				float width = 0;

				if (target->IsPlayerRef() && !RE::PlayerCharacter::GetSingleton()->playerFlags.isInThirdPersonMode) {
					width = target->IsSneaking() ? 89.3 : 93.5;  // these are values for default player size in 3rd person mode. Since 1st person always returns 1 in bounds, we manually set these.
				} else if (auto bounds = target->Get3D()) {
					width = bounds->worldBound.radius;
					//Mark(bounds->worldBound, Color::GetRandomColor(target));
				}

				auto offsetFractions = Calculations::RandomOffset(skill, distance, width, aimVariance);

				controller->aimOffset.x += aimOffset * offsetFractions.x;
				controller->aimOffset.y += aimOffset * offsetFractions.y;
				controller->aimOffset.z += aimOffset * offsetFractions.z;

				// This is for debug sampling.
				for (int i = 0; i < 100; i++) {
					auto         offsetFractions = Calculations::RandomOffset(skill, distance, width, aimVariance);
					RE::NiPoint3 offset = {
						aimOffset * offsetFractions.x,
						aimOffset * offsetFractions.y,
						aimOffset * offsetFractions.z
					};
					Mark(controller->actualAimPoint + offset, attacker, true);
				}

				DrawHandler::GetSingleton()->Update(*g_deltaTime);

				Mark2(controller->actualAimPoint + controller->aimOffset, attacker);
				Mark(controller->actualAimPoint + controller->aimOffset, controller->projectileLaunchPoint, attacker, true);
				Mark2(controller->projectileLaunchPoint, attacker, true);

				logger::info("\tfCombatAimProjectileRandomOffset: {:.4f}", aimOffset);
				logger::info("\tfCombatRangedAimVariance: {:.4f}", aimVariance);
				logger::info("\tWidth of {}: {:.4f} (normalized: {:.4f}", target->GetName(), width, Calculations::NormalizedTargetSize(width));
				logger::info("\tDistance to {}: {:.4f} (normalized: {:.4f})", target->GetName(), distance, Calculations::NormalizedDistance(distance));
				logger::info("\t{} Skill of {}: {:.4f}", skillToUse, attacker->GetName(), attacker->GetActorValue(RE::ActorValue::kArchery));
				logger::info("\taimOffset: {}", controller->aimOffset);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		/// Calculate and draw shot trajectory.
		struct WeapFireAmmo
		{
			static RE::ProjectileHandle* thunk(RE::ProjectileHandle* projectile, RE::Projectile::LaunchData& data) {
				const auto pitch = data.angleX;
				const auto yaw = -data.angleZ + std::numbers::pi_v<float> / 2;

				using namespace std;

				const auto xt = cosf(yaw) * cosf(pitch);
				const auto yt = sinf(yaw) * cosf(pitch);
				const auto zt = -sin(pitch);

				const float D = 1000;

				RE::NiPoint3 target = {
					data.origin.x + D * xt,
					data.origin.y + D * yt,
					data.origin.z + D * zt
				};

				/*DrawHandler::GetSingleton()->Update(*g_deltaTime);
				Mark(data.origin, data.shooter);
				Mark(data.origin, target, data.shooter);*/

				logger::info("{} Fired an arrow!", data.shooter->GetName());
				return func(projectile, data);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		/// Disable random offset for arrows.
		struct WeapFireAmmoRangomizeArrowDirection
		{
			static float thunk(float min, float max) {
				return 0;
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		inline void Install() {
			const REL::Relocation<std::uintptr_t> aim{ RELOCATION_ID(0, 44396) };
			const REL::Relocation<std::uintptr_t> update{ RELOCATION_ID(0, 44384) };
			const REL::Relocation<std::uintptr_t> weaponFire{ RELOCATION_ID(0, 18102) };

			stl::write_thunk_call<CalculateAim>(update.address() + OFFSET(0, 0x97));

			stl::write_thunk_call<WeapFireAmmo>(weaponFire.address() + OFFSET(0, 0xE60));

			stl::write_thunk_call<WeapFireAmmoRangomizeArrowDirection>(weaponFire.address() + OFFSET(0, 0xCD5));

			logger::info("Installed PerfectAim hooks");
		}
	}

	void Install() {
		logger::info("{:*^30}", "HOOKS");
		UpdateHooks::Install();
		Combat::Install();
	}

}
