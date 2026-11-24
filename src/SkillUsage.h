#pragma once

#include "Options.h"

namespace NLA
{
	struct SkillUsage : Options::SkillBasedValue
	{
		RE::ActorValue             skill{ RE::ActorValue::kNone };
		const Options::AttackKind* kind{ nullptr };

		Options::Bounds bounds{};

		SkillUsageContext context{ kAccuracy };

		/// What a fully blind shooter's skill factor is multiplied by. Only Accuracy, Precision and Retarget use it, see `Details::BlindedFactor`.
		float blindnessMultiplier{ 1.0f };

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

	/// Internals of `SkillUsage` and `RandomSpreadOffset`, open only so that the menu preview can replay the same maths with different settings.
	/// The live path goes through `SkillUsage`, which also handles a missing actor or skill.
	namespace Details
	{
		/// The skill that a `source` stands for. `spell` is null for weapons, and also when no particular spell is at hand (the menu preview): magic
		/// (`magic`) then falls back to the caster's best school, like a spell that has none.
		RE::ActorValue SkillForSource(Options::SkillSource source, const RE::MagicItem* spell, bool magic, RE::Actor* caster);

		/// The value of a skill-scaled option at the given effective skill (the skill times the option's skill multiplier).
		float SkillFactorFor(const Options::Bounds& bounds, float effectiveSkill);

		/// The skill factor of a `kAccuracy`, `kPrecision` or `kRetarget` usage for a shooter that is `blindness` (0 to 1) blind. `multiplier` is what the
		/// factor is multiplied by at full blindness: the change scales linearly with blindness, so x1.50 makes a fully blind shooter's Accuracy factor
		/// (the spread) and Retarget factor (the step) 50% bigger and their Precision factor (the focus) 50% smaller. Other usages are returned as they are.
		float BlindedFactor(SkillUsageContext context, float factor, float multiplier, float blindness);

		/// The widest deviation (in radians) for the given `kAccuracy` skill factor.
		float MaxSpreadFor(float accuracyFactor);

		/// The deviation for a given roll: `radiusRoll` is a uniform number from 0 to 1, `angle` the direction in radians. This is the maths behind
		/// `RandomSpreadOffset`.
		RE::NiPoint2 SpreadOffset(float maxSpread, float focus, float radiusRoll, float angle);
	}

	/// A random deviation in a random direction, never further than `maxSpread` (radians) from the aim.
	/// `focus` shapes how the shots are spread within that radius: the radius is `maxSpread * u^focus` for a uniform `u`,
	/// so 0.5 covers the disc evenly and higher values cluster the shots closer to the center. It is the skill factor of the `kPrecision` usage.
	RE::NiPoint2 RandomSpreadOffset(float maxSpread, float focus);

	/// The focus for a shot: the skill factor of the `kPrecision` usage, or an even spread over the disc when there is none.
	float SpreadFocus(const SkillUsage& precision, RE::Actor* actor);

	/// `RandomSpreadOffset` for a shot with the given `kAccuracy` (max spread) and `kPrecision` (focus) usages.
	RE::NiPoint2 RandomSpreadOffset(const SkillUsage& accuracy, const SkillUsage& precision, RE::Actor* actor);
}
