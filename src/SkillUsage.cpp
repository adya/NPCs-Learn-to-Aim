#include "SkillUsage.h"
#include <numbers>

namespace NLA
{
	SkillUsage::SkillUsage(const RE::TESObjectWEAP* weapon, RE::Actor* attacker, SkillUsageContext context) {
		if (!weapon || !attacker || !Options::NPC.ShouldLearn(attacker))
			return;

		if (weapon->GetWeaponType() == RE::WEAPON_TYPE::kCrossbow) {
			weaponType = kCrossbow;
			if (Options::For(attacker).crossbowAiming) {
				skill = RE::ActorValue::kArchery;
				multiplier = Options::For(attacker).MultiplierFor(context).crossbow;
			}
		} else if (weapon->GetWeaponType() == RE::WEAPON_TYPE::kBow) {
			weaponType = kBow;
			if (Options::For(attacker).bowAiming) {
				skill = RE::ActorValue::kArchery;
				multiplier = Options::For(attacker).MultiplierFor(context).bow;
			}
		}

		if (Options::For(attacker).blindnessAffectsAiming && attacker->AsActorValueOwner()->GetActorValue(RE::ActorValue::kBlindness) > 0) {
			multiplier -= (1 - Options::For(attacker).MultiplierFor(context).blindness);
		}
	}

	SkillUsage::SkillUsage(const RE::MagicItem* spell, RE::Actor* attacker, SkillUsageContext context) {
		if (!spell || !attacker || !Options::NPC.ShouldLearn(attacker))
			return;

		// Only support aimed spells for now.
		if (spell->GetDelivery() != RE::MagicSystem::Delivery::kAimed)
			return;

		RE::ActorValue magicSkill = RE::ActorValue::kNone;
		if (auto effect = spell->GetCostliestEffectItem(); effect) {
			if (auto base = effect->baseEffect; base) {
				magicSkill = base->GetMagickSkill();
			}
		}

		auto highestMagicSkill = [&attacker]() {
			std::array<RE::ActorValue, 5> properties = { RE::ActorValue::kAlteration, RE::ActorValue::kConjuration, RE::ActorValue::kDestruction, RE::ActorValue::kIllusion, RE::ActorValue::kRestoration };

			return *std::ranges::max_element(properties, [&attacker](RE::ActorValue a, RE::ActorValue b) {
				return attacker->AsActorValueOwner()->GetActorValue(a) < attacker->AsActorValueOwner()->GetActorValue(b);
			});
		};

		const bool isConcentration = spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration;

		if (!Options::For(attacker).concentrationSpellsAiming && isConcentration) {
			return;
		}

		switch (spell->GetSpellType()) {
		case RE::MagicSystem::SpellType::kSpell:
		case RE::MagicSystem::SpellType::kLeveledSpell:
			weaponType = kSpell;
			if (Options::For(attacker).spellAiming) {
				const bool useHighestMagicSkill = isConcentration ? Options::For(attacker).concentrationSpellsUseHighestMagicSkill : Options::For(attacker).spellsUseHighestMagicSkill;
				skill = useHighestMagicSkill ? highestMagicSkill() : magicSkill;
				multiplier = isConcentration ? Options::For(attacker).MultiplierFor(context).concentrationSpell : Options::For(attacker).MultiplierFor(context).spell;
			}
			break;
		case RE::MagicSystem::SpellType::kStaffEnchantment:
			weaponType = kStaff;
			if (Options::For(attacker).staffAiming) {
				if (Options::For(attacker).stavesUseEnchantingSkill) {
					skill = RE::ActorValue::kEnchanting;
				} else if (Options::For(attacker).stavesUseHighestMagicSkill) {
					skill = highestMagicSkill();
				} else {
					skill = magicSkill;
				}
				multiplier = Options::For(attacker).MultiplierFor(context).staff;
			}
			break;
		default:
			break;
		}

		if (Options::For(attacker).blindnessAffectsAiming && attacker->AsActorValueOwner()->GetActorValue(RE::ActorValue::kBlindness) > 0) {
			multiplier -= (1 - Options::For(attacker).MultiplierFor(context).blindness);
		}
	}
#ifndef NDEBUG
	float SkillUsage::OriginalSkillFactor(RE::Actor* actor) const {
		if (!actor || this->skill == RE::ActorValue::kNone)
			return 1.0f;
		const auto skill = EffectiveSkill(actor);
		return (std::min)((std::max)((100.0f - skill) * 0.01f, 0.0f), 1.0f);
	}
#endif

	float SkillUsage::SkillFactor(RE::Actor* actor) const {
		if (!actor || this->skill == RE::ActorValue::kNone)
			return 1.0f;
		const auto skill = EffectiveSkill(actor);
		return std::exp(0.025f * (10 - skill));
	}

	float SkillUsage::LerpSkillFactor(RE::Actor* actor, float minFactor, float maxFactor) const {
		if (maxFactor <= minFactor)
			return minFactor;

		constexpr float rate = 0.025f;
		const float     level = std::clamp(EffectiveSkill(actor), 0.0f, 100.0f);
		const float     progress = (1.0f - std::exp(-rate * level)) / (1.0f - std::exp(-rate * 100.0f));

		return minFactor + (maxFactor - minFactor) * progress;
	}

	float SkillUsage::MaxSpread(RE::Actor* actor) const {
		constexpr float toRadians = std::numbers::pi / 180.0f;
		return SkillFactor(actor) * Settings::fBowNPCSpreadAngle() * toRadians;
	}

	RE::NiPoint2 RandomSpreadOffset(float maxSpread) {
		std::uniform_real_distribution<float> angleRND(0, 2 * std::numbers::pi_v<float>);
		std::normal_distribution<float>       spreadRND(0, 0.7f * maxSpread);

		float spread = std::abs(spreadRND(rnd));
		float angle = angleRND(rnd);  // pick a random direction on a circle.

		return { std::sin(angle) * spread, std::cos(angle) * spread };
	}
}
