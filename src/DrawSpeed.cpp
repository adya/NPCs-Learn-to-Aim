#include "DrawSpeed.h"
#include "Helpers.h"
#include "Hooking.h"
#include "SkillUsage.h"

namespace NLA::Draw
{
	float SpeedMultiplier(RE::Actor* attacker, const RE::TESObjectWEAP* weapon) {
		if (auto skillUsage = SkillUsage(weapon, attacker, kChargeTime); skillUsage) {
			return skillUsage.SkillFactor(attacker);
		}
		return 1.0f;
	}
}

namespace NLA::Draw
{
	struct PollChannelUpdateImpl_GetWeaponSpeed
	{
		static inline constexpr REL::RelocationID  relocation{ 41694, 42779 };
		static inline constexpr REL::VariantOffset offset{ 0x29, 0x29, 0x29 };
		static inline constexpr Signature          signature{ "E8 ?? ?? ?? ?? 48 8B 44 24 50" };

		static inline void pre_hook() { logger::debug("Installing draw speed skill scaling..."); }
		static inline void post_hook() { logger::info("🪝 Installed draw speed skill scaling."); }

		static float thunk(RE::ActorValueOwner* owner, RE::TESObjectWEAP* weapon, bool leftHand) {
			float vanillaSpeed = func(owner, weapon, leftHand);

			auto attacker = Actor::FromActorValueOwner(owner);
			if (!attacker || !weapon)
				return vanillaSpeed;

			if (!Weapon::IsBowOrCrossbow(weapon->GetWeaponType()))
				return vanillaSpeed;

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
			return vanillaSpeed * SpeedMultiplier(Actor::FromActorValueOwner(owner), weapon);
		}
	};

	struct Projectile_Launch_GetWeaponSpeed
	{
		static inline constexpr REL::RelocationID  relocation{ 42928, 44108 };
		static inline constexpr REL::VariantOffset offset{ 0x5F9, 0x5CB, 0x5F9 };
		static inline constexpr Signature          signature{ "E8 ?? ?? ?? ?? 0F 28 C8 0F 28 C6 E8 ?? ?? ?? ?? F3 0F 11 83" };

		static inline void pre_hook() { logger::debug("Installing scaled draw speed power check..."); }
		static inline void post_hook() { logger::info("🪝 Installed scaled draw speed power check."); }

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

		static inline void pre_hook() { logger::debug("Installing scaled draw speed ranged kill-cam power check..."); }
		static inline void post_hook() { logger::info("🪝 Installed scaled draw speed ranged kill-cam power check."); }

		using Proxy = ScaledWeaponSpeed_Proxy<Actor_ProcessVATSAttack_GetWeaponSpeed>;
		static inline REL::Relocation<decltype(Proxy::thunk)> func;
	};

	void Install() {
		stl::install_hook<PollChannelUpdateImpl_GetWeaponSpeed>();
		stl::install_hook<Projectile_Launch_GetWeaponSpeed>();
		stl::install_hook<Actor_ProcessVATSAttack_GetWeaponSpeed>();
	}
}
