#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr AttackKind<HeadshotRows, SpeedRows, SpreadRows, FatigueRows> kScrolls{
			{
				.id = "Scrolls",
				.label = kScaleAimLabel,
				.tooltip = "When enabled, corresponding Magic skill will affect aiming with scrolls.",
				.kind = Options::AttackKind::kScroll,
				.accuracyTooltip = "How much the skill contributes to Accuracy with scrolls. The higher this multiplier is the easier it is to aim.",
				.precisionTooltip = "How much the skill contributes to Precision with scrolls. The higher this multiplier is the easier it is to hit the target.",
				.resetLabel = "Reset Scrolls to Default",
			},
			HeadshotRows{
				"Scroll Headshots",
				"When enabled, sufficiently skilled mages have a chance to aim scrolls at the head instead of the chest.",
				"mage",
				"aiming skill" },
			SpeedRows{
				kScaleChargeSpeedLabel,
				"When enabled, corresponding skill also affects how fast scrolls charge/cast. (the higher the skill the faster the cast)",
				kChargeSpeedGroup },
			SpreadRows{
				kSteadyingLabel,
				"When enabled, holding a fully charged scroll before releasing it tightens its spread. The longer it's held, the steadier the shot, with diminishing returns.",
				"The most spread that holding can remove, approached the longer the scroll is held. 50% means the spread can at best be halved.",
				"Duration of holding a fully charged scroll to get roughly half of the max spread reduction. Lower means the aim steadies faster. 1 sec with 50% reduction means that in 1 second spread will be reduced by 25%." },
			CastFatigue("When enabled, holding a scroll charged for too long tires the caster's arm, growing spread back the longer it's held past their fatigue threshold. That threshold grows with the caster's max Magicka."),
		};
	}

	void RenderScrollsPage(const PageContext& ctx) {
		RenderAttackKind(kScrolls, ctx);
	}
}
