#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr AttackKind<SkillShotRows, RangeRows, SpeedRows, SpreadRows, FatigueRows> kConcentrationSpells{
			{
				.id = "ConcentrationSpells",
				.label = kScaleAimLabel,
				.kind = Options::AttackKind::kConcentrationSpell,
				.accuracyTooltip = "How strongly skill improves accuracy with concentration spells. Higher means a narrower wander.",
				.precisionTooltip = "How strongly skill keeps the beam near the aim.",
				.resetLabel = "Reset Concentration Spells to Default",
			},
			SkillShotRows{
				"Skill Shots",
				"Skilled mages sometimes aim for a body part instead of the chest. Pick the parts on the General page.",
				"mage",
				"aiming skill" },
			RangeRows{
				"Range at 0 skill, as a multiple of normal. Very high values can look odd.",
				"Range at 100 skill, as a multiple of normal. Very high values can look odd.",
				kRangeGroup },
			SpeedRows{
				kScaleChargeSpeedLabel,
				kChargeSpeedGroup },
			SpreadRows{
				"Casting Steadies Aim",
				"The longer a spell is cast, the less the beam wanders.",
				"The most wander that casting can remove. 50% means at best half as wide.",
				"Time to reach half of the max spread reduction. Shorter means the beam steadies faster." },
			FatigueRows{
				"Prolonged Cast Fatigues Aim",
				"Casting too long tires the caster and the beam wanders wider. More Magicka delays this.",
				"How long a spell can be cast before the caster tires, with no Magicka. More Magicka extends it.",
				"How long a spell can be cast before the caster tires, with 100 max Magicka. More Magicka extends it further.",
				"The most extra wander that tiring can add. 100% means up to twice as wide." },
		};
	}

	void RenderConcentrationSpellsPage(const PageContext& ctx) {
		RenderAttackKind(kConcentrationSpells, ctx);
	}
}
