#pragma once

namespace NLA
{
	struct SkillUsage;

	/// How well the shooter's shot goes where it was aimed: the aim of the controller and the spread of the launch angles.
	namespace Aim
	{
		void Install();

		float AimVariance(const SkillUsage& aimUsage, RE::Actor* attacker);
	}

	/// Which part of the target the shooter aims at on purpose.
	namespace Skillshots
	{
		void Install();

#ifndef NDEBUG
		float SkillShotChance(RE::Actor* attacker, const SkillUsage& skillShotUsage);

		/// What the attacker is going for with its current target: "head", "left hand", ... or "body".
		const char* CurrentSkillShot(RE::Actor* attacker);
#endif
		void ClearSkillShot(RE::Actor* attacker);

		/// Forgets all skill shot decisions, e.g. when a different save is loaded.
		void Reset();

		/// Drops a shooter's skill shot bags when it leaves combat. Needs the game's event sources, so it can't be done before the data is loaded.
		void ListenForCombat();
	}
}
