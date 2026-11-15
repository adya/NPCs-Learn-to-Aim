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

		/// Maximum angular deviation (in radians) for the shot, based on the current skill. Use with the `kAccuracy` usage.
		float MaxSpread(RE::Actor* actor) const;
	};

	/// A random deviation in a random direction, never further than `maxSpread` (radians) from the aim.
	/// `focus` shapes how the shots are spread within that radius: the radius is `maxSpread * u^focus` for a uniform `u`,
	/// so 0.5 covers the disc evenly and higher values cluster the shots closer to the center. It is the skill factor of the `kPrecision` usage.
	RE::NiPoint2 RandomSpreadOffset(float maxSpread, float focus);

	/// The focus for a shot: the skill factor of the `kPrecision` usage, or an even spread over the disc when there is none.
	float SpreadFocus(const SkillUsage& precision, RE::Actor* actor);

	/// `RandomSpreadOffset` for a shot with the given `kAccuracy` (max spread) and `kPrecision` (focus) usages.
	RE::NiPoint2 RandomSpreadOffset(const SkillUsage& accuracy, const SkillUsage& precision, RE::Actor* actor);
}
