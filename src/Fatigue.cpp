#include "Fatigue.h"

namespace NLA::Fatigue
{
	float Threshold(const Options::Config::FatigueOptions& fatigue, RE::Actor* actor, WeaponType type) {
		if (!actor)
			return 0.0f;

		constexpr float baseline = 100.0f;
		constexpr float exponent = 0.7f;

		const bool  usesStamina = type == kBow || type == kCrossbow;
		const auto  attribute = usesStamina ? RE::ActorValue::kStamina : RE::ActorValue::kMagicka;
		const float ratio = std::max(actor->GetActorValueMax(attribute), 0.0f) / baseline;

		return fatigue.minThreshold + (fatigue.maxThreshold - fatigue.minThreshold) * std::pow(ratio, exponent);
	}

	float ConcentrationScale(RE::Actor* attacker, WeaponType type, float castSeconds) {
		if (type != kSpell && type != kStaff)
			return 1.0f;

		const auto& fatigue = Options::For(attacker).ConcentrationFatigueFor(type);
		if (!fatigue.enabled)
			return 1.0f;

		const float fatigueSeconds = std::max(0.0f, castSeconds - Threshold(fatigue, attacker, type));
		const float fatigueProgress = 1.0f - std::exp2(-fatigueSeconds / fatigue.halfDuration);

		return 1.0f + fatigue.maxSpreadMultiplier * fatigueProgress;
	}
}
