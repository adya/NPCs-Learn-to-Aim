#pragma once

namespace NLA::ChargeTime
{
	void Install();

#ifndef NDEBUG
	float SpeedMultiplier(RE::Actor*, const RE::MagicItem*);
#endif
}
