#include "Menu+Internal.h"
#include <SKSEMCP/SKSEMenuFramework.hpp>

using namespace ImGuiMCP;

namespace NLA::Menu
{
	namespace
	{
		constexpr float       kSliderWidth = 110.0f;
		constexpr std::array kSkillSourceLabels{ "Spell's Magic Skill", "Highest Magic Skill", "Enchanting" };
		constexpr std::array kSkillSourceDescriptions{ "the skill of the spell's school", "the highest Magic skill", "Enchanting skill" };
		// Only the magic skills are offered, so Archery and everything after it is left out.
		static_assert(kSkillSourceLabels.size() == static_cast<std::size_t>(Options::SkillSource::kArchery));

		constexpr const char* kWidestLabel = "Loaded Crossbow Fatigues Aim";  // The longest checkbox label.
		constexpr const char* kWidestSkill = "Highest Magic Skill";

			constexpr int kSliderColumns = 4;

			/// Sized for the longest label, so that every table of a page lines up.
			float LabelColumnWidth() {
				const float combo = CalcTextSize(kWidestSkill).x + GetFrameHeight() * 1.5f;
				const float skillRow = CalcTextSize(kScaleChargeSpeedLabel).x + GetStyle()->ItemSpacing.x * 3.0f + combo;
				const float toggleRow = CalcTextSize(kWidestLabel).x + GetFrameHeight() * 1.5f;
				return std::max(skillRow, toggleRow);
			}

		float RoundTo2Decimals(float value) {
			return std::round(value * 100.0f) / 100.0f;
		}

		void CenterCursorForText(const char* label) {
			ImVec2 avail = GetContentRegionAvail();
			float  textWidth = CalcTextSize(label).x;
			if (textWidth < avail.x) {
				SetCursorPosX(GetCursorPosX() + (avail.x - textWidth) * 0.5f);
			}
		}

		void CenteredHeaderCell(const HeaderLabel& header) {
			// As tall as a checkbox or drop-down in the same row, so that every header row has the same height.
			AlignTextToFramePadding();
			CenterCursorForText(header.text);
			TooltipLabel(header.text, header.tooltip);
		}

		void SaveAfterEdit(const char* tooltip) {
			if (IsItemDeactivatedAfterEdit()) {
				Options::Save();
			}
			ShowTooltip(tooltip);
		}
	}

	DisabledIf::DisabledIf(bool disabled) :
		disabled(disabled) {
		if (disabled) {
			BeginDisabled();
		}
	}

	DisabledIf::~DisabledIf() {
		if (disabled) {
			EndDisabled();
		}
	}

	void ShowTooltip(const char* text, bool whileHeld) {
		// The hand cursor tells that the item has a tooltip to read.
		if (IsItemHovered(whileHeld ? ImGuiHoveredFlags_AllowWhenBlockedByActiveItem : 0)) {
			SetMouseCursor(ImGuiMouseCursor_Hand);
		}

		// Not while the item is being used: a tooltip would only get in the way of dragging a slider or holding a button.
		// And not right away either: it appears once the mouse has rested on the item for a moment, each item having its own wait.
		const ImGuiHoveredFlags flags = ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay | (whileHeld ? ImGuiHoveredFlags_AllowWhenBlockedByActiveItem : 0);
		if (IsItemHovered(flags) && !IsItemActive()) {
			BeginTooltip();
			PushTextWrapPos(GetFontSize() * 35.0f);
			TextUnformatted(text);
			PopTextWrapPos();
			EndTooltip();
		}
	}

	void TooltipLabel(const char* label, const char* tooltip) {
		TextUnformatted(label);
		if (!tooltip) {
			return;
		}
		ShowTooltip(tooltip);
	}

	bool BeginPageTable(const char* id) {
		if (!BeginTable(id, 1 + kSliderColumns, ImGuiTableFlags_SizingFixedFit)) {
			return false;
		}
		TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, LabelColumnWidth());
		for (int i = 0; i < kSliderColumns; ++i) {
			TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, kSliderWidth);
		}
		return true;
	}

	float PageTableWidth() {
		// Every column has the cell padding on both sides.
		constexpr int columns = 1 + kSliderColumns;
		return LabelColumnWidth() + kSliderColumns * kSliderWidth + GetStyle()->CellPadding.x * 2 * columns;
	}

	void RenderTable(const char* id, const std::function<void()>& rows) {
		if (BeginPageTable(id)) {
			rows();
			EndTable();
		}
	}

	TableSections::~TableSections() {
		if (open) {
			EndTable();
		}
	}

	bool TableSections::Section(const char* separator) {
		if (separator || count == 0) {
			if (open) {
				EndTable();
			}
			if (separator) {
				SeparatorText(separator);
			}
			open = BeginPageTable(fmt::format("Table{}", count).c_str());
			++count;
		}
		return open;
	}

	void HeaderRow(HeaderLabel first, HeaderLabel second, HeaderLabel third, HeaderLabel fourth) {
		TableNextRow();
		const HeaderLabel headers[] = { first, second, third, fourth };
		for (int i = 0; i < kSliderColumns; ++i) {
			if (headers[i].text && *headers[i].text) {
				TableSetColumnIndex(i + 1);
				CenteredHeaderCell(headers[i]);
			}
		}
	}

	void ToggleHeaderRow(const char* label, bool& value, const char* tooltip, HeaderLabel first, HeaderLabel second, HeaderLabel third, HeaderLabel fourth) {
		HeaderRow(first, second, third, fourth);
		TableSetColumnIndex(0);
		if (Checkbox(label, &value)) {
			Options::Save();
		}
		ShowTooltip(tooltip);
	}

	void SkillHeaderRow(const char* prefix, bool& enabled, Options::SkillSource* source, const char* subject, HeaderLabel first, HeaderLabel second, HeaderLabel third, HeaderLabel fourth) {
		// Says what the current choice does, so it stays true after the value changes.
		const auto describe = [&] {
			if (!enabled) {
				return fmt::format("{} is unaffected by skill.", subject);
			}
			if (source) {
				return fmt::format("{} scales with {}.", subject, kSkillSourceDescriptions[std::min(static_cast<std::size_t>(*source), kSkillSourceDescriptions.size() - 1)]);
			}
			return fmt::format("{} scales with Archery skill.", subject);
		};

		HeaderRow(first, second, third, fourth);
		TableSetColumnIndex(0);
		AlignTextToFramePadding();
		TooltipLabel(prefix, describe().c_str());
		SameLine();

		// Without a source to pick, the skill is fixed and the only choice is whether it counts.
		std::vector<const char*> items;
		if (source) {
			items.assign(kSkillSourceLabels.begin(), kSkillSourceLabels.end());
		} else {
			items.push_back("Archery");
		}
		items.push_back("Nothing");
		const int nothing = static_cast<int>(items.size()) - 1;

		int current = nothing;
		if (enabled) {
			current = source ? std::min(static_cast<int>(*source), nothing - 1) : 0;
		}
		SetNextItemWidth(CalcTextSize(kWidestSkill).x + GetFrameHeight() * 1.5f);  // The gap before the sliders comes from the label column's minimum width.
		if (Combo(fmt::format("##{}", prefix).c_str(), &current, items.data(), static_cast<int>(items.size()))) {
			enabled = current != nothing;
			if (enabled && source) {
				*source = static_cast<Options::SkillSource>(current);
			}
			Options::Save();
		}

		ShowTooltip(describe().c_str());
	}

	void EmptyRow() {
		TableNextRow(0, GetTextLineHeightWithSpacing());
	}

	bool ResetButton(const char* label) {
		TableNextRow();
		TableNextColumn();
		return Button(label);
	}

	void SliderColumn(const char* id, float& value, const char* tooltip, Options::Limit limit, const char* format) {
		TableNextColumn();
		SetNextItemWidth(-1.0f);
		if (SliderFloat(id, &value, limit.min, limit.max, format)) {
			value = RoundTo2Decimals(value);
		}
		SaveAfterEdit(tooltip);
	}

	void SecondsSliderColumn(const char* id, float& value, const char* tooltip, Options::Limit limit) {
		SliderColumn(id, value, tooltip, limit, "%.1fs");
	}

	void PercentSliderColumn(const char* id, float& value, const char* tooltip, Options::Limit limit) {
		TableNextColumn();
		SetNextItemWidth(-1.0f);
		float percent = value * 100.0f;
		if (SliderFloat(id, &percent, limit.min * 100.0f, limit.max * 100.0f, "%.0f%%")) {
			value = std::round(percent) / 100.0f;
		}
		SaveAfterEdit(tooltip);
	}

	void AimRow(const char* label, const char* labelTooltip, const char* id, Options::Config::Aim& aim, const char* tooltip, const char* minTooltip, const char* maxTooltip, Options::Limit limit, const char* format, float* blindness, const char* blindnessTooltip, Options::Limit blindnessLimit) {
		TableNextRow();
		TableNextColumn();
		AlignTextToFramePadding();
		TooltipLabel(label, labelTooltip);
		SliderColumn(id, aim.skillMultiplier, tooltip);

		const auto minId = fmt::format("{}Min", id);
		const auto maxId = fmt::format("{}Max", id);
		SliderColumn(minId.c_str(), aim.bounds.from, minTooltip, limit, format);
		SliderColumn(maxId.c_str(), aim.bounds.to, maxTooltip, Options::Limits::ToLimit(limit, aim.bounds.from), format);
		if (blindness) {
			SliderColumn(fmt::format("{}Blindness", id).c_str(), *blindness, blindnessTooltip, blindnessLimit);
		}
	}

	void EmptyColumn() {
		TableNextColumn();
	}
}
