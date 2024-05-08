#include "Calculations.h"

namespace NLA::Calculations
{
	// C = sqrt(w/d^2)/2
	float ShotComplexity(float distance, float targetSize) {
		return std::sqrt(targetSize) / (2 * targetSize);
	}

	// c = 0.8/log(skill); skill >= 10
	float SkillFactor(float skill) {
		return 0.8f / std::log10((std::max)(skill, 10.0f));
	}

	// c2 = 0.5 * e^(2 * sqrt(c)) - 1 + c
	float SkillConsistency(float skillFactor) {
		return 0.5f * std::exp(2 * std::sqrt(skillFactor)) - 1 + skillFactor;
	}

	// normalize distance in range [10;100]
	// d = min + (distance - Dmin) * (max - min) / (Dmax - Dmin)
	float NormalizedDistance(float distance, float maxDistance) {
		return 10 + 90 * (std::clamp(distance, 0.0f, maxDistance) / maxDistance);
	}

	// normalize target size in range [10;100]
	// w = min + (size - Smin) * (max - min) / (Smax - Smin)
	float NormalizedTargetSize(float targetSize, float maxSize) {
		return 10 + 90 * (std::clamp(targetSize, 0.0f, maxSize) / maxSize);
	}

	// s = 0.5 * v * c
	float Deviation(float skillFactor, float variance) {
		return 0.5 * skillFactor * variance;
	}

	// m = v * c^2 * log(C)
	float Mean(float skillFactor, float shotComplexity, float variance) {
		return variance * std::pow(skillFactor, 2) * std::log10(shotComplexity);
	}

	RE::NiPoint3 RandomOffset(float skill, float distance, float targetSize, float varianceOption) {
		auto skillFactor = SkillFactor(skill);
		auto skillConsistency = SkillConsistency(skillFactor);

		auto variance = (std::max)(varianceOption, 0.1f);  // formula requires variance to be > 0

		auto normalizedDistance = NormalizedDistance(distance);
		auto normalizedTargetSize = NormalizedTargetSize(targetSize);
		auto shotComplexity = Options::Complexity::useComplexity ? ShotComplexity(normalizedDistance, normalizedTargetSize) : 10; // 10 will set shot complexity component to exactly 1, thus making it irrelevant.

		auto deviation = Deviation(skillFactor, variance);
		auto mean = Mean(skillFactor, shotComplexity, variance);

		std::uniform_real_distribution  combiner;
		std::normal_distribution<float> distrLeft(-mean, variance);
		std::normal_distribution<float> distrRight(mean, variance);

		return {
			skillConsistency * (combiner(rnd) > 0.5 ? distrLeft(rnd) : distrRight(rnd)),
			skillConsistency * (combiner(rnd) > 0.5 ? distrLeft(rnd) : distrRight(rnd)),
			skillConsistency * (combiner(rnd) > 0.5 ? distrLeft(rnd) : distrRight(rnd))
		};
	}
}
