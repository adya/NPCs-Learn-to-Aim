#pragma once

namespace NLA::Draw
{
	void Install();

	void Reset();

	/// Draw speed multiplier for the given actor's skill.
	float SpeedMultiplier(RE::Actor*, const RE::TESObjectWEAP*);

	/// Consumes and resets the tracked duration a bow or crossbow was held at full draw before its last release.
	float PopFullDrawDuration(RE::Actor*);
}
