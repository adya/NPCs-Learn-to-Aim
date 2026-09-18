#pragma once

#include "Options.h"

namespace NLA
{
	enum WeaponType : std::uint8_t
	{
		kNone = 0,
		kBow = 1,
		kCrossbow = 2,
		kSpell = 3,
		kStaff = 4
	};

	struct SkillUsage
	{
		RE::ActorValue skill{ RE::ActorValue::kNone };
		float          multiplier{ 0 };
		WeaponType     weaponType{ kNone };

		SkillUsage() = default;
		SkillUsage(RE::ActorValue skill, float multiplier, WeaponType weaponType) :
			skill(skill), multiplier(multiplier), weaponType(weaponType) {}

		SkillUsage(const RE::TESObjectWEAP* weapon, RE::Actor* attacker, SkillUsageContext context);

		SkillUsage(const RE::MagicItem* spell, RE::Actor* attacker, SkillUsageContext context);

		// When picking a skill failed either due to combination of options or invalid input,
		// we return false to indicate that there is no SkillUsage involved in the shot :)
		operator bool() const {
			return skill != RE::ActorValue::kNone;
		}
#ifndef NDEBUG
		float OriginalSkillFactor(RE::Actor* actor) const;
#endif
		/// A multiplier that controls how much current skill affects accuracy and precision.
		float SkillFactor(RE::Actor* actor) const;

		/// An interpolated multiplier for a skill in 0-100 range, controlling the growth of the factor in min-max range.
		float LerpSkillFactor(RE::Actor* actor, float minFactor, float maxFactor) const;

		float Skill(RE::Actor* actor) const {
			if (!actor || skill == RE::ActorValue::kNone)
				return 0.0f;
			return actor->AsActorValueOwner()->GetActorValue(skill);
		}

		float EffectiveSkill(RE::Actor* actor) const {
			return Skill(actor) * multiplier;
		}

		float MaxSpread(RE::Actor* actor) const;
	};

	RE::NiPoint2 RandomSpreadOffset(float maxSpread);
}
