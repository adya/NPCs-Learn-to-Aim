#include "Menu.h"
#include "CLIBUtil/string.hpp"
#include "Options.h"
#include <cmath>
#include <fmt/ranges.h>

#include <SKSEMCP/SKSEMenuFramework.hpp>

using namespace ImGuiMCP;

namespace NLA::Menu
{
	namespace
	{
		constexpr float kSliderWidth = 160.0f;

		constexpr const char* kSectionName = "NPCs Learn to Aim";
		constexpr const char* kNPCSectionItem = "NPC";
		constexpr const char* kPlayerSectionItem = "Player";
		constexpr const char* kActionsSectionItem = "Actions";

		constexpr const char* kAccuracyHeader = "Accuracy";
		constexpr const char* kPrecisionHeader = "Precision";
		constexpr const char* kNotApplicableText = "N/A";
		constexpr const char* kAccuracyTooltipNPC = "How accurately NPCs can aim their shots at a target. The higher the skill the more likely they can aim at the target.";
		constexpr const char* kAccuracyTooltipPlayer = "Not applicable to the Player: you use your mouse/controller to aim :)";
		constexpr const char* kPrecisionTooltipNPC = "How precisely NPCs can hit their target in the aimed point. The higher the skill the more precisely they can hit desired spot.";
		constexpr const char* kPrecisionTooltipPlayer = "How precisely you can hit the target in the aimed point. The higher the skill the more precisely you can hit the desired spot.";

		constexpr const char* kBowLabel = "Learn Bows";
		constexpr const char* kBowTooltip = "When enabled, Archery skill will affect aiming with bows.";
		constexpr const char* kBowAccuracyTooltip = "How much Archery skill contributes to Accuracy with bows. The higher this multiplier is the easier it is to aim.";
		constexpr const char* kBowPrecisionTooltip = "How much Archery skill contributes to Precision with bows. The higher this multiplier is the easier it is to hit the target.";
		constexpr const char* kResetBowsButton = "Reset Bows to Default";

		constexpr const char* kCrossbowLabel = "Learn Crossbows";
		constexpr const char* kCrossbowTooltip = "When enabled, Archery skill will affect aiming with crossbows.";
		constexpr const char* kCrossbowAccuracyTooltip = "How much Archery skill contributes to Accuracy with crossbows. The higher this multiplier is the easier it is to aim.";
		constexpr const char* kCrossbowPrecisionTooltip = "How much Archery skill contributes to Precision with crossbows. The higher this multiplier is the easier it is to hit the target.";
		constexpr const char* kResetCrossbowsButton = "Reset Crossbows to Default";

		constexpr const char* kStaffLabel = "Learn Staves";
		constexpr const char* kStaffTooltip = "When enabled, corresponding Magic skill will affect aiming with staves.";
		constexpr const char* kStaffAccuracyTooltip = "How much the skill contributes to Accuracy with staves. The higher this multiplier is the easier it is to aim.";
		constexpr const char* kStaffPrecisionTooltip = "How much the skill contributes to Precision with staves. The higher this multiplier is the easier it is to hit the target.";
		constexpr const char* kUseEnchantingSkillLabel = "Use Enchanting Skill";
		constexpr const char* kUseEnchantingSkillTooltip = "When enabled, staff aiming is based on the Enchanting skill instead of the staff's spell's Magic school.";
		constexpr const char* kResetStavesButton = "Reset Staves to Default";

		constexpr const char* kSpellLabel = "Learn Spells";
		constexpr const char* kSpellTooltip = "When enabled, corresponding Magic skill will affect aiming with spells.";
		constexpr const char* kSpellAccuracyTooltip = "How much the skill contributes to Accuracy with fire-and-forget spells. The higher this multiplier is the easier it is to aim.";
		constexpr const char* kSpellPrecisionTooltip = "How much the skill contributes to Precision with fire-and-forget spells. The higher this multiplier is the easier it is to hit the target.";
		constexpr const char* kUseHighestMagicSkillLabel = "Use Highest Magic Skill";
		constexpr const char* kUseHighestMagicSkillTooltip = "When enabled, spells aiming is based on the highest Magic skill, regardless of the spell's Magic school. (affects Concentration Spells too)";
		constexpr const char* kResetSpellsButton = "Reset Spells to Default";

		constexpr const char* kConcentrationSpellLabel = "Learn Concentration Spells";
		constexpr const char* kConcentrationSpellTooltip = "When enabled, corresponding Magic skill will affect aiming with concentration spells (flames/sparks/frost/etc). Stream/Beam will wander around the aimed point.";
		constexpr const char* kConcentrationSpellAccuracyTooltip = "How much the skill contributes to Accuracy with concentration spells. The higher this multiplier is the easier it is to aim.";
		constexpr const char* kConcentrationSpellPrecisionTooltip = "How much the skill contributes to Precision with concentration spells. The higher this multiplier is the easier it is to hit the target.";
		constexpr const char* kScaleRangeWithSkillLabel = "Scale Range With Skill";
		constexpr const char* kScaleRangeWithSkillTooltip = "When enabled, corresponding Magic skill also affects concentration spells range. (the higher the skill the farther mage can hit)";
		constexpr const char* kMinRangeMultiplierHeader = "Min Range";
		constexpr const char* kMaxRangeMultiplierHeader = "Max Range";
		constexpr const char* kMinRangeMultiplierTooltip = "Concentration spell range multiplier at 0 skill. Note that range also scales spell's visual effects, so setting this too high may produce poor visuals.";
		constexpr const char* kMaxRangeMultiplierTooltip = "Concentration spell range multiplier at 100 skill. Note that range also scales spell's visual effects, so setting this too high may produce poor visuals.";
		constexpr const char* kResetConcentrationSpellsButton = "Reset Concentration Spells to Default";

		constexpr const char* kBlindnessLabel = "Blindness Affects Aim";
		constexpr const char* kBlindnessTooltip = "When enabled, being blinded further affects aiming.";
		constexpr const char* kBlindnessAccuracyTooltip = "How much the blindness affects general accuracy. The higher this multiplier is the easier it is to aim.";
		constexpr const char* kBlindnessPrecisionTooltip = "How much the blindness affects general precision. The higher this multiplier is the easier it is to hit the target.";
		constexpr const char* kResetBlindnessButton = "Reset Blindness to Default";

		constexpr const char* kFiltersHeader = "Filters";
		constexpr const char* kFiltersDescription = "Only NPCs matching these keyword rules will learn to aim. Comma-separated keyword EditorIDs.";
		constexpr const char* kIncludeKeywordsLabel = "Include Keywords";
		constexpr const char* kIncludeKeywordsHint = "ActorTypeNPC";
		constexpr const char* kIncludeKeywordsTooltip = "Only NPCs with at least one of these keywords (on the NPC or its race) will learn to aim. Leave empty to skip this filter.";
		constexpr const char* kExcludeKeywordsLabel = "Exclude Keywords";
		constexpr const char* kExcludeKeywordsHint = "";
		constexpr const char* kExcludeKeywordsTooltip = "NPCs with any of these keywords (on the NPC or its race) will never learn to aim.";
		constexpr const char* kResetFiltersButton = "Reset Filters to Default";

		constexpr const char* kReloadSettingsButton = "Reload Settings";
		constexpr const char* kReloadSettingsTooltip = "Reload all settings from the INI file, discarding any unsaved in-memory changes.";

		void ShowTooltip(const char* text) {
			if (IsItemHovered()) {
				BeginTooltip();
				PushTextWrapPos(GetFontSize() * 35.0f);
				TextUnformatted(text);
				PopTextWrapPos();
				EndTooltip();
			}
		}

		float RoundTo2Decimals(float value) {
			return std::round(value * 100.0f) / 100.0f;
		}

		void CenterCursorForText(const char* label) {
			ImVec2 avail = GetContentRegionAvail();
			float textWidth = CalcTextSize(label).x;
			if (textWidth < avail.x) {
				SetCursorPosX(GetCursorPosX() + (avail.x - textWidth) * 0.5f);
			}
		}

		void CenteredHeaderCell(const char* label, bool disabled) {
			CenterCursorForText(label);
			if (disabled) {
				TextDisabled("%s", label);
			} else {
				Text("%s", label);
			}
		}

		bool BeginPageTable(const char* id, bool hasAccuracy) {
			if (!BeginTable(id, 3, ImGuiTableFlags_SizingFixedFit)) {
				return false;
			}
			TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
			TableSetupColumn(kAccuracyHeader, ImGuiTableColumnFlags_WidthFixed, kSliderWidth);
			TableSetupColumn(kPrecisionHeader, ImGuiTableColumnFlags_WidthFixed, kSliderWidth);

			TableNextRow();
			TableSetColumnIndex(1);
			CenteredHeaderCell(kAccuracyHeader, !hasAccuracy);
			ShowTooltip(hasAccuracy ? kAccuracyTooltipNPC : kAccuracyTooltipPlayer);
			TableSetColumnIndex(2);
			CenteredHeaderCell(kPrecisionHeader, false);
			ShowTooltip(hasAccuracy ? kPrecisionTooltipNPC : kPrecisionTooltipPlayer);

			return true;
		}

		void EmptyRow() {
			TableNextRow(0, GetTextLineHeightWithSpacing());
		}

		void ToggleColumn(const char* label, bool& value, const char* tooltip) {
			TableNextColumn();
			if (Checkbox(label, &value)) {
				Options::Save();
			}
			ShowTooltip(tooltip);
		}

		void SliderColumn(const char* id, float& value, const char* tooltip, float min = 0.0f, float max = 3.0f) {
			TableNextColumn();
			SetNextItemWidth(-1.0f);
			if (SliderFloat(id, &value, min, max, "%.2fx")) {
				value = RoundTo2Decimals(value);
			}
			if (IsItemDeactivatedAfterEdit()) {
				Options::Save();
			}
			ShowTooltip(tooltip);
		}

		void NotApplicableColumn() {
			TableNextColumn();
			BeginDisabled();
			CenterCursorForText(kNotApplicableText);
			TextUnformatted(kNotApplicableText);
			EndDisabled();
		}

		void RenderBowRows(Options::Config& cfg, const Options::Config& defaults, bool hasAccuracy) {
			PushID("Bows");
			TableNextRow();
			ToggleColumn(kBowLabel, cfg.bowAiming, kBowTooltip);
			if (!cfg.bowAiming) {
				BeginDisabled();
			}
			if (hasAccuracy) {
				SliderColumn("##BowAccuracy", cfg.accuracyMultipliers.bow, kBowAccuracyTooltip);
			} else {
				NotApplicableColumn();
			}
			SliderColumn("##BowPrecision", cfg.precisionMultipliers.bow, kBowPrecisionTooltip);
			if (!cfg.bowAiming) {
				EndDisabled();
			}

			TableNextRow();
			TableNextColumn();
			if (Button(kResetBowsButton)) {
				cfg.bowAiming = defaults.bowAiming;
				if (hasAccuracy) {
					cfg.accuracyMultipliers.bow = defaults.accuracyMultipliers.bow;
				}
				cfg.precisionMultipliers.bow = defaults.precisionMultipliers.bow;
				Options::Save();
			}
			EmptyRow();
			PopID();
		}

		void RenderCrossbowRows(Options::Config& cfg, const Options::Config& defaults, bool hasAccuracy) {
			PushID("Crossbows");
			TableNextRow();
			ToggleColumn(kCrossbowLabel, cfg.crossbowAiming, kCrossbowTooltip);
			if (!cfg.crossbowAiming) {
				BeginDisabled();
			}
			if (hasAccuracy) {
				SliderColumn("##CrossbowAccuracy", cfg.accuracyMultipliers.crossbow, kCrossbowAccuracyTooltip);
			} else {
				NotApplicableColumn();
			}
			SliderColumn("##CrossbowPrecision", cfg.precisionMultipliers.crossbow, kCrossbowPrecisionTooltip);
			if (!cfg.crossbowAiming) {
				EndDisabled();
			}

			TableNextRow();
			TableNextColumn();
			if (Button(kResetCrossbowsButton)) {
				cfg.crossbowAiming = defaults.crossbowAiming;
				if (hasAccuracy) {
					cfg.accuracyMultipliers.crossbow = defaults.accuracyMultipliers.crossbow;
				}
				cfg.precisionMultipliers.crossbow = defaults.precisionMultipliers.crossbow;
				Options::Save();
			}
			EmptyRow();
			PopID();
		}

		void RenderStaffRows(Options::Config& cfg, const Options::Config& defaults, bool hasAccuracy) {
			PushID("Staves");
			TableNextRow();
			ToggleColumn(kStaffLabel, cfg.staffAiming, kStaffTooltip);
			if (!cfg.staffAiming) {
				BeginDisabled();
			}
			if (hasAccuracy) {
				SliderColumn("##StaffAccuracy", cfg.accuracyMultipliers.staff, kStaffAccuracyTooltip);
			} else {
				NotApplicableColumn();
			}
			SliderColumn("##StaffPrecision", cfg.precisionMultipliers.staff, kStaffPrecisionTooltip);

			TableNextRow();
			ToggleColumn(kUseEnchantingSkillLabel, cfg.stavesUseEnchantingSkill, kUseEnchantingSkillTooltip);
			if (!cfg.staffAiming) {
				EndDisabled();
			}

			TableNextRow();
			TableNextColumn();
			if (Button(kResetStavesButton)) {
				cfg.staffAiming = defaults.staffAiming;
				if (hasAccuracy) {
					cfg.accuracyMultipliers.staff = defaults.accuracyMultipliers.staff;
				}
				cfg.precisionMultipliers.staff = defaults.precisionMultipliers.staff;
				cfg.stavesUseEnchantingSkill = defaults.stavesUseEnchantingSkill;
				Options::Save();
			}
			EmptyRow();
			PopID();
		}

		void RenderSpellRows(Options::Config& cfg, const Options::Config& defaults, bool hasAccuracy) {
			PushID("Spells");
			TableNextRow();
			ToggleColumn(kSpellLabel, cfg.spellAiming, kSpellTooltip);
			if (!cfg.spellAiming) {
				BeginDisabled();
			}
			if (hasAccuracy) {
				SliderColumn("##SpellAccuracy", cfg.accuracyMultipliers.spell, kSpellAccuracyTooltip);
			} else {
				NotApplicableColumn();
			}
			SliderColumn("##SpellPrecision", cfg.precisionMultipliers.spell, kSpellPrecisionTooltip);

			TableNextRow();
			ToggleColumn(kUseHighestMagicSkillLabel, cfg.spellsUseHighestMagicSkill, kUseHighestMagicSkillTooltip);
			if (!cfg.spellAiming) {
				EndDisabled();
			}

			TableNextRow();
			TableNextColumn();
			if (Button(kResetSpellsButton)) {
				cfg.spellAiming = defaults.spellAiming;
				if (hasAccuracy) {
					cfg.accuracyMultipliers.spell = defaults.accuracyMultipliers.spell;
				}
				cfg.precisionMultipliers.spell = defaults.precisionMultipliers.spell;
				cfg.spellsUseHighestMagicSkill = defaults.spellsUseHighestMagicSkill;
				Options::Save();
			}
			EmptyRow();
			PopID();
		}

		void RenderConcentrationSpellRows(Options::Config& cfg, const Options::Config& defaults, bool hasAccuracy) {
			PushID("ConcentrationSpells");

			TableNextRow();
			ToggleColumn(kConcentrationSpellLabel, cfg.concentrationSpellsAiming, kConcentrationSpellTooltip);

			if (!cfg.concentrationSpellsAiming) {
				BeginDisabled();
			}
			if (hasAccuracy) {
				SliderColumn("##ConcentrationSpellAccuracy", cfg.accuracyMultipliers.concentrationSpell, kConcentrationSpellAccuracyTooltip);
			} else {
				NotApplicableColumn();
			}
			SliderColumn("##ConcentrationSpellPrecision", cfg.precisionMultipliers.concentrationSpell, kConcentrationSpellPrecisionTooltip);

			TableNextRow();
			TableSetColumnIndex(1);
			CenteredHeaderCell(kMinRangeMultiplierHeader, false);
			TableSetColumnIndex(2);
			CenteredHeaderCell(kMaxRangeMultiplierHeader, false);

			TableNextRow();
			ToggleColumn(kScaleRangeWithSkillLabel, cfg.concentrationSpellRangeScaling, kScaleRangeWithSkillTooltip);
			if (!cfg.concentrationSpellRangeScaling) {
				BeginDisabled();
			}
			SliderColumn("##MinRangeMultiplier", cfg.concentrationSpellMinRangeMultiplier, kMinRangeMultiplierTooltip, 0.5f, 5.0f);
			SliderColumn("##MaxRangeMultiplier", cfg.concentrationSpellMaxRangeMultiplier, kMaxRangeMultiplierTooltip, 0.5f, 5.0f);
			if (!cfg.concentrationSpellRangeScaling) {
				EndDisabled();
			}
			if (!cfg.concentrationSpellsAiming) {
				EndDisabled();
			}

			TableNextRow();
			TableNextColumn();
			if (Button(kResetConcentrationSpellsButton)) {
				cfg.concentrationSpellsAiming = defaults.concentrationSpellsAiming;
				if (hasAccuracy) {
					cfg.accuracyMultipliers.concentrationSpell = defaults.accuracyMultipliers.concentrationSpell;
				}
				cfg.precisionMultipliers.concentrationSpell = defaults.precisionMultipliers.concentrationSpell;
				cfg.concentrationSpellRangeScaling = defaults.concentrationSpellRangeScaling;
				cfg.concentrationSpellMinRangeMultiplier = defaults.concentrationSpellMinRangeMultiplier;
				cfg.concentrationSpellMaxRangeMultiplier = defaults.concentrationSpellMaxRangeMultiplier;
				Options::Save();
			}
			EmptyRow();
			PopID();
		}

		void RenderBlindnessRows(Options::Config& cfg, const Options::Config& defaults) {
			PushID("Blindness");
			TableNextRow();
			ToggleColumn(kBlindnessLabel, cfg.blindnessAffectsAiming, kBlindnessTooltip);
			if (!cfg.blindnessAffectsAiming) {
				BeginDisabled();
			}
			SliderColumn("##BlindnessAccuracy", cfg.accuracyMultipliers.blindness, kBlindnessAccuracyTooltip);
			SliderColumn("##BlindnessPrecision", cfg.precisionMultipliers.blindness, kBlindnessPrecisionTooltip);
			if (!cfg.blindnessAffectsAiming) {
				EndDisabled();
			}

			TableNextRow();
			TableNextColumn();
			if (Button(kResetBlindnessButton)) {
				cfg.blindnessAffectsAiming = defaults.blindnessAffectsAiming;
				cfg.accuracyMultipliers.blindness = defaults.accuracyMultipliers.blindness;
				cfg.precisionMultipliers.blindness = defaults.precisionMultipliers.blindness;
				Options::Save();
			}
			EmptyRow();
			PopID();
		}

		void RenderKeywordFilters() {
			PushID("Filters");
			SeparatorText(kFiltersHeader);
			TextWrapped("%s", kFiltersDescription);
			Spacing();

			static char        includeBuffer[512] = "";
			static char        excludeBuffer[512] = "";
			static unsigned int buffersGeneration = 0;

			if (IsWindowAppearing() || buffersGeneration != Options::currentID) {
				strcpy_s(includeBuffer, fmt::format("{}", fmt::join(Options::NPC.includeKeywords, ",")).c_str());
				strcpy_s(excludeBuffer, fmt::format("{}", fmt::join(Options::NPC.excludeKeywords, ",")).c_str());
				buffersGeneration = Options::currentID;
			}

			Text("%s", kIncludeKeywordsLabel);
			if (InputTextWithHint("##IncludeKeywords", kIncludeKeywordsHint, includeBuffer, sizeof(includeBuffer))) {
				Options::NPC.includeKeywords = clib_util::string::split(std::string(includeBuffer), ",");
				Options::Save();
			}
			ShowTooltip(kIncludeKeywordsTooltip);
			Spacing();

			Text("%s", kExcludeKeywordsLabel);
			if (InputTextWithHint("##ExcludeKeywords", kExcludeKeywordsHint, excludeBuffer, sizeof(excludeBuffer))) {
				Options::NPC.excludeKeywords = clib_util::string::split(std::string(excludeBuffer), ",");
				Options::Save();
			}
			ShowTooltip(kExcludeKeywordsTooltip);

			Spacing();
			if (Button(kResetFiltersButton)) {
				Options::NPC.includeKeywords = Options::NPCConfig::defaultConfig.includeKeywords;
				Options::NPC.excludeKeywords = Options::NPCConfig::defaultConfig.excludeKeywords;
				strcpy_s(includeBuffer, fmt::format("{}", fmt::join(Options::NPC.includeKeywords, ",")).c_str());
				strcpy_s(excludeBuffer, fmt::format("{}", fmt::join(Options::NPC.excludeKeywords, ",")).c_str());
				Options::Save();
			}
			PopID();
		}

		void RenderNPCSection() {
			PushID("NPC");
			constexpr bool hasAccuracy = true;
			if (BeginPageTable("NPCTable", hasAccuracy)) {
				RenderBowRows(Options::NPC, Options::NPCConfig::defaultConfig, hasAccuracy);
				RenderCrossbowRows(Options::NPC, Options::NPCConfig::defaultConfig, hasAccuracy);
				RenderStaffRows(Options::NPC, Options::NPCConfig::defaultConfig, hasAccuracy);
				RenderSpellRows(Options::NPC, Options::NPCConfig::defaultConfig, hasAccuracy);
				RenderConcentrationSpellRows(Options::NPC, Options::NPCConfig::defaultConfig, hasAccuracy);
				RenderBlindnessRows(Options::NPC, Options::NPCConfig::defaultConfig);
				EndTable();
			}
			Spacing();
			RenderKeywordFilters();
			PopID();
		}

		void RenderPlayerSection() {
			PushID("Player");
			constexpr bool hasAccuracy = false;  // Player's accuracy multipliers are never used in gameplay.
			if (BeginPageTable("PlayerTable", hasAccuracy)) {
				RenderBowRows(Options::Player, Options::PlayerConfig::defaultConfig, hasAccuracy);
				RenderCrossbowRows(Options::Player, Options::PlayerConfig::defaultConfig, hasAccuracy);
				RenderStaffRows(Options::Player, Options::PlayerConfig::defaultConfig, hasAccuracy);
				RenderSpellRows(Options::Player, Options::PlayerConfig::defaultConfig, hasAccuracy);
				RenderConcentrationSpellRows(Options::Player, Options::PlayerConfig::defaultConfig, hasAccuracy);
				EndTable();
			}
			PopID();
		}

		void RenderActionsSection() {
			if (Button(kReloadSettingsButton, ImVec2(250, 0))) {
				Options::Load();
				logger::info("Settings reloaded via menu");
			}
			ShowTooltip(kReloadSettingsTooltip);
		}
	}

	void Register() {
		if (!SKSEMenuFramework::IsInstalled()) {
			logger::warn("SKSEMenuFramework is not installed");
			return;
		}

		SKSEMenuFramework::SetSection(kSectionName);

		SKSEMenuFramework::AddSectionItem(kNPCSectionItem, RenderNPCSection);
		SKSEMenuFramework::AddSectionItem(kPlayerSectionItem, RenderPlayerSection);
		SKSEMenuFramework::AddSectionItem(kActionsSectionItem, RenderActionsSection);

		logger::info("Added section to SKSEMenuFramework");
	}
}
