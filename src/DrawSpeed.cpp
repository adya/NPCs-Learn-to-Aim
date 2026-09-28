#include "DrawSpeed.h"
#ifndef NDEBUG
#	include "Aiming.h"
#endif
#include "Hooking.h"
#include "Options.h"
#include "SkillUsage.h"

#include <unordered_map>
#include <utility>

namespace NLA::Draw
{
	namespace
	{
		bool IsFullyDrawn(RE::ATTACK_STATE_ENUM state) {
			return state == RE::ATTACK_STATE_ENUM::kBowDrawn;
		}
#ifndef NDEBUG
		constexpr float kFullDrawLogInterval = 0.25f;
#endif
		struct Entry
		{
			RE::ATTACK_STATE_ENUM lastState{ RE::ATTACK_STATE_ENUM::kNone };

			std::uint32_t drawTimestamp{ 0 };

			float fullDrawDuration{ 0.0f };

#ifndef NDEBUG
			float lastLoggedHeld{ 0.0f };
#endif
		};

#ifndef NDEBUG
		float PercentSpreadChange(RE::Actor* attacker, RE::TESObjectWEAP* weapon, float heldDuration) {
			float fatigueThreshold = 0.0f;
			if (auto fatigueUsage = SkillUsage(weapon, attacker, kFatigueThreshold); fatigueUsage) {
				fatigueThreshold = fatigueUsage.LerpSkillFactor(attacker);
			}
			const auto& options = Options::For(attacker);
			const float scale = BowSpreadScale(heldDuration, options.bowSteadying, options.bowFatigue, fatigueThreshold);
			return (scale - 1.0f) * 100.0f;
		}
#endif

		RE::BSSpinLock                        entriesLock;
		std::unordered_map<RE::FormID, Entry> entries;
	}

	struct PollChannelUpdateImpl_GetWeaponSpeed
	{
		static inline constexpr REL::RelocationID  relocation{ 41694, 42779 };
		static inline constexpr REL::VariantOffset offset{ 0x29, 0x29, 0x29 };
		static inline constexpr Signature          signature{ "E8 ?? ?? ?? ?? 48 8B 44 24 50" };

		static inline void post_hook() { logger::info("Installed bow/crossbow draw/reload speed skill scaling"); }

		static float thunk(RE::ActorValueOwner* owner, RE::TESObjectWEAP* weapon, bool leftHand) {
			float vanillaSpeed = func(owner, weapon, leftHand);

			auto attacker = owner ? &REL::RelocateMemberIfNewer<RE::Actor>(SKSE::RUNTIME_SSE_1_6_629, owner, -0xB0, -0xB8) : nullptr;
			if (!attacker || !weapon)
				return vanillaSpeed;

			const auto weaponType = weapon->GetWeaponType();
			if (weaponType != RE::WEAPON_TYPE::kBow && weaponType != RE::WEAPON_TYPE::kCrossbow)
				return vanillaSpeed;

			const bool stateChanged = TrackHoldDuration(attacker, weapon);

			auto skillUsage = SkillUsage(weapon, attacker, kChargeTime);
			if (!skillUsage)
				return vanillaSpeed;

			const float multiplier = skillUsage.LerpSkillFactor(attacker);
#ifndef NDEBUG
			if (stateChanged) {
				logger::debug("DrawSpeed: {} draws {} at x{:.3f} speed ({:.3f} -> {:.3f})",
				              attacker->GetName(),
				              weapon->GetName(),
				              multiplier,
				              vanillaSpeed,
				              vanillaSpeed * multiplier);
			}
#endif
			return vanillaSpeed * multiplier;
		}

		/// Returns true if the actor's attack state changed since the last call.
		static bool TrackHoldDuration(RE::Actor* attacker, RE::TESObjectWEAP* weapon) {
			const auto state = attacker->AsActorState()->GetAttackState();
			const auto formID = attacker->GetFormID();
			const auto nowMs = RE::GetDurationOfApplicationRunTime();

			RE::BSSpinLockGuard lock{ entriesLock };
			auto&               entry = entries[formID];

			const bool changed = entry.lastState != state;
			if (changed) {
				if (IsFullyDrawn(entry.lastState)) {
					entry.fullDrawDuration = (nowMs - entry.drawTimestamp) / 1000.0f;
					entry.drawTimestamp = 0;
#ifndef NDEBUG
					entry.lastLoggedHeld = 0.0f;
					logger::debug("FullDraw: {} left full draw after holding it for {:.3f}s ({:+.0f}% spread) (state {} -> {})",
					              attacker->GetName(),
					              entry.fullDrawDuration,
					              PercentSpreadChange(attacker, weapon, entry.fullDrawDuration),
					              std::to_underlying(entry.lastState),
					              std::to_underlying(state));
#endif
				}
				if (IsFullyDrawn(state)) {
					entry.drawTimestamp = nowMs;
				}
				entry.lastState = state;
			}

			if (IsFullyDrawn(state)) {
				const float heldDuration = (nowMs - entry.drawTimestamp) / 1000.0f;
#ifndef NDEBUG
				if (heldDuration - entry.lastLoggedHeld >= kFullDrawLogInterval) {
					entry.lastLoggedHeld = heldDuration;
					logger::debug("FullDraw: {} holding {} at full draw for {:.2f}s ({:+.0f}% spread)",
					              attacker->GetName(),
					              weapon->GetName(),
					              heldDuration,
					              PercentSpreadChange(attacker, weapon, heldDuration));
				}
#endif
			}

			if (state == RE::ATTACK_STATE_ENUM::kNone && entry.fullDrawDuration == 0.0f) {
				entries.erase(formID);
			}

			return changed;
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install() {
		stl::install_hook<PollChannelUpdateImpl_GetWeaponSpeed>();
	}

	void Reset() {
		RE::BSSpinLockGuard lock{ entriesLock };
		entries.clear();
	}

	float PopBowFullDrawDuration(RE::Actor* actor) {
		if (!actor) {
			return 0.0f;
		}

		RE::BSSpinLockGuard lock{ entriesLock };

		auto it = entries.find(actor->GetFormID());
		if (it == entries.end()) {
			return 0.0f;
		}

		const float duration = it->second.fullDrawDuration;
		it->second.fullDrawDuration = 0.0f;

		if (it->second.lastState == RE::ATTACK_STATE_ENUM::kNone) {
			entries.erase(it);
		}

		return duration;
	}
}
