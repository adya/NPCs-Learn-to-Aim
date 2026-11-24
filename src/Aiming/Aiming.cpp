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

		struct PlayerShot
		{
			static inline std::unordered_map<TrackingKey, RE::NiPoint2> pending{};
			static inline RE::BSSpinLock                                lock{};

			static TrackingKey KeyFor(RE::Actor* actor, const RE::MagicItem* spell, RE::MagicSystem::CastingSource source) {
				return spell ? TrackingKey::ForCaster(actor, source) : TrackingKey::ForWeapon(actor);
			}

			static void Set(const TrackingKey& key, const RE::NiPoint2& offset) {
				RE::BSSpinLockGuard guard{ lock };
				pending[key] = offset;
			}

			static void Clear(const TrackingKey& key) {
				RE::BSSpinLockGuard guard{ lock };
				pending.erase(key);
			}

			static bool HasPending() {
				RE::BSSpinLockGuard guard{ lock };
				return !pending.empty();
			}

			static std::optional<RE::NiPoint2> Pop(const TrackingKey& key) {
				RE::BSSpinLockGuard guard{ lock };

				auto it = pending.find(key);
				if (it == pending.end())
					return std::nullopt;

				const auto offset = it->second;
				pending.erase(it);
				return offset;
			}
		};

		struct SteadyingModifier
		{
			float holdSeconds{ 0.0f };

			bool  fatigueEnabled{ false };
			float fatigueThreshold{ 0.0f };

			Fatigue::HoldScale hold{};

			static SteadyingModifier For(RE::Actor* attacker, const Options::AttackKind& kind, float holdSeconds) {
				const auto& options = Options::For(attacker);
				const auto& attack = options.For(kind);
				return { attacker, holdSeconds, kind, attack.steadying, attack.fatigue };
			}

			SteadyingModifier(RE::Actor* attacker, float holdSeconds, const Options::AttackKind& kind, const Options::Config::Spread& steadying, const Options::Config::Fatigue& fatigue) :
				holdSeconds(holdSeconds), fatigueEnabled(fatigue.enabled), fatigueThreshold(fatigue.enabled ? Fatigue::Threshold(fatigue, attacker, kind) : 0.0f) {
				hold = Fatigue::Hold(steadying, fatigue, fatigueThreshold, holdSeconds);
			}
		};

		struct ShotModifier
		{
			RE::Actor*         attacker{ nullptr };
			const RE::TESForm* source{ nullptr };

			/// How far the shot may deviate from the aim (max spread).
			SkillUsage accuracy{};

			/// How the shots are distributed within that radius (focus).
			SkillUsage precision{};

			std::optional<SteadyingModifier> steadying{};

			/// Where the spread is kept for whoever re-aims the projectile later. Only the player's shots have one.
			std::optional<TrackingKey> playerShot{};

			RE::NiPoint2 rawOffset{};
			RE::NiPoint2 offset{};

			/// Launch angles (x, z) before and after the spread was applied.
			RE::NiPoint2 originalAngles{};
			RE::NiPoint2 angles{};

			ShotModifier() = default;

			static ShotModifier ForSpell(RE::Actor* attacker, const RE::MagicItem* spell, RE::MagicSystem::CastingSource source) {
				ShotModifier modifier{ attacker, spell, SkillUsage(spell, attacker, kAccuracy), SkillUsage(spell, attacker, kPrecision) };
				if (attacker && attacker->IsPlayerRef())
					modifier.playerShot = PlayerShot::KeyFor(attacker, spell, source);

				const float holdSeconds = Steadying::SpellDuration(attacker, source);
				if (modifier.accuracy) {
					modifier.steadying = SteadyingModifier::For(attacker, *modifier.accuracy.kind, holdSeconds);
				}
				return modifier;
			}

			static ShotModifier ForWeapon(RE::Actor* attacker, const RE::TESObjectWEAP* weapon) {
				ShotModifier modifier{ attacker, weapon, SkillUsage(weapon, attacker, kAccuracy), SkillUsage(weapon, attacker, kPrecision) };
				if (attacker && attacker->IsPlayerRef())
					modifier.playerShot = PlayerShot::KeyFor(attacker, nullptr, RE::MagicSystem::CastingSource::kNone);

				// Without Accuracy there is no spread for steadying or fatigue to scale. That is also how a kind that is switched off stays untouched.
				if (auto kind = Weapon::KindOf(weapon); kind && modifier.accuracy) {
					modifier.steadying = SteadyingModifier::For(attacker, *kind, Steadying::FullDrawDuration(attacker));
				}
				return modifier;
			}

			/// Adds a random spread to the given launch angles.
			void Apply(float* angleX, float* angleZ) {
				if (playerShot)
					PlayerShot::Clear(*playerShot);

				if (!attacker || !accuracy)
					return;

				originalAngles = { *angleX, *angleZ };
				rawOffset = RandomSpreadOffset(accuracy, precision, attacker);
				offset = rawOffset * (steadying ? steadying->hold.scale : 1.0f);

				*angleX += offset.x;  // add random deviation to the launch angles.
				*angleZ += offset.y;

				angles = { *angleX, *angleZ };

				if (playerShot)
					PlayerShot::Set(*playerShot, offset);
			}

		private:
			ShotModifier(RE::Actor* attacker, const RE::TESForm* source, SkillUsage accuracy, SkillUsage precision) :
				attacker(attacker), source(source), accuracy(accuracy), precision(precision) {}
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

			if (const auto& accuracy = modifier.accuracy; accuracy) {
				logger::info("\tSkill: {:.1f} (base: {:.1f}), accuracy: {:.4f}, focus: {:.3f}", accuracy.EffectiveSkill(attacker), accuracy.Skill(attacker), accuracy.SkillFactor(attacker), SpreadFocus(modifier.precision, attacker));
			}

			if (const auto skillShotUsage = weapon ? SkillUsage(weapon, attacker, kSkillShot) : SkillUsage(spell, attacker, kSkillShot); skillShotUsage) {
				logger::info("\tSkill shot chance: {:.0f}% (aiming at: {})", Skillshots::SkillShotChance(attacker, skillShotUsage) * 100.0f, Skillshots::CurrentSkillShot(attacker));
			}

			const float speed = weapon ? Draw::SpeedMultiplier(attacker, weapon) : ChargeTime::SpeedMultiplier(attacker, spell);
			logger::info("\t{} speed: x{:.3f}", weapon ? "Draw" : "Charge", speed);

			if (modifier.steadying && weapon && modifier.steadying->holdSeconds <= 0.0f) {
				logger::info("\tHeld ready: no hold time counted ({})", weapon->IsCrossbow() ? "crossbow" : "bow");
			}

			if (modifier.steadying && modifier.steadying->holdSeconds > 0.0f) {
				const auto& steadying = *modifier.steadying;
				logger::info("\t{}: {:.3f}s (steadying: x{:.3f})", weapon ? (weapon->IsCrossbow() ? "Held loaded" : "Held at full draw") : "Held charged", steadying.holdSeconds, steadying.hold.steadied);
				if (steadying.fatigueEnabled) {
					logger::info("\tFatigue: sets in after {:.3f}s, fatigued for {:.3f}s ({:.0f}% of max debuff)", steadying.fatigueThreshold, steadying.hold.fatigueSeconds, steadying.hold.progress * 100.0f);
				}
				logger::info("\tSpread scale: x{:.3f} ({:+.0f}%)", steadying.hold.scale, (steadying.hold.scale - 1.0f) * 100.0f);
			}

			if (const auto& accuracy = modifier.accuracy; accuracy) {
				logger::info("\tSpread: {:.3f}° -> {:.3f}° (max: {:.3f}°)", RE::rad_to_deg(modifier.rawOffset.Length()), RE::rad_to_deg(modifier.offset.Length()), RE::rad_to_deg(accuracy.MaxSpread(attacker)));
				logger::info("\tAngleX: {:.3f}° -> {:.3f}°", RE::rad_to_deg(modifier.originalAngles.x), RE::rad_to_deg(modifier.angles.x));
				logger::info("\tAngleZ: {:.3f}° -> {:.3f}°", RE::rad_to_deg(modifier.originalAngles.y), RE::rad_to_deg(modifier.angles.y));
			}
#endif
		}

	}

	namespace Aim
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

				ShotModifier modifier{};

				// PlayerAutoAim will handle 3rd person aim.
				if (!IsHandledByPlayerAutoAim(attacker)) {
					modifier = ShotModifier::ForSpell(attacker, launchData.spell, launchData.castingSource);
					modifier.Apply(&launchData.angleX, &launchData.angleZ);
					LogShot(modifier);
				}

				func(projectile, launchData);

				// The shot is fired, the next one takes a new marble from the bag. A concentration spell keeps its pick for as long as it is cast.
				if (!Spell::IsConcentration(launchData.spell)) {
					Skillshots::ClearSkillShot(attacker);
				}
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

				Skillshots::ClearSkillShot(attacker);

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

				modifier.Apply(angleX, angleZ);
				LogShot(modifier);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		/// Points the projectile along the given direction.
		void Redirect(RE::Projectile* projectile, const RE::NiPoint3& direction) {
			auto& runtimeData = projectile->GetProjectileRuntimeData();
			runtimeData.linearVelocity = direction * runtimeData.linearVelocity.Length();

			projectile->data.angle.x = -std::asin(std::clamp(direction.z, -1.0f, 1.0f));
			projectile->data.angle.z = std::atan2(direction.x, direction.y);
			if (projectile->data.angle.z < 0.0f)
				projectile->data.angle.z += RE::NI_TWO_PI;

			if (auto* node = projectile->Get3D2(); node) {
				const float sb = direction.z;
				const float cb = std::sqrt(std::max(0.0f, 1.0f - sb * sb));
				if (cb > 1e-4f) {
					const float ca = direction.y / cb;
					const float sa = -direction.x / cb;
					auto&       m = node->local.rotate;
					m.entry[0][0] = ca;
					m.entry[0][1] = direction.x;
					m.entry[0][2] = sa * sb;
					m.entry[1][0] = sa;
					m.entry[1][1] = direction.y;
					m.entry[1][2] = -ca * sb;
					m.entry[2][0] = 0.0f;
					m.entry[2][1] = sb;
					m.entry[2][2] = cb;
				}
			}
		}

		/// Applies the spread offsets after True Directional Movement auto-aims a projectile.
		template <class ProjectileType>
		struct TDMPredictiveAimCompatibility
		{
			using Target = ProjectileType;

			static inline constexpr std::size_t index = 0x86;

			static void thunk(RE::Projectile* projectile, RE::NiPoint3& velocity) {
				if (!projectile || !PlayerShot::HasPending() || projectile->GetProjectileRuntimeData().desiredTarget.native_handle() != 0) {
					return func(projectile, velocity);
				}

				const auto&                 runtimeData = projectile->GetProjectileRuntimeData();
				const auto                  shooter = runtimeData.shooter.get();
				std::optional<RE::NiPoint2> offset;
				if (shooter && shooter->IsPlayerRef()) {
					offset = PlayerShot::Pop(PlayerShot::KeyFor(shooter->As<RE::Actor>(), runtimeData.spell, runtimeData.castingSource));
				}

				func(projectile, velocity);

				if (!offset)
					return;

				if (runtimeData.desiredTarget.native_handle() == 0)
					return;  // TDM sets desiredTarget when it re-aims a projectile, so it didn't: the spread in the launch angles is intact.

				auto direction = runtimeData.linearVelocity;
				if (direction.SqrLength() < 1e-6f)
					return;
				direction.Unitize();

				const float pitch = std::clamp(-std::asin(std::clamp(direction.z, -1.0f, 1.0f)) + offset->x, -RE::NI_HALF_PI, RE::NI_HALF_PI);
				const float yaw = std::atan2(direction.x, direction.y) + offset->y;

				Redirect(projectile, { std::sin(yaw) * std::cos(pitch), std::cos(yaw) * std::cos(pitch), -std::sin(pitch) });
				logger::debug("Re-applied spread ({:.3f}°, {:.3f}°) on top of the re-aimed projectile", RE::rad_to_deg(offset->x), RE::rad_to_deg(offset->y));
			}
			static inline REL::Relocation<decltype(thunk)> func;

			static inline void pre_hook() { logger::debug("Installing TDM predictive aim compatibility for {}...", typeid(ProjectileType).name()); }
			static inline void post_hook() { logger::info("🪝 Installed TDM predictive aim compatibility for {}", typeid(ProjectileType).name()); }
		};

		void Install() {
			stl::install_hook<WeapFireAmmoRandomizeArrowDirection>();
			stl::install_hook<LaunchSpellProjectile>();
			stl::install_hook<WeaponFireProjectile>();
			stl::install_hook<PlayerAutoAim>();

			if (GetModuleHandleW(L"TrueDirectionalMovement.dll")) {
				stl::install_hook<TDMPredictiveAimCompatibility<RE::Projectile>>();
				stl::install_hook<TDMPredictiveAimCompatibility<RE::ArrowProjectile>>();
				stl::install_hook<TDMPredictiveAimCompatibility<RE::MissileProjectile>>();
			} else {
				logger::debug("True Directional Movement is not installed, skipping its predictive aim compatibility");
			}
		}
	}
}
