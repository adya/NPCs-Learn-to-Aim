#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr Category<SkillSourceRows, SpeedRows, SpreadRows, FatigueRows> kStaves{
			{
				.id = "Staves",
				.label = kScaleAimLabel,
				.tooltip = "When enabled, corresponding Magic skill will affect aiming with staves.",
				.aiming = &Config::staffAiming,
				.multiplier = &Config::SkillMultiplier::staff,
				.accuracyTooltip = "How much the skill contributes to Accuracy with staves. The higher this multiplier is the easier it is to aim.",
				.precisionTooltip = "How much the skill contributes to Precision with staves. The higher this multiplier is the easier it is to hit the target.",
				.resetLabel = "Reset Staves to Default",
			},
			SkillSourceRows{
				.useEnchantingSkill = &Config::stavesUseEnchantingSkill,
				.useEnchantingSkillTooltip = "When enabled, staff aiming is based on the Enchanting skill instead of the staff's spell's Magic school. Mutually exclusive with Use Highest Magic Skill.",
				.useHighestMagicSkill = &Config::stavesUseHighestMagicSkill,
				.useHighestMagicSkillTooltip = "When enabled, staff aiming is based on the highest Magic skill, regardless of the staff's spell's Magic school. Mutually exclusive with Use Enchanting Skill.",
				.sharedWith = &Config::concentrationStaffAiming },
			SpeedRows{
				&Config::staffChargeSpeed,
				kScaleChargeSpeedLabel,
				"When enabled, corresponding skill also affects how fast staves charge/cast. (the higher the skill the faster the cast)",
					kChargeSpeedGroup },
			SpreadRows{
				&Config::staffOvercharge,
				kOverchargeLabel,
				"When enabled, holding a fully charged staff before releasing it tightens its spread. The longer it's held, the steadier the shot, with diminishing returns.",
				"The most spread that overcharging can remove, approached the longer the staff is held. 50% means the spread can at best be halved.",
				"Duration of holding a fully charged staff to get roughly half of the max spread reduction. Lower means the aim steadies faster. 1 sec with 50% reduction means that in 1 second spread will be reduced by 25%." },
			CastFatigue(&Config::staffFatigue, "When enabled, holding a staff overcharged for too long tires the caster's arm, growing spread back the longer it's held past their fatigue threshold. That threshold scales with Magic skill."),
		};

		constexpr Category<RangeRows, SpeedRows> kConcentrationStaves{
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
			RangeRows{
				&Config::concentrationStaffRangeScaling, &Config::concentrationStaffMinRangeMultiplier, &Config::concentrationStaffMaxRangeMultiplier,
				"When enabled, corresponding skill also affects concentration staves range. (the higher the skill the farther mage can hit)",
				"Concentration staff range multiplier at 0 skill. Note that range also scales spell's visual effects, so setting this too high may produce poor visuals.",
				"Concentration staff range multiplier at 100 skill. Note that range also scales spell's visual effects, so setting this too high may produce poor visuals." },
			SpeedRows{
				&Config::concentrationStaffChargeSpeed,
				kScaleChargeSpeedLabel,
				"When enabled, corresponding skill also affects how fast concentration staves charge/cast. (the higher the skill the faster the cast)" },
		};
	}

	void RenderStavesPage(const PageContext& ctx) {
		RenderCategory(kStaves, ctx);
		RenderCategory(kConcentrationStaves, ctx, "Concentration Staves");
	}
}
