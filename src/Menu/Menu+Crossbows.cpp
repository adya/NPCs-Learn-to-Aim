#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr AttackKind<SkillShotRows, SpeedRows, SpreadRows, FatigueRows> kCrossbows{
			{
				.id = "Crossbows",
				.label = kScaleAimLabel,
				.kind = Options::AttackKind::kCrossbow,
				.accuracyTooltip = "How strongly Archery skill improves accuracy with crossbows. Higher means smaller misses.",
				.precisionTooltip = "How strongly Archery skill bunches shots near the aim.",
				.resetLabel = "Reset Crossbows to Default",
			},
			SkillShotRows{
				"Skill Shots",
				"Skilled crossbow users sometimes aim for a body part instead of the chest. Pick the parts on the General page.",
				"crossbowman",
				"Archery skill" },
			SpeedRows{
				kScaleDrawSpeedLabel,
				kDrawSpeedGroup },
			SpreadRows{
				"Loaded Crossbow Steadies Aim",
				"Holding a loaded crossbow makes the aim steadier over time.",
				"The most spread that holding can remove. It takes a while to get there.",
				"Time to reach half of the max spread reduction. Shorter means the aim steadies faster." },
			FatigueRows{
				"Loaded Crossbow Fatigues Aim",
				"Holding a loaded crossbow too long tires the shooter and the aim gets worse. More Stamina delays this.",
				"How long a crossbow can be held before the shooter tires, with no Stamina. More Stamina extends it.",
				"How long a crossbow can be held before the shooter tires, with 100 max Stamina. More Stamina extends it further.",
				"The most extra spread that tiring can add. It also cancels any steadying bonus." },
		};
	}

	void RenderCrossbowsPage(const PageContext& ctx) {
		RenderAttackKind(kCrossbows, ctx);
	}
}
