#pragma once

#include "Options.h"

namespace NLA::Fatigue
{
	/// Seconds the shot can be held before fatigue sets in. Grows with the actor's max Stamina (bows and crossbows) or Magicka (spells), with diminishing returns.
	float Threshold(const Options::Config::FatigueOptions&, RE::Actor*, WeaponType);

	/// How far (0-1) something that approaches its maximum has progressed after `seconds`: half of it is reached after `halfDuration`, three quarters after twice that, and so on.
	float Progress(float seconds, float halfDuration);

	/// Spread scale from casting a concentration spell/staff continuously for `castSeconds`. 1 when fatigue is disabled.
	float ConcentrationScale(RE::Actor*, WeaponType, float castSeconds);
}
