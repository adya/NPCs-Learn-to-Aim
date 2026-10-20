#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr Category<SpeedRows, SpreadRows, FatigueRows> kStaves{
			{
				.id = "Staves",
				.label = kScaleAimLabel,
				.tooltip = "When enabled, corresponding Magic skill will affect aiming with staves.",
				.aiming = &Config::staffAiming,
				.multiplier = &Config::SkillMultiplier::staff,
				.accuracyTooltip = "How much the skill contributes to Accuracy with staves. The higher this multiplier is the easier it is to aim.",
				.precisionTooltip = "How much the skill contributes to Precision with staves. The higher this multiplier is the easier it is to hit the target.",
				.resetLabel = "Reset Staves to Default",
				.aimSkill = AimingSkill(&Config::staffSkills),
			},
			SpeedRows{
				&Config::staffChargeSpeed,
				kScaleChargeSpeedLabel,
				"When enabled, corresponding skill also affects how fast staves charge/cast. (the higher the skill the faster the cast)",
				kChargeSpeedGroup,
				ChargeSpeedSkill(&Config::staffSkills) },
			SpreadRows{
				&Config::staffSteadying,
				kSteadyingLabel,
				"When enabled, holding a fully charged staff before releasing it tightens its spread. The longer it's held, the steadier the shot, with diminishing returns.",
				"The most spread that holding can remove, approached the longer the staff is held. 50% means the spread can at best be halved.",
				"Duration of holding a fully charged staff to get roughly half of the max spread reduction. Lower means the aim steadies faster. 1 sec with 50% reduction means that in 1 second spread will be reduced by 25%." },
			CastFatigue(&Config::staffFatigue, "When enabled, holding a staff charged for too long tires the caster's arm, growing spread back the longer it's held past their fatigue threshold. That threshold grows with the caster's max Magicka."),
		};
	}

	void RenderStavesPage(const PageContext& ctx) {
		RenderCategory(kStaves, ctx);
	}
}
