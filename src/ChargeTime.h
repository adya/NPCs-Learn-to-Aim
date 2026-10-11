#pragma once

namespace NLA::ChargeTime
{
	void Install();

	void Reset();

	/// Charge speed multiplier for the given actor's skill.
	float SpeedMultiplier(RE::Actor*, const RE::MagicItem*);

	float PopSpellOverchargeDuration(RE::Actor*, RE::MagicSystem::CastingSource);
}
