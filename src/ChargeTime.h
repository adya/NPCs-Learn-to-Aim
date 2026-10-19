#pragma once

namespace NLA::ChargeTime
{
	void Install();

	void Reset();

	/// Charge speed multiplier for the given actor's skill.
	float SpeedMultiplier(RE::Actor*, const RE::MagicItem*);

	float PopSpellHoldDuration(RE::Actor*, RE::MagicSystem::CastingSource);

	/// Time in seconds that the actor has been continuously casting a concentration spell from the given casting source.
	float ConcentrationDuration(RE::Actor*, RE::MagicSystem::CastingSource);
}
