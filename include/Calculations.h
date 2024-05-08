#pragma once
#include "Options.h"

namespace NLA::Calculations
{
	static std::default_random_engine rnd;

	float NormalizedDistance(float distance, float maxDistance = Options::Complexity::maxDistance);

	float NormalizedTargetSize(float targetSize, float maxSize = Options::Complexity::maxTargetSize);

	RE::NiPoint3 RandomOffset(float skill, float distance, float targetSize, float varianceOption);
}
