#include "Fatigue.h"
#include "Helpers.h"

namespace NLA::Fatigue
{
	float Threshold(const Options::Config::Fatigue& fatigue, RE::Actor* actor, const Options::AttackKind& kind) {
		if (!actor)
			return 0.0f;

		constexpr float baseline = 100.0f;
		constexpr float exponent = 0.7f;

		const auto  attribute = !kind.magic ? RE::ActorValue::kStamina : RE::ActorValue::kMagicka;
		const float ratio = std::max(actor->GetActorValueMax(attribute), 0.0f) / baseline;

		return std::max(0.0f, fatigue.threshold.from + (fatigue.threshold.to - fatigue.threshold.from) * std::pow(ratio, exponent));
	}

	float Progress(float seconds, float halfDuration) {
		return 1.0f - std::exp2(-seconds / halfDuration);
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
			state.fatigueSeconds = std::max(0.0f, castSeconds - state.threshold);
			state.progress = Progress(state.fatigueSeconds, fatigue.halfDuration);
		}

		if (steadying.enabled && castSeconds > 0.0f) {
			const float steadyingSeconds = state.enabled ? std::min(castSeconds, state.threshold) : castSeconds;
			state.steadied = 1.0f - steadying.maxSpreadMultiplier * Progress(steadyingSeconds, steadying.halfDuration);
		}

		state.scale = state.steadied;
		if (state.enabled) {
			state.scale += (1.0f + fatigue.maxSpreadMultiplier - state.steadied) * state.progress;
		}
		return state;
	}

	float ConcentrationScale(RE::Actor* attacker, const Options::AttackKind& kind, float castSeconds) {
		return Concentration(attacker, kind, castSeconds).scale;
	}
}
