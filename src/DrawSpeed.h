#pragma once

namespace NLA::Draw
{
	void Install();

	void Reset();

	/// Consumes and resets the tracked duration a bow was held at full draw before its last release.
	float PopBowFullDrawDuration(RE::Actor*);
}
