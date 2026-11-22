#include "Menu+Internal.h"
#include <SKSEMCP/SKSEMenuFramework.hpp>

using namespace ImGuiMCP;

namespace NLA::Menu
{
	namespace
	{
		constexpr const char* kSkillMultiplierHeader = "Skill Mult";

			/// The least width left to the preview next to the settings.
			constexpr float kMinPreviewWidth = 300.0f;

		/// The header row of a part driven by skill: "`prefix` [skill]". Only magic kinds can pick the skill; "Nothing" turns the part off.
		/// `subject` is what the skill scales, as the tooltip names it (e.g. "Draw Speed").
		void SkillHeader(const KindContext& kind, const char* prefix, bool& enabled, Options::SkillSource Config::SkillSources::*field, const char* subject, HeaderLabel first, HeaderLabel second, HeaderLabel third, HeaderLabel fourth = {}) {
			SkillHeaderRow(prefix, enabled, kind.info.magic ? &(kind.cfg.skills.*field) : nullptr, subject, first, second, third, fourth);
		}

		/// `what` is the thing the skill drives, e.g. "draw speed".
		std::string SkillMultiplierTooltip(std::string_view what) {
			return fmt::format("How much skill matters for {}. x1.00 is normal, higher makes skill count for more.", what);
		}
	}

	void SkillShotRows::Render(const KindContext& kind, TableSections& sections) const {
		if (!kind.page.isNPC || !sections.Section(nullptr)) {
			return;
		}
		auto& skillShot = kind.cfg.skillShot;

		EmptyRow();
		ToggleHeaderRow(label, skillShot.enabled, tooltip, kSkillMultiplierHeader, "Min Chance", "Max Chance");
		TableNextRow();
		EmptyColumn();
		DisabledIf off{ !skillShot.enabled };
		SliderColumn("##SkillShotSkillMultiplier", skillShot.skillMultiplier, SkillMultiplierTooltip("the skill shot chance").c_str());
		PercentSliderColumn("##MinSkillShotChance", skillShot.bounds.from, fmt::format("Chance that {} by a {} with 0 {} is a skill shot.", kind.info.concentration ? "a beam cast" : "a shot", shooter, skill).c_str(), Options::Limits::kSkillShotChance);
		PercentSliderColumn("##MaxSkillShotChance", skillShot.bounds.to, fmt::format("Chance that {} by a {} with 100 {} is a skill shot.", kind.info.concentration ? "a beam cast" : "a shot", shooter, skill).c_str(), Options::Limits::kSkillShotChance);
	}

	void RangeRows::Render(const KindContext& kind, TableSections& sections) const {
		if (!sections.Section(group)) {
			return;
		}
		auto& range = kind.cfg.range;

		SkillHeader(kind, "Scale with", range.enabled, &Config::SkillSources::range, group ? group : "Range", kSkillMultiplierHeader, "Min Range", "Max Range");
		TableNextRow();
		EmptyColumn();
		DisabledIf off{ !range.enabled };
		SliderColumn("##RangeSkillMultiplier", range.skillMultiplier, SkillMultiplierTooltip("how far the spell reaches").c_str());
		SliderColumn("##MinRange", range.bounds.from, minTooltip, Options::Limits::kRangeMultiplier);
		SliderColumn("##MaxRange", range.bounds.to, maxTooltip, Options::Limits::kRangeMultiplier);
	}

	void SpeedRows::Render(const KindContext& kind, TableSections& sections) const {
		if (!sections.Section(group)) {
			return;
		}
		auto& speed = kind.cfg.speed;

		SkillHeader(kind, label, speed.enabled, &Config::SkillSources::speed, group ? group : "Speed", kSkillMultiplierHeader, "Min Speed", "Max Speed");
		TableNextRow();
		EmptyColumn();
		DisabledIf off{ !speed.enabled };
		SliderColumn("##SpeedSkillMultiplier", speed.skillMultiplier, SkillMultiplierTooltip("the speed").c_str());
		SliderColumn("##MinSpeed", speed.bounds.from, "Speed at 0 skill. x1.00 is normal, higher is faster.", Options::Limits::kSpeedMultiplier);
		SliderColumn("##MaxSpeed", speed.bounds.to, "Speed at 100 skill. x1.00 is normal, higher is faster.", Options::Limits::kSpeedMultiplier);
	}

	void SpreadRows::Render(const KindContext& kind, TableSections& sections) const {
		if (!sections.Section("Steadying")) {
			return;
		}
		auto& spread = kind.cfg.steadying;

		ToggleHeaderRow(label, spread.enabled, tooltip, nullptr, "Spread", "Half Time");
		TableNextRow();
		EmptyColumn();
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

		// Bows and crossbows tire the archer's Stamina; every magic kind depends on Magicka.
		const char* resource = kind.info.magic ? "Magicka" : "Stamina";
		const auto  minTip = fmt::format("Threshold when max {} is 0.", resource);
		const auto  maxTip = fmt::format("Threshold when max {} is 100.", resource);
		const auto  thresholdTip = fmt::format("How long the hold lasts before tiring sets in. It grows evenly with max {0}, and keeps growing past the Max value when there is more than 100 {0}.", resource);

		ToggleHeaderRow(label, fatigue.enabled, tooltip, nullptr, HeaderLabel{ "Min", minTip.c_str() }, HeaderLabel{ "Max", maxTip.c_str() });
		TableNextRow();
		TableNextColumn();
		{
			DisabledIf rowOff{ !fatigue.enabled };
			AlignTextToFramePadding();
			TooltipLabel("Threshold", thresholdTip.c_str());
		}
		{
			DisabledIf off{ !fatigue.enabled };
			EmptyColumn();
			SecondsSliderColumn("##MinThreshold", fatigue.threshold.from, minThresholdTooltip, Options::Limits::kFatigueThreshold);
			SecondsSliderColumn("##MaxThreshold", fatigue.threshold.to, maxThresholdTooltip, Options::Limits::kFatigueThreshold);
		}

		HeaderRow(nullptr, "Spread", "Half Time");
		TableNextRow();
		TableNextColumn();
		DisabledIf off{ !fatigue.enabled };
		EmptyColumn();
		PercentSliderColumn("##MaxPenalty", fatigue.maxSpreadMultiplier, maxPenaltyTooltip, Options::Limits::kFatiguePenalty);
		SecondsSliderColumn("##FatigueHalfDuration", fatigue.halfDuration, "Time after tiring sets in to reach half of the max fatigue. Shorter means the aim falls apart faster.", Options::Limits::kHalfDuration);
	}

	void RenderAttackKindTable(const AttackKindBase& kind, const PageContext& ctx, const char* separator, const std::function<void(const KindContext&, TableSections&)>& renderParts) {
		const auto& info = kind.kind;
		auto&       attack = ctx.cfg.*info.config;
		KindContext context{ ctx, attack, info };

		PushID(kind.id);

		// The preview keeps its size however wide the window is, and the settings take the rest, so that the effect of a change is visible without leaving the page.
		// When the window is narrow the preview gives way first, down to a minimum, so that the settings keep their table.
		const float spacing = GetStyle()->ItemSpacing.x;
		const float tableWidth = PageTableWidth() + GetStyle()->ScrollbarSize + GetStyle()->WindowPadding.x * 2;
		const float available = GetContentRegionAvail().x;
		const float previewWidth = std::clamp(available - tableWidth - spacing, kMinPreviewWidth, PreviewPaneWidth());
		const float settingsWidth = std::max(200.0f, available - previewWidth - spacing);
		const bool  settingsVisible = BeginChild("##Settings", ImVec2(settingsWidth, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
		if (settingsVisible) {
			TableSections sections;
			if (sections.Section(separator)) {
				SkillHeader(context, kind.label, attack.aiming, &Config::SkillSources::aiming, "Aiming", kSkillMultiplierHeader, "At 0 Skill", "At 100 Skill", "When Blind");
				DisabledIf off{ !attack.aiming };
				// A beam doesn't miss or fire shots: it wanders around the aim.
				const bool beam = info.concentration;
				AimRow("Accuracy", beam ? "How far the beam can wander from the aim at most. Skill shrinks it." : "How far a shot can miss the aim at most. Skill shrinks it.", "##Accuracy", attack.accuracy, kind.accuracyTooltip,
				       "Accuracy multiplier at 0 skill. Lower is more accurate.",
				       "Accuracy multiplier at 100 skill. Lower is more accurate.",
				       Options::Limits::kAccuracyFactor, "x%.2f",
				       &attack.accuracyBlindness, "Accuracy multiplier at full blindness. Higher is less accurate.",
				       Options::Limits::kAccuracyBlindness);
				AimRow("Precision", beam ? "How the beam's wander is spread within the wander area. Skill keeps it near the aim." : "How shots are spread within the miss area. Skill bunches them near the aim.", "##Precision", attack.precision, kind.precisionTooltip,
				       beam ? "Precision at 0 skill. 0.50 spreads the wander evenly, higher keeps the beam near the aim, lower pushes it to the edge." : "Precision at 0 skill. 0.50 spreads shots evenly, higher bunches them near the aim, lower pushes them to the edge.",
				       beam ? "Precision at 100 skill. 0.50 spreads the wander evenly, higher keeps the beam near the aim, lower pushes it to the edge." : "Precision at 100 skill. 0.50 spreads shots evenly, higher bunches them near the aim, lower pushes them to the edge.",
				       Options::Limits::kFocus, "%.2f",
				       &attack.precisionBlindness, "Precision loss at full blindness. x1.50 is 50% less precise, x0.80 is 20% more.",
				       Options::Limits::kPrecisionBlindness);
				if (beam) {
					AimRow("Retargeting", "How far the beam can move in one step. Skill makes it steadier.", "##Retarget", attack.retarget,
					       "How strongly skill limits the beam's steps. Higher means smaller steps.",
					       "Step size at 0 skill, as a share of the wander area. 2.00 or more means no limit. Lower is steadier.",
					       "Step size at 100 skill, as a share of the wander area. Lower is steadier.",
					       Options::Limits::kRetarget, "%.2f",
					       &attack.retargetBlindness, "Step size multiplier at full blindness. Higher is less steady.",
					       Options::Limits::kRetargetBlindness);
				}
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
		EndChild();

		SameLine();
		if (BeginChild("##Preview", ImVec2(previewWidth, 0), ImGuiChildFlags_Border, 0)) {
			RenderSpreadPreview(context);
		}
		EndChild();
		PopID();
	}
}
