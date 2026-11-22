#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr AttackKind<SkillShotRows, SpeedRows, SpreadRows, FatigueRows> kBows{
			{
				.id = "Bows",
				.label = kScaleAimLabel,
				.kind = Options::AttackKind::kBow,
				.accuracyTooltip = "How strongly Archery skill improves accuracy with bows. Higher means smaller misses.",
				.precisionTooltip = "How strongly Archery skill bunches shots near the aim.",
				.resetLabel = "Reset Bows to Default",
			},
			SkillShotRows{
				"Skill Shots",
				"Skilled archers sometimes aim for a body part instead of the chest. Pick the parts on the General page.",
				"archer",
				"Archery skill" },
			SpeedRows{
				kScaleDrawSpeedLabel,
				kDrawSpeedGroup },
			SpreadRows{
				"Full Draw Steadies Aim",
				"Holding a full draw makes the aim steadier over time.",
				"The most spread that holding can remove. It takes a while to get there.",
				"Time to reach half of the max spread reduction. Shorter means the aim steadies faster." },
			FatigueRows{
				"Full Draw Fatigues Aim",
				"Holding a full draw too long tires the archer and the aim gets worse. More Stamina delays this.",
				"How long a draw can be held before the archer tires, with no Stamina. More Stamina extends it.",
				"How long a draw can be held before the archer tires, with 100 max Stamina. More Stamina extends it further.",
				"The most extra spread that tiring can add. It also cancels any steadying bonus." },
		};
	}

	void RenderBowsPage(const PageContext& ctx) {
		RenderAttackKind(kBows, ctx);
	}
}
