#pragma once

#include "Options.h"

namespace NLA
{
	namespace Accuracy
	{
		void Install();
	}

	namespace Precision
	{
		void Install();
	}

#ifndef NDEBUG // TODO: Remove after debugging
	float BowSpreadScale(float holdSeconds, const Options::Config::Steadying& steadying, const Options::Config::DrawFatigue& fatigue, float fatigueThresholdSeconds);
#endif
}
