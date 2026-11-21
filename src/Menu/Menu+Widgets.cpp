#include "Menu+Internal.h"
#include <SKSEMCP/SKSEMenuFramework.hpp>

using namespace ImGuiMCP;

namespace NLA::Menu
{
	namespace
	{
		constexpr float       kSliderWidth = 160.0f;
		float                 gPageRight = 0.0f;  // Right edge of the visible page, in screen space.
		constexpr std::array kSkillSourceLabels{ "Spell's Magic Skill", "Highest Magic Skill", "Enchanting" };
		// Only the magic skills are offered, so Archery and everything after it is left out.
		static_assert(kSkillSourceLabels.size() == static_cast<std::size_t>(Options::SkillSource::kArchery));

		constexpr const char* kWidestLabel = kScaleChargeSpeedLabel;

			constexpr int kSliderColumns = 3;

			/// Sized for the longest label, so that every table of a page lines up.
			float LabelColumnWidth() {
				return CalcTextSize(kWidestLabel).x + GetFrameHeight() * 1.5f;
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

	bool BeginPageTable(const char* id) {
		// Outside the table, the available region is the page itself (scrollbar excluded).
		gPageRight = GetCursorScreenPos().x + GetContentRegionAvail().x;
		if (!BeginTable(id, 4, ImGuiTableFlags_SizingFixedFit)) {
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

	void HeaderRow(const char* first, const char* second, const char* third) {
		TableNextRow();
		const char* headers[] = { first, second, third };
		for (int i = 0; i < 3; ++i) {
			if (headers[i] && *headers[i]) {
				TableSetColumnIndex(i + 1);
				CenteredHeaderCell(headers[i]);
			}
		}
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

	void SkillSourceRow(const char* label, Options::SkillSource& value, const char* tooltip) {
		TableNextRow();
		TableNextColumn();
		AlignTextToFramePadding();
		TextUnformatted(label);
		TableNextColumn();

		// The drop-down spans all three slider columns, which must stay empty in this row.
		const ImVec2 cell = GetCursorScreenPos();
		const auto*  style = GetStyle();

		// Keep the drop-down inside the visible part of the window, even if the table is wider than it.
		const float fullWidth = 3 * kSliderWidth + style->CellPadding.x * 4;
		const float width = std::clamp(gPageRight - cell.x - style->CellPadding.x, 60.0f, fullWidth);
		PushClipRect(cell, ImVec2(cell.x + width, cell.y + GetFrameHeightWithSpacing()), false);
		SetNextItemWidth(width);

		int current = static_cast<int>(value);
		if (Combo(fmt::format("##{}", label).c_str(), &current, kSkillSourceLabels.data(), static_cast<int>(kSkillSourceLabels.size()))) {
			value = static_cast<Options::SkillSource>(current);
			Options::Save();
		}
		ShowTooltip(tooltip);
		PopClipRect();
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

	void AimRow(const char* label, const char* id, Options::Config::Aim& aim, const char* tooltip, const char* minTooltip, const char* maxTooltip, Options::Limit limit, const char* format) {
		TableNextRow();
		TableNextColumn();
		AlignTextToFramePadding();
		TextUnformatted(label);
		SliderColumn(id, aim.skillMultiplier, tooltip);

		const auto minId = fmt::format("{}Min", id);
		const auto maxId = fmt::format("{}Max", id);
		SliderColumn(minId.c_str(), aim.bounds.from, minTooltip, limit, format);
		SliderColumn(maxId.c_str(), aim.bounds.to, maxTooltip, Options::Limits::ToLimit(limit, aim.bounds.from), format);
	}

	void EmptyColumn() {
		TableNextColumn();
	}
}
