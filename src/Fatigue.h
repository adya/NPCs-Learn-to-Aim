#pragma once

#include "Options.h"

namespace NLA::Fatigue
{
	/// Seconds the shot can be held before fatigue sets in. Grows with the actor's max Stamina (bows and crossbows) or Magicka (spells), with diminishing returns.
	float Threshold(const Options::Config::FatigueOptions&, RE::Actor*, WeaponType);

	/// Spread scale from casting a concentration spell/staff continuously for `castSeconds`. 1 when fatigue is disabled.
	float ConcentrationScale(RE::Actor*, WeaponType, float castSeconds);
}
