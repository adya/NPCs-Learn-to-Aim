#pragma once

#include "Options.h"
#include <variant>

namespace NLA
{
	struct SkillUsage
	{
		struct Range
		{
			float min{ 1.0f };
			float max{ 1.0f };
		};

		RE::ActorValue             skill{ RE::ActorValue::kNone };
		std::variant<float, Range> multiplier{ 0.0f };
		WeaponType                 weaponType{ kNone };

		SkillUsage() = default;

		SkillUsage(const RE::TESObjectWEAP* weapon, RE::Actor* attacker, SkillUsageContext context);

		SkillUsage(const RE::MagicItem* spell, RE::Actor* attacker, SkillUsageContext context);

		// When picking a skill failed either due to combination of options or invalid input,
		// we return false to indicate that there is no SkillUsage involved in the shot :)
		operator bool() const {
			return skill != RE::ActorValue::kNone;
		}

		/// A multiplier that controls how much current skill affects accuracy and precision.
		float SkillFactor(RE::Actor* actor) const;

		/// An interpolated multiplier for a skill in 0-100 range, controlling the growth of the factor in min-max range.
		float LerpSkillFactor(RE::Actor* actor) const;

		float Skill(RE::Actor* actor) const {
			if (!actor || skill == RE::ActorValue::kNone)
				return 0.0f;
			return actor->AsActorValueOwner()->GetActorValue(skill);
		}

		float EffectiveSkill(RE::Actor* actor) const {
			const float* sensitivity = std::get_if<float>(&multiplier);
			return Skill(actor) * (sensitivity ? *sensitivity : 1.0f);
		}

		/// Maximum angular deviation (in radians) for the shot, based on the current skill.
		float MaxSpread(RE::Actor* actor) const;

		/// A random deviation in a random direction, with magnitude normally distributed within MaxSpread.
		RE::NiPoint2 RandomSpreadOffset(RE::Actor* actor) const;
	};

	RE::ActorValue HighestMagicSkill(RE::Actor* actor);

	RE::ActorValue MagicSkillFor(const RE::MagicItem* spell, RE::Actor* caster);
}
