#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr Category<UseHighestMagicSkillRow, SpeedRows, SpreadRows, FatigueRows> kSpells{
			{
				.id = "Spells",
				.label = kScaleAimLabel,
				.tooltip = "When enabled, corresponding Magic skill will affect aiming with spells.",
				.aiming = &Config::spellAiming,
				.multiplier = &Config::SkillMultiplier::spell,
				.accuracyTooltip = "How much the skill contributes to Accuracy with fire-and-forget spells. The higher this multiplier is the easier it is to aim.",
				.precisionTooltip = "How much the skill contributes to Precision with fire-and-forget spells. The higher this multiplier is the easier it is to hit the target.",
				.resetLabel = "Reset Spells to Default",
			},
			UseHighestMagicSkillRow{
				&Config::spellsUseHighestMagicSkill,
				"When enabled, spell aiming (including concentration spells) is based on the highest Magic skill, regardless of the spell's Magic school.",
				&Config::concentrationSpellsAiming },
			SpeedRows{
				&Config::spellChargeSpeed,
				kScaleChargeSpeedLabel,
				"When enabled, corresponding Magic skill also affects how fast fire-and-forget spells charge/cast. (the higher the skill the faster the cast)",
				kChargeSpeedGroup },
			SpreadRows{
				&Config::spellOvercharge,
				kOverchargeLabel,
				"When enabled, holding a fully charged spell before releasing it tightens its spread. The longer it's held, the steadier the shot, with diminishing returns.",
				"The most spread that overcharging can remove, approached the longer the spell is held. 50% means the spread can at best be halved.",
				"Duration of holding a fully charged spell to get roughly half of the max spread reduction. Lower means the aim steadies faster. 1 sec with 50% reduction means that in 1 second spread will be reduced by 25%." },
			CastFatigue(&Config::spellFatigue, "When enabled, holding a spell overcharged for too long tires the caster's arm, growing spread back the longer it's held past their fatigue threshold. That threshold scales with Magic skill."),
		};

		constexpr Category<RangeRows, SpeedRows> kConcentrationSpells{
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
			RangeRows{
				&Config::concentrationSpellRangeScaling, &Config::concentrationSpellMinRangeMultiplier, &Config::concentrationSpellMaxRangeMultiplier,
				"When enabled, corresponding Magic skill also affects concentration spells range. (the higher the skill the farther mage can hit)",
				"Concentration spell range multiplier at 0 skill. Note that range also scales spell's visual effects, so setting this too high may produce poor visuals.",
				"Concentration spell range multiplier at 100 skill. Note that range also scales spell's visual effects, so setting this too high may produce poor visuals." },
			SpeedRows{
				&Config::concentrationSpellChargeSpeed,
				kScaleChargeSpeedLabel,
				"When enabled, corresponding Magic skill also affects how fast concentration spells charge/cast. (the higher the skill the faster the cast)" },
		};
	}

	void RenderSpellsPage(const PageContext& ctx) {
		RenderCategory(kSpells, ctx);
		RenderCategory(kConcentrationSpells, ctx, "Concentration Spells");
	}
}
