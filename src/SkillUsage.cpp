#include "SkillUsage.h"
#include <numbers>

namespace NLA
{
	namespace
	{
		/// Lowers the multiplier by how blind the attacker is, if blindness affects aiming.
		float WithBlindness(float multiplier, RE::Actor* attacker, const Options::Config& options, SkillUsageContext context) {
			if (!options.blindnessAffectsAiming)
				return multiplier;

			const float blindness = std::clamp(attacker->AsActorValueOwner()->GetActorValue(RE::ActorValue::kBlindness), 0.0f, 1.0f);
			return std::max(0.0f, multiplier - (1 - options.MultiplierFor(context).blindness) * blindness);
		}
	}

	SkillUsage::SkillUsage(const RE::TESObjectWEAP* weapon, RE::Actor* attacker, SkillUsageContext context) {
		if (!weapon || !attacker || !Options::ShouldLearn(attacker))
			return;

		const auto weaponKind = weapon->GetWeaponType();
		if (weaponKind != RE::WEAPON_TYPE::kBow && weaponKind != RE::WEAPON_TYPE::kCrossbow)
			return;

		const auto& options = Options::For(attacker);
		const bool  isCrossbow = weaponKind == RE::WEAPON_TYPE::kCrossbow;

		weaponType = isCrossbow ? kCrossbow : kBow;

		switch (context) {
		case kChargeTime:
			{
				const auto& drawSpeed = isCrossbow ? options.crossbowDrawSpeed : options.bowDrawSpeed;
				if (drawSpeed.enabled) {
					skill = RE::ActorValue::kArchery;
					multiplier = Range{ drawSpeed.minMultiplier, drawSpeed.maxMultiplier };
				}
				break;
			}
		case kHeadshot:
			{
				// Headshots are an NPC-only feature.
				if (attacker->IsPlayerRef())
					break;

				const auto& headshots = Options::NPC.headshots;
				if (isCrossbow ? headshots.crossbow : headshots.bow) {
					skill = RE::ActorValue::kArchery;
					multiplier = isCrossbow ? Range{ headshots.crossbowMinChance, headshots.crossbowMaxChance } : Range{ headshots.bowMinChance, headshots.bowMaxChance };
				}
				break;
			}
		case kAccuracy:
		case kPrecision:
			{
				if (isCrossbow ? options.crossbowAiming : options.bowAiming) {
					const auto& multipliers = options.MultiplierFor(context);

					skill = RE::ActorValue::kArchery;
					multiplier = WithBlindness(isCrossbow ? multipliers.crossbow : multipliers.bow, attacker, options, context);
				}
				break;
			}
		default:
			break;
		}
	}

	namespace
	{
		RE::ActorValue HighestMagicSkill(RE::Actor* actor) {
			std::array<RE::ActorValue, 5> properties = { RE::ActorValue::kAlteration, RE::ActorValue::kConjuration, RE::ActorValue::kDestruction, RE::ActorValue::kIllusion, RE::ActorValue::kRestoration };

			return *std::ranges::max_element(properties, [&actor](RE::ActorValue a, RE::ActorValue b) {
				return actor->AsActorValueOwner()->GetActorValue(a) < actor->AsActorValueOwner()->GetActorValue(b);
			});
		}
	}

	RE::ActorValue MagicSkillFor(const RE::MagicItem* spell, RE::Actor* caster, SkillUsageContext context) {
		if (!spell || !caster)
			return RE::ActorValue::kNone;

		const auto& options = Options::For(caster);

		const bool isConcentration = spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration;

		const Options::Config::SkillSources* sources = nullptr;
		switch (spell->GetSpellType()) {
		case RE::MagicSystem::SpellType::kSpell:
		case RE::MagicSystem::SpellType::kLeveledSpell:
			sources = isConcentration ? &options.concentrationSpellSkills : &options.spellSkills;
			break;
		case RE::MagicSystem::SpellType::kStaffEnchantment:
			sources = isConcentration ? &options.concentrationStaffSkills : &options.staffSkills;
			break;
		case RE::MagicSystem::SpellType::kScroll:
			sources = &options.scrollSkills;
			break;
		default:
			break;
		}

		Options::SkillSource source = Options::SkillSource::kSpellSkill;
		if (sources) {
			switch (context) {
			case kChargeTime:
				source = sources->chargeSpeed;
				break;
			case kRange:
				source = sources->range;
				break;
			default:
				source = sources->aiming;
				break;
			}
		}

		switch (source) {
		case Options::SkillSource::kEnchanting:
			return RE::ActorValue::kEnchanting;
		case Options::SkillSource::kHighest:
			return HighestMagicSkill(caster);
		default:
			if (auto effect = spell->GetCostliestEffectItem(); effect) {
				if (auto base = effect->baseEffect; base) {
					if (auto skill = base->GetMagickSkill(); skill != RE::ActorValue::kNone) {
						return skill;
					}
				}
			}
			// Spells without a magic school (e.g. some abilities or modded effects) still aim by the caster's best school.
			return HighestMagicSkill(caster);
		}
	}

	SkillUsage::SkillUsage(const RE::MagicItem* spell, RE::Actor* attacker, SkillUsageContext context) {
		if (!spell || !attacker || !Options::ShouldLearn(attacker))
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
					skill = MagicSkillFor(spell, attacker, context);
					multiplier = Range{ options.concentrationSpellChargeSpeed.minMultiplier, options.concentrationSpellChargeSpeed.maxMultiplier };
				} else if (isFireAndForget && options.spellChargeSpeed.enabled) {
					skill = MagicSkillFor(spell, attacker, context);
					multiplier = Range{ options.spellChargeSpeed.minMultiplier, options.spellChargeSpeed.maxMultiplier };
				}
				break;
			case RE::MagicSystem::SpellType::kStaffEnchantment:
				weaponType = kStaff;
				if (isConcentration && options.concentrationStaffChargeSpeed.enabled) {
					skill = MagicSkillFor(spell, attacker, context);
					multiplier = Range{ options.concentrationStaffChargeSpeed.minMultiplier, options.concentrationStaffChargeSpeed.maxMultiplier };
				} else if (isFireAndForget && options.staffChargeSpeed.enabled) {
					skill = MagicSkillFor(spell, attacker, context);
					multiplier = Range{ options.staffChargeSpeed.minMultiplier, options.staffChargeSpeed.maxMultiplier };
				}
				break;
			case RE::MagicSystem::SpellType::kScroll:
				weaponType = kScroll;
				if (options.scrollChargeSpeed.enabled) {
					skill = MagicSkillFor(spell, attacker, context);
					multiplier = Range{ options.scrollChargeSpeed.minMultiplier, options.scrollChargeSpeed.maxMultiplier };
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
					if (options.concentrationSpellRange.enabled) {
						weaponType = kSpell;
						skill = MagicSkillFor(spell, attacker, context);
						multiplier = Range{ options.concentrationSpellRange.minMultiplier, options.concentrationSpellRange.maxMultiplier };
					}
					break;
				case RE::MagicSystem::SpellType::kStaffEnchantment:
					if (options.concentrationStaffRange.enabled) {
						weaponType = kStaff;
						skill = MagicSkillFor(spell, attacker, context);
						multiplier = Range{ options.concentrationStaffRange.minMultiplier, options.concentrationStaffRange.maxMultiplier };
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
				if (isConcentration && options.concentrationSpellAiming) {
					skill = MagicSkillFor(spell, attacker, context);
					multiplier = options.MultiplierFor(context).concentrationSpell;
				} else if (isFireAndForget && options.spellAiming) {
					skill = MagicSkillFor(spell, attacker, context);
					multiplier = options.MultiplierFor(context).spell;
				}
				break;
			case RE::MagicSystem::SpellType::kStaffEnchantment:
				weaponType = kStaff;
				if (isConcentration && options.concentrationStaffAiming) {
					skill = MagicSkillFor(spell, attacker, context);
					multiplier = options.MultiplierFor(context).concentrationStaff;
				} else if (isFireAndForget && options.staffAiming) {
					skill = MagicSkillFor(spell, attacker, context);
					multiplier = options.MultiplierFor(context).staff;
				}
				break;
			case RE::MagicSystem::SpellType::kScroll:
				weaponType = kScroll;
				if (options.scrollAiming) {
					skill = MagicSkillFor(spell, attacker, context);
					multiplier = options.MultiplierFor(context).scroll;
				}
				break;
			default:
				break;
			}

			multiplier = WithBlindness(std::get<float>(multiplier), attacker, options, context);
			break;

		default:
			break;
		}
	}

	float SkillUsage::SkillFactor(RE::Actor* actor) const {
		if (!actor || this->skill == RE::ActorValue::kNone)
			return 1.0f;
		const auto effectiveSkill = EffectiveSkill(actor);
		return std::exp(0.025f * (10 - effectiveSkill));
	}

	float SkillUsage::LerpSkillFactor(RE::Actor* actor) const {
		const Range* range = std::get_if<Range>(&multiplier);
		if (!range)
			return 1.0f;
		if (range->max == range->min)
			return range->min;

		constexpr float shape = 1.8f;
		constexpr float saturation = 2.7f;

		const float t = std::max(0.0f, EffectiveSkill(actor)) / 100.0f;
		const float progress = (1.0f - std::exp(-saturation * std::pow(t, shape))) / (1.0f - std::exp(-saturation));

		return range->min + (range->max - range->min) * progress;
	}

	float SkillUsage::MaxSpread(RE::Actor* actor) const {
		constexpr float toRadians = std::numbers::pi_v<float> / 180.0f;
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
}
