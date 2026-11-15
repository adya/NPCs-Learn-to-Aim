#include "HandThreat.h"

namespace NLA::Skillshots::HandThreat
{
	namespace
	{
		/// A hand has to be this much more dangerous than the other to be called stronger.
		constexpr float kDamageThreshold = 1.25f;

		float GetWeaponDamage(RE::Actor* actor, RE::TESObjectWEAP* weapon, float tempering) {
			using func_t = float(RE::Actor*, RE::TESObjectWEAP*, RE::TESAmmo*, float);
			static REL::Relocation<func_t> func{ RELOCATION_ID(39215, 40291) };
			return func(actor, weapon, nullptr, tempering);
		}

		bool IsTwoHanded(RE::TESForm* form) {
			auto* weapon = form ? form->As<RE::TESObjectWEAP>() : nullptr;
			return weapon && (weapon->IsTwoHandedAxe() || weapon->IsTwoHandedSword() || weapon->IsBow() || weapon->IsCrossbow());
		}

		float DamageOf(RE::MagicItem* spell) {
			float total = 0.0f;
			for (auto* effect : spell->effects) {
				if (effect && effect->baseEffect && effect->baseEffect->IsHostile()) {
					total += effect->effectItem.magnitude * static_cast<float>(std::max<std::uint32_t>(effect->effectItem.duration, 1));
				}
			}
			return total;
		}

		float DamageOf(RE::Actor* actor, bool left) {
			auto* form = actor->GetEquippedObject(left);
			if (!form) {
				return 0.0f;
			}

			if (auto* weapon = form->As<RE::TESObjectWEAP>(); weapon) {
				if (weapon->IsHandToHandMelee()) {
					return 0.0f;
				}

				float          tempering = 1.0f;
				RE::MagicItem* enchantment = weapon->formEnchanting;
				if (auto* entry = actor->GetEquippedEntryData(left); entry) {
					if (auto* ench = entry->GetEnchantment(); ench) {
						enchantment = ench;
					}
					if (entry->extraLists) {
						for (auto* xList : *entry->extraLists) {
							if (auto* health = xList ? xList->GetByType<RE::ExtraHealth>() : nullptr; health) {
								tempering = std::max(tempering, health->health);
								break;
							}
						}
					}
				}

				const float enchanted = enchantment ? DamageOf(enchantment) : 0.0f;
				if (weapon->IsStaff()) {
					return enchanted;
				}
				return GetWeaponDamage(actor, weapon, tempering) + enchanted;
			}

			if (auto* spell = form->As<RE::MagicItem>(); spell) {
				return spell->IsHostile() ? DamageOf(spell) : 1.0f;  // non-hostile spells considered to be low priority, but still above shields.
			}
			return 0.0f;
		}
	}

	Hand StrongerHand(RE::Actor* actor) {
		if (IsTwoHanded(actor->GetEquippedObject(true)) || IsTwoHanded(actor->GetEquippedObject(false))) {
			return Hand::kEven;
		}

		const float left = DamageOf(actor, true);
		const float right = DamageOf(actor, false);
		if (right > left * kDamageThreshold) {
			return Hand::kRight;
		}
		if (left > right * kDamageThreshold) {
			return Hand::kLeft;
		}
		return Hand::kEven;
	}
}
