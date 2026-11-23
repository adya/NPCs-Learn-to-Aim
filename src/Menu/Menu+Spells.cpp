#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr AttackKind<SkillShotRows, SpeedRows, SpreadRows, FatigueRows> kSpells{
			{
				.id = "Spells",
				.toggleLabel = "Improve Spells",
				.kind = Options::AttackKind::kSpell,
				.accuracyTooltip = "How strongly skill improves accuracy with spells. Higher means smaller misses.",
				.precisionTooltip = "How strongly skill bunches spells near the aim.",
			},
			SkillShotRows{
				"Skill Shots",
				"Skilled mages sometimes aim for a body part instead of the chest. Pick the parts on the General page.",
				"mage",
				"aiming skill" },
			SpeedRows{
				kChargeSpeedGroup },
			SpreadRows{
				"Holding a charged spell makes the aim steadier over time.",
				"The most spread that holding can remove. 50% means at best half as wide.",
				"Time to reach half of the max spread reduction. Shorter means the aim steadies faster." },
			CastFatigue("Holding a charged spell too long tires the caster and the aim gets worse. More Magicka delays this."),
		};
	}

	void RenderSpellsPage(const PageContext& ctx) {
		RenderAttackKind(kSpells, ctx);
	}
}
