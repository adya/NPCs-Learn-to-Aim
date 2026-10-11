#include "Menu+Internal.h"

namespace NLA::Menu
{
	namespace
	{
		constexpr Category<HeadshotRows, SpeedRows> kCrossbows{
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
		};
	}

	void RenderCrossbowsPage(const PageContext& ctx) {
		RenderCategory(kCrossbows, ctx);
	}
}
