#include "ChargeTime.h"
#include "Hooking.h"
#include "Options.h"
#include "SkillUsage.h"

#include <unordered_map>
#include <utility>

namespace NLA::ChargeTime
{
	namespace
	{
		struct Entry
		{
			RE::MagicCaster::State lastState{ RE::MagicCaster::State::kNone };

			/// Whether the mod applies to the actor. Decided once when a cast starts, to avoid looking up keywords every frame.
			bool learns{ false };

			/// Time that spell is held in charged state in given casting source (e.g. left/right hand) for a given actor.
			float chargedDuration{ 0.0f };

			/// Total time that a fully charged spell was held for a given actor and casting source.
			float holdDuration{ 0.0f };

			/// Time that a concentration spell has been continuously cast in given casting source for a given actor.
			float concentrationDuration{ 0.0f };
		};

		RE::BSSpinLock                           entriesLock;
		std::unordered_map<std::uint64_t, Entry> entries;

		bool IsReady(RE::MagicCaster::State state) {
			return state == RE::MagicCaster::State::kReady;
		}

		bool IsCharging(RE::MagicCaster::State state) {
			return state == RE::MagicCaster::State::kUnk02;  // Actually, the state during charging is 2, unlike enum's suggested kCharging = 5.
		}

		bool IsInterrupted(RE::MagicCaster::State state) {
			return state == RE::MagicCaster::State::kUnk08 || state == RE::MagicCaster::State::kUnk09;
		}

		bool IsConcentration(const RE::MagicItem* spell) {
			return spell && spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration;
		}

		/// Whether the caster is actively casting a concentration spell (past its charge-up and not yet released/interrupted).
		bool IsConcentrating(const RE::ActorMagicCaster* caster, RE::MagicCaster::State state) {
			return IsConcentration(caster->currentSpell) &&
			       state != RE::MagicCaster::State::kNone &&
			       state != RE::MagicCaster::State::kUnk01 &&
			       !IsCharging(state) &&
			       !IsInterrupted(state);
		}

		/// Updates the caster's tracked durations and returns whether the mod applies to its actor.
		bool Track(const RE::ActorMagicCaster* caster, RE::MagicCaster::State state, float delta) {
			auto* actor = caster->actor;
			if (!actor) {
				return false;
			}

			const auto key = CasterKey(actor->GetFormID(), caster->castingSource);

			RE::BSSpinLockGuard lock{ entriesLock };

			auto it = entries.find(key);
			if (it == entries.end()) {
				if (state == RE::MagicCaster::State::kNone) {
					return false;  // idle casters are the vast majority, so don't even track them.
				}
				it = entries.try_emplace(key).first;
			}

			auto&      entry = it->second;
			const auto prev = entry.lastState;

			if (prev != state) {
				if (prev == RE::MagicCaster::State::kNone) {
					entry.learns = Options::ShouldLearn(actor);
				}

				if (entry.learns) {
					if (IsReady(prev)) {
						const float duration = entry.chargedDuration;
						entry.chargedDuration = 0.0f;

						if (IsInterrupted(state)) {
							entry.holdDuration = 0.0f;
						} else {
							entry.holdDuration = duration;
						}
					} else if (IsCharging(state) || IsInterrupted(state)) {
						entry.holdDuration = 0.0f;  // a new cast is starting or the previous one was cancelled.
					}
				}

				entry.lastState = state;
			}

			if (entry.learns) {
				if (IsReady(state)) {
					entry.chargedDuration += delta;
				}

				if (IsConcentrating(caster, state)) {
					entry.concentrationDuration += delta;
				} else {
					entry.concentrationDuration = 0.0f;
				}
			}

			const bool learns = entry.learns;
			if (state == RE::MagicCaster::State::kNone && entry.holdDuration == 0.0f) {
				entries.erase(it);
			}
			return learns;
		}
	}

	std::uint64_t CasterKey(RE::FormID formID, RE::MagicSystem::CastingSource source) {
		return (std::uint64_t(formID) << 8) | std::to_underlying(source);
	}

	float SpeedMultiplier(RE::Actor* actor, const RE::MagicItem* spell) {
		if (auto skillUsage = SkillUsage(spell, actor, kChargeTime); skillUsage) {
			return skillUsage.LerpSkillFactor(actor);
		}
		return 1.0f;
	}

	float PopSpellHoldDuration(RE::Actor* actor, RE::MagicSystem::CastingSource source) {
		if (!actor) {
			return 0.0f;
		}

		RE::BSSpinLockGuard lock{ entriesLock };

		auto it = entries.find(CasterKey(actor->GetFormID(), source));
		if (it == entries.end()) {
			return 0.0f;
		}

		const float duration = it->second.holdDuration;
		it->second.holdDuration = 0.0f;

		if (it->second.lastState == RE::MagicCaster::State::kNone) {
			entries.erase(it);
		}

		return duration;
	}

	float ConcentrationDuration(RE::Actor* actor, RE::MagicSystem::CastingSource source) {
		if (!actor) {
			return 0.0f;
		}

		RE::BSSpinLockGuard lock{ entriesLock };

		auto it = entries.find(CasterKey(actor->GetFormID(), source));
		return it != entries.end() ? it->second.concentrationDuration : 0.0f;
	}

	void Reset() {
		RE::BSSpinLockGuard lock{ entriesLock };
		entries.clear();
	}
}

namespace NLA::ChargeTime
{
	struct MagicCaster_Update
	{
		using Target = RE::ActorMagicCaster;

		static inline constexpr VariantIndex index{ 0x1D, 0x1D, 0x1F };

		static void thunk(RE::ActorMagicCaster* caster, float delta) {
			if (caster) {
				const auto state = caster->state.get();

				if (Track(caster, state, delta) && IsCharging(state)) {
					delta *= SpeedMultiplier(caster->actor, caster->currentSpell);
				}
			}

			func(caster, delta);
		}

		static inline REL::Relocation<decltype(thunk)> func;

		static inline void pre_hook() { logger::info("Installing ChargeTime hook..."); }
		static inline void post_hook() { logger::info("Installed ChargeTime hook."); }
	};

	void Install() {
		stl::install_hook<MagicCaster_Update>();
	}
}
