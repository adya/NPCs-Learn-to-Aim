#include "Aiming.h"
#include "ChargeTime.h"
#include "DrawSpeed.h"
#include "Fatigue.h"
#include "Helpers.h"
#include "Hooking.h"
#include "Options.h"
#include "SkillUsage.h"
#include "Steadying.h"

namespace NLA
{
	namespace
	{
		bool IsHandledByPlayerAutoAim(const RE::Actor* attacker) {
			if (!attacker || !attacker->IsPlayerRef())
				return false;
			auto* camera = RE::PlayerCamera::GetSingleton();
			return camera && !camera->IsInFirstPerson();
		}

		struct SteadyingModifier
		{
			float holdSeconds{ 0.0f };

			/// Spread scale from steadying.
			float steadied{ 1.0f };

			bool  fatigueEnabled{ false };
			float fatigueThreshold{ 0.0f };
			float fatigueSeconds{ 0.0f };

			/// How far the fatigue debuff has progressed towards its max (0-1).
			float fatigueProgress{ 0.0f };

			/// Final spread scale, combining steadying and fatigue.
			float scale{ 1.0f };

			static SteadyingModifier For(RE::Actor* attacker, const Options::AttackKind& kind, float holdSeconds) {
				const auto& options = Options::For(attacker);
				const auto& attack = options.For(kind);
				return { attacker, holdSeconds, kind, attack.steadying, attack.fatigue };
			}

			SteadyingModifier(RE::Actor* attacker, float holdSeconds, const Options::AttackKind& kind, const Options::Config::Spread& steadying, const Options::Config::Fatigue& fatigue) :
				holdSeconds(holdSeconds), fatigueEnabled(fatigue.enabled), fatigueThreshold(fatigue.enabled ? Fatigue::Threshold(fatigue, attacker, kind) : 0.0f) {
				if (holdSeconds <= 0.0f)
					return;

				const float steadyingSeconds = fatigue.enabled ? std::min(holdSeconds, fatigueThreshold) : holdSeconds;

				if (steadying.enabled) {
					steadied = 1.0f - steadying.maxSpreadMultiplier * Fatigue::Progress(steadyingSeconds, steadying.halfDuration);
				}
				scale = steadied;

				if (!fatigue.enabled)
					return;

				fatigueSeconds = std::max(0.0f, holdSeconds - fatigueThreshold);
				fatigueProgress = Fatigue::Progress(fatigueSeconds, fatigue.halfDuration);

				const float fatigueTarget = 1.0f + fatigue.maxSpreadMultiplier;
				scale = steadied + (fatigueTarget - steadied) * fatigueProgress;
			}
		};

		struct ShotModifier
		{
			RE::Actor*         attacker{ nullptr };
			const RE::TESForm* source{ nullptr };
			SkillUsage         precision{};

			std::optional<SteadyingModifier> steadying{};

			RE::NiPoint2 rawOffset{};
			RE::NiPoint2 offset{};

			/// Launch angles (x, z) before and after the spread was applied.
			RE::NiPoint2 originalAngles{};
			RE::NiPoint2 angles{};

			ShotModifier() = default;

			static ShotModifier ForSpell(RE::Actor* attacker, const RE::MagicItem* spell, RE::MagicSystem::CastingSource source) {
				ShotModifier modifier{ attacker, spell, SkillUsage(spell, attacker, kPrecision) };

				const float holdSeconds = Steadying::SpellDuration(attacker, source);
				if (modifier.precision) {
					modifier.steadying = SteadyingModifier::For(attacker, *modifier.precision.kind, holdSeconds);
				}
				return modifier;
			}

			static ShotModifier ForWeapon(RE::Actor* attacker, const RE::TESObjectWEAP* weapon) {
				ShotModifier modifier{ attacker, weapon, SkillUsage(weapon, attacker, kPrecision) };

				if (auto kind = Weapon::KindOf(weapon); kind) {
					modifier.steadying = SteadyingModifier::For(attacker, *kind, Steadying::FullDrawDuration(attacker));
				}
				return modifier;
			}

			/// Adds a random spread to the given launch angles.
			void Apply(float* angleX, float* angleZ) {
				if (!attacker || !precision)
					return;

				originalAngles = { *angleX, *angleZ };
				rawOffset = precision.RandomSpreadOffset(attacker);
				offset = rawOffset * (steadying ? steadying->scale : 1.0f);

				*angleX += offset.x;  // add random deviation to the launch angles.
				*angleZ += offset.y;

				angles = { *angleX, *angleZ };
			}

		private:
			ShotModifier(RE::Actor* attacker, const RE::TESForm* source, SkillUsage precision) :
				attacker(attacker), source(source), precision(precision) {}
		};

		void LogShot([[maybe_unused]] const ShotModifier& modifier) {
#ifndef NDEBUG
			auto* attacker = modifier.attacker;
			if (!attacker)
				return;

			const auto* weapon = modifier.source ? modifier.source->As<RE::TESObjectWEAP>() : nullptr;
			const auto* spell = modifier.source ? modifier.source->As<RE::MagicItem>() : nullptr;

			if (Spell::IsConcentration(spell))
				return;

			logger::info("");
			logger::info("Shooting: {} with {}", *attacker, modifier.source ? modifier.source->GetName() : "?");

			if (const auto& precision = modifier.precision; precision) {
				logger::info("\tSkill: {:.1f} (base: {:.1f}), factor: {:.4f}", precision.EffectiveSkill(attacker), precision.Skill(attacker), precision.SkillFactor(attacker));
			}

			if (auto accuracy = weapon ? SkillUsage(weapon, attacker, kAccuracy) : SkillUsage(spell, attacker, kAccuracy); accuracy) {
				const float variance = Accuracy::AimVariance(accuracy, attacker);
				const float aimOffset = Settings::fCombatAimProjectileRandomOffset();
				logger::info("\tAim variance: {:.4f} (max aim offset: {:.2f}, base: {:.2f})", variance, aimOffset * variance, aimOffset);
			}

			if (auto headshotUsage = SkillUsage(weapon, attacker, kHeadshot); headshotUsage) {
				logger::info("\tHeadshot chance: {:.0f}% ({})", Accuracy::HeadshotChance(attacker, headshotUsage) * 100.0f, Accuracy::IsGoingForHeadshot(attacker) ? "going for a headshot" : "aiming at the body");
			}

			const float speed = weapon ? Draw::SpeedMultiplier(attacker, weapon) : ChargeTime::SpeedMultiplier(attacker, spell);
			logger::info("\t{} speed: x{:.3f}", weapon ? "Draw" : "Charge", speed);

			if (modifier.steadying && weapon && modifier.steadying->holdSeconds <= 0.0f) {
				logger::info("\tHeld ready: no hold time counted ({})", weapon->IsCrossbow() ? "crossbow" : "bow");
			}

			if (modifier.steadying && modifier.steadying->holdSeconds > 0.0f) {
				const auto& steadying = *modifier.steadying;
				logger::info("\t{}: {:.3f}s (steadying: x{:.3f})", weapon ? (weapon->IsCrossbow() ? "Held loaded" : "Held at full draw") : "Held charged", steadying.holdSeconds, steadying.steadied);
				if (steadying.fatigueEnabled) {
					logger::info("\tFatigue: sets in after {:.3f}s, fatigued for {:.3f}s ({:.0f}% of max debuff)", steadying.fatigueThreshold, steadying.fatigueSeconds, steadying.fatigueProgress * 100.0f);
				}
				logger::info("\tSpread scale: x{:.3f} ({:+.0f}%)", steadying.scale, (steadying.scale - 1.0f) * 100.0f);
			}

			if (const auto& precision = modifier.precision; precision) {
				logger::info("\tSpread: {:.4f} -> {:.4f} (max: {:.4f})", modifier.rawOffset.Length(), modifier.offset.Length(), precision.MaxSpread(attacker));
				logger::info("\tAngleX: {:.4f} -> {:.4f}", modifier.originalAngles.x, modifier.angles.x);
				logger::info("\tAngleZ: {:.4f} -> {:.4f}", modifier.originalAngles.y, modifier.angles.y);
			}
#endif
		}
	}

	namespace Precision
	{
		/// Disable original random offset for arrows.
		/// This allows replacing randomization logic with our own.
		/// This also means that if a particular aiming is disabled, NPCs will always shoot right where they aim at.
		struct WeapFireAmmoRandomizeArrowDirection
		{
			static inline constexpr REL::RelocationID  relocation{ 17693, 18102 };
			static inline constexpr REL::VariantOffset offset{ 0xCB0, 0xCD5, 0xFFF };

			static inline constexpr VariantSignature<"E8 ?? ?? ?? ?? F3 0F 59 3D ?? ?? ?? ?? F3 0F 59",
			                                         "E8 ?? ?? ?? ?? 0F 28 F0 41 0F 28 CF 41 0F 28 C2",
			                                         "E8 ?? ?? ?? ?? F3 0F 59 3D ?? ?? ?? ?? F3 0F 59">
				signature;

			static inline void pre_hook() { logger::debug("Disabling default arrow deviation logic..."); }
			static inline void post_hook() { logger::info("🪝 Disabled default arrow deviation logic"); }

			static float thunk(float, float) {
				return 0;
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct LaunchSpellProjectile
		{
			static inline constexpr REL::RelocationID  relocation{ 33672, 34452 };
			static inline constexpr REL::VariantOffset offset{ 0x377, 0x354, 0x377 };
			static inline constexpr Signature          signature{ "E8 ?? ?? ?? ?? 48 C7 44 24 ?? 00 00 00 00 48 8D" };

			static inline void pre_hook() { logger::debug("Installing spells deviation logic..."); }
			static inline void post_hook() { logger::info("🪝 Installed spells deviation logic"); }

			static void thunk(RE::ProjectileHandle& projectile, RE::Projectile::LaunchData& launchData) {
				auto attacker = launchData.shooter ? launchData.shooter->As<RE::Actor>() : nullptr;

				// PlayerAutoAim will handle 3rd person aim.
				if (!IsHandledByPlayerAutoAim(attacker)) {
					auto modifier = ShotModifier::ForSpell(attacker, launchData.spell, launchData.castingSource);
					modifier.Apply(&launchData.angleX, &launchData.angleZ);
					LogShot(modifier);
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

			static inline void pre_hook() { logger::debug("Installing modded arrow deviation logic..."); }
			static inline void post_hook() { logger::info("🪝 Installed modded arrow deviation logic"); }

			static void thunk(RE::ProjectileHandle& projectile, RE::Projectile::LaunchData& launchData) {
				auto attacker = launchData.shooter ? launchData.shooter->As<RE::Actor>() : nullptr;

				// PlayerAutoAim will handle 3rd person aim.
				if (!IsHandledByPlayerAutoAim(attacker)) {
					auto modifier = ShotModifier::ForWeapon(attacker, launchData.weaponSource);
					modifier.Apply(&launchData.angleX, &launchData.angleZ);
					LogShot(modifier);
				}

				Accuracy::ClearHeadshot(attacker);

				func(projectile, launchData);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct PlayerAutoAim
		{
			static inline constexpr REL::RelocationID  relocation{ 43009, 44200 };
			static inline constexpr REL::VariantOffset offset{ 0x201, 0x201, 0x201 };
			static inline constexpr Signature          signature{ "E8 ?? ?? ?? ?? 8B 87 ?? ?? ?? ?? C1 E8 11 A8 01" };

			static inline void pre_hook() { logger::debug("Installing Player 3rd person projectiles deviation logic..."); }
			static inline void post_hook() { logger::info("🪝 Installed Player 3rd person projectiles deviation logic"); }

			static void thunk(RE::PlayerCharacter* player, RE::Projectile* projectile, RE::NiNode* fireNode, float* angleZ, float* angleX, RE::NiPoint3* defaultOrigin, float tiltZ, float tiltX) {
				func(player, projectile, fireNode, angleZ, angleX, defaultOrigin, tiltZ, tiltX);

				if (!projectile)
					return;

				// In 1st person we ignore the auto-aiming as it seems to only correct aim in 3rd person.
				if (!IsHandledByPlayerAutoAim(player))
					return;

				const auto&  runtimeData = projectile->GetProjectileRuntimeData();
				ShotModifier modifier{};
				if (auto spell = runtimeData.spell; spell) {
					modifier = ShotModifier::ForSpell(player, spell, runtimeData.castingSource);
				} else if (auto weapon = runtimeData.weaponSource; weapon) {
					modifier = ShotModifier::ForWeapon(player, weapon);
				}

				if (modifier.precision) {
					// we need to preserve calculated tilt angles and only apply random spread to the launch angles.
					*angleZ -= tiltZ;
					*angleX -= tiltX;
					modifier.Apply(angleX, angleZ);
					*angleZ += tiltZ;
					*angleX += tiltX;
					LogShot(modifier);
				}
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		void Install() {
			stl::install_hook<WeapFireAmmoRandomizeArrowDirection>();
			stl::install_hook<LaunchSpellProjectile>();
			stl::install_hook<WeaponFireProjectile>();
			stl::install_hook<PlayerAutoAim>();
		}
	}
}
