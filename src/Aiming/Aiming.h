#pragma once

namespace NLA
{
	struct SkillUsage;

	namespace Accuracy
	{
		void Install();
#ifndef NDEBUG
		float AimVariance(const SkillUsage& accuracyUsage, RE::Actor* attacker);

		float HeadshotChance(RE::Actor* attacker, const SkillUsage& headshotUsage);

		bool IsGoingForHeadshot(RE::Actor* attacker);
#endif
		void ClearHeadshot(RE::Actor* attacker);

		/// Forgets all headshot decisions, e.g. when a different save is loaded.
		void Reset();
	}

	namespace Precision
	{
		void Install();
	}
}
