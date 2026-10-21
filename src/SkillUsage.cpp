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

		const auto& kind = Options::AttackKind::For(weaponType);

		switch (context) {
		case kChargeTime:
			{
				const auto& drawSpeed = options.*kind.speed;
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
				if (options.*kind.aiming) {
					skill = RE::ActorValue::kArchery;
					multiplier = WithBlindness(options.MultiplierFor(context).*kind.multiplier, attacker, options, context);
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

		RE::ActorValue SkillFor(const Options::AttackKind& kind, const RE::MagicItem* spell, RE::Actor* caster, SkillUsageContext context) {
			const auto& sources = Options::For(caster).*kind.skills;

			Options::SkillSource source;
			switch (context) {
			case kChargeTime:
				source = sources.chargeSpeed;
				break;
			case kRange:
				source = sources.range;
				break;
			default:
				source = sources.aiming;
				break;
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
				// Spells without a magic school still aim by the caster's best school.
				return HighestMagicSkill(caster);
			}
		}
	}

	SkillUsage::SkillUsage(const RE::MagicItem* spell, RE::Actor* attacker, SkillUsageContext context) {
		if (!spell || !attacker || !Options::ShouldLearn(attacker))
			return;

		const auto* kind = Options::AttackKind::For(spell);
		if (!kind)
			return;

		const auto castingType = spell->GetCastingType();
		if (kind->type != kScroll && castingType != RE::MagicSystem::CastingType::kFireAndForget && castingType != RE::MagicSystem::CastingType::kConcentration)
			return;

		const auto& options = Options::For(attacker);
		const bool  isAimed = spell->GetDelivery() == RE::MagicSystem::Delivery::kAimed;

		weaponType = kind->type;

		const auto use = [&](std::variant<float, Range> value) {
			skill = SkillFor(*kind, spell, attacker, context);
			multiplier = value;
		};

		switch (context) {
		case kChargeTime:
			if (const auto& speed = options.*kind->speed; speed.enabled) {
				use(Range{ speed.minMultiplier, speed.maxMultiplier });
			}
			break;

		case kRange:
			if (isAimed && kind->range) {
				if (const auto& range = options.*kind->range; range.enabled) {
					use(Range{ range.minMultiplier, range.maxMultiplier });
				}
			}
			break;

		case kAccuracy:
		case kPrecision:
			if (isAimed && options.*kind->aiming) {
				use(WithBlindness(options.MultiplierFor(context).*kind->multiplier, attacker, options, context));
			}
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
