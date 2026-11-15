#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr AttackKind<HeadshotRows, SpeedRows, SpreadRows, FatigueRows> kBows{
			{
				.id = "Bows",
				.label = kScaleAimLabel,
				.tooltip = "When enabled, Archery skill will affect aiming with bows.",
				.kind = Options::AttackKind::kBow,
				.aimTooltip = "How much Archery skill contributes to Aim with bows. The higher this multiplier is the easier it is to aim at the target.",
				.accuracyTooltip = "How much Archery skill contributes to Accuracy with bows. The higher this multiplier is the smaller the widest miss can be.",
				.precisionTooltip = "How much Archery skill contributes to Precision with bows. The higher this multiplier is the more shots land close to the aim.",
				.resetLabel = "Reset Bows to Default",
			},
			HeadshotRows{
				"Bow Headshots",
				"When enabled, sufficiently skilled archers have a chance to aim for the head instead of the chest.",
				"archer",
				"Archery skill" },
			SpeedRows{
				kScaleDrawSpeedLabel,
				"When enabled, Archery skill also affects how fast a bow reaches full draw. (the higher the skill the faster the draw)",
				kDrawSpeedGroup },
			SpreadRows{
				"Full Draw Steadies Aim",
				"When enabled, holding a bow at full draw tightens its spread, with diminishing returns.",
				"The most spread that holding a full draw can remove, approached the longer it's held.",
				"Duration of holding a full draw to get roughly half of the max spread reduction. Lower means the aim steadies faster." },
			FatigueRows{
				"Full Draw Fatigues Aim",
				"When enabled, holding a bow at full draw for too long tires the archer's arm, growing spread back the longer it's held past their fatigue threshold. That threshold grows with the archer's max Stamina.",
				"Seconds a full draw can be held before fatigue sets in for an archer with no Stamina. A higher max Stamina extends it.",
				"Seconds a full draw can be held before fatigue sets in for an archer with 100 max Stamina. Max Stamina above that extends it further, with diminishing returns.",
				"The most spread that fatigue can add. Note, that in fatigued state, the bonus from steadied aim is negated before adding extra spread." },
		};
	}

	void RenderBowsPage(const PageContext& ctx) {
		RenderAttackKind(kBows, ctx);
	}
}
