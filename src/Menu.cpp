#include "Menu.h"
#include "AimDebug.h"
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
		constexpr const char* kUseEnchantingSkillTooltip = "When enabled, staff aiming is based on the Enchanting skill instead of the staff's spell's Magic school. Mutually exclusive with Use Highest Magic Skill.";
		constexpr const char* kStaffUseHighestMagicSkillTooltip = "When enabled, staff aiming is based on the highest Magic skill, regardless of the staff's spell's Magic school. Mutually exclusive with Use Enchanting Skill.";
		constexpr const char* kResetStavesButton = "Reset Staves to Default";

		constexpr const char* kSpellLabel = "Learn Spells";
		constexpr const char* kSpellTooltip = "When enabled, corresponding Magic skill will affect aiming with spells.";
		constexpr const char* kSpellAccuracyTooltip = "How much the skill contributes to Accuracy with fire-and-forget spells. The higher this multiplier is the easier it is to aim.";
		constexpr const char* kSpellPrecisionTooltip = "How much the skill contributes to Precision with fire-and-forget spells. The higher this multiplier is the easier it is to hit the target.";
		constexpr const char* kUseHighestMagicSkillLabel = "Use Highest Magic Skill";
		constexpr const char* kUseHighestMagicSkillTooltip = "When enabled, fire-and-forget spell aiming is based on the highest Magic skill, regardless of the spell's Magic school.";
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
		constexpr const char* kConcentrationSpellUseHighestMagicSkillTooltip = "When enabled, concentration spell aiming is based on the highest Magic skill, regardless of the spell's Magic school.";
		constexpr const char* kResetConcentrationSpellsButton = "Reset Concentration Spells to Default";

		constexpr const char* kScaleChargeTimeWithSkillLabel = "Scale Charge Speed With Skill";
		constexpr const char* kMinChargeSpeedMultiplierHeader = "Min Speed";
		constexpr const char* kMaxChargeSpeedMultiplierHeader = "Max Speed";
		constexpr const char* kScaleSpellChargeTimeWithSkillTooltip = "When enabled, corresponding Magic skill also affects how fast fire-and-forget spells charge/cast. (the higher the skill the faster the cast)";
		constexpr const char* kScaleConcentrationSpellChargeTimeWithSkillTooltip = "When enabled, corresponding Magic skill also affects how fast concentration spells charge/cast. (the higher the skill the faster the cast)";
		constexpr const char* kScaleStaffChargeTimeWithSkillTooltip = "When enabled, corresponding skill also affects how fast staves charge/cast. (the higher the skill the faster the cast)";
		constexpr const char* kMinChargeSpeedMultiplierTooltip = "Charge speed multiplier at 0 skill. 1.0 is vanilla speed, higher is faster.";
		constexpr const char* kMaxChargeSpeedMultiplierTooltip = "Charge speed multiplier at 100 skill (fastest). 1.0 is vanilla speed, higher is faster.";

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

		constexpr const char* kMinHeadshotChanceTooltipFmt = "Chance for a skilled {} to go for a headshot at 0 effective Archery skill.";
		constexpr const char* kMaxHeadshotChanceTooltipFmt = "Chance for a skilled {} to go for a headshot at 100 effective Archery skill.";

		constexpr const char* kBowHeadshotLabel = "Bow Headshots";
		constexpr const char* kBowHeadshotTooltip = "When enabled, sufficiently skilled archers have a chance to aim for the head instead of the chest.";
		constexpr const char* kCrossbowHeadshotLabel = "Crossbow Headshots";
		constexpr const char* kCrossbowHeadshotTooltip = "When enabled, sufficiently skilled crossbowmen have a chance to aim for the head instead of the chest.";

		constexpr const char* kReloadSettingsButton = "Reload Settings";
		constexpr const char* kReloadSettingsTooltip = "Reload all settings from the INI file, discarding any unsaved in-memory changes.";

		constexpr const char* kShowAimDebugLabel = "Show Aim Debug Overlay";
		constexpr const char* kShowAimDebugTooltip = "Draws a marker at the point each NPC is currently aiming at (yellow), or red if it's a headshot. For debugging only.";

		constexpr const char* kEnableNPCLabel = "Enable for NPCs";
		constexpr const char* kEnableNPCTooltip = "Master switch for NPCs. When disabled, NPCs have vanilla aiming and all settings below are ignored.";
		constexpr const char* kEnablePlayerLabel = "Enable for Player";
		constexpr const char* kEnablePlayerTooltip = "Master switch for the Player. When disabled, the Player has vanilla aiming and all settings below are ignored.";

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

		void ExclusiveToggleColumn(const char* label, bool& value, bool& other, const char* tooltip) {
			TableNextColumn();
			if (Checkbox(label, &value)) {
				if (value) {
					other = false;
				}
				Options::Save();
			}
			ShowTooltip(tooltip);
		}

		void SliderColumn(const char* id, float& value, const char* tooltip, float min = 0.01f, float max = 10.0f, const char* format = "x%.2f") {
			TableNextColumn();
			SetNextItemWidth(-1.0f);
			if (SliderFloat(id, &value, min, max, format)) {
				value = RoundTo2Decimals(value);
			}
			if (IsItemDeactivatedAfterEdit()) {
				Options::Save();
			}
			ShowTooltip(tooltip);
		}

		/// Like SliderColumn, but for a 0-1 fraction that should read as a percentage (e.g. a chance)
		/// rather than a raw decimal or "xN.NN" multiplier.
		void PercentSliderColumn(const char* id, float& value, const char* tooltip) {
			TableNextColumn();
			SetNextItemWidth(-1.0f);
			float percent = value * 100.0f;
			if (SliderFloat(id, &percent, 0.0f, 100.0f, "%.0f%%")) {
				value = percent / 100.0f;
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

		void RenderBowRows(Options::Config& cfg, const Options::Config& defaults, bool hasAccuracy, Options::NPCConfig::HeadshotSettings* headshots = nullptr, const Options::NPCConfig::HeadshotSettings* headshotDefaults = nullptr) {
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

			if (headshots) {
				TableNextRow();
				ToggleColumn(kBowHeadshotLabel, headshots->bow, kBowHeadshotTooltip);
				if (!headshots->bow) {
					BeginDisabled();
				}
				PercentSliderColumn("##BowMinHeadshotChance", headshots->bowMinChance, fmt::format(kMinHeadshotChanceTooltipFmt, "archer").c_str());
				PercentSliderColumn("##BowMaxHeadshotChance", headshots->bowMaxChance, fmt::format(kMaxHeadshotChanceTooltipFmt, "archer").c_str());
				if (!headshots->bow) {
					EndDisabled();
				}
			}

			TableNextRow();
			TableNextColumn();
			if (Button(kResetBowsButton)) {
				cfg.bowAiming = defaults.bowAiming;
				if (hasAccuracy) {
					cfg.accuracyMultipliers.bow = defaults.accuracyMultipliers.bow;
				}
				cfg.precisionMultipliers.bow = defaults.precisionMultipliers.bow;
				if (headshots && headshotDefaults) {
					headshots->bow = headshotDefaults->bow;
					headshots->bowMinChance = headshotDefaults->bowMinChance;
					headshots->bowMaxChance = headshotDefaults->bowMaxChance;
				}
				Options::Save();
			}
			EmptyRow();
			PopID();
		}

		void RenderCrossbowRows(Options::Config& cfg, const Options::Config& defaults, bool hasAccuracy, Options::NPCConfig::HeadshotSettings* headshots = nullptr, const Options::NPCConfig::HeadshotSettings* headshotDefaults = nullptr) {
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

			if (headshots) {
				TableNextRow();
				ToggleColumn(kCrossbowHeadshotLabel, headshots->crossbow, kCrossbowHeadshotTooltip);
				if (!headshots->crossbow) {
					BeginDisabled();
				}
				PercentSliderColumn("##CrossbowMinHeadshotChance", headshots->crossbowMinChance, fmt::format(kMinHeadshotChanceTooltipFmt, "crossbowman").c_str());
				PercentSliderColumn("##CrossbowMaxHeadshotChance", headshots->crossbowMaxChance, fmt::format(kMaxHeadshotChanceTooltipFmt, "crossbowman").c_str());
				if (!headshots->crossbow) {
					EndDisabled();
				}
			}

			TableNextRow();
			TableNextColumn();
			if (Button(kResetCrossbowsButton)) {
				cfg.crossbowAiming = defaults.crossbowAiming;
				if (hasAccuracy) {
					cfg.accuracyMultipliers.crossbow = defaults.accuracyMultipliers.crossbow;
				}
				cfg.precisionMultipliers.crossbow = defaults.precisionMultipliers.crossbow;
				if (headshots && headshotDefaults) {
					headshots->crossbow = headshotDefaults->crossbow;
					headshots->crossbowMinChance = headshotDefaults->crossbowMinChance;
					headshots->crossbowMaxChance = headshotDefaults->crossbowMaxChance;
				}
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
			ExclusiveToggleColumn(kUseEnchantingSkillLabel, cfg.stavesUseEnchantingSkill, cfg.stavesUseHighestMagicSkill, kUseEnchantingSkillTooltip);

			TableNextRow();
			ExclusiveToggleColumn(kUseHighestMagicSkillLabel, cfg.stavesUseHighestMagicSkill, cfg.stavesUseEnchantingSkill, kStaffUseHighestMagicSkillTooltip);
			if (!cfg.staffAiming) {
				EndDisabled();
			}

			TableNextRow();
			TableSetColumnIndex(1);
			CenteredHeaderCell(kMinChargeSpeedMultiplierHeader, false);
			TableSetColumnIndex(2);
			CenteredHeaderCell(kMaxChargeSpeedMultiplierHeader, false);

			TableNextRow();
			ToggleColumn(kScaleChargeTimeWithSkillLabel, cfg.staffChargeTimeScaling, kScaleStaffChargeTimeWithSkillTooltip);
			if (!cfg.staffChargeTimeScaling) {
				BeginDisabled();
			}
			SliderColumn("##StaffMinChargeSpeedMultiplier", cfg.staffChargeSpeedMinMultiplier, kMinChargeSpeedMultiplierTooltip, 0.1f, 3.0f);
			SliderColumn("##StaffMaxChargeSpeedMultiplier", cfg.staffChargeSpeedMaxMultiplier, kMaxChargeSpeedMultiplierTooltip, 0.1f, 3.0f);
			if (!cfg.staffChargeTimeScaling) {
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
				cfg.stavesUseHighestMagicSkill = defaults.stavesUseHighestMagicSkill;
				cfg.staffChargeTimeScaling = defaults.staffChargeTimeScaling;
				cfg.staffChargeSpeedMinMultiplier = defaults.staffChargeSpeedMinMultiplier;
				cfg.staffChargeSpeedMaxMultiplier = defaults.staffChargeSpeedMaxMultiplier;
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
			TableSetColumnIndex(1);
			CenteredHeaderCell(kMinChargeSpeedMultiplierHeader, false);
			TableSetColumnIndex(2);
			CenteredHeaderCell(kMaxChargeSpeedMultiplierHeader, false);

			TableNextRow();
			ToggleColumn(kScaleChargeTimeWithSkillLabel, cfg.spellChargeTimeScaling, kScaleSpellChargeTimeWithSkillTooltip);
			if (!cfg.spellChargeTimeScaling) {
				BeginDisabled();
			}
			SliderColumn("##SpellMinChargeSpeedMultiplier", cfg.spellChargeSpeedMinMultiplier, kMinChargeSpeedMultiplierTooltip, 0.1f, 3.0f);
			SliderColumn("##SpellMaxChargeSpeedMultiplier", cfg.spellChargeSpeedMaxMultiplier, kMaxChargeSpeedMultiplierTooltip, 0.1f, 3.0f);
			if (!cfg.spellChargeTimeScaling) {
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
				cfg.spellChargeTimeScaling = defaults.spellChargeTimeScaling;
				cfg.spellChargeSpeedMinMultiplier = defaults.spellChargeSpeedMinMultiplier;
				cfg.spellChargeSpeedMaxMultiplier = defaults.spellChargeSpeedMaxMultiplier;
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

			TableNextRow();
			ToggleColumn(kUseHighestMagicSkillLabel, cfg.concentrationSpellsUseHighestMagicSkill, kConcentrationSpellUseHighestMagicSkillTooltip);
			if (!cfg.concentrationSpellsAiming) {
				EndDisabled();
			}

			TableNextRow();
			TableSetColumnIndex(1);
			CenteredHeaderCell(kMinChargeSpeedMultiplierHeader, false);
			TableSetColumnIndex(2);
			CenteredHeaderCell(kMaxChargeSpeedMultiplierHeader, false);

			TableNextRow();
			ToggleColumn(kScaleChargeTimeWithSkillLabel, cfg.concentrationSpellChargeTimeScaling, kScaleConcentrationSpellChargeTimeWithSkillTooltip);
			if (!cfg.concentrationSpellChargeTimeScaling) {
				BeginDisabled();
			}
			SliderColumn("##ConcentrationSpellMinChargeSpeedMultiplier", cfg.concentrationSpellChargeSpeedMinMultiplier, kMinChargeSpeedMultiplierTooltip, 0.1f, 3.0f);
			SliderColumn("##ConcentrationSpellMaxChargeSpeedMultiplier", cfg.concentrationSpellChargeSpeedMaxMultiplier, kMaxChargeSpeedMultiplierTooltip, 0.1f, 3.0f);
			if (!cfg.concentrationSpellChargeTimeScaling) {
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
				cfg.concentrationSpellsUseHighestMagicSkill = defaults.concentrationSpellsUseHighestMagicSkill;
				cfg.concentrationSpellChargeTimeScaling = defaults.concentrationSpellChargeTimeScaling;
				cfg.concentrationSpellChargeSpeedMinMultiplier = defaults.concentrationSpellChargeSpeedMinMultiplier;
				cfg.concentrationSpellChargeSpeedMaxMultiplier = defaults.concentrationSpellChargeSpeedMaxMultiplier;
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

			if (Checkbox(kEnableNPCLabel, &Options::NPC.enabled)) {
				Options::Save();
			}
			ShowTooltip(kEnableNPCTooltip);
			Spacing();

			if (!Options::NPC.enabled) {
				BeginDisabled();
			}
			constexpr bool hasAccuracy = true;
			if (BeginPageTable("NPCTable", hasAccuracy)) {
				RenderBowRows(Options::NPC, Options::NPCConfig::defaultConfig, hasAccuracy, &Options::NPC.headshots, &Options::NPCConfig::defaultConfig.headshots);
				RenderCrossbowRows(Options::NPC, Options::NPCConfig::defaultConfig, hasAccuracy, &Options::NPC.headshots, &Options::NPCConfig::defaultConfig.headshots);
				RenderStaffRows(Options::NPC, Options::NPCConfig::defaultConfig, hasAccuracy);
				RenderSpellRows(Options::NPC, Options::NPCConfig::defaultConfig, hasAccuracy);
				RenderConcentrationSpellRows(Options::NPC, Options::NPCConfig::defaultConfig, hasAccuracy);
				RenderBlindnessRows(Options::NPC, Options::NPCConfig::defaultConfig);
				EndTable();
			}
			Spacing();
			RenderKeywordFilters();
			if (!Options::NPC.enabled) {
				EndDisabled();
			}
			PopID();
		}

		void RenderPlayerSection() {
			PushID("Player");

			if (Checkbox(kEnablePlayerLabel, &Options::Player.enabled)) {
				Options::Save();
			}
			ShowTooltip(kEnablePlayerTooltip);
			Spacing();

			if (!Options::Player.enabled) {
				BeginDisabled();
			}
			constexpr bool hasAccuracy = false;  // Player's accuracy multipliers are never used in gameplay.
			if (BeginPageTable("PlayerTable", hasAccuracy)) {
				RenderBowRows(Options::Player, Options::PlayerConfig::defaultConfig, hasAccuracy);
				RenderCrossbowRows(Options::Player, Options::PlayerConfig::defaultConfig, hasAccuracy);
				RenderStaffRows(Options::Player, Options::PlayerConfig::defaultConfig, hasAccuracy);
				RenderSpellRows(Options::Player, Options::PlayerConfig::defaultConfig, hasAccuracy);
				RenderConcentrationSpellRows(Options::Player, Options::PlayerConfig::defaultConfig, hasAccuracy);
				EndTable();
			}
			if (!Options::Player.enabled) {
				EndDisabled();
			}
			PopID();
		}

		void RenderActionsSection() {
			if (Button(kReloadSettingsButton, ImVec2(250, 0))) {
				Options::Load();
				logger::info("Settings reloaded via menu");
			}
			ShowTooltip(kReloadSettingsTooltip);

			Spacing();
			Checkbox(kShowAimDebugLabel, &AimDebug::enabled);
			ShowTooltip(kShowAimDebugTooltip);
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
