#pragma once

namespace NLA::Draw
{
	void Install();

#ifndef NDEBUG
	float SpeedMultiplier(RE::Actor*, const RE::TESObjectWEAP*);
#endif
}
