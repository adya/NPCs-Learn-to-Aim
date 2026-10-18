#include "SkillUsage.h"
#include <numbers>

namespace NLA
{
	SkillUsage::SkillUsage(const RE::TESObjectWEAP* weapon, RE::Actor* attacker, SkillUsageContext context) {
		if (!weapon || !attacker || !Options::NPC.ShouldLearn(attacker))
			return;

		const auto& options = Options::For(attacker);

		switch (context) {
		case kChargeTime:
			if (weapon->GetWeaponType() == RE::WEAPON_TYPE::kCrossbow) {
				weaponType = kCrossbow;
				if (options.crossbowDrawSpeed.enabled) {
					skill = RE::ActorValue::kArchery;
					multiplier = Range{ options.crossbowDrawSpeed.minMultiplier, options.crossbowDrawSpeed.maxMultiplier };
				}
			} else if (weapon->GetWeaponType() == RE::WEAPON_TYPE::kBow) {
				weaponType = kBow;
				if (options.bowDrawSpeed.enabled) {
					skill = RE::ActorValue::kArchery;
					multiplier = Range{ options.bowDrawSpeed.minMultiplier, options.bowDrawSpeed.maxMultiplier };
				}
			}
			break;
		case kFatigueThreshold:
			if (weapon->GetWeaponType() == RE::WEAPON_TYPE::kBow) {
				weaponType = kBow;
				if (options.bowFatigue.enabled) {
					skill = RE::ActorValue::kArchery;
					multiplier = Range{ options.bowFatigue.minThreshold, options.bowFatigue.maxThreshold };
				}
			}
			break;
		case kHeadshot:
			{
				// Headshots are an NPC-only feature.
				if (attacker->IsPlayerRef())
					break;

				const auto& headshots = Options::NPC.headshots;
				if (weapon->GetWeaponType() == RE::WEAPON_TYPE::kCrossbow) {
					weaponType = kCrossbow;
					if (headshots.crossbow) {
						skill = RE::ActorValue::kArchery;
						multiplier = Range{ headshots.crossbowMinChance, headshots.crossbowMaxChance };
					}
				} else if (weapon->GetWeaponType() == RE::WEAPON_TYPE::kBow) {
					weaponType = kBow;
					if (headshots.bow) {
						skill = RE::ActorValue::kArchery;
						multiplier = Range{ headshots.bowMinChance, headshots.bowMaxChance };
					}
				}
				break;
			}
		case kAccuracy:
		case kPrecision:
			{
				if (weapon->GetWeaponType() == RE::WEAPON_TYPE::kCrossbow) {
					weaponType = kCrossbow;
					if (options.crossbowAiming) {
						skill = RE::ActorValue::kArchery;
						multiplier = options.MultiplierFor(context).crossbow;
					}
				} else if (weapon->GetWeaponType() == RE::WEAPON_TYPE::kBow) {
					weaponType = kBow;
					if (options.bowAiming) {
						skill = RE::ActorValue::kArchery;
						multiplier = options.MultiplierFor(context).bow;
					}
				}

				if (options.blindnessAffectsAiming) {
					const float blindness = std::clamp(attacker->AsActorValueOwner()->GetActorValue(RE::ActorValue::kBlindness), 0.0f, 1.0f);
					std::get<float>(multiplier) -= (1 - options.MultiplierFor(context).blindness) * blindness;
				}
				break;
			}
		default:
			break;
		}
	}

	RE::ActorValue MagicSkillFor(const RE::MagicItem* spell, RE::Actor* caster) {
		if (!spell || !caster)
			return RE::ActorValue::kNone;

		const auto& options = Options::For(caster);

		RE::ActorValue magicSkill = RE::ActorValue::kNone;
		if (auto effect = spell->GetCostliestEffectItem(); effect) {
			if (auto base = effect->baseEffect; base) {
				magicSkill = base->GetMagickSkill();
			}
		}

		const bool isConcentration = spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration;
		const bool isFireAndForget = spell->GetCastingType() == RE::MagicSystem::CastingType::kFireAndForget;

		switch (spell->GetSpellType()) {
		case RE::MagicSystem::SpellType::kSpell:
		case RE::MagicSystem::SpellType::kLeveledSpell:
			if (isConcentration) {
				return options.concentrationSpellsUseHighestMagicSkill ? HighestMagicSkill(caster) : magicSkill;
			} else if (isFireAndForget) {
				return options.spellsUseHighestMagicSkill ? HighestMagicSkill(caster) : magicSkill;
			}
			return magicSkill;
		case RE::MagicSystem::SpellType::kStaffEnchantment:
			{
				const bool useEnchanting = isConcentration ? options.concentrationStavesUseEnchantingSkill : options.stavesUseEnchantingSkill;
				const bool useHighest = isConcentration ? options.concentrationStavesUseHighestMagicSkill : options.stavesUseHighestMagicSkill;
				if (useEnchanting) {
					return RE::ActorValue::kEnchanting;
				} else if (useHighest) {
					return HighestMagicSkill(caster);
				}
				return magicSkill;
			}
		case RE::MagicSystem::SpellType::kScroll:
			if (options.scrollsUseEnchantingSkill) {
				return RE::ActorValue::kEnchanting;
			} else if (options.scrollsUseHighestMagicSkill) {
				return HighestMagicSkill(caster);
			}
			return magicSkill;
		default:
			return magicSkill;
		}
	}

	SkillUsage::SkillUsage(const RE::MagicItem* spell, RE::Actor* attacker, SkillUsageContext context) {
		if (!spell || !attacker || !Options::NPC.ShouldLearn(attacker))
			return;

		const auto& options = Options::For(attacker);

		const bool isConcentration = spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration;
		const bool isFireAndForget = spell->GetCastingType() == RE::MagicSystem::CastingType::kFireAndForget;
		const bool isAimed = spell->GetDelivery() == RE::MagicSystem::Delivery::kAimed;

		switch (context) {
		case kChargeTime:
			switch (spell->GetSpellType()) {
			case RE::MagicSystem::SpellType::kSpell:
			case RE::MagicSystem::SpellType::kLeveledSpell:
				weaponType = kSpell;
				if (isConcentration && options.concentrationSpellChargeSpeed.enabled) {
					skill = MagicSkillFor(spell, attacker);
					multiplier = Range{ options.concentrationSpellChargeSpeed.minMultiplier, options.concentrationSpellChargeSpeed.maxMultiplier };
				} else if (isFireAndForget && options.spellChargeSpeed.enabled) {
					skill = MagicSkillFor(spell, attacker);
					multiplier = Range{ options.spellChargeSpeed.minMultiplier, options.spellChargeSpeed.maxMultiplier };
				}
				break;
			case RE::MagicSystem::SpellType::kStaffEnchantment:
				weaponType = kStaff;
				if (isConcentration && options.concentrationStaffChargeSpeed.enabled) {
					skill = MagicSkillFor(spell, attacker);
					multiplier = Range{ options.concentrationStaffChargeSpeed.minMultiplier, options.concentrationStaffChargeSpeed.maxMultiplier };
				} else if (isFireAndForget && options.staffChargeSpeed.enabled) {
					skill = MagicSkillFor(spell, attacker);
					multiplier = Range{ options.staffChargeSpeed.minMultiplier, options.staffChargeSpeed.maxMultiplier };
				}
				break;
			case RE::MagicSystem::SpellType::kScroll:
				weaponType = kScroll;
				if (options.scrollChargeSpeed.enabled) {
					skill = MagicSkillFor(spell, attacker);
					multiplier = Range{ options.scrollChargeSpeed.minMultiplier, options.scrollChargeSpeed.maxMultiplier };
				}
				break;
			default:
				break;
			}
			break;

		case kFatigueThreshold:
			switch (spell->GetSpellType()) {
			case RE::MagicSystem::SpellType::kSpell:
			case RE::MagicSystem::SpellType::kLeveledSpell:
				weaponType = kSpell;
				if (options.spellFatigue.enabled) {
					skill = MagicSkillFor(spell, attacker);
					multiplier = Range{ options.spellFatigue.minThreshold, options.spellFatigue.maxThreshold };
				}
				break;
			case RE::MagicSystem::SpellType::kStaffEnchantment:
				weaponType = kStaff;
				if (options.staffFatigue.enabled) {
					skill = MagicSkillFor(spell, attacker);
					multiplier = Range{ options.staffFatigue.minThreshold, options.staffFatigue.maxThreshold };
				}
				break;
			case RE::MagicSystem::SpellType::kScroll:
				weaponType = kScroll;
				if (options.scrollFatigue.enabled) {
					skill = MagicSkillFor(spell, attacker);
					multiplier = Range{ options.scrollFatigue.minThreshold, options.scrollFatigue.maxThreshold };
				}
				break;
			default:
				break;
			}
			break;

		case kRange:
			if (isAimed && isConcentration) {
				switch (spell->GetSpellType()) {
				case RE::MagicSystem::SpellType::kSpell:
				case RE::MagicSystem::SpellType::kLeveledSpell:
					if (options.concentrationSpellRangeScaling) {
						weaponType = kSpell;
						skill = MagicSkillFor(spell, attacker);
						multiplier = Range{ options.concentrationSpellMinRangeMultiplier, options.concentrationSpellMaxRangeMultiplier };
					}
					break;
				case RE::MagicSystem::SpellType::kStaffEnchantment:
					if (options.concentrationStaffRangeScaling) {
						weaponType = kStaff;
						skill = MagicSkillFor(spell, attacker);
						multiplier = Range{ options.concentrationStaffMinRangeMultiplier, options.concentrationStaffMaxRangeMultiplier };
					}
					break;
				default:
					break;
				}
			}
			break;

		case kAccuracy:
		case kPrecision:
			// Only support aimed spells.
			if (!isAimed)
				break;

			switch (spell->GetSpellType()) {
			case RE::MagicSystem::SpellType::kSpell:
			case RE::MagicSystem::SpellType::kLeveledSpell:
				weaponType = kSpell;
				if (isConcentration && options.concentrationSpellsAiming) {
					skill = MagicSkillFor(spell, attacker);
					multiplier = options.MultiplierFor(context).concentrationSpell;
				} else if (isFireAndForget && options.spellAiming) {
					skill = MagicSkillFor(spell, attacker);
					multiplier = options.MultiplierFor(context).spell;
				}
				break;
			case RE::MagicSystem::SpellType::kStaffEnchantment:
				weaponType = kStaff;
				if (isConcentration && options.concentrationStaffAiming) {
					skill = MagicSkillFor(spell, attacker);
					multiplier = options.MultiplierFor(context).concentrationStaff;
				} else if (isFireAndForget && options.staffAiming) {
					skill = MagicSkillFor(spell, attacker);
					multiplier = options.MultiplierFor(context).staff;
				}
				break;
			case RE::MagicSystem::SpellType::kScroll:
				weaponType = kScroll;
				if (options.scrollAiming) {
					skill = MagicSkillFor(spell, attacker);
					multiplier = options.MultiplierFor(context).scroll;
				}
				break;
			default:
				break;
			}

			if (options.blindnessAffectsAiming) {
				const float blindness = std::clamp(attacker->AsActorValueOwner()->GetActorValue(RE::ActorValue::kBlindness), 0.0f, 1.0f);
				std::get<float>(multiplier) -= (1 - options.MultiplierFor(context).blindness) * blindness;
			}
			break;

		default:
			break;
		}
	}

	float SkillUsage::SkillFactor(RE::Actor* actor) const {
		if (!actor || this->skill == RE::ActorValue::kNone)
			return 1.0f;
		const auto skill = EffectiveSkill(actor);
		return std::exp(0.025f * (10 - skill));
	}

	float SkillUsage::LerpSkillFactor(RE::Actor* actor) const {
		const Range* range = std::get_if<Range>(&multiplier);
		if (!range)
			return 1.0f;
		if (range->max == range->min)
			return range->min;

		constexpr float rate = 0.025f;
		const float     level = std::max(0.0f, EffectiveSkill(actor));
		const float     progress = (1.0f - std::exp(-rate * level)) / (1.0f - std::exp(-rate * 100.0f));

		return range->min + (range->max - range->min) * progress;
	}

	float SkillUsage::FatigueThreshold(RE::Actor* actor) const {
		const Range* range = std::get_if<Range>(&multiplier);
		if (!actor || !range)
			return 0.0f;

		constexpr float baseline = 100.0f;
		constexpr float exponent = 0.7f;

		const auto  attribute = weaponType == kBow ? RE::ActorValue::kStamina : RE::ActorValue::kMagicka;
		const float ratio = std::max(actor->GetActorValueMax(attribute), 0.0f) / baseline;

		return range->min + (range->max - range->min) * std::pow(ratio, exponent);
	}

	float SkillUsage::MaxSpread(RE::Actor* actor) const {
		constexpr float toRadians = std::numbers::pi / 180.0f;
		return SkillFactor(actor) * Settings::fBowNPCSpreadAngle() * toRadians;
	}

	RE::NiPoint2 SkillUsage::RandomSpreadOffset(RE::Actor* actor) const {
		const float maxSpread = MaxSpread(actor);

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
