#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr Category<SkillSourceRows, RangeRows, SpeedRows> kConcentrationStaves{
			{
				.id = "ConcentrationStaves",
				.label = kScaleAimLabel,
				.tooltip = "When enabled, corresponding Magic skill will affect aiming with staves that cast concentration spells (e.g. Staff of Flames). Stream/Beam will wander around the aimed point.",
				.aiming = &Config::concentrationStaffAiming,
				.multiplier = &Config::SkillMultiplier::concentrationStaff,
				.accuracyTooltip = "How much the skill contributes to Accuracy with concentration staves. The higher this multiplier is the easier it is to aim.",
				.precisionTooltip = "How much the skill contributes to Precision with concentration staves. The higher this multiplier is the easier it is to hit the target.",
				.resetLabel = "Reset Concentration Staves to Default",
			},
			SkillSourceRows{
				.useEnchantingSkill = &Config::concentrationStavesUseEnchantingSkill,
				.useEnchantingSkillTooltip = "When enabled, all concentration staff options are based on the Enchanting skill instead of the staff's spell's Magic school. Mutually exclusive with Use Highest Magic Skill.",
				.useHighestMagicSkill = &Config::concentrationStavesUseHighestMagicSkill,
				.useHighestMagicSkillTooltip = "When enabled, all concentration staff options are based on the highest Magic skill, regardless of the staff's spell's Magic school. Mutually exclusive with Use Enchanting Skill." },
			RangeRows{
				&Config::concentrationStaffRangeScaling, &Config::concentrationStaffMinRangeMultiplier, &Config::concentrationStaffMaxRangeMultiplier,
				"When enabled, corresponding skill also affects concentration staves range. (the higher the skill the farther mage can hit)",
				"Concentration staff range multiplier at 0 skill. Note that range also scales spell's visual effects, so setting this too high may produce poor visuals.",
				"Concentration staff range multiplier at 100 skill. Note that range also scales spell's visual effects, so setting this too high may produce poor visuals.",
				kRangeGroup },
			SpeedRows{
				&Config::concentrationStaffChargeSpeed,
				kScaleChargeSpeedLabel,
				"When enabled, corresponding skill also affects how fast concentration staves charge/cast. (the higher the skill the faster the cast)",
				kChargeSpeedGroup },
		};
	}

	void RenderConcentrationStavesPage(const PageContext& ctx) {
		RenderCategory(kConcentrationStaves, ctx);
	}
}
