#include "Fatigue.h"

namespace NLA::Fatigue
{
	float Threshold(const Options::Config::Fatigue& fatigue, RE::Actor* actor, WeaponType type) {
		if (!actor)
			return 0.0f;

		constexpr float baseline = 100.0f;
		constexpr float exponent = 0.7f;

		const bool  usesStamina = type == kBow || type == kCrossbow;
		const auto  attribute = usesStamina ? RE::ActorValue::kStamina : RE::ActorValue::kMagicka;
		const float ratio = std::max(actor->GetActorValueMax(attribute), 0.0f) / baseline;

		// Max attribute above the baseline extends the threshold past `maxThreshold`, so a max below `min` could otherwise go negative.
		return std::max(0.0f, fatigue.minThreshold + (fatigue.maxThreshold - fatigue.minThreshold) * std::pow(ratio, exponent));
	}

	float Progress(float seconds, float halfDuration) {
		return 1.0f - std::exp2(-seconds / halfDuration);
	}

	ConcentrationState Concentration(RE::Actor* attacker, WeaponType type, float castSeconds) {
		ConcentrationState state{ .castSeconds = castSeconds };
		if (type != kSpell && type != kStaff)
			return state;

		const auto& fatigue = Options::Config::Fatigue::For(Options::For(attacker), type, true);
		if (!fatigue.enabled)
			return state;

		state.enabled = true;
		state.threshold = Threshold(fatigue, attacker, type);
		state.fatigueSeconds = std::max(0.0f, castSeconds - state.threshold);
		state.progress = Progress(state.fatigueSeconds, fatigue.halfDuration);
		state.scale = 1.0f + fatigue.maxSpreadMultiplier * state.progress;
		return state;
	}

	float ConcentrationScale(RE::Actor* attacker, WeaponType type, float castSeconds) {
		return Concentration(attacker, type, castSeconds).scale;
	}
}
