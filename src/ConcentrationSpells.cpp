#include "ConcentrationSpells.h"
#include "ChargeTime.h"
#include "Fatigue.h"
#include "Hooking.h"
#include "SkillUsage.h"

namespace NLA::ConcentrationSpells
{
	namespace
	{
		struct WanderAnimation
		{
			RE::NiPoint2 start{ 0.0f, 0.0f };
			RE::NiPoint2 finish{ 0.0f, 0.0f };
			RE::NiPoint2 current{ 0.0f, 0.0f };

			std::uint32_t elapsedTicks = 0;
			bool          paused = false;

			/// Duration in which animation should glide from start to finish.
			constexpr static std::uint32_t travelTicks = 45;

			/// Time that beam would stay at the same point before starting to glide to the next one.
			constexpr static std::uint32_t pauseTicks = 20;

			bool Advance(RE::Actor* attacker, const SkillUsage& skillUsage) {
				if (skillUsage.MaxSpread(attacker) <= 0.0f) {
					return false;
				}

				if (!paused && elapsedTicks >= travelTicks) {
					paused = true;
					elapsedTicks = 0;
				} else if (paused && elapsedTicks >= pauseTicks) {
					start = finish;

					finish = skillUsage.RandomSpreadOffset(attacker);

					paused = false;
					elapsedTicks = 0;
				}

				++elapsedTicks;

				if (paused) {
					current = finish;
				} else {
					float t = static_cast<float>(elapsedTicks) / travelTicks;
					t = t * t * (3.0f - 2.0f * t);
					current.x = start.x + (finish.x - start.x) * t;
					current.y = start.y + (finish.y - start.y) * t;
				}

				return true;
			}
		};

		/// One for each hand of each caster. Projectile updates run on the main thread, which is also where the state is reset, so no locking is needed.
		std::unordered_map<std::uint64_t, WanderAnimation> wanderAnimations;

		struct LocalContext
		{
			RE::Projectile* projectile = nullptr;

			/// pitch (x) and yaw (y).
			RE::NiPoint2 rotation{ 0.0f, 0.0f };
		};
		thread_local LocalContext localContext;

		RE::Actor* GetAttacker(RE::Projectile* projectile) {
			auto& runtimeData = projectile->GetProjectileRuntimeData();
			return runtimeData.shooter.get() ? runtimeData.shooter.get()->As<RE::Actor>() : nullptr;
		}

		/// Spread scale from casting the concentration spell continuously for longer than the caster's fatigue threshold.
		float FatigueScale(RE::Projectile* projectile, RE::Actor* attacker, WeaponType type) {
			auto& runtimeData = projectile->GetProjectileRuntimeData();

			return Fatigue::ConcentrationScale(attacker, type, ChargeTime::ConcentrationDuration(attacker, runtimeData.castingSource));
		}

		RE::NiPoint2 ApplyWanderOffset(RE::NiMatrix3* rotation, RE::Actor* attacker, RE::MagicSystem::CastingSource source, const SkillUsage& skillUsage, float scale) {
			auto& anim = wanderAnimations[ChargeTime::CasterKey(attacker->GetFormID(), source)];
			anim.Advance(attacker, skillUsage);

			const RE::NiPoint2 offset = anim.current * scale;

			RE::NiPoint3 euler{};
			rotation->ToEulerAnglesXYZ(euler);
			euler.x += offset.x;
			euler.z += offset.y;
			rotation->SetEulerAnglesXYZ(euler);

			return offset;
		}

		void ApplyRangeScaling(RE::Projectile* projectile, RE::Actor* attacker) {
			auto& runtimeData = projectile->GetProjectileRuntimeData();
			if (!runtimeData.impacts.empty())  // already hit something, don't change range anymore.
				return;

			auto skillUsage = SkillUsage(runtimeData.spell, attacker, kRange);
			if (!skillUsage)
				return;

			if (auto base = projectile->GetProjectileBase(); base) {
				float rangeMult = skillUsage.LerpSkillFactor(attacker);

				runtimeData.range = base->data.range * rangeMult;
				//logger::debug("Scaling range from {} to {} (multiplier: {})", base->data.range, runtimeData.range, rangeMult);
			}
		}

		/// Randomly offset direction of the free-aiming beam.
		template <typename Call>
		struct CopyRotation_Proxy
		{
			static void thunk(RE::Projectile* projectile, RE::NiMatrix3* rotation, RE::NiAVObject* fireNode) {
				Call::func(projectile, rotation, fireNode);
				localContext = {};
				if (projectile && rotation) {
					if (auto attacker = GetAttacker(projectile); attacker) {
						auto& runtimeData = projectile->GetProjectileRuntimeData();
						if (auto skillUsage = SkillUsage(runtimeData.spell, attacker, kPrecision); skillUsage) {
							auto offset = ApplyWanderOffset(rotation, attacker, runtimeData.castingSource, skillUsage, FatigueScale(projectile, attacker, skillUsage.weaponType));
							localContext = { projectile, offset };
						}
						ApplyRangeScaling(projectile, attacker);
					}
				}
			}
		};

		/// Randomly offset direction of the target-locked beam.
		template <typename Call>
		struct MakeRotation_Proxy
		{
			static void thunk(RE::NiMatrix3* out, float yaw, float roll, float pitch) {
				const auto& context = localContext;

				if (context.projectile && context.projectile->IsBeamProjectile()) {
					context.projectile->data.angle.x += context.rotation.x;
					context.projectile->data.angle.z += context.rotation.y;
				}
				Call::func(out, yaw + context.rotation.y, roll, pitch + context.rotation.x);
			}
		};
	}

	// When BeamProjectile::UpdateImpl is called, it:
	// 1) first copies rotation from caster's fire-node, so that beam points wherever caster is aiming.
	// 2) then, if beam locks onto a valid target (desiredTarget), it recomputes rotation so that beam points at that target instead.
	namespace BeamProjectile
	{
		struct UpdateImpl_CopyRotation
		{
			static inline constexpr REL::RelocationID  relocation{ 42586, 43749 };
			static inline constexpr REL::VariantOffset offset{ 0x144, 0x140, 0x144 };
			static inline constexpr Signature          signature{ "E8 ?? ?? ?? ?? 48 85 DB" };

			using Proxy = CopyRotation_Proxy<UpdateImpl_CopyRotation>;
			static inline REL::Relocation<decltype(Proxy::thunk)> func;

			static inline void post_hook() { logger::info("Installed free beam wander logic"); }
		};

		struct UpdateImpl_MakeRotation
		{
			static inline constexpr REL::RelocationID  relocation{ 42586, 43749 };
			static inline constexpr REL::VariantOffset offset{ 0x1E3, 0x1DF, 0x1E3 };
			static inline constexpr Signature          signature{ "E8 ?? ?? ?? ?? 0F 10 45 B7" };

			using Proxy = MakeRotation_Proxy<UpdateImpl_MakeRotation>;
			static inline REL::Relocation<decltype(Proxy::thunk)> func;

			static inline void post_hook() { logger::info("Installed locked beam wander logic"); }
		};

		/// Scales range for raycasts when beam finds a target.
		struct RunTargetPick
		{
			using Target = RE::BeamProjectile;

			static inline constexpr VariantIndex index{ 0xB7, 0xB7, 0xB8 };

			static bool thunk(RE::Projectile* projectile) {
				if (projectile) {
					if (auto attacker = GetAttacker(projectile); attacker) {
						ApplyRangeScaling(projectile, attacker);
					}
				}
				return func(projectile);
			}
			static inline REL::Relocation<decltype(thunk)> func;

			static inline void post_hook() { logger::info("Installed concentration beam range scaling logic"); }
		};
	}

	namespace FlameProjectile
	{
		struct UpdateImpl_MakeRotation
		{
			static inline constexpr REL::RelocationID  relocation{ 42727, 43894 };
			static inline constexpr REL::VariantOffset offset{ 0x15E, 0x15D, 0x161 };
			static inline constexpr Signature          signature{ "E8 ?? ?? ?? ?? 0F 10 44 24 50" };

			using Proxy = MakeRotation_Proxy<UpdateImpl_MakeRotation>;
			static inline REL::Relocation<decltype(Proxy::thunk)> func;

			static inline void post_hook() { logger::info("Installed free flame wander logic"); }
		};

		struct UpdateImpl_CopyRotation
		{
			static inline constexpr REL::RelocationID  relocation{ 42727, 43894 };
			static inline constexpr REL::VariantOffset offset{ 0xDB, 0xDB, 0xDE };
			static inline constexpr Signature          signature{ "E8 ?? ?? ?? ?? 80 7B" };

			using Proxy = CopyRotation_Proxy<UpdateImpl_CopyRotation>;
			static inline REL::Relocation<decltype(Proxy::thunk)> func;

			static inline void post_hook() { logger::info("Installed locked flame wander logic"); }
		};
	}

	void Reset() {
		wanderAnimations.clear();
	}

	void Install() {
		stl::install_hook<BeamProjectile::UpdateImpl_CopyRotation>();
		stl::install_hook<BeamProjectile::UpdateImpl_MakeRotation>();
		stl::install_hook<BeamProjectile::RunTargetPick>();
		stl::install_hook<FlameProjectile::UpdateImpl_CopyRotation>();
		stl::install_hook<FlameProjectile::UpdateImpl_MakeRotation>();
	}
}
