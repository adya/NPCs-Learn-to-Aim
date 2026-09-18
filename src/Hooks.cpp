#include "Hooks.h"
#include "Options.h"
#include "RE/C/CombatProjectileAimController.h"
#include "Hooking.h"
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
			if (!weapon || !attacker || !Options::NPC.ShouldLearn(attacker))
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

			if (attacker->AsActorValueOwner()->GetActorValue(RE::ActorValue::kBlindness) > 0) {
				multiplier -= (1 - Options::For(attacker).MultiplierFor(context).blindness);
			}
		}

		SkillUsage(const RE::MagicItem* spell, RE::Actor* attacker, SkillUsageContext context) :
			SkillUsage() {
			if (!spell || !attacker || !Options::NPC.ShouldLearn(attacker))
				return;

			// Only support aimed spells for now.
			if (spell->GetDelivery() != RE::MagicSystem::Delivery::kAimed)
				return;

			RE::ActorValue magicSkill = RE::ActorValue::kNone;

			if (attacker && Options::For(attacker).spellsUseHighestMagicSkill) {
				std::array<RE::ActorValue, 5> properties = { RE::ActorValue::kAlteration, RE::ActorValue::kConjuration, RE::ActorValue::kDestruction, RE::ActorValue::kIllusion, RE::ActorValue::kRestoration };

				auto highestMagicSkill = std::ranges::max_element(properties, [&attacker](RE::ActorValue a, RE::ActorValue b) {
					return attacker->AsActorValueOwner()->GetActorValue(a) < attacker->AsActorValueOwner()->GetActorValue(b);
				});
				magicSkill = *highestMagicSkill;
			}

			if (magicSkill == RE::ActorValue::kNone) {
				if (auto effect = spell->GetCostliestEffectItem(); effect) {
					if (auto base = effect->baseEffect; base) {
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

			if (attacker->AsActorValueOwner()->GetActorValue(RE::ActorValue::kBlindness) > 0) {
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
			return actor->AsActorValueOwner()->GetActorValue(skill);
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
		std::normal_distribution<float>       spreadRND(0, 0.7f * maxSpread);

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
		namespace Accuracy
		{
			// The full mod should be a combination of both hooks: weapon/spell fire and aiming in CombatProjectileAimController.
			// While the former provides a random deviation of projectile angle, the latter still randomizes the aim point:
			// * Controller determines general ability of NPC to aim at a specific point accurately.
			// * Firing function determines NPCs ability to actually make the projectile go in the desired direction.
			struct CombatProjectileAimController_Update
			{
				static inline constexpr REL::RelocationID  relocation{ 43162, 44384 };
				static inline constexpr REL::VariantOffset offset{ 0x103, 0x97, 0x103 };
				static inline constexpr Signature           signature{ "E8 ?? ?? ?? ?? 48 8B CB E8 ?? ?? ?? ?? 84 C0 74" };

				static inline void post_hook() { logger::info("Installed Aiming logic"); }

				static void thunk(RE::CombatProjectileAimController* controller, RE::Actor* target) {
#ifndef NDEBUG
					logger::info("Calculating NPC's Aim...");
#endif
					if (!controller || !controller->combat_control) {
						func(controller, target);
						return;
					}

					RE::Actor* attacker = controller->combat_control->attackerHandle.get().get();
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

					// This works for bows, crossbows and... guns :)
					if (const auto weapon = reinterpret_cast<RE::TESObjectWEAP*>(controller->mcaster); weapon && weapon->formType == RE::FormType::Weapon) {
						skillUsage = SkillUsage(weapon, attacker, kAim);
					} else if (const auto caster = controller->mcaster; caster) {  // This also works for staves.
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

		namespace Precision
		{
			/// Disable original random offset for arrows.
			/// This allows replacing randomization logic with our own.
			/// This also means that if a particular aiming is disabled, NPCs will always shoot right where they aim at.
			struct WeapFireAmmoRangomizeArrowDirection
			{
				static inline constexpr REL::RelocationID  relocation{ 17693, 18102 };
				static inline constexpr REL::VariantOffset offset{ 0xCB0, 0xCD5, 0xFFF };
				static inline constexpr VariantSignature<"E8 ?? ?? ?? ?? F3 0F 59 3D ?? ?? ?? ?? F3 0F 59", "E8 ?? ?? ?? ?? 0F 28 F0 41 0F 28 CF 41 0F 28 C2", "E8 ?? ?? ?? ?? F3 0F 59 3D ?? ?? ?? ?? F3 0F 59"> signature;

				static inline void post_hook() { logger::info("Disabled default arrow deviation logic"); }

				static float thunk(float min, float max) {
					return 0;
				}

				static inline REL::Relocation<decltype(thunk)> func;
			};

			struct LaunchSpellProjectile
			{
				static inline constexpr REL::RelocationID  relocation{ 33672, 34452 };
				static inline constexpr REL::VariantOffset offset{ 0x377, 0x354, 0x377 };
				static inline constexpr Signature           signature{ "E8 ?? ?? ?? ?? 48 C7 44 24 ?? 00 00 00 00 48 8D" };

				static inline void post_hook() { logger::info("Installed spells deviation logic"); }

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
				static inline constexpr REL::RelocationID  relocation{ 17693, 18102 };
				static inline constexpr REL::VariantOffset offset{ 0xE82, 0xE60, 0x11ED };
				static inline constexpr VariantSignature<"E8 ?? ?? ?? ?? 48 8D 55 ?? 48 8D 4D ?? E8", "E8 ?? ?? ?? ?? 8B 75 88 85 F6 0F 84", "E8 ?? ?? ?? ?? 48 8D 55 ?? 48 8D 4D ?? E8"> signature;

				static inline void post_hook() { logger::info("Installed modded arrow deviation logic"); }

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
				static inline constexpr REL::RelocationID  relocation{ 43009, 44200 };
				static inline constexpr REL::VariantOffset offset{ 0x201, 0x201, 0x201 };
				static inline constexpr Signature           signature{ "E8 ?? ?? ?? ?? 8B 87 ?? ?? ?? ?? C1 E8 11 A8 01" };

				static inline void post_hook() { logger::info("Installed Player 3rd person projectiles deviation logic"); }

				static void thunk(RE::PlayerCharacter* player, RE::Projectile* projectile, RE::NiNode* fireNode, float* angleZ, float* angleX, RE::NiPoint3* defaultOrigin, float tiltZ, float tiltX) {
					func(player, projectile, fireNode, angleZ, angleX, defaultOrigin, tiltZ, tiltX);

					if (!projectile)
						return;

					SkillUsage skillUsage{};
					if (auto spell = projectile->GetProjectileRuntimeData().spell; spell) {
						skillUsage = SkillUsage(spell, player, kRelease);
					} else if (auto weapon = projectile->GetProjectileRuntimeData().weaponSource; weapon) {
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

		void Install() {
			stl::install_hook<Accuracy::CalculateAim>();
			stl::install_hook<Precision::PlayerAutoAim>();
			stl::install_hook<Precision::WeapFireAmmoRangomizeArrowDirection>();
			stl::install_hook<Precision::WeaponFireProjectile>();
			stl::install_hook<Precision::LaunchSpellProjectile>();
		}
	}

	void Install() {
		logger::info("{:*^40}", "HOOKS");
		Hooks::Install();
	}
}
