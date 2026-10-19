#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr Category<HeadshotRows, SpeedRows, SpreadRows> kCrossbows{
			{
				.id = "Crossbows",
				.label = kScaleAimLabel,
				.tooltip = "When enabled, Archery skill will affect aiming with crossbows.",
				.aiming = &Config::crossbowAiming,
				.multiplier = &Config::SkillMultiplier::crossbow,
				.accuracyTooltip = "How much Archery skill contributes to Accuracy with crossbows. The higher this multiplier is the easier it is to aim.",
				.precisionTooltip = "How much Archery skill contributes to Precision with crossbows. The higher this multiplier is the easier it is to hit the target.",
				.resetLabel = "Reset Crossbows to Default",
			},
			HeadshotRows{
				&HeadshotSettings::crossbow, &HeadshotSettings::crossbowMinChance, &HeadshotSettings::crossbowMaxChance,
				"Crossbow Headshots",
				"When enabled, sufficiently skilled crossbowmen have a chance to aim for the head instead of the chest.",
				"crossbowman" },
			SpeedRows{
				&Config::crossbowDrawSpeed,
				kScaleDrawSpeedLabel,
				"When enabled, Archery skill also affects how fast a crossbow is reloaded. (the higher the skill the faster the reload)",
					kDrawSpeedGroup },
				SpreadRows{
					&Config::crossbowSteadying,
					"Holding a Loaded Crossbow Steadies Aim",
					"When enabled, holding a loaded crossbow before firing tightens its spread, with diminishing returns. Crossbows never fatigue the shooter.",
					"The most spread that holding a loaded crossbow can remove, approached the longer it's held.",
					"Duration of holding a loaded crossbow to get roughly half of the max spread reduction. Lower means the aim steadies faster." },
		};
	}

	void RenderCrossbowsPage(const PageContext& ctx) {
		RenderCategory(kCrossbows, ctx);
	}
}
