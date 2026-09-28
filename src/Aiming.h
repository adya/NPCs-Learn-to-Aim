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
	float HoldSpreadScale(float holdSeconds, const Options::Config::SpreadOptions& steadying, const Options::Config::FatigueOptions& fatigue, float fatigueThresholdSeconds);
#endif
}
