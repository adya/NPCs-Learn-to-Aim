#include "Menu+Internal.h"
#include <cmath>
#include <fmt/format.h>
#include <SKSEMCP/SKSEMenuFramework.hpp>

using namespace ImGuiMCP;

namespace NLA::Menu
{
	namespace
	{
		constexpr float       kSliderWidth = 160.0f;
		constexpr const char* kAccuracyHeader = "Accuracy";
		constexpr const char* kPrecisionHeader = "Precision";
		constexpr const char* kNotApplicableText = "N/A";
		constexpr const char* kWidestLabel = "Scale Charge Speed With Skill";

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

		void CenteredHeaderCell(const char* label, bool disabled = false) {
			CenterCursorForText(label);
			if (disabled) {
				TextDisabled("%s", label);
			} else {
				Text("%s", label);
			}
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

	void ShowTooltip(const char* text) {
		if (IsItemHovered()) {
			BeginTooltip();
			PushTextWrapPos(GetFontSize() * 35.0f);
			TextUnformatted(text);
			PopTextWrapPos();
			EndTooltip();
		}
	}

	bool BeginPageTable(const char* id, bool hasAccuracy, bool withHeader) {
		if (!BeginTable(id, 3, ImGuiTableFlags_SizingFixedFit)) {
			return false;
		}
		// Sized for the longest label, so that every table of a page lines up.
		const float labelWidth = CalcTextSize(kWidestLabel).x + GetFrameHeight() * 1.5f;
		TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, labelWidth);
		TableSetupColumn(kAccuracyHeader, ImGuiTableColumnFlags_WidthFixed, kSliderWidth);
		TableSetupColumn(kPrecisionHeader, ImGuiTableColumnFlags_WidthFixed, kSliderWidth);

		if (withHeader) {
			TableNextRow();
			TableSetColumnIndex(1);
			CenteredHeaderCell(kAccuracyHeader, !hasAccuracy);
			ShowTooltip(hasAccuracy ?
							"How accurately NPCs can aim their shots at a target. The higher the skill the more likely they can aim at the target." :
							"Not applicable to the Player: you use your mouse/controller to aim :)");
			TableSetColumnIndex(2);
			CenteredHeaderCell(kPrecisionHeader);
			ShowTooltip(hasAccuracy ?
							"How precisely NPCs can hit their target in the aimed point. The higher the skill the more precisely they can hit desired spot." :
							"How precisely you can hit the target in the aimed point. The higher the skill the more precisely you can hit the desired spot.");
		}
		return true;
	}

	void RenderTable(const char* id, bool hasAccuracy, const std::function<void()>& rows) {
		if (BeginPageTable(id, hasAccuracy, true)) {
			rows();
			EndTable();
		}
	}

	TableSections::TableSections(bool hasAccuracy) :
		hasAccuracy(hasAccuracy) {}

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
			open = BeginPageTable(fmt::format("Table{}", count).c_str(), hasAccuracy, count == 0);
			++count;
		}
		return open;
	}

	void HeaderRow(const char* first, const char* second) {
		TableNextRow();
		TableSetColumnIndex(1);
		CenteredHeaderCell(first);
		TableSetColumnIndex(2);
		CenteredHeaderCell(second);
	}

	void EmptyRow() {
		TableNextRow(0, GetTextLineHeightWithSpacing());
	}

	bool ResetButton(const char* label) {
		TableNextRow();
		TableNextColumn();
		return Button(label);
	}

	void ToggleColumn(const char* label, bool& value, const char* tooltip) {
		TableNextColumn();
		if (Checkbox(label, &value)) {
			Options::Save();
		}
		ShowTooltip(tooltip);
	}

	void SliderColumn(const char* id, float& value, const char* tooltip, float min, float max, const char* format) {
		TableNextColumn();
		SetNextItemWidth(-1.0f);
		if (SliderFloat(id, &value, min, max, format)) {
			value = RoundTo2Decimals(value);
		}
		SaveAfterEdit(tooltip);
	}

	void SecondsSliderColumn(const char* id, float& value, const char* tooltip, float min, float max) {
		SliderColumn(id, value, tooltip, min, max, "%.1fs");
	}

	void SkillSourceRow(const char* label, Options::SkillSource& value, const char* tooltip) {
		TableNextRow();
		TableNextColumn();
		AlignTextToFramePadding();
		TextUnformatted(label);
		TableNextColumn();

		// The drop-down spans both slider columns, which must stay empty in this row.
		const ImVec2 cell = GetCursorScreenPos();
		const float  width = 2 * kSliderWidth + GetStyle()->CellPadding.x * 2;
		PushClipRect(cell, ImVec2(cell.x + width, cell.y + GetFrameHeightWithSpacing()), false);
		SetNextItemWidth(width);

		constexpr const char* kItems[] = { "Spell's Magic Skill", "Highest Magic Skill", "Enchanting" };

		int current = static_cast<int>(value);
		if (Combo(fmt::format("##{}", label).c_str(), &current, kItems, static_cast<int>(std::size(kItems)))) {
			value = static_cast<Options::SkillSource>(current);
			Options::Save();
		}
		ShowTooltip(tooltip);
		PopClipRect();
	}

	void PercentSliderColumn(const char* id, float& value, const char* tooltip) {
		TableNextColumn();
		SetNextItemWidth(-1.0f);
		float percent = value * 100.0f;
		if (SliderFloat(id, &percent, 0.0f, 100.0f, "%.0f%%")) {
			value = percent / 100.0f;
		}
		SaveAfterEdit(tooltip);
	}

	void NotApplicableColumn() {
		TableNextColumn();
		DisabledIf disabled{ true };
		CenterCursorForText(kNotApplicableText);
		TextUnformatted(kNotApplicableText);
	}
}
