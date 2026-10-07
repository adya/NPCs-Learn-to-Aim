#include "DrawSpeed.h"
#include "Hooking.h"
#include "Options.h"
#include "SkillUsage.h"

#include <unordered_map>

namespace NLA::Draw
{
	namespace
	{
		bool IsFullyDrawn(RE::ATTACK_STATE_ENUM state) {
			return state == RE::ATTACK_STATE_ENUM::kBowDrawn;
		}

		struct Entry
		{
			RE::ATTACK_STATE_ENUM lastState{ RE::ATTACK_STATE_ENUM::kNone };

			std::uint32_t drawTimestamp{ 0 };

			float fullDrawDuration{ 0.0f };
		};

		RE::BSSpinLock                        entriesLock;
		std::unordered_map<RE::FormID, Entry> entries;

		RE::Actor* ActorFromWeaponSpeedOwner(RE::ActorValueOwner* owner) {
			return owner ? &REL::RelocateMemberIfNewer<RE::Actor>(SKSE::RUNTIME_SSE_1_6_629, owner, -0xB0, -0xB8) : nullptr;
		}

		void TrackHoldDuration(RE::Actor* attacker) {
			const auto state = attacker->AsActorState()->GetAttackState();
			const auto formID = attacker->GetFormID();
			const auto nowMs = RE::GetDurationOfApplicationRunTime();

			RE::BSSpinLockGuard lock{ entriesLock };
			auto&               entry = entries[formID];

			if (entry.lastState != state) {
				if (IsFullyDrawn(entry.lastState)) {
					entry.fullDrawDuration = (nowMs - entry.drawTimestamp) / 1000.0f;
					entry.drawTimestamp = 0;
				}
				if (IsFullyDrawn(state)) {
					entry.drawTimestamp = nowMs;
				}
				entry.lastState = state;
			}

			if (state == RE::ATTACK_STATE_ENUM::kNone && entry.fullDrawDuration == 0.0f) {
				entries.erase(formID);
			}
		}
	}

	float SpeedMultiplier(RE::Actor* attacker, const RE::TESObjectWEAP* weapon) {
		if (!attacker || !weapon)
			return 1.0f;

		const auto weaponType = weapon->GetWeaponType();
		if (weaponType != RE::WEAPON_TYPE::kBow && weaponType != RE::WEAPON_TYPE::kCrossbow)
			return 1.0f;

		auto skillUsage = SkillUsage(weapon, attacker, kChargeTime);
		if (!skillUsage)
			return 1.0f;

		return skillUsage.LerpSkillFactor(attacker);
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

namespace NLA::Draw
{
	struct PollChannelUpdateImpl_GetWeaponSpeed
	{
		static inline constexpr REL::RelocationID  relocation{ 41694, 42779 };
		static inline constexpr REL::VariantOffset offset{ 0x29, 0x29, 0x29 };
		static inline constexpr Signature          signature{ "E8 ?? ?? ?? ?? 48 8B 44 24 50" };

		static inline void post_hook() { logger::info("Installed draw speed skill scaling."); }

		static float thunk(RE::ActorValueOwner* owner, RE::TESObjectWEAP* weapon, bool leftHand) {
			float vanillaSpeed = func(owner, weapon, leftHand);

			auto attacker = ActorFromWeaponSpeedOwner(owner);
			if (!attacker || !weapon)
				return vanillaSpeed;

			const auto weaponType = weapon->GetWeaponType();
			if (weaponType != RE::WEAPON_TYPE::kBow && weaponType != RE::WEAPON_TYPE::kCrossbow)
				return vanillaSpeed;

			TrackHoldDuration(attacker);

			return vanillaSpeed * SpeedMultiplier(attacker, weapon);
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};

	/// Scales vanilla weapon speed by the bow/crossbow draw speed multiplier.
	template <typename Call>
	struct ScaledWeaponSpeed_Proxy
	{
		static float thunk(RE::ActorValueOwner* owner, RE::TESObjectWEAP* weapon, bool leftHand) {
			float vanillaSpeed = Call::func(owner, weapon, leftHand);
			return vanillaSpeed * SpeedMultiplier(ActorFromWeaponSpeedOwner(owner), weapon);
		}
	};

	struct Projectile_Launch_GetWeaponSpeed
	{
		static inline constexpr REL::RelocationID  relocation{ 42928, 44108 };
		static inline constexpr REL::VariantOffset offset{ 0x5F9, 0x5CB, 0x5F9 };
		static inline constexpr Signature          signature{ "E8 ?? ?? ?? ?? 0F 28 C8 0F 28 C6 E8 ?? ?? ?? ?? F3 0F 11 83" };

		static inline void post_hook() { logger::info("Installed scaled draw speed power check."); }

		using Proxy = ScaledWeaponSpeed_Proxy<Projectile_Launch_GetWeaponSpeed>;
		static inline REL::Relocation<decltype(Proxy::thunk)> func;
	};

	struct Actor_ProcessVATSAttack_GetWeaponSpeed
	{
		static inline constexpr REL::RelocationID  relocation{ 40230, 41233 };
		static inline constexpr REL::VariantOffset offset{ 0x871, 0x947 };

		// For VR there is no kill-cam, so we skip this hook.
		static inline constexpr REL::Module::Runtime runtime{ REL::Module::Runtime::SE | REL::Module::Runtime::AE };

		static inline constexpr VariantSignature<"E8 ?? ?? ?? ?? 0F 28 C8 0F 28 C6 E8 ?? ?? ?? ?? 48 8B 7C 24 58",
		                                         "E8 ?? ?? ?? ?? 0F 28 C8 0F 28 C6 E8 ?? ?? ?? ?? 0F 2F 05 ?? ?? ?? ??">
			signature;

		static inline void post_hook() { logger::info("Installed scaled draw speed ranged kill-cam power check."); }

		using Proxy = ScaledWeaponSpeed_Proxy<Actor_ProcessVATSAttack_GetWeaponSpeed>;
		static inline REL::Relocation<decltype(Proxy::thunk)> func;
	};

	void Install() {
		stl::install_hook<PollChannelUpdateImpl_GetWeaponSpeed>();
		stl::install_hook<Projectile_Launch_GetWeaponSpeed>();
		stl::install_hook<Actor_ProcessVATSAttack_GetWeaponSpeed>();
	}

	void Reset() {
		RE::BSSpinLockGuard lock{ entriesLock };
		entries.clear();
	}
}
