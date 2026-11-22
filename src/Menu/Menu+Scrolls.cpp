#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr AttackKind<SkillShotRows, SpeedRows, SpreadRows, FatigueRows> kScrolls{
			{
				.id = "Scrolls",
				.label = kScaleAimLabel,
				.kind = Options::AttackKind::kScroll,
				.accuracyTooltip = "How strongly skill improves accuracy with scrolls. Higher means smaller misses.",
				.precisionTooltip = "How strongly skill bunches scroll shots near the aim.",
				.resetLabel = "Reset Scrolls to Default",
			},
			SkillShotRows{
				"Skill Shots",
				"Skilled mages sometimes aim for a body part instead of the chest. Pick the parts on the General page.",
				"mage",
				"aiming skill" },
			SpeedRows{
				kScaleChargeSpeedLabel,
				kChargeSpeedGroup },
			SpreadRows{
				kSteadyingLabel,
				"Holding a charged scroll makes the aim steadier over time.",
				"The most spread that holding can remove. 50% means at best half as wide.",
				"Time to reach half of the max spread reduction. Shorter means the aim steadies faster." },
			CastFatigue("Holding a charged scroll too long tires the caster and the aim gets worse. More Magicka delays this."),
		};
	}

	void RenderScrollsPage(const PageContext& ctx) {
		RenderAttackKind(kScrolls, ctx);
	}
}
