#include "Menu+Internal.h"
#include <SKSEMCP/SKSEMenuFramework.hpp>
#include <fmt/format.h>

using namespace ImGuiMCP;

namespace NLA::Menu
{
	namespace
	{
		void RenderSkillSource(const PageContext& ctx, const SkillSourceRef& ref, const char* label, const char* what) {
			if (!ref.sources) {
				return;
			}
			const auto tooltip = fmt::format("The skill that affects {}. Spell's Magic Skill uses the skill of the spell's Magic school, Highest Magic Skill uses the highest of the Magic skills regardless of the spell, Enchanting uses the Enchanting skill.", what);
			SkillSourceRow(label, (ctx.cfg.*ref.sources).*ref.field, tooltip.c_str());
		}

		void ResetSkillSource(const PageContext& ctx, const SkillSourceRef& ref) {
			if (ref.sources) {
				(ctx.cfg.*ref.sources).*ref.field = (ctx.defaults.*ref.sources).*ref.field;
			}
		}

		template <typename T>
		void ResetMember(const PageContext& ctx, T Config::*member) {
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
		PercentSliderColumn("##MinHeadshotChance", headshots.*minChance, fmt::format("Chance for a skilled {} to go for a headshot at 0 effective Archery skill.", shooter).c_str(), Options::Limits::kHeadshotChance);
		PercentSliderColumn("##MaxHeadshotChance", headshots.*maxChance, fmt::format("Chance for a skilled {} to go for a headshot at 100 effective Archery skill.", shooter).c_str(), Options::Limits::kHeadshotChance);
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
		auto& range = ctx.cfg.*options;

		HeaderRow("Min Range", "Max Range");
		TableNextRow();
		ToggleColumn("Scale Range With Skill", range.enabled, tooltip);
		DisabledIf off{ !range.enabled };
		SliderColumn("##MinRange", range.minMultiplier, minTooltip, Options::Limits::kRangeMultiplier);
		SliderColumn("##MaxRange", range.maxMultiplier, maxTooltip, Options::Limits::kRangeMultiplier);
		RenderSkillSource(ctx, skill, "Range Skill", "how far the spell reaches");
	}

	void RangeRows::Reset(const PageContext& ctx) const {
		ResetMember(ctx, options);
		ResetSkillSource(ctx, skill);
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
		SliderColumn("##MinSpeed", speed.minMultiplier, "Speed multiplier at 0 skill. 1.0 is vanilla speed, higher is faster.", Options::Limits::kSpeedMultiplier);
		SliderColumn("##MaxSpeed", speed.maxMultiplier, "Speed multiplier at 100 skill (fastest). 1.0 is vanilla speed, higher is faster.", Options::Limits::kSpeedMultiplier);
		RenderSkillSource(ctx, skill, "Speed Skill", "how fast the spell charges/casts");
	}

	void SpeedRows::Reset(const PageContext& ctx) const {
		ResetMember(ctx, options);
		ResetSkillSource(ctx, skill);
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
		PercentSliderColumn("##MaxReduction", spread.maxSpreadMultiplier, maxReductionTooltip, Options::Limits::kSteadyingReduction);
		SecondsSliderColumn("##SpreadHalfDuration", spread.halfDuration, halfDurationTooltip, Options::Limits::kHalfDuration);
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
			SecondsSliderColumn("##MinThreshold", fatigue.minThreshold, minThresholdTooltip, Options::Limits::kFatigueThreshold);
			SecondsSliderColumn("##MaxThreshold", fatigue.maxThreshold, maxThresholdTooltip, Options::Limits::kFatigueThreshold);
		}

		HeaderRow("Max Fatigue", "Half Duration");
		TableNextRow();
		TableNextColumn();
		DisabledIf off{ !fatigue.enabled };
		PercentSliderColumn("##MaxPenalty", fatigue.maxSpreadMultiplier, maxPenaltyTooltip, Options::Limits::kFatiguePenalty);
		SecondsSliderColumn("##FatigueHalfDuration", fatigue.halfDuration, "Seconds past the fatigue threshold to regrow roughly half of the max fatigue penalty. Lower duration means the aim shakes apart faster once fatigue sets in.", Options::Limits::kHalfDuration);
	}

	void FatigueRows::Reset(const PageContext& ctx) const {
		ResetMember(ctx, options);
	}

	void RenderCategoryTable(const CategoryBase& category, const PageContext& ctx, const char* separator, const std::function<void(bool aiming, TableSections& sections)>& renderParts, const std::function<void()>& resetParts) {
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
				RenderSkillSource(ctx, category.aimSkill, "Aiming Skill", "aiming");
			}

			renderParts(aiming, sections);

			// The last section is still open, so the button lands in its first column.
			if (sections.Section(nullptr) && ResetButton(category.resetLabel)) {
				ResetMember(ctx, category.aiming);
				cfg.accuracyMultipliers.*category.multiplier = ctx.defaults.accuracyMultipliers.*category.multiplier;
				cfg.precisionMultipliers.*category.multiplier = ctx.defaults.precisionMultipliers.*category.multiplier;
				ResetSkillSource(ctx, category.aimSkill);
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
