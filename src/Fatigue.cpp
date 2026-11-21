#include "Fatigue.h"
#include "Helpers.h"

namespace NLA::Fatigue
{
	float Details::Threshold(const Options::Config::Fatigue& fatigue, float maxAttribute) {
		constexpr float baseline = 100.0f;
		constexpr float exponent = 0.7f;

		const float ratio = std::max(maxAttribute, 0.0f) / baseline;

		return std::max(0.0f, fatigue.threshold.from + (fatigue.threshold.to - fatigue.threshold.from) * std::pow(ratio, exponent));
	}

	float Threshold(const Options::Config::Fatigue& fatigue, RE::Actor* actor, const Options::AttackKind& kind) {
		if (!actor)
			return 0.0f;

		const auto attribute = !kind.magic ? RE::ActorValue::kStamina : RE::ActorValue::kMagicka;
		return Details::Threshold(fatigue, actor->GetActorValueMax(attribute));
	}

	float Progress(float seconds, float halfDuration) {
		return 1.0f - std::exp2(-seconds / halfDuration);
	}

	HoldScale Hold(const Options::Config::Steadying& steadying, const Options::Config::Fatigue& fatigue, float threshold, float seconds) {
		HoldScale hold;
		if (seconds <= 0.0f)
			return hold;

		const float steadyingSeconds = fatigue.enabled ? std::min(seconds, threshold) : seconds;

		if (steadying.enabled) {
			hold.steadied = 1.0f - steadying.maxSpreadMultiplier * Progress(steadyingSeconds, steadying.halfDuration);
		}
		hold.scale = hold.steadied;

		if (!fatigue.enabled)
			return hold;

		hold.fatigueSeconds = std::max(0.0f, seconds - threshold);
		hold.progress = Progress(hold.fatigueSeconds, fatigue.halfDuration);

		const float fatigueTarget = 1.0f + fatigue.maxSpreadMultiplier;
		hold.scale = hold.steadied + (fatigueTarget - hold.steadied) * hold.progress;
		return hold;
	}

	ConcentrationState Concentration(RE::Actor* attacker, const Options::AttackKind& kind, float castSeconds) {
		ConcentrationState state{ .castSeconds = castSeconds };
		if (!kind.concentration)
			return state;

		const auto& options = Options::For(attacker);
		const auto& steadying = options.For(kind).steadying;
		const auto& fatigue = options.For(kind).fatigue;

		if (fatigue.enabled) {
			state.enabled = true;
			state.threshold = Threshold(fatigue, attacker, kind);
		}

		state.hold = Hold(steadying, fatigue, state.threshold, castSeconds);
		return state;
	}

	float ConcentrationScale(RE::Actor* attacker, const Options::AttackKind& kind, float castSeconds) {
		return Concentration(attacker, kind, castSeconds).hold.scale;
	}
}
