#include "Hooks.h"
#include "CombatProjectileAimController.h"
#include "Options.h"
#include <algorithm>
#include <numbers>

namespace NLA
{
	std::default_random_engine rnd{};

	enum WeaponType : std::uint8_t
	{
		kNone = 0,
		kBow = 1,
		kCrossbow = 2,
		kSpell = 3,
		kStaff = 4
	};

	struct SkillUsage
	{
		RE::ActorValue skill;
		float          multiplier;
		WeaponType     weaponType;

		SkillUsage() :
			skill(RE::ActorValue::kNone), multiplier(0), weaponType(kNone) {}

		SkillUsage(RE::ActorValue a_skill, float a_multiplier, WeaponType weaponType) :
			skill(a_skill), multiplier(a_multiplier), weaponType(weaponType) {}

		SkillUsage(const RE::TESObjectWEAP* weapon, RE::Actor* attacker, SkillUsageContext context) :
			SkillUsage() {
			if (!weapon)
				return;
			if (!Options::NPC.ShouldLearn(attacker))
				return;

			if (weapon->GetWeaponType() == RE::WEAPON_TYPE::kCrossbow) {
				weaponType = kCrossbow;
				if (Options::For(attacker).crossbowAiming) {
					skill = RE::ActorValue::kArchery;
					multiplier = Options::For(attacker).MultiplierFor(context).crossbow;
				}
			} else if (weapon->GetWeaponType() == RE::WEAPON_TYPE::kBow) {
				weaponType = kBow;
				if (Options::For(attacker).bowAiming) {
					skill = RE::ActorValue::kArchery;
					multiplier = Options::For(attacker).MultiplierFor(context).bow;
				}
			}

			if (attacker->GetActorValue(RE::ActorValue::kBlindness) > 0) {
				multiplier -= (1 - Options::For(attacker).MultiplierFor(context).blindness);
			}
		}

		SkillUsage(const RE::MagicItem* spell, RE::Actor* attacker, SkillUsageContext context) :
			SkillUsage() {
			if (!spell)
				return;
			if (!Options::NPC.ShouldLearn(attacker))
				return;

			RE::ActorValue magicSkill = RE::ActorValue::kNone;

			if (attacker && Options::For(attacker).spellsUseHighestMagicSkill) {
				std::array<RE::ActorValue, 5> properties = { RE::ActorValue::kAlteration, RE::ActorValue::kConjuration, RE::ActorValue::kDestruction, RE::ActorValue::kIllusion, RE::ActorValue::kRestoration };

				auto highestMagicSkill = std::ranges::max_element(properties, [&attacker](RE::ActorValue a, RE::ActorValue b) {
					return attacker->GetActorValue(a) < attacker->GetActorValue(b);
				});
				magicSkill = *highestMagicSkill;
			}

			if (magicSkill == RE::ActorValue::kNone) {
				if (auto effect = spell->GetCostliestEffectItem()) {
					if (auto base = effect->baseEffect) {
						magicSkill = base->GetMagickSkill();
					}
				}
			}

			if (!Options::For(attacker).concetrationSpellsRequireContinuousAim && spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration) {
				return;
			}

			switch (spell->GetSpellType()) {
			case RE::MagicSystem::SpellType::kSpell:
			case RE::MagicSystem::SpellType::kLeveledSpell:
				weaponType = kSpell;
				if (Options::For(attacker).spellAiming) {
					skill = magicSkill;
					multiplier = Options::For(attacker).MultiplierFor(context).spell;
				}
				break;
			case RE::MagicSystem::SpellType::kStaffEnchantment:
				weaponType = kStaff;
				if (Options::For(attacker).staffAiming) {
					skill = Options::For(attacker).stavesUseEnchantingSkill ? RE::ActorValue::kEnchanting : magicSkill;
					multiplier = Options::For(attacker).MultiplierFor(context).staff;
				}
				break;
			default:
				break;
			}

			if (attacker->GetActorValue(RE::ActorValue::kBlindness) > 0) {
				multiplier -= (1 - Options::For(attacker).MultiplierFor(context).blindness);
			}
		}

		// When picking a skill failed either due to combination of options or invalid input,
		// we return false to indicate that there is no SkillUsage involved in the shot :)
		operator bool() const {
			return skill != RE::ActorValue::kNone;
		}
#ifndef NDEBUG
		float OriginalSkillFactor(RE::Actor* actor) const {
			if (!actor || this->skill == RE::ActorValue::kNone)
				return 1.0f;
			const auto skill = EffectiveSkill(actor);
			return (std::min)((std::max)((100.0f - skill) * 0.01f, 0.0f), 1.0f);
		}
#endif
		float SkillFactor(RE::Actor* actor) const {
			if (!actor || this->skill == RE::ActorValue::kNone)
				return 1.0f;
			const auto skill = EffectiveSkill(actor);
			return std::exp(0.025f * (10 - skill));
		}

		inline float Skill(RE::Actor* actor) const {
			if (!actor || skill == RE::ActorValue::kNone)
				return 0.0f;
			return actor->GetActorValue(skill);
		}

		inline float EffectiveSkill(RE::Actor* actor) const {
			return Skill(actor) * multiplier;
		}
	};

	void AddRandomSpread(float* angleX, float* angleZ, RE::Actor* attacker, const SkillUsage& skillUsage) {
		if (!attacker)
			return;

		constexpr float toRadians = std::numbers::pi / 180.0f;

		
		float skillFactor = skillUsage.SkillFactor(attacker);
		float maxSpread = skillFactor * Settings::fBowNPCSpreadAngle() * toRadians;
#ifndef NDEBUG
		float originalSkillFactor = skillUsage.OriginalSkillFactor(attacker);
#endif

		std::uniform_real_distribution<float> angleRND(0, 2 * std::numbers::pi);
		std::normal_distribution<float> spreadRND(0, 0.7f * maxSpread);

		float spread = std::abs(spreadRND(rnd));
		float angle = angleRND(rnd);  // pick a random direction on a circle.

#ifndef NDEBUG
		auto originalAngleX = *angleX;
		auto originalAngleZ = *angleZ;
#endif
		*angleX += std::sin(angle) * spread;  // add random deviation to the launch angles.
		*angleZ += std::cos(angle) * spread;
#ifndef NDEBUG
		logger::info("");
		logger::info("Shooting:");
		logger::info("\tSkill: {} (base: {})", skillUsage.EffectiveSkill(attacker), skillUsage.Skill(attacker));
		logger::info("\tSkill factor: {:.4f}", skillFactor);
		logger::info("\tSpread: {:.4f} (max: {:.4f}, vanilla max: {:.4f})", spread, maxSpread, originalSkillFactor * Settings::fBowNPCSpreadAngle() * toRadians);
		logger::info("\tAngleX: {:.4f} ({:.4f})", *angleX, originalAngleX);
		logger::info("\tAngleZ: {:.4f} ({:.4f})", *angleZ, originalAngleZ);
#endif
	}

	void AddRandomSpread(RE::Projectile::LaunchData& launchData, const SkillUsage& skillUsage) {
		if (!launchData.shooter)
			return;

		auto attacker = launchData.shooter ? launchData.shooter->As<RE::Actor>() : nullptr;
		AddRandomSpread(&launchData.angleX, &launchData.angleZ, attacker, skillUsage);
	}

	namespace Hooks
	{
		namespace Aim
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
					if (!Options::NPC.ShouldLearn(attacker)) {
#ifndef NDEBUG
						logger::info("\t{} is not an NPC that can learn", *attacker);
#endif
						func(controller, target);
						return;
					}

					SkillUsage skillUsage{};

					if (controller->projectile->IsArrow()) {  // This also works for bolts.
						if (const auto weapon = reinterpret_cast<RE::TESObjectWEAP*>(controller->mcaster)) {
							skillUsage = SkillUsage(weapon, attacker, kAim);
						}
					} else if (const auto caster = controller->mcaster) {  // This also works for staffs.
						skillUsage = SkillUsage(caster->currentSpell, attacker, kAim);
					}

					// Often when NPC is a magic caster the currentSpell only gets set when NPC starts charging/casting it,
					// in such cases we can't determine the skill to use and should fallback to default logic.
					// Frankly, calls to aim in such cases are meaningless anyways, so we're not missing much.
					if (skillUsage) {
						float aimVariance = Settings::fCombatRangedAimVariance();
						controller->aimVariance = 2.0f * aimVariance * skillUsage.SkillFactor(attacker);
#ifndef NDEBUG
						float aimOffset = Settings::fCombatAimProjectileRandomOffset();
						logger::info("Aiming:");
						logger::info("\tSkill: {} (base: {})", skillUsage.EffectiveSkill(attacker), skillUsage.Skill(attacker));
						logger::info("\tVariance: {:.4f} (base: {:.4f})", controller->aimVariance, aimVariance);
						logger::info("\taimOffset: {} (base: {:.4f}, max: {:.4f})", controller->aimOffset, aimOffset, aimOffset * controller->aimVariance);
#endif
					}
					func(controller, target);
				}

				static inline REL::Relocation<decltype(thunk)> func;
			};
		}

		namespace Release
		{
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
				static void thunk(RE::ProjectileHandle& projectile, RE::Projectile::LaunchData& launchData) {
#ifndef NDEBUG
					Options::Load();
#endif
					auto attacker = launchData.shooter ? launchData.shooter->As<RE::Actor>() : nullptr;
					if (auto skillUsage = SkillUsage(launchData.spell, attacker, kRelease); skillUsage) {
						AddRandomSpread(launchData, skillUsage);
					}
					func(projectile, launchData);
				}

				static inline REL::Relocation<decltype(thunk)> func;
			};

			struct WeaponFireProjectile
			{
				static void thunk(RE::ProjectileHandle& projectile, RE::Projectile::LaunchData& launchData) {
#ifndef NDEBUG
					Options::Load();
#endif
					auto attacker = launchData.shooter ? launchData.shooter->As<RE::Actor>() : nullptr;
					if (auto skillUsage = SkillUsage(launchData.weaponSource, attacker, kRelease); skillUsage) {
						AddRandomSpread(launchData, skillUsage);
					}
					func(projectile, launchData);
				}

				static inline REL::Relocation<decltype(thunk)> func;
			};

			struct PlayerAutoAim
			{
				static void thunk(RE::PlayerCharacter* player, RE::Projectile* projectile, RE::NiNode* fireNode, float* angleZ, float* angleX, RE::NiPoint3* defaultOrigin, float tiltZ, float tiltX) {
					func(player, projectile, fireNode, angleZ, angleX, defaultOrigin, tiltZ, tiltX);

					SkillUsage skillUsage{};
					if (auto spell = projectile->spell) {
						skillUsage = SkillUsage(spell, player, kRelease);
					} else if (auto weapon = projectile->weaponSource) {
						skillUsage = SkillUsage(weapon, player, kRelease);
					}

					if (skillUsage) {
						// we need to preserve calculated tilt angles and only apply random spread to the launch angles.
						*angleZ -= tiltZ;
						*angleX -= tiltX;
						AddRandomSpread(angleX, angleZ, player, skillUsage);
						*angleZ += tiltZ;
						*angleX += tiltX;
					}
				}

				static inline REL::Relocation<decltype(thunk)> func;
			};
		}

		/* Speed is out of scope for now.
		namespace Draw
		{
			struct DrawWeaponAnimationChannel_GetWeaponSpeed
			{
				static float thunk(RE::ActorValueOwner* owner, RE::TESObjectWEAP* weapon, bool leftHand) {
					float speed = func(owner, weapon, leftHand);
					logger::info("ACTOR: Draw speed mult: {:.4f}", speed);
					return speed;
				}

				static inline REL::Relocation<decltype(thunk)> func;
			};

			struct ActorMagicCaster_Update
			{
				static void thunk(RE::ActorMagicCaster* caster, float remainingTime) {
					func(caster, remainingTime);
					logger::info("ACTOR: Magic caster update: {:.4f}", remainingTime);
				}

				static inline REL::Relocation<decltype(thunk)> func;

				static inline constexpr std::size_t index{ 0 };
				static inline constexpr std::size_t size{ 0x1D };
			};
		}*/

		void Install() {
			const REL::Relocation<std::uintptr_t> combatControllerUpdate{ RELOCATION_ID(43162, 44384) };
			const REL::Relocation<std::uintptr_t> weaponFire{ RELOCATION_ID(17693, 18102) };
			const REL::Relocation<std::uintptr_t> launchSpell{ RELOCATION_ID(33672, 34452) };
			const REL::Relocation<std::uintptr_t> autoAim{ RELOCATION_ID(43009, 44200) };
			
			/* Speed is out of scope for now.
			const REL::Relocation<std::uintptr_t> weaponanimchannel{ RELOCATION_ID(0, 42779) };
			stl::write_vfunc<RE::ActorMagicCaster, Draw::ActorMagicCaster_Update>();
			stl::write_thunk_call<Draw::DrawWeaponAnimationChannel_GetWeaponSpeed>(weaponanimchannel.address() + OFFSET(0, 0x29));*/

			stl::write_thunk_call<Aim::CalculateAim>(combatControllerUpdate.address() + OFFSET(0x103, 0x97));
			logger::info("Installed Aiming logic");

			stl::write_thunk_call<Release::PlayerAutoAim>(autoAim.address() + OFFSET(0x201, 0x201));
			logger::info("Installed Player Auto-Aiming logic");

			stl::write_thunk_call<Release::WeapFireAmmoRangomizeArrowDirection>(weaponFire.address() + OFFSET(0xCB0, 0xCD5));
			logger::info("Disabled default arrow deviation logic");

			stl::write_thunk_call<Release::WeaponFireProjectile>(weaponFire.address() + OFFSET(0xE82, 0xE60));
			logger::info("Installed modded arrow deviation logic");

			stl::write_thunk_call<Release::LaunchSpellProjectile>(launchSpell.address() + OFFSET(0x377, 0x354));
			logger::info("Installed spells deviation logic");
		}
	}

	void Install() {
		logger::info("{:*^40}", "HOOKS");
		Hooks::Install();
	}
}
