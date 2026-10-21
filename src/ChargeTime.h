#pragma once

namespace NLA::ChargeTime
{
	void Install();

	void Reset();

	/// Identifies a hand (casting source) of an actor, for state tracked separately for each of them.
	std::uint64_t CasterKey(RE::FormID, RE::MagicSystem::CastingSource);

	/// Charge speed multiplier for the given actor's skill.
	float SpeedMultiplier(RE::Actor*, const RE::MagicItem*);

	float PopSpellHoldDuration(RE::Actor*, RE::MagicSystem::CastingSource);

	/// Time in seconds that the actor has been continuously casting a concentration spell from the given casting source.
	float ConcentrationDuration(RE::Actor*, RE::MagicSystem::CastingSource);
}
