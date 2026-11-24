#include "Menu+Internal.h"
#include <SKSEMCP/SKSEMenuFramework.hpp>

using namespace ImGuiMCP;

namespace NLA::Menu
{
	namespace
	{
		constexpr float      kSliderWidth = 110.0f;
		constexpr std::array kSkillSourceLabels{ "Spell's Magic Skill", "Highest Magic Skill", "Enchanting" };
		constexpr std::array kSkillSourceDescriptions{ "the skill of the spell's school", "the highest Magic skill", "Enchanting skill" };
		// Only the magic skills are offered, so Archery and everything after it is left out.
		static_assert(kSkillSourceLabels.size() == static_cast<std::size_t>(Options::SkillSource::kArchery));

		// The labels that sit in the first column, on the right: it is as wide as the widest of them, with a checkbox if they have one.
		constexpr std::array  kRowLabels{ "Scale with", "Steady aim", "Tire aim", "Threshold" };
		constexpr const char* kWidestSkill = "Highest Magic Skill";

		constexpr int kSliderColumns = 3;

		/// The width of the combo that picks a skill.
		float SkillComboWidth() {
			return CalcTextSize(kWidestSkill).x + GetFrameHeight() * 1.5f;
		}

		/// The widest label of the first column.
		float WidestRowLabel() {
			float width = 0.0f;
			for (const char* label : kRowLabels) {
				width = std::max(width, CalcTextSize(label).x);
			}
			return width;
		}

		/// Sized for the widest label and a checkbox, so that every table of a page lines up.
		float LabelColumnWidth() {
			return WidestRowLabel() + GetFrameHeight() * 1.5f;
		}

		/// Moves the cursor so that something `width` wide ends at the right edge of the column.
		void AlignRight(float width) {
			SetCursorPosX(GetCursorPosX() + std::max(0.0f, GetContentRegionAvail().x - width));
		}

		/// A checkbox in the first column. Its box is where the box of the widest label would be, at the right of the column, so that the boxes of
		/// all the rows line up whatever their labels are, and each label is next to what follows it.
		bool AlignedCheckbox(const char* label, bool* value) {
			AlignRight(WidestRowLabel() + GetFrameHeight() + GetStyle()->ItemInnerSpacing.x);
			return Checkbox(label, value);
		}

		float RoundTo2Decimals(float value) {
			return std::round(value * 100.0f) / 100.0f;
		}

		void CenterCursorForText(const char* label, float width) {
			const float textWidth = CalcTextSize(label).x;
			if (textWidth < width) {
				SetCursorPosX(GetCursorPosX() + (width - textWidth) * 0.5f);
			}
		}

		void CenteredHeaderCell(const HeaderLabel& header, float width) {
			// As tall as a checkbox or drop-down in the same row, so that every header row has the same height.
			AlignTextToFramePadding();
			CenterCursorForText(header.text, width);
			TooltipLabel(header.text, header.tooltip);
		}

		void SaveAfterEdit(const char* tooltip) {
			if (IsItemDeactivatedAfterEdit()) {
				Options::Save();
			}
			ShowTooltip(tooltip);
		}

		/// The width of the two slider columns from the first one on, which a drop-down spans.
		float TwoColumnsWidth() {
			return GetContentRegionAvail().x * 2.0f + GetStyle()->CellPadding.x * 2.0f;
		}

		void FieldColumn(const SentenceField& field) {
			TableNextColumn();
			if (!field.value) {
				return;
			}
			SetNextItemWidth(-1.0f);

			float& value = *field.value;
			switch (field.format) {
			case ValueFormat::kPercent:
				{
					float percent = value * 100.0f;
					if (SliderFloat(field.id, &percent, field.limit.min * 100.0f, field.limit.max * 100.0f, "%.0f%%")) {
						value = std::round(percent) / 100.0f;
					}
				}
				break;
			case ValueFormat::kSeconds:
				if (SliderFloat(field.id, &value, field.limit.min, field.limit.max, "%.1fs")) {
					value = RoundTo2Decimals(value);
				}
				break;
			case ValueFormat::kPlain:
				if (SliderFloat(field.id, &value, field.limit.min, field.limit.max, "%.2f")) {
					value = RoundTo2Decimals(value);
				}
				break;
			default:
				if (SliderFloat(field.id, &value, field.limit.min, field.limit.max, "x%.2f")) {
					value = RoundTo2Decimals(value);
				}
				break;
			}
			SaveAfterEdit(field.tooltip);
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
		// Not clipped, so that a drop-down can span two columns.
		if (!BeginTable(id, 1 + kSliderColumns, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip)) {
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

	Sections::~Sections() {
		if (open) {
			EndTable();
		}
	}

	bool Sections::Section(const char* separator) {
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

	void HeaderRow(HeaderLabel first, HeaderLabel second, HeaderLabel third) {
		TableNextRow();
		const HeaderLabel headers[] = { first, second, third };
		for (int i = 0; i < kSliderColumns; ++i) {
			if (headers[i].text && *headers[i].text) {
				TableSetColumnIndex(i + 1);
				CenteredHeaderCell(headers[i], GetContentRegionAvail().x);
			}
		}
	}

	void EmptyRow() {
		TableNextRow(0, GetTextLineHeightWithSpacing());
	}

	void EmptyColumn() {
		TableNextColumn();
	}

	void ValueRow(const char* label, const char* labelTooltip, const SentenceField& first, const SentenceField& second, const SentenceField& third) {
		TableNextRow();
		TableNextColumn();
		AlignTextToFramePadding();
		if (label && *label) {
			AlignRight(CalcTextSize(label).x);
			TooltipLabel(label, labelTooltip);
		}
		FieldColumn(first);
		FieldColumn(second);
		FieldColumn(third);
	}

	void ScaleRows(const char* id, bool* enabled, const char* enabledTooltip, Options::SkillSource* source, const char* subject, float& skillMultiplier, const char* multiplierTooltip) {
		const bool on = !enabled || *enabled;

		// Says what the current choice does, so it stays true after the value changes.
		const auto describe = [&] {
			if (!on) {
				return fmt::format("{} is unaffected by skill.", subject);
			}
			if (source) {
				return fmt::format("{} scales with {}.", subject, kSkillSourceDescriptions[std::min(static_cast<std::size_t>(*source), kSkillSourceDescriptions.size() - 1)]);
			}
			return fmt::format("{} scales with Archery skill.", subject);
		};

		// The skill multiplier is under its own title in the third column; "Scale with" is the label of the row, and the drop-down spans the first two.
		HeaderRow({}, {}, { "Skill Mult" });

		TableNextRow();
		TableSetColumnIndex(0);

		// "Scale with" is next to the skill that follows it, in the next column. Its checkbox lines up with the others.
		if (enabled) {
			if (AlignedCheckbox("Scale with", enabled)) {
				Options::Save();
			}
			ShowTooltip(enabledTooltip ? enabledTooltip : describe().c_str());
		} else {
			AlignRight(CalcTextSize("Scale with").x);
			AlignTextToFramePadding();
			TooltipLabel("Scale with", describe().c_str());
		}

		DisabledIf off{ !on };

		TableNextColumn();
		if (source) {
			SetNextItemWidth(TwoColumnsWidth());
			int current = std::min(static_cast<int>(*source), static_cast<int>(kSkillSourceLabels.size()) - 1);
			if (Combo(fmt::format("##{}Skill", id).c_str(), &current, kSkillSourceLabels.data(), static_cast<int>(kSkillSourceLabels.size()))) {
				*source = static_cast<Options::SkillSource>(current);
				Options::Save();
			}
			ShowTooltip(describe().c_str());
		} else {
			// A weapon's skill is fixed: Archery. It is a drop-down with that one choice, greyed out, so that the row looks like the magic kinds' rows.
			constexpr std::array archery{ "Archery" };
			int                  current = 0;
			SetNextItemWidth(TwoColumnsWidth());
			{
				BeginDisabled();
				Combo(fmt::format("##{}Skill", id).c_str(), &current, archery.data(), static_cast<int>(archery.size()));
				EndDisabled();
			}
			// A greyed-out item only shows its tooltip when asked to.
			if (IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay)) {
				BeginTooltip();
				PushTextWrapPos(GetFontSize() * 35.0f);
				TextUnformatted(describe().c_str());
				PopTextWrapPos();
				EndTooltip();
			}
		}
		TableNextColumn();  // The second column of the drop-down.
		FieldColumn({ fmt::format("##{}SkillMultiplier", id).c_str(), &skillMultiplier, Options::Limits::kSkillMultiplier, ValueFormat::kMultiplier, multiplierTooltip });

		// A little air between this row and the values under it, which are a different thing.
		RowGap();
	}

	void RowGap() {
		TableNextRow(0, GetFrameHeight() * 0.5f);
	}

	void ToggleRow(const char* label, bool& value, const char* tooltip, const SentenceField& first, const SentenceField& second) {
		TableNextRow();
		TableSetColumnIndex(0);
		if (AlignedCheckbox(label, &value)) {
			Options::Save();
		}
		ShowTooltip(tooltip);

		DisabledIf off{ !value };
		FieldColumn(first);
		FieldColumn(second);
	}

	bool KindRow(const char* label, bool& enabled, const char* tooltip) {
		TableNextRow();
		TableSetColumnIndex(0);
		if (AlignedCheckbox(label, &enabled)) {
			Options::Save();
		}
		ShowTooltip(tooltip);

		// The button takes the two last slider columns.
		TableSetColumnIndex(2);
		const bool reset = Button("Reset to Defaults", ImVec2(TwoColumnsWidth(), 0));
		ShowTooltip("Puts every setting of this page back to its default.");
		return reset;
	}
}
