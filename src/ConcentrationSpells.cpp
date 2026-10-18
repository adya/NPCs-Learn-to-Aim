#include "ConcentrationSpells.h"
#include "ChargeTime.h"
#include "Hooking.h"
#include "Options.h"
#include "SkillUsage.h"
#include <unordered_map>

namespace NLA::ConcentrationSpells
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

		static std::unordered_map<RE::FormID, WanderAnimation> active;

		bool Advance(RE::Actor* attacker, const SkillUsage& skillUsage) {
			if (skillUsage.MaxSpread(attacker) <= 0.0f) {
				return false;
			}

			if (!paused && elapsedTicks >= WanderAnimation::travelTicks) {
				paused = true;
				elapsedTicks = 0;
			} else if (paused && elapsedTicks >= WanderAnimation::pauseTicks) {
				start = finish;

				finish = skillUsage.RandomSpreadOffset(attacker);

				paused = false;
				elapsedTicks = 0;
			}

			++elapsedTicks;

			if (paused) {
				current = finish;
			} else {
				float t = static_cast<float>(elapsedTicks) / WanderAnimation::travelTicks;
				t = t * t * (3.0f - 2.0f * t);
				current.x = start.x + (finish.x - start.x) * t;
				current.y = start.y + (finish.y - start.y) * t;
			}

			return true;
		}
	};
	std::unordered_map<RE::FormID, WanderAnimation> WanderAnimation::active{};

	/// Spread scale from casting the concentration spell continuously for longer than the caster's fatigue threshold.
	float FatigueScale(RE::Projectile* projectile, RE::Actor* attacker) {
		auto& runtimeData = projectile->GetProjectileRuntimeData();

		auto fatigueUsage = SkillUsage(runtimeData.spell, attacker, kFatigueThreshold);
		if (!fatigueUsage)
			return 1.0f;

		const auto& fatigue = Options::For(attacker).ConcentrationFatigueFor(fatigueUsage.weaponType);

		const float threshold = fatigueUsage.FatigueThreshold(attacker);

		const float fatigueSeconds = std::max(0.0f, ChargeTime::ConcentrationDuration(attacker, runtimeData.castingSource) - threshold);
		const float fatigueProgress = 1.0f - std::exp2(-fatigueSeconds / fatigue.halfDuration);

		return 1.0f + fatigue.maxSpreadMultiplier * fatigueProgress;
	}

	RE::NiPoint2 ApplyWanderOffset(RE::NiMatrix3* rotation, RE::Actor* attacker, const SkillUsage& skillUsage, float scale) {
		auto& anim = WanderAnimation::active[attacker->formID];
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

	struct LocalContext
	{
		RE::Projectile* projectile = nullptr;

		/// pitch (x) and yaw (y).
		RE::NiPoint2 rotation{ 0.0f, 0.0f };

		static thread_local LocalContext current;
	};
	thread_local LocalContext LocalContext::current{};

	RE::Actor* GetAttacker(RE::Projectile* projectile) {
		auto& runtimeData = projectile->GetProjectileRuntimeData();
		return runtimeData.shooter.get() ? runtimeData.shooter.get()->As<RE::Actor>() : nullptr;
	}

	namespace
	{
		/// Randomly offset direction of the free-aiming beam.
		template <typename Call>
		struct CopyRotation_Proxy
		{
			static void thunk(RE::Projectile* projectile, RE::NiMatrix3* rotation, RE::NiAVObject* fireNode) {
				Call::func(projectile, rotation, fireNode);
				LocalContext::current = {};
				if (projectile && rotation) {
					if (auto attacker = GetAttacker(projectile); attacker) {
						auto& runtimeData = projectile->GetProjectileRuntimeData();
						if (auto skillUsage = SkillUsage(runtimeData.spell, attacker, kPrecision); skillUsage) {
							auto offset = ApplyWanderOffset(rotation, attacker, skillUsage, FatigueScale(projectile, attacker));
							LocalContext::current = { projectile, offset };
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
				const auto& context = LocalContext::current;

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

	void Install() {
		stl::install_hook<BeamProjectile::UpdateImpl_CopyRotation>();
		stl::install_hook<BeamProjectile::UpdateImpl_MakeRotation>();
		stl::install_hook<BeamProjectile::RunTargetPick>();
		stl::install_hook<FlameProjectile::UpdateImpl_CopyRotation>();
		stl::install_hook<FlameProjectile::UpdateImpl_MakeRotation>();
	}
}
