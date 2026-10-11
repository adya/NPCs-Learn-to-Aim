#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr Category<SkillSourceRows, SpeedRows, SpreadRows, FatigueRows> kScrolls{
			{
				.id = "Scrolls",
				.label = kScaleAimLabel,
				.tooltip = "When enabled, corresponding Magic skill will affect aiming with scrolls.",
				.aiming = &Config::scrollAiming,
				.multiplier = &Config::SkillMultiplier::scroll,
				.accuracyTooltip = "How much the skill contributes to Accuracy with scrolls. The higher this multiplier is the easier it is to aim.",
				.precisionTooltip = "How much the skill contributes to Precision with scrolls. The higher this multiplier is the easier it is to hit the target.",
				.resetLabel = "Reset Scrolls to Default",
			},
			SkillSourceRows{
				.useEnchantingSkill = &Config::scrollsUseEnchantingSkill,
				.useEnchantingSkillTooltip = "When enabled, scroll aiming is based on the Enchanting skill instead of the scroll spell's Magic school. Mutually exclusive with Use Highest Magic Skill.",
				.useHighestMagicSkill = &Config::scrollsUseHighestMagicSkill,
				.useHighestMagicSkillTooltip = "When enabled, scroll aiming is based on the highest Magic skill, regardless of the scroll spell's Magic school. Mutually exclusive with Use Enchanting Skill." },
			SpeedRows{
				&Config::scrollChargeSpeed,
				kScaleChargeSpeedLabel,
				"When enabled, corresponding skill also affects how fast scrolls charge/cast. (the higher the skill the faster the cast)",
					kChargeSpeedGroup },
			SpreadRows{
				&Config::scrollOvercharge,
				kOverchargeLabel,
				"When enabled, holding a fully charged scroll before releasing it tightens its spread. The longer it's held, the steadier the shot, with diminishing returns.",
				"The most spread that overcharging can remove, approached the longer the scroll is held. 50% means the spread can at best be halved.",
				"Duration of holding a fully charged scroll to get roughly half of the max spread reduction. Lower means the aim steadies faster. 1 sec with 50% reduction means that in 1 second spread will be reduced by 25%." },
			CastFatigue(&Config::scrollFatigue, "When enabled, holding a scroll overcharged for too long tires the caster's arm, growing spread back the longer it's held past their fatigue threshold. That threshold scales with Magic skill."),
		};
	}

	void RenderScrollsPage(const PageContext& ctx) {
		RenderCategory(kScrolls, ctx);
	}
}
