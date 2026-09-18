#include "Aiming.h"
#include "Options.h"
#include "RE/C/CombatProjectileAimController.h"
#include "Hooking.h"
#include "SkillUsage.h"

namespace NLA
{
	namespace
	{
		bool IsBeamProjectile(const RE::BGSProjectile* base) {
			return base && base->data.types.any(RE::BGSProjectileData::Type::kBeam);
		}

		using MCasterSource = std::variant<RE::TESObjectWEAP*, RE::MagicItem*>;

		/// Attempts to retrieve the source of the projectile that is being launched.
		/// There were reports of access violation which bypass the usual nullptr checks, 
		/// so this is an attempt to catch such exception and handle gracefully.
		std::optional<MCasterSource> GetSource(RE::MagicCaster* mcaster) {
			if (!mcaster)
				return std::nullopt;

			__try {
				if (auto* weapon = reinterpret_cast<RE::TESObjectWEAP*>(mcaster); weapon->formType == RE::FormType::Weapon) {
					return weapon;
				}

				return mcaster->currentSpell;
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				logger::warn("Caught an access violation while probing CombatProjectileAimController's caster.");
				return std::nullopt;
			}
		}
	}

	void AddRandomSpread(float* angleX, float* angleZ, RE::Actor* attacker, const SkillUsage& skillUsage) {
		if (!attacker)
			return;

		float skillFactor = skillUsage.SkillFactor(attacker);
		float maxSpread = skillUsage.MaxSpread(attacker);
#ifndef NDEBUG
		float originalSkillFactor = skillUsage.OriginalSkillFactor(attacker);
#endif

		auto offset = RandomSpreadOffset(maxSpread);

#ifndef NDEBUG
		auto originalAngleX = *angleX;
		auto originalAngleZ = *angleZ;
#endif
		*angleX += offset.x;  // add random deviation to the launch angles.
		*angleZ += offset.y;
#ifndef NDEBUG
		constexpr float toRadians = std::numbers::pi / 180.0f;
		logger::info("");
		logger::info("Shooting:");
		logger::info("\tSkill: {} (base: {})", skillUsage.EffectiveSkill(attacker), skillUsage.Skill(attacker));
		logger::info("\tSkill factor: {:.4f}", skillFactor);
		logger::info("\tSpread: {:.4f} (max: {:.4f}, vanilla max: {:.4f})", offset.Length(), maxSpread, originalSkillFactor * Settings::fBowNPCSpreadAngle() * toRadians);
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
			static inline constexpr Signature          signature{ "E8 ?? ?? ?? ?? 48 8B CB E8 ?? ?? ?? ?? 84 C0 74" };

			static inline void post_hook() { logger::info("Installed Accuracy logic"); }

			static void thunk(RE::CombatProjectileAimController* controller, RE::Actor* target) {
#ifndef NDEBUG
				logger::info("Calculating NPC's Accuracy...");
#endif
				if (!controller || !controller->combat_control) {
					func(controller, target);
					return;
				}

				RE::Actor* attacker = controller->combat_control->attackerHandle.get().get();
				if (!attacker) {
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

				// This works for bows, crossbows, staves, spells and... guns :)
				if (auto source = GetSource(controller->mcaster); source) {
					skillUsage = std::visit([&](auto* form) { return SkillUsage(form, attacker, kAccuracy); }, *source);
				}

				// Often when NPC is a magic caster the currentSpell only gets set when NPC starts charging/casting it,
				// in such cases we can't determine the skill to use and should fallback to default logic.
				// Frankly, calls to aim in such cases are meaningless anyways, so we're not missing much.
				if (skillUsage) {
					float aimVariance = Settings::fCombatRangedAimVariance();
					controller->aimVariance = 2.0f * aimVariance * skillUsage.SkillFactor(attacker);
#ifndef NDEBUG
					float aimOffset = Settings::fCombatAimProjectileRandomOffset();
					logger::info("Accuracy:");
					logger::info("\tSkill: {} (base: {})", skillUsage.EffectiveSkill(attacker), skillUsage.Skill(attacker));
					logger::info("\tVariance: {:.4f} (base: {:.4f})", controller->aimVariance, aimVariance);
					logger::info("\taimOffset: {} (base: {:.4f}, max: {:.4f})", controller->aimOffset, aimOffset, aimOffset * controller->aimVariance);
#endif
				}
				func(controller, target);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		void Install() {
			stl::install_hook<CombatProjectileAimController_Update>();
		}
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

			static inline constexpr VariantSignature<"E8 ?? ?? ?? ?? F3 0F 59 3D ?? ?? ?? ?? F3 0F 59",
			                                         "E8 ?? ?? ?? ?? 0F 28 F0 41 0F 28 CF 41 0F 28 C2",
			                                         "E8 ?? ?? ?? ?? F3 0F 59 3D ?? ?? ?? ?? F3 0F 59">
				signature;

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
			static inline constexpr Signature          signature{ "E8 ?? ?? ?? ?? 48 C7 44 24 ?? 00 00 00 00 48 8D" };

			static inline void post_hook() { logger::info("Installed spells deviation logic"); }

			static void thunk(RE::ProjectileHandle& projectile, RE::Projectile::LaunchData& launchData) {
				if (!IsBeamProjectile(launchData.projectileBase)) { // A beam seems to be re-aimed from the caster every tick, so it's handled separately.
					auto attacker = launchData.shooter ? launchData.shooter->As<RE::Actor>() : nullptr;
					if (auto skillUsage = SkillUsage(launchData.spell, attacker, kPrecision); skillUsage) {
						AddRandomSpread(launchData, skillUsage);
					}
				}
				func(projectile, launchData);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct WeaponFireProjectile
		{
			static inline constexpr REL::RelocationID  relocation{ 17693, 18102 };
			static inline constexpr REL::VariantOffset offset{ 0xE82, 0xE60, 0x11ED };

			static inline constexpr VariantSignature<"E8 ?? ?? ?? ?? 48 8D 55 ?? 48 8D 4D ?? E8",
			                                         "E8 ?? ?? ?? ?? 8B 75 88 85 F6 0F 84",
			                                         "E8 ?? ?? ?? ?? 48 8D 55 ?? 48 8D 4D ?? E8">
				signature;

			static inline void post_hook() { logger::info("Installed modded arrow deviation logic"); }

			static void thunk(RE::ProjectileHandle& projectile, RE::Projectile::LaunchData& launchData) {
				auto attacker = launchData.shooter ? launchData.shooter->As<RE::Actor>() : nullptr;
				if (auto skillUsage = SkillUsage(launchData.weaponSource, attacker, kPrecision); skillUsage) {
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
			static inline constexpr Signature          signature{ "E8 ?? ?? ?? ?? 8B 87 ?? ?? ?? ?? C1 E8 11 A8 01" };

			static inline void post_hook() { logger::info("Installed Player 3rd person projectiles deviation logic"); }

			static void thunk(RE::PlayerCharacter* player, RE::Projectile* projectile, RE::NiNode* fireNode, float* angleZ, float* angleX, RE::NiPoint3* defaultOrigin, float tiltZ, float tiltX) {
				func(player, projectile, fireNode, angleZ, angleX, defaultOrigin, tiltZ, tiltX);

				if (!projectile)
					return;

				SkillUsage skillUsage{};
				if (auto spell = projectile->GetProjectileRuntimeData().spell; spell) {
					skillUsage = SkillUsage(spell, player, kPrecision);
				} else if (auto weapon = projectile->GetProjectileRuntimeData().weaponSource; weapon) {
					skillUsage = SkillUsage(weapon, player, kPrecision);
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

		void Install() {
			stl::install_hook<WeapFireAmmoRangomizeArrowDirection>();
			stl::install_hook<LaunchSpellProjectile>();
			stl::install_hook<WeaponFireProjectile>();
			stl::install_hook<PlayerAutoAim>();
		}
	}
}
