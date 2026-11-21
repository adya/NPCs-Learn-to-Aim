#pragma once

#include "Options.h"

namespace NLA::Fatigue
{
	/// Seconds the shot can be held before fatigue sets in. Grows with the actor's max Stamina (bows and crossbows) or Magicka (spells), with diminishing returns.
	float Threshold(const Options::Config::Fatigue&, RE::Actor*, const Options::AttackKind&);

	/// Internals of `Threshold`, open only so that the menu preview can use it without an actor.
	namespace Details
	{
		/// `Threshold` for a given max Stamina (bows and crossbows) or Magicka (spells).
		float Threshold(const Options::Config::Fatigue&, float maxAttribute);
	}

	/// What holding a shot for some time does to its spread.
	struct HoldScale
	{
		/// Spread scale from steadying alone, 1 when steadying is disabled.
		float steadied{ 1.0f };

		/// Seconds past the fatigue threshold, 0 when fatigue is disabled.
		float fatigueSeconds{ 0.0f };

		/// How far the fatigue debuff has progressed towards its max (0-1).
		float progress{ 0.0f };

		/// Final spread scale, combining steadying and fatigue.
		float scale{ 1.0f };
	};

	/// The spread scale after holding the shot for `seconds`, given the fatigue `threshold` (ignored when fatigue is disabled).
	/// Fatigue negates the steadying gained so far before adding its own spread.
	HoldScale Hold(const Options::Config::Steadying&, const Options::Config::Fatigue&, float threshold, float seconds);

	/// How far (0-1) something that approaches its maximum has progressed after `seconds`: half of it is reached after `halfDuration`, three quarters after twice that, and so on.
	float Progress(float seconds, float halfDuration);

	struct ConcentrationState
	{
		bool  enabled{ false };
		float castSeconds{ 0.0f };
		float threshold{ 0.0f };

		HoldScale hold{};
	};

	/// Steadying and fatigue from casting a concentration spell/staff continuously for `castSeconds`.
	ConcentrationState Concentration(RE::Actor*, const Options::AttackKind&, float castSeconds);

	/// Spread scale from casting a concentration spell/staff continuously for `castSeconds`. 1 when fatigue is disabled.
	float ConcentrationScale(RE::Actor*, const Options::AttackKind&, float castSeconds);
}
