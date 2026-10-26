#include "Steadying.h"
#include "Helpers.h"
#include "Hooking.h"
#include "Options.h"

namespace NLA::Steadying
{
	namespace
	{
		struct Duration
		{
			/// Caches ShouldLearn for the tracked shot or cast, this avoids repeated calls to ShouldLearn.
			bool enabled{ false };

			float seconds{ 0.0f };

			static std::unordered_map<TrackingKey, Duration> current;
			static RE::BSSpinLock                              lock;

			static float Peek(const TrackingKey& key) {
				RE::BSSpinLockGuard guard{ lock };

				auto it = current.find(key);
				return it != current.end() ? it->second.seconds : 0.0f;
			}

			static void Clear() {
				RE::BSSpinLockGuard guard{ lock };
				current.clear();
			}
		};
		std::unordered_map<TrackingKey, Duration> Duration::current;
		RE::BSSpinLock                            Duration::lock;

		/// Delta of the animation graph update that is currently running on this thread (0 outside of one).
		thread_local float graphDelta{ 0.0f };

		void TrackBow(RE::Actor* attacker) {
			const auto state = attacker->AsActorState()->GetAttackState();
			const bool drawn = Weapon::IsFullyDrawn(state);
			if (!drawn && !Weapon::IsReleased(state)) {
				return;  // still drawing, nothing to start or finish.
			}

			const auto key = TrackingKey::ForWeapon(attacker);

			RE::BSSpinLockGuard guard{ Duration::lock };

			if (drawn) {
				auto it = Duration::current.find(key);
				if (it == Duration::current.end()) {
					it = Duration::current.try_emplace(key).first;
					it->second.enabled = Options::ShouldLearn(attacker);
				}
				if (it->second.enabled) {
					it->second.seconds += graphDelta;
				}
			} else {
				Duration::current.erase(key);  // released or dropped.
			}
		}

		void TrackSpell(const RE::ActorMagicCaster* caster, RE::MagicCaster::State state, float delta) {
			auto* actor = caster->actor;
			if (!actor) {
				return;
			}

			const auto key = TrackingKey::ForCaster(actor, caster->castingSource);

			if (state == RE::MagicCaster::State::kNone) {
				RE::BSSpinLockGuard guard{ Duration::lock };
				Duration::current.erase(key);  // the cast is over, our launch hooks have already read the duration.
				return;
			}

			// Held ready (fire-and-forget spell) or being cast (concentration spell).
			const bool holding = Spell::IsConcentration(caster->currentSpell) ? Caster::IsConcentrating(caster, state) : Caster::IsReady(state);
			if (!holding) {
				return;  // in between, the duration stays as is until the caster is idle.
			}

			RE::BSSpinLockGuard guard{ Duration::lock };

			auto it = Duration::current.find(key);
			if (it == Duration::current.end()) {
				it = Duration::current.try_emplace(key).first;
				it->second.enabled = Options::ShouldLearn(actor);
			}
			if (it->second.enabled) {
				it->second.seconds += delta;
			}
		}
	}

	float FullDrawDuration(RE::Actor* actor) {
		return actor ? Duration::Peek(TrackingKey::ForWeapon(actor)) : 0.0f;
	}

	float SpellDuration(RE::Actor* actor, RE::MagicSystem::CastingSource source) {
		return actor ? Duration::Peek(TrackingKey::ForCaster(actor, source)) : 0.0f;
	}

	void Reset() {
		Duration::Clear();
	}
}

namespace NLA::Steadying
{
	/// Publishes the delta of the animation graph update, so the weapon speed poll that runs inside it can measure game time (affected by slow time).
	/// Only the actors' own updates are hooked. The weapon speed is also polled by the update of the equipped weapon's graph, which must not count again.
	template <typename Call>
	struct GraphDelta_Proxy
	{
		static bool thunk(void* holder, const RE::BSAnimationUpdateData* data) {
			const auto previous = graphDelta;
			graphDelta = data ? data->deltaTime : 0.0f;
			const bool result = Call::func(holder, data);
			graphDelta = previous;
			return result;
		}
	};

	/// Animation update of actors, where it is inlined into `TESObjectREFR::UpdateAnimation`.
	/// On SE and VR that calls the same function as the animation job instead (see below), so this is AE only.
	struct TESObjectREFR_UpdateAnimation_Update
	{
		static inline constexpr REL::RelocationID    relocation{ 0, 20119 };
		static inline constexpr REL::VariantOffset   offset{ 0x1E7, 0x1E7, 0x1E7 };
		static inline constexpr REL::Module::Runtime runtime{ REL::Module::Runtime::AE };
		static inline constexpr Signature            signature{ "E8 ?? ?? ?? ?? 48 89 7C 24 48" };

		static inline void post_hook() { logger::info("🪝 Installed animation update delta tracking."); }

		using Proxy = GraphDelta_Proxy<TESObjectREFR_UpdateAnimation_Update>;
		static inline REL::Relocation<decltype(Proxy::thunk)> func;
	};

	/// Animation update of actors by the animation job (and by `TESObjectREFR::UpdateAnimation` on SE and VR).
	struct TESObjectREFR_AnimationJob_Update
	{
		static inline constexpr REL::RelocationID  relocation{ 19729, 20123 };
		static inline constexpr REL::VariantOffset offset{ 0x96, 0x96, 0x96 };
		static inline constexpr Signature          signature{ "E8 ?? ?? ?? ?? 48 89 74 24 48" };

		static inline void post_hook() { logger::info("🪝 Installed animation job delta tracking."); }

		using Proxy = GraphDelta_Proxy<TESObjectREFR_AnimationJob_Update>;
		static inline REL::Relocation<decltype(Proxy::thunk)> func;
	};

	/// Animation update of the player.
	struct PlayerCharacter_UpdateAnimation_Update
	{
		static inline constexpr REL::RelocationID  relocation{ 39445, 40521 };
		static inline constexpr REL::VariantOffset offset{ 0x97, 0x97, 0x156 };

		static inline constexpr VariantSignature<"E8 ?? ?? ?? ?? 84 C0 74 47 4C 89 74 24 48",
		                                         "E8 ?? ?? ?? ?? 84 C0 74 47 4C 89 74 24 48",
		                                         "E8 ?? ?? ?? ?? 84 C0 0F 84 B4 0C 00 00 40 84 FF">
			signature;

		static inline void post_hook() { logger::info("🪝 Installed player animation delta tracking."); }

		using Proxy = GraphDelta_Proxy<PlayerCharacter_UpdateAnimation_Update>;
		static inline REL::Relocation<decltype(Proxy::thunk)> func;
	};

	/// Hooks the same call as the draw speed scaling, to track the hold time from within the animation graph update.
	struct PollChannelUpdateImpl_GetWeaponSpeed
	{
		static inline constexpr REL::RelocationID  relocation{ 41694, 42779 };
		static inline constexpr REL::VariantOffset offset{ 0x29, 0x29, 0x29 };
		static inline constexpr Signature          signature{ "E8 ?? ?? ?? ?? 48 8B 44 24 50" };

		static inline void post_hook() { logger::info("🪝 Installed bow/crossbow hold time tracking."); }

		static float thunk(RE::ActorValueOwner* owner, RE::TESObjectWEAP* weapon, bool leftHand) {
			const float speed = func(owner, weapon, leftHand);

			auto attacker = Actor::FromActorValueOwner(owner);
			if (!attacker || !weapon)
				return speed;

			if (Weapon::IsBowOrCrossbow(weapon->GetWeaponType())) {
				TrackBow(attacker);
			}
			return speed;
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};

	/// Must be installed after the hooks that modify the delta (e.g. ChargeTime), so that it observes the original one.
	struct MagicCaster_Update
	{
		using Target = RE::ActorMagicCaster;

		static inline constexpr VariantIndex index{ 0x1D, 0x1D, 0x1F };

		static void thunk(RE::ActorMagicCaster* caster, float delta) {
			if (caster) {
				TrackSpell(caster, caster->state.get(), delta);
			}
			func(caster, delta);
		}

		static inline REL::Relocation<decltype(thunk)> func;

		static inline void post_hook() { logger::info("🪝 Installed spell hold time tracking."); }
	};

	void Install() {
		stl::install_hook<TESObjectREFR_UpdateAnimation_Update>();
		stl::install_hook<TESObjectREFR_AnimationJob_Update>();
		stl::install_hook<PlayerCharacter_UpdateAnimation_Update>();
		stl::install_hook<PollChannelUpdateImpl_GetWeaponSpeed>();
		stl::install_hook<MagicCaster_Update>();
	}
}
