#pragma once

namespace NLA::ChargeTime
{
	void Install();

	void Reset();

	float PopSpellOverchargeDuration(RE::Actor*, RE::MagicSystem::CastingSource);
}
