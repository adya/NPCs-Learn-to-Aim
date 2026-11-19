#include "Menu+Internal.h"
#include <SKSEMCP/SKSEMenuFramework.hpp>

using namespace ImGuiMCP;

namespace NLA::Menu
{
	namespace
	{
		constexpr const char* kSkillMultiplierHeader = "Skill Mult";

		void RenderSkillSource(const KindContext& kind, Options::SkillSource Config::SkillSources::*field, const char* label, const char* what) {
			if (!kind.info.magic) {
				return;
			}
			const auto tooltip = fmt::format("The skill that affects {}. Spell's Magic Skill uses the skill of the spell's Magic school, Highest Magic Skill uses the highest of the Magic skills regardless of the spell, Enchanting uses the Enchanting skill.", what);
			SkillSourceRow(label, kind.cfg.skills.*field, tooltip.c_str());
		}

		/// `what` is the thing the skill drives, e.g. "draw speed".
		std::string SkillMultiplierTooltip(std::string_view what) {
			return fmt::format("Multiplies the skill before it affects {}. Above 1.0 the skill counts for more, below 1.0 for less.", what);
		}
	}

	void SkillShotRows::Render(const KindContext& kind, TableSections& sections) const {
		if (!kind.page.isNPC || !sections.Section(nullptr)) {
			return;
		}
		auto& skillShot = kind.cfg.skillShot;

		HeaderRow(kSkillMultiplierHeader, "Min Chance", "Max Chance");
		TableNextRow();
		ToggleColumn(label, skillShot.enabled, tooltip);
		DisabledIf off{ !skillShot.enabled };
		SliderColumn("##SkillShotSkillMultiplier", skillShot.skillMultiplier, SkillMultiplierTooltip("the skill shot chance").c_str());
		PercentSliderColumn("##MinSkillShotChance", skillShot.bounds.from, fmt::format("Share of shots that a skilled {} takes as skill shots at 0 effective {}. Out of every 10 shots this many aim at a specific body part.", shooter, skill).c_str(), Options::Limits::kSkillShotChance);
		PercentSliderColumn("##MaxSkillShotChance", skillShot.bounds.to, fmt::format("Share of shots that a skilled {} takes as skill shots at 100 effective {}. Out of every 10 shots this many aim at a specific body part.", shooter, skill).c_str(), Options::Limits::kSkillShotChance);
	}

	void RangeRows::Render(const KindContext& kind, TableSections& sections) const {
		if (!sections.Section(group)) {
			return;
		}
		auto& range = kind.cfg.range;

		HeaderRow(kSkillMultiplierHeader, "Min Range", "Max Range");
		TableNextRow();
		ToggleColumn("Scale Range With Skill", range.enabled, tooltip);
		DisabledIf off{ !range.enabled };
		SliderColumn("##RangeSkillMultiplier", range.skillMultiplier, SkillMultiplierTooltip("how far the spell reaches").c_str());
		SliderColumn("##MinRange", range.bounds.from, minTooltip, Options::Limits::kRangeMultiplier);
		SliderColumn("##MaxRange", range.bounds.to, maxTooltip, Options::Limits::kRangeMultiplier);
		RenderSkillSource(kind, &Config::SkillSources::range, "Range Skill", "how far the spell reaches");
	}

	void SpeedRows::Render(const KindContext& kind, TableSections& sections) const {
		if (!sections.Section(group)) {
			return;
		}
		auto& speed = kind.cfg.speed;

		HeaderRow(kSkillMultiplierHeader, "Min Speed", "Max Speed");
		TableNextRow();
		ToggleColumn(label, speed.enabled, tooltip);
		DisabledIf off{ !speed.enabled };
		SliderColumn("##SpeedSkillMultiplier", speed.skillMultiplier, SkillMultiplierTooltip("the speed").c_str());
		SliderColumn("##MinSpeed", speed.bounds.from, "Speed multiplier at 0 skill. 1.0 is vanilla speed, higher is faster.", Options::Limits::kSpeedMultiplier);
		SliderColumn("##MaxSpeed", speed.bounds.to, "Speed multiplier at 100 skill (fastest). 1.0 is vanilla speed, higher is faster.", Options::Limits::kSpeedMultiplier);
		RenderSkillSource(kind, &Config::SkillSources::speed, "Speed Skill", "how fast the spell charges/casts");
	}

	void SpreadRows::Render(const KindContext& kind, TableSections& sections) const {
		if (!sections.Section("Steadying")) {
			return;
		}
		auto& spread = kind.cfg.steadying;

		HeaderRow(nullptr, "Max Reduction", "Half Duration");
		TableNextRow();
		ToggleColumn(label, spread.enabled, tooltip);
		DisabledIf off{ !spread.enabled };
		EmptyColumn();
		PercentSliderColumn("##MaxReduction", spread.maxSpreadMultiplier, maxReductionTooltip, Options::Limits::kSteadyingReduction);
		SecondsSliderColumn("##SpreadHalfDuration", spread.halfDuration, halfDurationTooltip, Options::Limits::kHalfDuration);
	}

	void FatigueRows::Render(const KindContext& kind, TableSections& sections) const {
		if (!sections.Section("Fatigue")) {
			return;
		}
		auto& fatigue = kind.cfg.fatigue;

		HeaderRow(nullptr, "Min Threshold", "Max Threshold");
		TableNextRow();
		ToggleColumn(label, fatigue.enabled, tooltip);
		{
			DisabledIf off{ !fatigue.enabled };
			EmptyColumn();
			SecondsSliderColumn("##MinThreshold", fatigue.threshold.from, minThresholdTooltip, Options::Limits::kFatigueThreshold);
			SecondsSliderColumn("##MaxThreshold", fatigue.threshold.to, maxThresholdTooltip, Options::Limits::kFatigueThreshold);
		}

		HeaderRow(nullptr, "Max Fatigue", "Half Duration");
		TableNextRow();
		TableNextColumn();
		DisabledIf off{ !fatigue.enabled };
		EmptyColumn();
		PercentSliderColumn("##MaxPenalty", fatigue.maxSpreadMultiplier, maxPenaltyTooltip, Options::Limits::kFatiguePenalty);
		SecondsSliderColumn("##FatigueHalfDuration", fatigue.halfDuration, "Seconds past the fatigue threshold to regrow roughly half of the max fatigue penalty. Lower duration means the aim shakes apart faster once fatigue sets in.", Options::Limits::kHalfDuration);
	}

	void RenderAttackKindTable(const AttackKindBase& kind, const PageContext& ctx, const char* separator, const std::function<void(const KindContext&, TableSections&)>& renderParts) {
		const auto& info = kind.kind;
		auto&       attack = ctx.cfg.*info.config;
		KindContext context{ ctx, attack, info };

		PushID(kind.id);
		{
			TableSections sections;
			if (sections.Section(separator)) {
				HeaderRow(kSkillMultiplierHeader, "At 0 Skill", "At 100 Skill");
				TableNextRow();
				ToggleColumn(kind.label, attack.aiming, kind.tooltip);
				DisabledIf off{ !attack.aiming };
				AimRow("Accuracy", "##Accuracy", attack.accuracy, kind.accuracyTooltip,
				       "The widest angle a shot can deviate from the aim at 0 effective skill (worst accuracy), as a multiplier of the base spread angle. No shot goes further than this.",
				       "The widest angle a shot can deviate from the aim at 100 effective skill (best accuracy), as a multiplier of the base spread angle. No shot goes further than this.");
				AimRow("Precision", "##Precision", attack.precision, kind.precisionTooltip,
				       "How shots are distributed within the accuracy radius at 0 effective skill. 0.5 spreads them evenly over the whole circle, higher values pull them towards the center, lower ones push them to the edge.",
				       "How shots are distributed within the accuracy radius at 100 effective skill. 0.5 spreads them evenly over the whole circle, higher values pull them towards the center, lower ones push them to the edge.",
				       Options::Limits::kFocus, "%.2f");
				if (info.concentration) {
					AimRow("Retargeting", "##Retarget", attack.retarget,
					       "How much the skill contributes to Retargeting. The higher this multiplier is the smaller the steps the stream takes when it wanders.",
					       "How far the stream can move when it picks its next point to wander to at 0 effective skill, as a share of the accuracy radius. 2.00 or more doesn't limit it at all: the stream can jump anywhere within the circle.",
					       "How far the stream can move when it picks its next point to wander to at 100 effective skill, as a share of the accuracy radius. Lower values make the stream drift in small steps instead of jumping across the circle.",
					       Options::Limits::kRetarget, "%.2f");
				}
				RenderSkillSource(context, &Config::SkillSources::aiming, "Aiming Skill", "aiming");
			}

			renderParts(context, sections);

			// The last section is still open, so the button lands in its first column.
			if (sections.Section(nullptr) && ResetButton(kind.resetLabel)) {
				attack = ctx.defaults.*info.config;
				Options::Save();
			}
			if (sections.Section(nullptr)) {
				EmptyRow();
			}
		}
		PopID();
	}
}
