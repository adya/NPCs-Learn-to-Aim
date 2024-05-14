#include "Hooks.h"
#include "CombatProjectileAimController.h"
#include "Options.h"
#include <algorithm>
#include <numbers>

namespace NLA
{
	std::default_random_engine rnd{};

	// The full mod should be a combination of both hooks: weapon/spell fire and aiming in CombatProjectileAimController.
	// While the former provides a random deviation of projectile angle, the latter still randomizes the aim point:
	// * Controller determines general ability of NPC to aim at a specific point accurately.
	// * Firing function determines NPCs ability to actually make the projectile go in the desired direction.
	struct CalculateAim
	{
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
			if (!Options::NPC::ShouldLearn(attacker)) {
#ifndef NDEBUG
				logger::info("{} is not an NPC that can learn", *attacker);
#endif
				func(controller, target);
				return;
			}

			RE::ActorValue skillToUse = RE::ActorValue::kNone;
			float          skillMultiplier = 1.0f;

			if (controller->projectile->IsArrow()) {  // This also works for bolts.
				if (const auto weapon = reinterpret_cast<RE::TESObjectWEAP*>(controller->mcaster); weapon->GetWeaponType() == RE::WEAPON_TYPE::kCrossbow) {
					if (Options::General::crossbowAiming) {
						skillMultiplier = Options::Skills::crossbowSkillMultiplier;
						skillToUse = RE::ActorValue::kArchery;
					}
				} else if (Options::General::bowAiming) {
					skillToUse = RE::ActorValue::kArchery;
				}
			} else {
				if (auto caster = controller->mcaster) {  // This also works for staffs.
					if (auto spell = caster->currentSpell) {
						RE::ActorValue effectSkill = RE::ActorValue::kNone;
						if (auto effect = spell->GetCostliestEffectItem()) {
							if (auto base = effect->baseEffect) {
								effectSkill = base->GetMagickSkill();
							}
						}
						if (spell->GetSpellType() == RE::MagicSystem::SpellType::kStaffEnchantment) {
							if (Options::General::staffAiming) {
								skillToUse = Options::Skills::staffsUseEnchantingSkill ? RE::ActorValue::kEnchanting : effectSkill;
								skillMultiplier = Options::Skills::crossbowSkillMultiplier;
							}
						} else if (Options::General::spellAiming) {
							skillToUse = effectSkill;
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

			float aimVariance = Settings::fCombatRangedAimVariance();

			float skill = attacker->GetActorValue(skillToUse) * skillMultiplier;

			controller->aimVariance = aimVariance * std::exp(0.02f * (50 - skill));
			func(controller, target);

#ifndef NDEBUG
			float aimOffset = Settings::fCombatAimProjectileRandomOffset();
			logger::info("\tfCombatAimProjectileRandomOffset: {:.4f}", aimOffset);
			logger::info("\tfCombatRangedAimVariance: {:.4f}", aimVariance);
			logger::info("\t{} Skill of {}: {:.4f}", skillToUse, *attacker, skill);
			logger::info("\taimOffset: {}", controller->aimOffset);
#endif
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};

	// TODO: This hook must be preserved to allow replacing randomization logic with our own.
	/// Disable original random offset for arrows.
	struct WeapFireAmmoRangomizeArrowDirection
	{
		static float thunk(float min, float max) {
			return Options::General::bowAiming || Options::General::crossbowAiming ? 0 : func(min, max);
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct LaunchSpellProjectile
	{
		static void thunk(RE::ProjectileHandle projectile, RE::Projectile::LaunchData& launchData) {
			// TODO: Do the thing with spells.
			logger::info("LaunchSpellProjectile");
			func(projectile, launchData);
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct WeaponFireProjectile
	{
		static void thunk(RE::ProjectileHandle projectile, RE::Projectile::LaunchData& launchData) {
			//kBow = 7,
			//kStaff = 8,
			//kCrossbow = 9,
			auto type = launchData.weaponSource->GetWeaponType();

			if (type == RE::WEAPON_TYPE::kBow || type == RE::WEAPON_TYPE::kCrossbow) {
				// TODO: Do logic for arrows similar to original one.
				// set angleX and angleZ
				logger::info("WeaponFireProjectile");

				auto skill = launchData.shooter->As<RE::Actor>()->GetActorValue(RE::ActorValue::kArchery);

				float                                 skillFactor = (std::min)((std::max)((100.0f - skill) * 0.001f, 0.0f), 1.0f);
				std::uniform_real_distribution<float> spreadRND(0, skillFactor * Settings::fBowNPCSpreadAngle());
				std::uniform_real_distribution<float> angleRND(0, 2 * std::numbers::pi);
				float                                 spread = spreadRND(rnd);

				float angle = angleRND(rnd);
				float angleX = std::sin(angle) * spread;
				float angleZ = std::cos(angle) * spread;
			}
			func(projectile, launchData);
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install() {
		logger::info("{:*^30}", "HOOKS");
		const REL::Relocation<std::uintptr_t> update{ RELOCATION_ID(43162, 44384) };
		const REL::Relocation<std::uintptr_t> weaponFire{ RELOCATION_ID(17693, 18102) };
		const REL::Relocation<std::uintptr_t> launchSpell{ RELOCATION_ID(0, 34452) };

		stl::write_thunk_call<CalculateAim>(update.address() + OFFSET(0x103, 0x97));
		logger::info("Installed Aiming hooks");
		
		stl::write_thunk_call<WeapFireAmmoRangomizeArrowDirection>(weaponFire.address() + OFFSET(0xCB0, 0xCD5));
		logger::info("Disabled default arrow deviation logic");
		
		stl::write_thunk_call<WeaponFireProjectile>(weaponFire.address() + OFFSET(0, 0xE60));
		logger::info("Installed modded arrow deviation logic");
		
		stl::write_thunk_call<LaunchSpellProjectile>(launchSpell.address() + OFFSET(0, 0x354));
		logger::info("Installed spells deviation logic");

		logger::info("{:*^30}", "");
	}
}
