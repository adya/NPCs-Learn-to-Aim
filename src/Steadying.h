#pragma once

namespace NLA::Steadying
{
	void Install();

	void Reset();

	/// Time in seconds that a bow or crossbow was held at full draw.
	float FullDrawDuration(RE::Actor*);

	/// Time in seconds that a spell was held ready or, for a concentration spell, being casted.
	float SpellDuration(RE::Actor*, RE::MagicSystem::CastingSource);
}
