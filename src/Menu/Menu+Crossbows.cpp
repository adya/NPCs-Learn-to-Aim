#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr AttackKind<SkillShotRows, SpeedRows, SpreadRows, FatigueRows> kCrossbows{
			{
				.id = "Crossbows",
				.label = kScaleAimLabel,
				.tooltip = "When enabled, Archery skill will affect aiming with crossbows.",
				.kind = Options::AttackKind::kCrossbow,
				.accuracyTooltip = "How much Archery skill contributes to Accuracy with crossbows. The higher this multiplier is the smaller the widest miss can be.",
				.precisionTooltip = "How much Archery skill contributes to Precision with crossbows. The higher this multiplier is the more shots land close to the aim.",
				.resetLabel = "Reset Crossbows to Default",
			},
			SkillShotRows{
				"Crossbow Skill Shots",
				"When enabled, sufficiently skilled crossbowmans take skill shots: a share of their shots, set below, aims at a specific body part instead of the chest. Which parts it may pick is set on the General page.",
				"crossbowman",
				"Archery skill" },
			SpeedRows{
				kScaleDrawSpeedLabel,
				"When enabled, Archery skill also affects how fast a crossbow is reloaded. (the higher the skill the faster the reload)",
				kDrawSpeedGroup },
			SpreadRows{
				"Holding a Loaded Crossbow Steadies Aim",
				"When enabled, holding a loaded crossbow before firing tightens its spread, with diminishing returns.",
				"The most spread that holding a loaded crossbow can remove, approached the longer it's held.",
				"Duration of holding a loaded crossbow to get roughly half of the max spread reduction. Lower means the aim steadies faster." },
			FatigueRows{
				"Loaded Crossbow Fatigues Aim",
				"When enabled, holding a loaded crossbow for too long tires the shooter's arms, growing spread back the longer it's held past their fatigue threshold. That threshold grows with the shooter's max Stamina.",
				"Seconds a loaded crossbow can be held before fatigue sets in for a shooter with no Stamina. A higher max Stamina extends it.",
				"Seconds a loaded crossbow can be held before fatigue sets in for a shooter with 100 max Stamina. Max Stamina above that extends it further, with diminishing returns.",
				"The most spread that fatigue can add. Note, that in fatigued state, the bonus from steadied aim is negated before adding extra spread." },
		};
	}

	void RenderCrossbowsPage(const PageContext& ctx) {
		RenderAttackKind(kCrossbows, ctx);
	}
}
