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
			std::get<float>(multiplier) -= (1 - Options::For(attacker).MultiplierFor(context).blindness);
		}
	}

	SkillUsage::SkillUsage(const RE::MagicItem* spell, RE::Actor* attacker, SkillUsageContext context) {
		if (!spell || !attacker || !Options::NPC.ShouldLearn(attacker))
			return;

		const auto& options = Options::For(attacker);

		RE::ActorValue magicSkill = RE::ActorValue::kNone;
		if (auto effect = spell->GetCostliestEffectItem(); effect) {
			if (auto base = effect->baseEffect; base) {
				magicSkill = base->GetMagickSkill();
			}
		}

		const bool isConcentration = spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration;
		const bool isFireAndForget = spell->GetCastingType() == RE::MagicSystem::CastingType::kFireAndForget;

		if (context == kChargeTime) {
			switch (spell->GetSpellType()) {
			case RE::MagicSystem::SpellType::kSpell:
			case RE::MagicSystem::SpellType::kLeveledSpell:
				weaponType = kSpell;
				if (isConcentration && options.concentrationSpellChargeTimeScaling) {
					const bool useHighestMagicSkill = options.concentrationSpellsUseHighestMagicSkill;
					skill = useHighestMagicSkill ? HighestMagicSkill(attacker) : magicSkill;
					multiplier = Range{ options.concentrationSpellChargeSpeedMinMultiplier, options.concentrationSpellChargeSpeedMaxMultiplier };
				} else if (isFireAndForget && options.spellChargeTimeScaling) {
					const bool useHighestMagicSkill = options.spellsUseHighestMagicSkill;
					skill = useHighestMagicSkill ? HighestMagicSkill(attacker) : magicSkill;
					multiplier = Range{ options.spellChargeSpeedMinMultiplier, options.spellChargeSpeedMaxMultiplier };
				}
				break;
			case RE::MagicSystem::SpellType::kStaffEnchantment:
				weaponType = kStaff;
				if (options.staffChargeTimeScaling) {
					if (options.stavesUseEnchantingSkill) {
						skill = RE::ActorValue::kEnchanting;
					} else if (options.stavesUseHighestMagicSkill) {
						skill = HighestMagicSkill(attacker);
					} else {
						skill = magicSkill;
					}
					multiplier = Range{ options.staffChargeSpeedMinMultiplier, options.staffChargeSpeedMaxMultiplier };
				}
				break;
			default:
				// TODO: Think about fitting scrolls into all of this.
				break;
			}
			return;
		}

		// Only support aimed spells.
		if (spell->GetDelivery() != RE::MagicSystem::Delivery::kAimed)
			return;

		switch (spell->GetSpellType()) {
		case RE::MagicSystem::SpellType::kSpell:
		case RE::MagicSystem::SpellType::kLeveledSpell:
			weaponType = kSpell;
			if (isConcentration && options.concentrationSpellsAiming) {
				const bool useHighestMagicSkill = options.concentrationSpellsUseHighestMagicSkill;
				skill = useHighestMagicSkill ? HighestMagicSkill(attacker) : magicSkill;
				multiplier = options.MultiplierFor(context).concentrationSpell;
			} else if (isFireAndForget && options.spellAiming) {
				const bool useHighestMagicSkill = options.spellsUseHighestMagicSkill;
				skill = useHighestMagicSkill ? HighestMagicSkill(attacker) : magicSkill;
				multiplier = options.MultiplierFor(context).spell;
			}
			break;
		case RE::MagicSystem::SpellType::kStaffEnchantment:
			weaponType = kStaff;
			if (options.staffAiming) {
				if (options.stavesUseEnchantingSkill) {
					skill = RE::ActorValue::kEnchanting;
				} else if (options.stavesUseHighestMagicSkill) {
					skill = HighestMagicSkill(attacker);
				} else {
					skill = magicSkill;
				}
				multiplier = options.MultiplierFor(context).staff;
			}
			break;
		default:
			break;
		}

		if (options.blindnessAffectsAiming && attacker->AsActorValueOwner()->GetActorValue(RE::ActorValue::kBlindness) > 0) {
			std::get<float>(multiplier) -= (1 - options.MultiplierFor(context).blindness);
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
		if (maxFactor == minFactor)
			return minFactor;

		constexpr float rate = 0.025f;
		const float     level = std::clamp(EffectiveSkill(actor), 0.0f, 100.0f);
		const float     progress = (1.0f - std::exp(-rate * level)) / (1.0f - std::exp(-rate * 100.0f));

		return minFactor + (maxFactor - minFactor) * progress;
	}

	float SkillUsage::LerpSkillFactor(RE::Actor* actor) const {
		const Range* range = std::get_if<Range>(&multiplier);
		return range ? LerpSkillFactor(actor, range->min, range->max) : 1.0f;
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

	RE::ActorValue HighestMagicSkill(RE::Actor* actor) {
		std::array<RE::ActorValue, 5> properties = { RE::ActorValue::kAlteration, RE::ActorValue::kConjuration, RE::ActorValue::kDestruction, RE::ActorValue::kIllusion, RE::ActorValue::kRestoration };

		return *std::ranges::max_element(properties, [&actor](RE::ActorValue a, RE::ActorValue b) {
			return actor->AsActorValueOwner()->GetActorValue(a) < actor->AsActorValueOwner()->GetActorValue(b);
		});
	}
}
