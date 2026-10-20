#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr Category<RangeRows, SpeedRows, FatigueRows> kConcentrationSpells{
			{
				.id = "ConcentrationSpells",
				.label = kScaleAimLabel,
				.tooltip = "When enabled, corresponding Magic skill will affect aiming with concentration spells (flames/sparks/frost/etc). Stream/Beam will wander around the aimed point.",
				.aiming = &Config::concentrationSpellAiming,
				.multiplier = &Config::SkillMultiplier::concentrationSpell,
				.accuracyTooltip = "How much the skill contributes to Accuracy with concentration spells. The higher this multiplier is the easier it is to aim.",
				.precisionTooltip = "How much the skill contributes to Precision with concentration spells. The higher this multiplier is the easier it is to hit the target.",
				.resetLabel = "Reset Concentration Spells to Default",
				.aimSkill = AimingSkill(&Config::concentrationSpellSkills),
			},
			RangeRows{
				&Config::concentrationSpellRange,
				"When enabled, corresponding Magic skill also affects concentration spells range. (the higher the skill the farther mage can hit)",
				"Concentration spell range multiplier at 0 skill. Note that range also scales spell's visual effects, so setting this too high may produce poor visuals.",
				"Concentration spell range multiplier at 100 skill. Note that range also scales spell's visual effects, so setting this too high may produce poor visuals.",
				kRangeGroup,
				RangeSkill(&Config::concentrationSpellSkills) },
			SpeedRows{
				&Config::concentrationSpellChargeSpeed,
				kScaleChargeSpeedLabel,
				"When enabled, corresponding Magic skill also affects how fast concentration spells charge/cast. (the higher the skill the faster the cast)",
				kChargeSpeedGroup,
				ChargeSpeedSkill(&Config::concentrationSpellSkills) },
			FatigueRows{
				&Config::concentrationSpellFatigue,
				"Prolonged Cast Fatigues Aim",
				"When enabled, casting a concentration spell continuously for too long tires the caster's arm, making the stream/beam wander wider the longer it's cast past their fatigue threshold. That threshold grows with the caster's max Magicka.",
				"Seconds a concentration spell can be cast continuously before fatigue sets in for a caster with no Magicka. A higher max Magicka extends it.",
				"Seconds a concentration spell can be cast continuously before fatigue sets in for a caster with 100 max Magicka. Max Magicka above that extends it further, with diminishing returns.",
				"The most extra spread that fatigue can add on top of the normal stream/beam wander. 100% means the wander can grow up to twice as wide." },
		};
	}

	void RenderConcentrationSpellsPage(const PageContext& ctx) {
		RenderCategory(kConcentrationSpells, ctx);
	}
}
