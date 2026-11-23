#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr AttackKind<SkillShotRows, SpeedRows, SpreadRows, FatigueRows> kStaves{
			{
				.id = "Staves",
				.toggleLabel = "Improve Staves",
				.kind = Options::AttackKind::kStaff,
				.accuracyTooltip = "How strongly skill improves accuracy with staves. Higher means smaller misses.",
				.precisionTooltip = "How strongly skill bunches staff shots near the aim.",
			},
			SkillShotRows{
				"Skill Shots",
				"Skilled mages sometimes aim for a body part instead of the chest. Pick the parts on the General page.",
				"mage",
				"aiming skill" },
			SpeedRows{
				kChargeSpeedGroup },
			SpreadRows{
				"Holding a charged staff makes the aim steadier over time.",
				"The most spread that holding can remove. 50% means at best half as wide.",
				"Time to reach half of the max spread reduction. Shorter means the aim steadies faster." },
			CastFatigue("Holding a charged staff too long tires the caster and the aim gets worse. More Magicka delays this."),
		};
	}

	void RenderStavesPage(const PageContext& ctx) {
		RenderAttackKind(kStaves, ctx);
	}
}
