#pragma once

#include "Options.h"

namespace NLA
{
	struct SkillUsage : Options::SkillBasedValue
	{
		RE::ActorValue skill{ RE::ActorValue::kNone };
		const Options::AttackKind* kind{ nullptr };

		Options::Bounds bounds{};

		SkillUsage() = default;

		SkillUsage(const RE::TESObjectWEAP* weapon, RE::Actor* attacker, SkillUsageContext context);

		SkillUsage(const RE::MagicItem* spell, RE::Actor* attacker, SkillUsageContext context);

		// When picking a skill failed either due to combination of options or invalid input,
		// we return false to indicate that there is no SkillUsage involved in the shot :)
		operator bool() const {
			return skill != RE::ActorValue::kNone;
		}

		float SkillFactor(RE::Actor* actor) const;

		float Skill(RE::Actor* actor) const {
			if (!actor || skill == RE::ActorValue::kNone)
				return 0.0f;
			return actor->AsActorValueOwner()->GetActorValue(skill);
		}

		float EffectiveSkill(RE::Actor* actor) const {
			return Skill(actor) * skillMultiplier;
		}

		/// Maximum angular deviation (in radians) for the shot, based on the current skill.
		float MaxSpread(RE::Actor* actor) const;

		/// A random deviation in a random direction, with magnitude normally distributed within MaxSpread.
		RE::NiPoint2 RandomSpreadOffset(RE::Actor* actor) const;
	};
}
