#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr AttackKind<HeadshotRows, SpeedRows, SpreadRows, FatigueRows> kStaves{
			{
				.id = "Staves",
				.label = kScaleAimLabel,
				.tooltip = "When enabled, corresponding Magic skill will affect aiming with staves.",
				.kind = Options::AttackKind::kStaff,
				.aimTooltip = "How much the skill contributes to Aim with staves. The higher this multiplier is the easier it is to aim at the target.",
				.accuracyTooltip = "How much the skill contributes to Accuracy with staves. The higher this multiplier is the smaller the widest miss can be.",
				.precisionTooltip = "How much the skill contributes to Precision with staves. The higher this multiplier is the more shots land close to the aim.",
				.resetLabel = "Reset Staves to Default",
			},
			HeadshotRows{
				"Staff Headshots",
				"When enabled, sufficiently skilled mages have a chance to aim staves at the head instead of the chest.",
				"mage",
				"aiming skill" },
			SpeedRows{
				kScaleChargeSpeedLabel,
				"When enabled, corresponding skill also affects how fast staves charge/cast. (the higher the skill the faster the cast)",
				kChargeSpeedGroup },
			SpreadRows{
				kSteadyingLabel,
				"When enabled, holding a fully charged staff before releasing it tightens its spread. The longer it's held, the steadier the shot, with diminishing returns.",
				"The most spread that holding can remove, approached the longer the staff is held. 50% means the spread can at best be halved.",
				"Duration of holding a fully charged staff to get roughly half of the max spread reduction. Lower means the aim steadies faster. 1 sec with 50% reduction means that in 1 second spread will be reduced by 25%." },
			CastFatigue("When enabled, holding a staff charged for too long tires the caster's arm, growing spread back the longer it's held past their fatigue threshold. That threshold grows with the caster's max Magicka."),
		};
	}

	void RenderStavesPage(const PageContext& ctx) {
		RenderAttackKind(kStaves, ctx);
	}
}
