#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr AttackKind<HeadshotRows, RangeRows, SpeedRows, SpreadRows, FatigueRows> kConcentrationStaves{
			{
				.id = "ConcentrationStaves",
				.label = kScaleAimLabel,
				.tooltip = "When enabled, corresponding Magic skill will affect aiming with staves that cast concentration spells (e.g. Staff of Flames). Stream/Beam will wander around the aimed point.",
				.kind = Options::AttackKind::kConcentrationStaff,
				.accuracyTooltip = "How much the skill contributes to Accuracy with concentration staves. The higher this multiplier is the easier it is to aim.",
				.precisionTooltip = "How much the skill contributes to Precision with concentration staves. The higher this multiplier is the easier it is to hit the target.",
				.resetLabel = "Reset Concentration Staves to Default",
			},
			HeadshotRows{
				"Concentration Staff Headshots",
				"When enabled, sufficiently skilled mages have a chance to aim concentration staves at the head instead of the chest.",
				"mage",
				"aiming skill" },
			RangeRows{
				"When enabled, corresponding skill also affects concentration staves range. (the higher the skill the farther mage can hit)",
				"Concentration staff range multiplier at 0 skill. Note that range also scales spell's visual effects, so setting this too high may produce poor visuals.",
				"Concentration staff range multiplier at 100 skill. Note that range also scales spell's visual effects, so setting this too high may produce poor visuals.",
				kRangeGroup },
			SpeedRows{
				kScaleChargeSpeedLabel,
				"When enabled, corresponding skill also affects how fast concentration staves charge/cast. (the higher the skill the faster the cast)",
				kChargeSpeedGroup },
			SpreadRows{
				"Casting Steadies Aim",
				"When enabled, the longer a concentration staff is cast, the steadier the stream/beam wanders, with diminishing returns.",
				"The most wander that casting can remove, approached the longer the staff is cast. 50% means the wander can at best be halved.",
				"Duration of casting a concentration staff to get roughly half of the max wander reduction. Lower means the aim steadies faster. 1 sec with 50% reduction means that in 1 second wander will be reduced by 25%." },
			FatigueRows{
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
