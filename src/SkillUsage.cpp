#include "SkillUsage.h"
#include "Helpers.h"

namespace NLA
{
	namespace
	{
		RE::ActorValue SkillFor(const Options::AttackKind& kind, const RE::MagicItem* spell, RE::Actor* caster, SkillUsageContext context);

		/// Lowers the multiplier by how blind the attacker is, if blindness affects aiming.
		float WithBlindness(float multiplier, RE::Actor* attacker, const Options::Config& options, SkillUsageContext context) {
			if (!options.blindnessAffectsAiming)
				return multiplier;

			const float blindness = std::clamp(attacker->AsActorValueOwner()->GetActorValue(RE::ActorValue::kBlindness), 0.0f, 1.0f);
			return std::max(0.0f, multiplier - (1 - options.BlindnessFor(context)) * blindness);
		}
	}

	SkillUsage::SkillUsage(const RE::TESObjectWEAP* weapon, RE::Actor* attacker, SkillUsageContext context) {
		if (!weapon || !attacker || !Options::ShouldLearn(attacker))
			return;

		const auto* weaponKind = Weapon::KindOf(weapon);
		if (!weaponKind)
			return;

		const auto& options = Options::For(attacker);

		kind = weaponKind;

		const auto& attack = options.For(*kind);

		const auto use = [&](const Options::Bounds& scale, const Options::SkillBasedValue& value) {
			skill = SkillFor(*kind, nullptr, attacker, context);
			bounds = scale;
			static_cast<Options::SkillBasedValue&>(*this) = value;
		};

		switch (context) {
		case kChargeTime:
			if (attack.speed.enabled) {
				use(attack.speed.bounds, attack.speed);
			}
			break;

		case kHeadshot:
			// Headshots are an NPC-only feature.
			if (!attacker->IsPlayerRef() && attack.headshot.enabled) {
				use(attack.headshot.bounds, attack.headshot);
			}
			break;

		case kAim:
		case kAccuracy:
		case kPrecision:
			if (attack.aiming) {
				const auto& aim = attack.AimFor(context);
				use(aim.bounds, { WithBlindness(aim.skillMultiplier, attacker, options, context) });
			}
			break;

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

		/// `spell` is null for weapons.
		RE::ActorValue SkillFor(const Options::AttackKind& kind, const RE::MagicItem* spell, RE::Actor* caster, SkillUsageContext context) {
			const auto& sources = (Options::For(caster).*kind.config).skills;

			Options::SkillSource source;
			switch (context) {
			case kChargeTime:
				source = sources.speed;
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
			case Options::SkillSource::kArchery:
				return RE::ActorValue::kArchery;
			case Options::SkillSource::kHighest:
				return HighestMagicSkill(caster);
			default:
				// A weapon has no magic school.
				if (!spell)
					return RE::ActorValue::kArchery;

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

		const auto* spellKind = Options::AttackKind::For(spell);
		if (!spellKind)
			return;

		const auto castingType = spell->GetCastingType();
		if (spellKind != &Options::AttackKind::kScroll && castingType != RE::MagicSystem::CastingType::kFireAndForget && castingType != RE::MagicSystem::CastingType::kConcentration)
			return;

		const auto& options = Options::For(attacker);
		const bool  isAimed = spell->GetDelivery() == RE::MagicSystem::Delivery::kAimed;

		kind = spellKind;

		const auto& attack = options.For(*kind);

		const auto use = [&](const Options::Bounds& scale, const Options::SkillBasedValue& value) {
			skill = SkillFor(*kind, spell, attacker, context);
			bounds = scale;
			static_cast<Options::SkillBasedValue&>(*this) = value;
		};

		switch (context) {
		case kChargeTime:
			if (attack.speed.enabled) {
				use(attack.speed.bounds, attack.speed);
			}
			break;

		case kRange:
			if (isAimed && kind->range && attack.range.enabled) {
				use(attack.range.bounds, attack.range);
			}
			break;

		case kHeadshot:
			// Headshots are an NPC-only feature.
			if (isAimed && !attacker->IsPlayerRef() && attack.headshot.enabled) {
				use(attack.headshot.bounds, attack.headshot);
			}
			break;

		case kAim:
		case kAccuracy:
		case kPrecision:
			if (isAimed && attack.aiming) {
				const auto& aim = attack.AimFor(context);
				use(aim.bounds, { WithBlindness(aim.skillMultiplier, attacker, options, context) });
			}
			break;

		default:
			break;
		}
	}

	float SkillProgress(float min, float max, float skill) {
		constexpr float shape = 1.6f;
		constexpr float saturation = 2.8f;

		const float t = std::max(0.0f, skill) / 100.0f;
		const float progress = (1.0f - std::exp(-saturation * std::pow(t, shape))) / (1.0f - std::exp(-saturation));

		return min + (max - min) * progress;
	}

	float SkillUsage::SkillFactor(RE::Actor* actor) const {
		if (!actor || skill == RE::ActorValue::kNone)
			return 1.0f;
		if (bounds.from == bounds.to)
			return bounds.from;

		return SkillProgress(bounds.from, bounds.to, EffectiveSkill(actor));
	}

	float SkillUsage::MaxSpread(RE::Actor* actor) const {
		constexpr float toRadians = std::numbers::pi_v<float> / 180.0f;
		return SkillFactor(actor) * Settings::fBowNPCSpreadAngle() * toRadians;
	}

	RE::NiPoint2 RandomSpreadOffset(float maxSpread, float focus) {
		// The floor keeps `pow` finite and the distribution a valid one when the skill curve overshoots a decreasing focus.
		constexpr float minFocus = 0.05f;

		std::uniform_real_distribution<float> angleRND(0, 2 * std::numbers::pi_v<float>);
		std::uniform_real_distribution<float> radiusRND(0, 1);

		// `focus` 0.5 spreads shots evenly over the disc, higher pulls them towards the center and lower pushes them to the edge.
		float spread = maxSpread * std::pow(radiusRND(rnd), std::max(focus, minFocus));
		float angle = angleRND(rnd);  // pick a random direction on a circle.

		return { std::sin(angle) * spread, std::cos(angle) * spread };
	}

	float SpreadFocus(const SkillUsage& precision, RE::Actor* actor) {
		constexpr float evenFocus = 0.5f;
		return precision ? precision.SkillFactor(actor) : evenFocus;
	}

	RE::NiPoint2 RandomSpreadOffset(const SkillUsage& accuracy, const SkillUsage& precision, RE::Actor* actor) {
		return RandomSpreadOffset(accuracy.MaxSpread(actor), SpreadFocus(precision, actor));
	}
}
