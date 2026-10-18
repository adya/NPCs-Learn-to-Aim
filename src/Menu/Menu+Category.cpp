#include "Menu+Internal.h"
#include <fmt/format.h>
#include <SKSEMCP/SKSEMenuFramework.hpp>

using namespace ImGuiMCP;

namespace NLA::Menu
{
	namespace
	{
		constexpr const char* kUseHighestMagicSkillLabel = "Use Highest Magic Skill";

		template <typename T>
		void ResetMember(const PageContext& ctx, T Config::* member) {
			ctx.cfg.*member = ctx.defaults.*member;
		}
	}

	void HeadshotRows::Render(const PageContext& ctx, bool, TableSections& sections) const {
		if (!ctx.isNPC || !sections.Section(nullptr)) {
			return;
		}
		auto& headshots = Options::NPC.headshots;
		auto& on = headshots.*enabled;

		HeaderRow("Min Chance", "Max Chance");
		TableNextRow();
		ToggleColumn(label, on, tooltip);
		DisabledIf off{ !on };
		PercentSliderColumn("##MinHeadshotChance", headshots.*minChance, fmt::format("Chance for a skilled {} to go for a headshot at 0 effective Archery skill.", shooter).c_str());
		PercentSliderColumn("##MaxHeadshotChance", headshots.*maxChance, fmt::format("Chance for a skilled {} to go for a headshot at 100 effective Archery skill.", shooter).c_str());
	}

	void HeadshotRows::Reset(const PageContext& ctx) const {
		if (!ctx.isNPC) {
			return;
		}
		auto&       headshots = Options::NPC.headshots;
		const auto& defaults = Options::NPCConfig::defaultConfig.headshots;
		headshots.*enabled = defaults.*enabled;
		headshots.*minChance = defaults.*minChance;
		headshots.*maxChance = defaults.*maxChance;
	}

	void RangeRows::Render(const PageContext& ctx, bool, TableSections& sections) const {
		if (!sections.Section(group)) {
			return;
		}
		auto& cfg = ctx.cfg;
		auto& on = cfg.*enabled;

		HeaderRow("Min Range", "Max Range");
		TableNextRow();
		ToggleColumn("Scale Range With Skill", on, tooltip);
		DisabledIf off{ !on };
		SliderColumn("##MinRange", cfg.*min, minTooltip, 0.5f, 5.0f);
		SliderColumn("##MaxRange", cfg.*max, maxTooltip, 0.5f, 5.0f);
	}

	void RangeRows::Reset(const PageContext& ctx) const {
		ResetMember(ctx, enabled);
		ResetMember(ctx, min);
		ResetMember(ctx, max);
	}

	void UseHighestMagicSkillRow::Render(const PageContext& ctx, bool, TableSections& sections) const {
		if (!sections.Section(kSkillSourceGroup)) {
			return;
		}
		TableNextRow();
		ToggleColumn(kUseHighestMagicSkillLabel, ctx.cfg.*enabled, tooltip);
	}

	void UseHighestMagicSkillRow::Reset(const PageContext& ctx) const {
		ResetMember(ctx, enabled);
	}

	void SkillSourceRows::Render(const PageContext& ctx, bool, TableSections& sections) const {
		if (!sections.Section(kSkillSourceGroup)) {
			return;
		}
		auto&      cfg = ctx.cfg;
		auto&      useEnchanting = cfg.*useEnchantingSkill;
		auto&      useHighest = cfg.*useHighestMagicSkill;

		TableNextRow();
		ExclusiveToggleColumn("Use Enchanting Skill", useEnchanting, useHighest, useEnchantingSkillTooltip);
		ExclusiveToggleColumn(kUseHighestMagicSkillLabel, useHighest, useEnchanting, useHighestMagicSkillTooltip, true);
	}

	void SkillSourceRows::Reset(const PageContext& ctx) const {
		ResetMember(ctx, useEnchantingSkill);
		ResetMember(ctx, useHighestMagicSkill);
	}

	void SpeedRows::Render(const PageContext& ctx, bool, TableSections& sections) const {
		if (!sections.Section(group)) {
			return;
		}
		auto& speed = ctx.cfg.*options;

		HeaderRow("Min Speed", "Max Speed");
		TableNextRow();
		ToggleColumn(label, speed.enabled, tooltip);
		DisabledIf off{ !speed.enabled };
		SliderColumn("##MinSpeed", speed.minMultiplier, "Speed multiplier at 0 skill. 1.0 is vanilla speed, higher is faster.", 0.1f, 3.0f);
		SliderColumn("##MaxSpeed", speed.maxMultiplier, "Speed multiplier at 100 skill (fastest). 1.0 is vanilla speed, higher is faster.", 0.1f, 3.0f);
	}

	void SpeedRows::Reset(const PageContext& ctx) const {
		ResetMember(ctx, options);
	}

	void SpreadRows::Render(const PageContext& ctx, bool, TableSections& sections) const {
		if (!sections.Section("Steadying")) {
			return;
		}
		auto& spread = ctx.cfg.*options;

		HeaderRow("Max Reduction", "Half Duration");
		TableNextRow();
		ToggleColumn(label, spread.enabled, tooltip);
		DisabledIf off{ !spread.enabled };
		PercentSliderColumn("##MaxReduction", spread.maxSpreadMultiplier, maxReductionTooltip);
		SecondsSliderColumn("##SpreadHalfDuration", spread.halfDuration, halfDurationTooltip, 0.1f, 5.0f);
	}

	void SpreadRows::Reset(const PageContext& ctx) const {
		ResetMember(ctx, options);
	}

	void FatigueRows::Render(const PageContext& ctx, bool, TableSections& sections) const {
		if (!sections.Section("Fatigue")) {
			return;
		}
		auto& fatigue = ctx.cfg.*options;

		HeaderRow("Min Threshold", "Max Threshold");
		TableNextRow();
		ToggleColumn(label, fatigue.enabled, tooltip);
		{
			DisabledIf off{ !fatigue.enabled };
			SecondsSliderColumn("##MinThreshold", fatigue.minThreshold, minThresholdTooltip, 0.0f, thresholdSliderMax);
			SecondsSliderColumn("##MaxThreshold", fatigue.maxThreshold, maxThresholdTooltip, 0.0f, thresholdSliderMax);
		}

		HeaderRow("Max Fatigue", "Half Duration");
		TableNextRow();
		TableNextColumn();
		DisabledIf off{ !fatigue.enabled };
		PercentSliderColumn("##MaxPenalty", fatigue.maxSpreadMultiplier, maxPenaltyTooltip);
		SecondsSliderColumn("##FatigueHalfDuration", fatigue.halfDuration, "Seconds past the fatigue threshold to regrow roughly half of the max fatigue penalty. Lower duration means the aim shakes apart faster once fatigue sets in.", 0.1f, 5.0f);
	}

	void FatigueRows::Reset(const PageContext& ctx) const {
		ResetMember(ctx, options);
	}

	void RenderCategoryTable(const CategoryBase& category, const PageContext& ctx, const char* separator,
		const std::function<void(bool aiming, TableSections& sections)>& renderParts, const std::function<void()>& resetParts) {
		auto& cfg = ctx.cfg;
		auto& aiming = cfg.*category.aiming;

		PushID(category.id);
		{
			TableSections sections{ ctx.isNPC };
			if (sections.Section(separator)) {
				TableNextRow();
				ToggleColumn(category.label, aiming, category.tooltip);
				DisabledIf off{ !aiming };
				if (ctx.isNPC) {
					SliderColumn("##Accuracy", cfg.accuracyMultipliers.*category.multiplier, category.accuracyTooltip);
				} else {
					NotApplicableColumn();
				}
				SliderColumn("##Precision", cfg.precisionMultipliers.*category.multiplier, category.precisionTooltip);
			}

			renderParts(aiming, sections);

			// The last section is still open, so the button lands in its first column.
			if (sections.Section(nullptr) && ResetButton(category.resetLabel)) {
				ResetMember(ctx, category.aiming);
				cfg.accuracyMultipliers.*category.multiplier = ctx.defaults.accuracyMultipliers.*category.multiplier;
				cfg.precisionMultipliers.*category.multiplier = ctx.defaults.precisionMultipliers.*category.multiplier;
				resetParts();
				Options::Save();
			}
			if (sections.Section(nullptr)) {
				EmptyRow();
			}
		}
		PopID();
	}
}
