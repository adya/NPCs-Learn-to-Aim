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
	}
}
