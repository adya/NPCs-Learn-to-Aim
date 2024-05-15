#include "Hooks.h"
#include "CombatProjectileAimController.h"
#include "Options.h"
#include <algorithm>
#include <numbers>

namespace NLA
{
	std::default_random_engine rnd{};

	struct SkillUsage
	{
		RE::ActorValue skill;
		float          multiplier;

		SkillUsage() :
			skill(RE::ActorValue::kNone), multiplier(0) {}

		SkillUsage(RE::ActorValue a_skill, float a_multiplier) :
			skill(a_skill), multiplier(a_multiplier) {}

		SkillUsage(const RE::TESObjectWEAP* weapon) :
			SkillUsage() {
			if (!weapon)
				return;
			if (weapon->GetWeaponType() == RE::WEAPON_TYPE::kCrossbow) {
				if (Options::General::crossbowAiming) {
					skill = RE::ActorValue::kArchery;
					multiplier = Options::Skills::crossbowSkillMultiplier;
				}
			} else if (Options::General::bowAiming) {
				skill = RE::ActorValue::kArchery;
				multiplier = 1;
			}
		}

		SkillUsage(const RE::MagicItem* spell) :
			SkillUsage() {
			if (!spell)
				return;
			RE::ActorValue effectSkill = RE::ActorValue::kNone;
			if (auto effect = spell->GetCostliestEffectItem()) {
				if (auto base = effect->baseEffect) {
					effectSkill = base->GetMagickSkill();
				}
			}
			if (spell->GetSpellType() == RE::MagicSystem::SpellType::kStaffEnchantment) {
				if (Options::General::staffAiming) {
					skill = Options::Skills::staffsUseEnchantingSkill ? RE::ActorValue::kEnchanting : effectSkill;
					multiplier = Options::Skills::staffSkillMultiplier;
				}
			} else if (Options::General::spellAiming) {
				skill = effectSkill;
				multiplier = 1;
			}
		}

		operator bool() const {
			return skill != RE::ActorValue::kNone;
		}

		float OriginalSkillFactor(RE::Actor* actor) const {
			if (!actor || !this)
				return 1.0f;
			const auto skill = actor->GetActorValue(this->skill) * this->multiplier;
			return (std::min)((std::max)((100.0f - skill) * 0.01f, 0.0f), 1.0f);
		}

		float SkillFactor(RE::Actor* actor) const {
			if (!actor)
				return 1.0f;
			const auto skill = actor->GetActorValue(this->skill) * this->multiplier;
			return std::exp(0.01 * (50 - skill));  // 50 here is the median at which aiming uses exact values of related settings.
		}
	};

	void SetAngles(RE::Projectile::LaunchData& launchData, const SkillUsage& skillUsage) {
		if (!launchData.shooter)
			return;

		constexpr float toRadians = std::numbers::pi / 180.0f;
		float           skillFactor = skillUsage.SkillFactor(launchData.shooter->As<RE::Actor>());

		// TODO: Consider using normal distribution for a more consistent decrease of spread with increasing skill.
		std::uniform_real_distribution<float> spreadRND(0, skillFactor * Settings::fBowNPCSpreadAngle() * toRadians);  // to radians
		std::uniform_real_distribution<float> angleRND(0, 2 * std::numbers::pi);

		float spread = spreadRND(rnd);
		float angle = angleRND(rnd);

#ifndef NDEBUG
		auto originalAngleX = launchData.angleX;
		auto originalAngleZ = launchData.angleZ;
#endif
		launchData.angleX += std::sin(angle) * spread;
		launchData.angleZ += std::cos(angle) * spread;
#ifndef NDEBUG
		logger::info("Skill factor: {:.4f}", skillFactor);
		logger::info("Spread: {:.4f}", spread);
		logger::info("AngleX: {:.4f} ({:.4f})", launchData.angleX, originalAngleX);
		logger::info("AngleZ: {:.4f} ({:.4f})", launchData.angleZ, originalAngleZ);
#endif
	}

	namespace Hooks
	{
		// The full mod should be a combination of both hooks: weapon/spell fire and aiming in CombatProjectileAimController.
		// While the former provides a random deviation of projectile angle, the latter still randomizes the aim point:
		// * Controller determines general ability of NPC to aim at a specific point accurately.
		// * Firing function determines NPCs ability to actually make the projectile go in the desired direction.
		struct CalculateAim
		{
			static void thunk(RE::CombatProjectileAimController* controller, RE::Actor* target) {
#ifndef NDEBUG
				logger::info("Calculating NPC's Aim...");
#endif
				RE::Actor* attacker = controller->combatController->attackerHandle.get().get();
				if (!attacker) {  // without an attacker we can't know the skill level.
					func(controller, target);
					return;
				}

#ifndef NDEBUG
				logger::info("\t{} is shooting at {}", *attacker, *target);
#endif
				if (!Options::NPC::ShouldLearn(attacker)) {
#ifndef NDEBUG
					logger::info("\t{} is not an NPC that can learn", *attacker);
#endif
					func(controller, target);
					return;
				}

				SkillUsage skillUsage{};

				if (controller->projectile->IsArrow()) {  // This also works for bolts.
					if (const auto weapon = reinterpret_cast<RE::TESObjectWEAP*>(controller->mcaster)) {
						skillUsage = SkillUsage(weapon);
					}
				} else {
					if (const auto caster = controller->mcaster) {  // This also works for staffs.
						skillUsage = SkillUsage(caster->currentSpell);
					}
				}

				// Often when NPC is a magic caster the currentSpell only gets set when NPC starts charging/casting it,
				// in such cases we can't determine the skill to use and should fallback to default logic.
				// Frankly, calls to aim in such cases are meaningless anyways, so we're not missing much.
				if (skillUsage) {
					float aimVariance = Settings::fCombatRangedAimVariance();
					controller->aimVariance = aimVariance * skillUsage.SkillFactor(attacker);
#ifndef NDEBUG
					float aimOffset = Settings::fCombatAimProjectileRandomOffset();
					logger::info("\tfCombatAimProjectileRandomOffset: {:.4f}", aimOffset);
					logger::info("\tfCombatRangedAimVariance: {:.4f}", aimVariance);
					logger::info("\t{} Skill of {}: {:.4f}", skillUsage.skill, *attacker, attacker->GetActorValue(skillUsage.skill));
					logger::info("\taimOffset: {}", controller->aimOffset);
#endif
				}

				func(controller, target);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		/// Disable original random offset for arrows.
		/// This allows replacing randomization logic with our own.
		/// This also means that if a particular aiming is disabled, NPCs will always shoot right where they aim at.
		struct WeapFireAmmoRangomizeArrowDirection
		{
			static float thunk(float min, float max) {
				return 0;
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct LaunchSpellProjectile
		{
			static void thunk(RE::ProjectileHandle projectile, RE::Projectile::LaunchData& launchData) {
#ifndef NDEBUG
				Options::Load(true);
#endif
				if (auto skillUsage = SkillUsage(launchData.spell); skillUsage) {
					SetAngles(launchData, skillUsage);
				}
				func(projectile, launchData);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct WeaponFireProjectile
		{
			static void thunk(RE::ProjectileHandle projectile, RE::Projectile::LaunchData& launchData) {
#ifndef NDEBUG
				Options::Load(true);
#endif
				if (auto skillUsage = SkillUsage(launchData.weaponSource); skillUsage) {
					SetAngles(launchData, skillUsage);
				}
				func(projectile, launchData);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		void Install() {
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
		}
	}

	void Install() {
		logger::info("{:*^40}", "HOOKS");
		Hooks::Install();
	}
}
