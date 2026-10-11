#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr Category<UseHighestMagicSkillRow, RangeRows, SpeedRows> kConcentrationSpells{
			{
				.id = "ConcentrationSpells",
				.label = kScaleAimLabel,
				.tooltip = "When enabled, corresponding Magic skill will affect aiming with concentration spells (flames/sparks/frost/etc). Stream/Beam will wander around the aimed point.",
				.aiming = &Config::concentrationSpellsAiming,
				.multiplier = &Config::SkillMultiplier::concentrationSpell,
				.accuracyTooltip = "How much the skill contributes to Accuracy with concentration spells. The higher this multiplier is the easier it is to aim.",
				.precisionTooltip = "How much the skill contributes to Precision with concentration spells. The higher this multiplier is the easier it is to hit the target.",
				.resetLabel = "Reset Concentration Spells to Default",
			},
			UseHighestMagicSkillRow{
				&Config::concentrationSpellsUseHighestMagicSkill,
				"When enabled, all concentration spell options are based on the highest Magic skill, regardless of the spell's Magic school." },
			RangeRows{
				&Config::concentrationSpellRangeScaling, &Config::concentrationSpellMinRangeMultiplier, &Config::concentrationSpellMaxRangeMultiplier,
				"When enabled, corresponding Magic skill also affects concentration spells range. (the higher the skill the farther mage can hit)",
				"Concentration spell range multiplier at 0 skill. Note that range also scales spell's visual effects, so setting this too high may produce poor visuals.",
				"Concentration spell range multiplier at 100 skill. Note that range also scales spell's visual effects, so setting this too high may produce poor visuals.",
				kRangeGroup },
			SpeedRows{
				&Config::concentrationSpellChargeSpeed,
				kScaleChargeSpeedLabel,
				"When enabled, corresponding Magic skill also affects how fast concentration spells charge/cast. (the higher the skill the faster the cast)",
				kChargeSpeedGroup },
		};
	}

	void RenderConcentrationSpellsPage(const PageContext& ctx) {
		RenderCategory(kConcentrationSpells, ctx);
	}
}
