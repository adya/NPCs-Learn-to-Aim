#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr AttackKind<RangeRows, SpeedRows, FatigueRows> kConcentrationStaves{
			{
				.id = "ConcentrationStaves",
				.label = kScaleAimLabel,
				.tooltip = "When enabled, corresponding Magic skill will affect aiming with staves that cast concentration spells (e.g. Staff of Flames). Stream/Beam will wander around the aimed point.",
				.aiming = &Config::concentrationStaffAiming,
				.multiplier = &Config::SkillMultiplier::concentrationStaff,
				.accuracyTooltip = "How much the skill contributes to Accuracy with concentration staves. The higher this multiplier is the easier it is to aim.",
				.precisionTooltip = "How much the skill contributes to Precision with concentration staves. The higher this multiplier is the easier it is to hit the target.",
				.resetLabel = "Reset Concentration Staves to Default",
				.aimSkill = AimingSkill(&Config::concentrationStaffSkills),
			},
			RangeRows{
				&Config::concentrationStaffRange,
				"When enabled, corresponding skill also affects concentration staves range. (the higher the skill the farther mage can hit)",
				"Concentration staff range multiplier at 0 skill. Note that range also scales spell's visual effects, so setting this too high may produce poor visuals.",
				"Concentration staff range multiplier at 100 skill. Note that range also scales spell's visual effects, so setting this too high may produce poor visuals.",
				kRangeGroup,
				RangeSkill(&Config::concentrationStaffSkills) },
			SpeedRows{
				&Config::concentrationStaffChargeSpeed,
				kScaleChargeSpeedLabel,
				"When enabled, corresponding skill also affects how fast concentration staves charge/cast. (the higher the skill the faster the cast)",
				kChargeSpeedGroup,
				ChargeSpeedSkill(&Config::concentrationStaffSkills) },
			FatigueRows{
				&Config::concentrationStaffFatigue,
				"Prolonged Cast Fatigues Aim",
				"When enabled, casting a concentration staff continuously for too long tires the caster's arm, making the stream/beam wander wider the longer it's cast past their fatigue threshold. That threshold grows with the caster's max Magicka.",
				"Seconds a concentration staff can be cast continuously before fatigue sets in for a caster with no Magicka. A higher max Magicka extends it.",
				"Seconds a concentration staff can be cast continuously before fatigue sets in for a caster with 100 max Magicka. Max Magicka above that extends it further, with diminishing returns.",
				"The most extra spread that fatigue can add on top of the normal stream/beam wander. 100% means the wander can grow up to twice as wide." },
		};
	}

	void RenderConcentrationStavesPage(const PageContext& ctx) {
		RenderAttackKind(kConcentrationStaves, ctx);
	}
}
