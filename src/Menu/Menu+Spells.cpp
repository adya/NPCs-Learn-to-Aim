#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr Category<SpeedRows, SpreadRows, FatigueRows> kSpells{
			{
				.id = "Spells",
				.label = kScaleAimLabel,
				.tooltip = "When enabled, corresponding Magic skill will affect aiming with spells.",
				.aiming = &Config::spellAiming,
				.multiplier = &Config::SkillMultiplier::spell,
				.accuracyTooltip = "How much the skill contributes to Accuracy with fire-and-forget spells. The higher this multiplier is the easier it is to aim.",
				.precisionTooltip = "How much the skill contributes to Precision with fire-and-forget spells. The higher this multiplier is the easier it is to hit the target.",
				.resetLabel = "Reset Spells to Default",
				.aimSkill = AimingSkill(&Config::spellSkills),
			},
			SpeedRows{
				&Config::spellChargeSpeed,
				kScaleChargeSpeedLabel,
				"When enabled, corresponding Magic skill also affects how fast fire-and-forget spells charge/cast. (the higher the skill the faster the cast)",
				kChargeSpeedGroup,
				ChargeSpeedSkill(&Config::spellSkills) },
			SpreadRows{
				&Config::spellSteadying,
				kSteadyingLabel,
				"When enabled, holding a fully charged spell before releasing it tightens its spread. The longer it's held, the steadier the shot, with diminishing returns.",
				"The most spread that holding can remove, approached the longer the spell is held. 50% means the spread can at best be halved.",
				"Duration of holding a fully charged spell to get roughly half of the max spread reduction. Lower means the aim steadies faster. 1 sec with 50% reduction means that in 1 second spread will be reduced by 25%." },
			CastFatigue(&Config::spellFatigue, "When enabled, holding a spell charged for too long tires the caster's arm, growing spread back the longer it's held past their fatigue threshold. That threshold grows with the caster's max Magicka."),
		};
	}

	void RenderSpellsPage(const PageContext& ctx) {
		RenderCategory(kSpells, ctx);
	}
}
