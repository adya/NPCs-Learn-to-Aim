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
		constexpr const char* kScaleBowDrawSpeedWithSkillTooltip = "When enabled, Archery skill also affects how fast a bow reaches full draw. (the higher the skill the faster the draw)";
		constexpr const char* kResetBowsButton = "Reset Bows to Default";

		constexpr const char* kBowSteadyingLabel = "Full Draw Steadies Aim";
		constexpr const char* kBowSteadyingTooltip = "When enabled, holding a bow at full draw tightens its spread, with diminishing returns.";
		constexpr const char* kBowSteadyingMaxReductionTooltip = "The most spread that holding a full draw can remove, approached the longer it's held.";
		constexpr const char* kBowSteadyingHalfDurationTooltip = "Duration of holding a full draw to get roughly half of the max spread reduction. Lower means the aim steadies faster.";

		constexpr const char* kBowFatigueLabel = "Full Draw Fatigues Aim";
		constexpr const char* kBowFatigueTooltip = "When enabled, holding a bow at full draw for too long tires the archer's arm, growing spread back the longer it's held past their fatigue threshold. That threshold scales with Archery skill.";
		constexpr const char* kBowFatigueMinThresholdHeader = "Min Threshold";
		constexpr const char* kBowFatigueMaxThresholdHeader = "Max Threshold";
		constexpr const char* kBowFatigueMinThresholdTooltip = "Seconds a full draw can be held at 0 Archery skill before fatigue sets in.";
		constexpr const char* kBowFatigueMaxThresholdTooltip = "Seconds a full draw can be held at 100 Archery skill before fatigue sets in.";
		constexpr const char* kBowMaxFatiguePenaltyHeader = "Max Fatigue";
		constexpr const char* kBowFatigueHalfDurationHeader = "Half Duration";
		constexpr const char* kBowMaxFatiguePenaltyTooltip = "The most spread that prolonged fatigue can add. Note, that in fatigued state, the bonus from steadied aim is negated before adding extra spread.";
		constexpr const char* kBowFatigueHalfDurationTooltip = "Seconds past the fatigue threshold to regrow roughly half of the max fatigue penalty. Lower duration means the aim shakes apart faster once fatigue sets in.";

		constexpr const char* kCrossbowLabel = "Learn Crossbows";
		constexpr const char* kCrossbowTooltip = "When enabled, Archery skill will affect aiming with crossbows.";
		constexpr const char* kCrossbowAccuracyTooltip = "How much Archery skill contributes to Accuracy with crossbows. The higher this multiplier is the easier it is to aim.";
		constexpr const char* kCrossbowPrecisionTooltip = "How much Archery skill contributes to Precision with crossbows. The higher this multiplier is the easier it is to hit the target.";
		constexpr const char* kScaleCrossbowDrawSpeedWithSkillTooltip = "When enabled, Archery skill also affects how fast a crossbow is reloaded. (the higher the skill the faster the reload)";
		constexpr const char* kResetCrossbowsButton = "Reset Crossbows to Default";

		constexpr const char* kScaleDrawSpeedWithSkillLabel = "Scale Draw Speed With Skill";

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

		constexpr const char* kScrollLabel = "Learn Scrolls";
		constexpr const char* kScrollTooltip = "When enabled, corresponding Magic skill will affect aiming with scrolls.";
		constexpr const char* kScrollAccuracyTooltip = "How much the skill contributes to Accuracy with scrolls. The higher this multiplier is the easier it is to aim.";
		constexpr const char* kScrollPrecisionTooltip = "How much the skill contributes to Precision with scrolls. The higher this multiplier is the easier it is to hit the target.";
		constexpr const char* kScrollUseHighestMagicSkillTooltip = "When enabled, scroll aiming is based on the highest Magic skill, regardless of the scroll spell's Magic school.";
		constexpr const char* kScrollUseEnchantingSkillTooltip = "When enabled, scroll aiming is based on the Enchanting skill instead of the scroll spell's Magic school. Mutually exclusive with Use Highest Magic Skill.";
		constexpr const char* kScaleScrollChargeTimeWithSkillTooltip = "When enabled, corresponding skill also affects how fast scrolls charge/cast. (the higher the skill the faster the cast)";
		constexpr const char* kResetScrollsButton = "Reset Scrolls to Default";

		constexpr const char* kScaleChargeTimeWithSkillLabel = "Scale Charge Speed With Skill";
		constexpr const char* kMinChargeSpeedMultiplierHeader = "Min Speed";
		constexpr const char* kMaxChargeSpeedMultiplierHeader = "Max Speed";
		constexpr const char* kScaleSpellChargeTimeWithSkillTooltip = "When enabled, corresponding Magic skill also affects how fast fire-and-forget spells charge/cast. (the higher the skill the faster the cast)";
		constexpr const char* kScaleConcentrationSpellChargeTimeWithSkillTooltip = "When enabled, corresponding Magic skill also affects how fast concentration spells charge/cast. (the higher the skill the faster the cast)";
		constexpr const char* kScaleStaffChargeTimeWithSkillTooltip = "When enabled, corresponding skill also affects how fast staves charge/cast. (the higher the skill the faster the cast)";
		constexpr const char* kMinChargeSpeedMultiplierTooltip = "Charge speed multiplier at 0 skill. 1.0 is vanilla speed, higher is faster.";
		constexpr const char* kMaxChargeSpeedMultiplierTooltip = "Charge speed multiplier at 100 skill (fastest). 1.0 is vanilla speed, higher is faster.";

		constexpr const char* kSpellOverchargeLabel = "Overcharge Steadies Aim";
		constexpr const char* kSpellOverchargeTooltip = "When enabled, holding a fully charged spell before releasing it tightens its spread. The longer it's held, the steadier the shot, with diminishing returns.";
		constexpr const char* kSpellOverchargeMaxReductionTooltip = "The most spread that overcharging can remove, approached the longer the spell is held. 50% means the spread can at best be halved.";
		constexpr const char* kSpellOverchargeHalfDurationTooltip = "Duration of holding a fully charged spell to get roughly half of the max spread reduction. Lower means the aim steadies faster. 1 sec with 50% reduction means that in 1 second spread will be reduced by 25%.";
		constexpr const char* kStaffOverchargeTooltip = "When enabled, holding a fully charged staff before releasing it tightens its spread. The longer it's held, the steadier the shot, with diminishing returns.";
		constexpr const char* kStaffOverchargeMaxReductionTooltip = "The most spread that overcharging can remove, approached the longer the staff is held. 50% means the spread can at best be halved.";
		constexpr const char* kStaffOverchargeHalfDurationTooltip = "Duration of holding a fully charged staff to get roughly half of the max spread reduction. Lower means the aim steadies faster. 1 sec with 50% reduction means that in 1 second spread will be reduced by 25%.";
		constexpr const char* kScrollOverchargeTooltip = "When enabled, holding a fully charged scroll before releasing it tightens its spread. The longer it's held, the steadier the shot, with diminishing returns.";
		constexpr const char* kScrollOverchargeMaxReductionTooltip = "The most spread that overcharging can remove, approached the longer the scroll is held. 50% means the spread can at best be halved.";
		constexpr const char* kScrollOverchargeHalfDurationTooltip = "Duration of holding a fully charged scroll to get roughly half of the max spread reduction. Lower means the aim steadies faster. 1 sec with 50% reduction means that in 1 second spread will be reduced by 25%.";
		constexpr const char* kSpellOverchargeMaxReductionHeader = "Max Reduction";
		constexpr const char* kSpellOverchargeHalfDurationHeader = "Half Duration";

		/// Explanations shown for one category's overcharge controls.
		struct OverchargeTooltips
		{
			const char* toggle;
			const char* maxReduction;
			const char* halfDuration;
		};
		constexpr OverchargeTooltips kSpellOverchargeTooltips{ kSpellOverchargeTooltip, kSpellOverchargeMaxReductionTooltip, kSpellOverchargeHalfDurationTooltip };
		constexpr OverchargeTooltips kStaffOverchargeTooltips{ kStaffOverchargeTooltip, kStaffOverchargeMaxReductionTooltip, kStaffOverchargeHalfDurationTooltip };
		constexpr OverchargeTooltips kScrollOverchargeTooltips{ kScrollOverchargeTooltip, kScrollOverchargeMaxReductionTooltip, kScrollOverchargeHalfDurationTooltip };

		constexpr const char* kSpellFatigueLabel = "Overcharge Fatigues Aim";
		constexpr const char* kSpellFatigueTooltip = "When enabled, holding a spell overcharged for too long tires the caster's arm, growing spread back the longer it's held past their fatigue threshold. That threshold scales with Magic skill.";
		constexpr const char* kStaffFatigueTooltip = "When enabled, holding a staff overcharged for too long tires the caster's arm, growing spread back the longer it's held past their fatigue threshold. That threshold scales with Magic skill.";
		constexpr const char* kScrollFatigueTooltip = "When enabled, holding a scroll overcharged for too long tires the caster's arm, growing spread back the longer it's held past their fatigue threshold. That threshold scales with Magic skill.";
		constexpr const char* kFatigueMinThresholdHeader = "Min Threshold";
		constexpr const char* kFatigueMaxThresholdHeader = "Max Threshold";
		constexpr const char* kFatigueMinThresholdTooltip = "Seconds a spell/staff/scroll can be held overcharged at 0 skill before fatigue sets in.";
		constexpr const char* kFatigueMaxThresholdTooltip = "Seconds a spell/staff/scroll can be held overcharged at 100 skill before fatigue sets in.";
		constexpr const char* kMaxFatiguePenaltyHeader = "Max Fatigue";
		constexpr const char* kFatigueHalfDurationHeader = "Half Duration";
		constexpr const char* kMaxFatiguePenaltyTooltip = "The most spread that prolonged fatigue can add. Note, that in fatigued state, the bonus from overcharging is negated before adding extra spread.";
		constexpr const char* kFatigueHalfDurationTooltip = "Seconds past the fatigue threshold to regrow roughly half of the max fatigue penalty. Lower duration means the aim shakes apart faster once fatigue sets in.";

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

		void RenderChargeSpeedRows(Options::Config::SpeedOptions& chargeSpeed, const char* label, const char* toggleTooltip, const char* minSliderID, const char* maxSliderID) {
			TableNextRow();
			TableSetColumnIndex(1);
			CenteredHeaderCell(kMinChargeSpeedMultiplierHeader, false);
			TableSetColumnIndex(2);
			CenteredHeaderCell(kMaxChargeSpeedMultiplierHeader, false);

			TableNextRow();
			ToggleColumn(label, chargeSpeed.enabled, toggleTooltip);
			if (!chargeSpeed.enabled) {
				BeginDisabled();
			}
			SliderColumn(minSliderID, chargeSpeed.minMultiplier, kMinChargeSpeedMultiplierTooltip, 0.1f, 3.0f);
			SliderColumn(maxSliderID, chargeSpeed.maxMultiplier, kMaxChargeSpeedMultiplierTooltip, 0.1f, 3.0f);
			if (!chargeSpeed.enabled) {
				EndDisabled();
			}
		}

		void RenderBowSteadyingRows(Options::Config::Steadying& steadying) {
			TableNextRow();
			TableSetColumnIndex(1);
			CenteredHeaderCell(kSpellOverchargeMaxReductionHeader, false);
			TableSetColumnIndex(2);
			CenteredHeaderCell(kSpellOverchargeHalfDurationHeader, false);

			TableNextRow();
			ToggleColumn(kBowSteadyingLabel, steadying.enabled, kBowSteadyingTooltip);
			if (!steadying.enabled) {
				BeginDisabled();
			}
			PercentSliderColumn("##BowSteadyingMaxReduction", steadying.maxSpreadMultiplier, kBowSteadyingMaxReductionTooltip);
			SliderColumn("##BowSteadyingHalfDuration", steadying.halfDuration, kBowSteadyingHalfDurationTooltip, 0.1f, 5.0f, "%.1fs");
			if (!steadying.enabled) {
				EndDisabled();
			}
		}

		void RenderBowFatigueRows(Options::Config::DrawFatigue& fatigue) {
			TableNextRow();
			TableSetColumnIndex(1);
			CenteredHeaderCell(kBowFatigueMinThresholdHeader, false);
			TableSetColumnIndex(2);
			CenteredHeaderCell(kBowFatigueMaxThresholdHeader, false);

			TableNextRow();
			ToggleColumn(kBowFatigueLabel, fatigue.enabled, kBowFatigueTooltip);
			if (!fatigue.enabled) {
				BeginDisabled();
			}
			SliderColumn("##BowFatigueMinThreshold", fatigue.minThreshold, kBowFatigueMinThresholdTooltip, 0.0f, 10.0f, "%.1fs");
			SliderColumn("##BowFatigueMaxThreshold", fatigue.maxThreshold, kBowFatigueMaxThresholdTooltip, 0.0f, 10.0f, "%.1fs");

			TableNextRow();
			TableSetColumnIndex(1);
			CenteredHeaderCell(kBowMaxFatiguePenaltyHeader, false);
			TableSetColumnIndex(2);
			CenteredHeaderCell(kBowFatigueHalfDurationHeader, false);

			TableNextRow();
			TableNextColumn();
			PercentSliderColumn("##BowFatigueMaxPenalty", fatigue.maxSpreadMultiplier, kBowMaxFatiguePenaltyTooltip);
			SliderColumn("##BowFatigueHalfDuration", fatigue.halfDuration, kBowFatigueHalfDurationTooltip, 0.1f, 5.0f, "%.1fs");
			if (!fatigue.enabled) {
				EndDisabled();
			}
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

			RenderChargeSpeedRows(cfg.bowDrawSpeed, kScaleDrawSpeedWithSkillLabel, kScaleBowDrawSpeedWithSkillTooltip, "##BowMinDrawSpeedMultiplier", "##BowMaxDrawSpeedMultiplier");

			RenderBowSteadyingRows(cfg.bowSteadying);

			RenderBowFatigueRows(cfg.bowFatigue);

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
				cfg.bowDrawSpeed = defaults.bowDrawSpeed;
				cfg.bowSteadying = defaults.bowSteadying;
				cfg.bowFatigue = defaults.bowFatigue;
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

			RenderChargeSpeedRows(cfg.crossbowDrawSpeed, kScaleDrawSpeedWithSkillLabel, kScaleCrossbowDrawSpeedWithSkillTooltip, "##CrossbowMinDrawSpeedMultiplier", "##CrossbowMaxDrawSpeedMultiplier");

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
				cfg.crossbowDrawSpeed = defaults.crossbowDrawSpeed;
				Options::Save();
			}
			EmptyRow();
			PopID();
		}

		void RenderSpellOverchargeRows(Options::Config::Overcharge& overcharge, const OverchargeTooltips& tooltips) {
			TableNextRow();
			TableSetColumnIndex(1);
			CenteredHeaderCell(kSpellOverchargeMaxReductionHeader, false);
			TableSetColumnIndex(2);
			CenteredHeaderCell(kSpellOverchargeHalfDurationHeader, false);

			TableNextRow();
			ToggleColumn(kSpellOverchargeLabel, overcharge.enabled, tooltips.toggle);
			if (!overcharge.enabled) {
				BeginDisabled();
			}
			PercentSliderColumn("##OverchargeMaxReduction", overcharge.maxSpreadMultiplier, tooltips.maxReduction);
			SliderColumn("##OverchargeHalfDuration", overcharge.halfDuration, tooltips.halfDuration, 0.1f, 5.0f, "%.1fs");
			if (!overcharge.enabled) {
				EndDisabled();
			}
		}

		void RenderCastFatigueRows(Options::Config::CastFatigue& fatigue, const char* toggleTooltip) {
			TableNextRow();
			TableSetColumnIndex(1);
			CenteredHeaderCell(kFatigueMinThresholdHeader, false);
			TableSetColumnIndex(2);
			CenteredHeaderCell(kFatigueMaxThresholdHeader, false);

			TableNextRow();
			ToggleColumn(kSpellFatigueLabel, fatigue.enabled, toggleTooltip);
			if (!fatigue.enabled) {
				BeginDisabled();
			}
			SliderColumn("##FatigueMinThreshold", fatigue.minThreshold, kFatigueMinThresholdTooltip, 0.0f, 10.0f, "%.1fs");
			SliderColumn("##FatigueMaxThreshold", fatigue.maxThreshold, kFatigueMaxThresholdTooltip, 0.0f, 10.0f, "%.1fs");

			TableNextRow();
			TableSetColumnIndex(1);
			CenteredHeaderCell(kMaxFatiguePenaltyHeader, false);
			TableSetColumnIndex(2);
			CenteredHeaderCell(kFatigueHalfDurationHeader, false);

			TableNextRow();
			TableNextColumn();
			PercentSliderColumn("##FatigueMaxPenalty", fatigue.maxSpreadMultiplier, kMaxFatiguePenaltyTooltip);
			SliderColumn("##FatigueHalfDuration", fatigue.halfDuration, kFatigueHalfDurationTooltip, 0.1f, 5.0f, "%.1fs");
			if (!fatigue.enabled) {
				EndDisabled();
			}
		}

		struct StaffOrScrollCategory
		{
			const char* pushID;

			const char* label;
			const char* tooltip;
			bool Options::Config::* aiming;

			const char*                            accuracySliderID;
			const char*                            accuracyTooltip;
			const char*                            precisionSliderID;
			const char*                            precisionTooltip;
			float Options::Config::SkillMultiplier::* multiplier;

			const char* useEnchantingSkillTooltip;
			bool Options::Config::* useEnchantingSkill;

			const char* useHighestMagicSkillTooltip;
			bool Options::Config::* useHighestMagicSkill;

			const char* chargeSpeedTooltip;
			const char* minChargeSpeedID;
			const char* maxChargeSpeedID;
			Options::Config::SpeedOptions Options::Config::* chargeSpeed;

			Options::Config::Overcharge Options::Config::* overcharge;
			const OverchargeTooltips*                           overchargeTooltips;

			Options::Config::CastFatigue Options::Config::* fatigue;
			const char*                                         fatigueToggleTooltip;

			const char* resetButtonLabel;
		};

		constexpr StaffOrScrollCategory kStaffCategory{
			"Staves",
			kStaffLabel, kStaffTooltip, &Options::Config::staffAiming,
			"##StaffAccuracy", kStaffAccuracyTooltip, "##StaffPrecision", kStaffPrecisionTooltip, &Options::Config::SkillMultiplier::staff,
			kUseEnchantingSkillTooltip, &Options::Config::stavesUseEnchantingSkill,
			kStaffUseHighestMagicSkillTooltip, &Options::Config::stavesUseHighestMagicSkill,
			kScaleStaffChargeTimeWithSkillTooltip, "##StaffMinChargeSpeedMultiplier", "##StaffMaxChargeSpeedMultiplier", &Options::Config::staffChargeSpeed,
			&Options::Config::staffOvercharge, &kStaffOverchargeTooltips,
			&Options::Config::staffFatigue, kStaffFatigueTooltip,
			kResetStavesButton
		};

		constexpr StaffOrScrollCategory kScrollCategory{
			"Scrolls",
			kScrollLabel, kScrollTooltip, &Options::Config::scrollAiming,
			"##ScrollAccuracy", kScrollAccuracyTooltip, "##ScrollPrecision", kScrollPrecisionTooltip, &Options::Config::SkillMultiplier::scroll,
			kScrollUseEnchantingSkillTooltip, &Options::Config::scrollsUseEnchantingSkill,
			kScrollUseHighestMagicSkillTooltip, &Options::Config::scrollsUseHighestMagicSkill,
			kScaleScrollChargeTimeWithSkillTooltip, "##ScrollMinChargeSpeedMultiplier", "##ScrollMaxChargeSpeedMultiplier", &Options::Config::scrollChargeSpeed,
			&Options::Config::scrollOvercharge, &kScrollOverchargeTooltips,
			&Options::Config::scrollFatigue, kScrollFatigueTooltip,
			kResetScrollsButton
		};

		void RenderStaffOrScrollRows(const StaffOrScrollCategory& category, Options::Config& cfg, const Options::Config& defaults, bool hasAccuracy) {
			PushID(category.pushID);
			TableNextRow();
			ToggleColumn(category.label, cfg.*category.aiming, category.tooltip);
			if (!(cfg.*category.aiming)) {
				BeginDisabled();
			}
			if (hasAccuracy) {
				SliderColumn(category.accuracySliderID, cfg.accuracyMultipliers.*category.multiplier, category.accuracyTooltip);
			} else {
				NotApplicableColumn();
			}
			SliderColumn(category.precisionSliderID, cfg.precisionMultipliers.*category.multiplier, category.precisionTooltip);

			TableNextRow();
			ExclusiveToggleColumn(kUseEnchantingSkillLabel, cfg.*category.useEnchantingSkill, cfg.*category.useHighestMagicSkill, category.useEnchantingSkillTooltip);

			TableNextRow();
			ExclusiveToggleColumn(kUseHighestMagicSkillLabel, cfg.*category.useHighestMagicSkill, cfg.*category.useEnchantingSkill, category.useHighestMagicSkillTooltip);
			if (!(cfg.*category.aiming)) {
				EndDisabled();
			}

			RenderChargeSpeedRows(cfg.*category.chargeSpeed, kScaleChargeTimeWithSkillLabel, category.chargeSpeedTooltip, category.minChargeSpeedID, category.maxChargeSpeedID);

			RenderSpellOverchargeRows(cfg.*category.overcharge, *category.overchargeTooltips);

			RenderCastFatigueRows(cfg.*category.fatigue, category.fatigueToggleTooltip);

			TableNextRow();
			TableNextColumn();
			if (Button(category.resetButtonLabel)) {
				cfg.*category.aiming = defaults.*category.aiming;
				if (hasAccuracy) {
					cfg.accuracyMultipliers.*category.multiplier = defaults.accuracyMultipliers.*category.multiplier;
				}
				cfg.precisionMultipliers.*category.multiplier = defaults.precisionMultipliers.*category.multiplier;
				cfg.*category.useEnchantingSkill = defaults.*category.useEnchantingSkill;
				cfg.*category.useHighestMagicSkill = defaults.*category.useHighestMagicSkill;
				cfg.*category.chargeSpeed = defaults.*category.chargeSpeed;
				cfg.*category.overcharge = defaults.*category.overcharge;
				cfg.*category.fatigue = defaults.*category.fatigue;
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

			RenderChargeSpeedRows(cfg.spellChargeSpeed, kScaleChargeTimeWithSkillLabel, kScaleSpellChargeTimeWithSkillTooltip, "##SpellMinChargeSpeedMultiplier", "##SpellMaxChargeSpeedMultiplier");

			RenderSpellOverchargeRows(cfg.spellOvercharge, kSpellOverchargeTooltips);

			RenderCastFatigueRows(cfg.spellFatigue, kSpellFatigueTooltip);

			TableNextRow();
			TableNextColumn();
			if (Button(kResetSpellsButton)) {
				cfg.spellAiming = defaults.spellAiming;
				if (hasAccuracy) {
					cfg.accuracyMultipliers.spell = defaults.accuracyMultipliers.spell;
				}
				cfg.precisionMultipliers.spell = defaults.precisionMultipliers.spell;
				cfg.spellsUseHighestMagicSkill = defaults.spellsUseHighestMagicSkill;
				cfg.spellChargeSpeed = defaults.spellChargeSpeed;
				cfg.spellOvercharge = defaults.spellOvercharge;
				cfg.spellFatigue = defaults.spellFatigue;
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

			RenderChargeSpeedRows(cfg.concentrationSpellChargeSpeed, kScaleChargeTimeWithSkillLabel, kScaleConcentrationSpellChargeTimeWithSkillTooltip, "##ConcentrationSpellMinChargeSpeedMultiplier", "##ConcentrationSpellMaxChargeSpeedMultiplier");

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
				cfg.concentrationSpellChargeSpeed = defaults.concentrationSpellChargeSpeed;
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
				RenderStaffOrScrollRows(kStaffCategory, Options::NPC, Options::NPCConfig::defaultConfig, hasAccuracy);
				RenderSpellRows(Options::NPC, Options::NPCConfig::defaultConfig, hasAccuracy);
				RenderConcentrationSpellRows(Options::NPC, Options::NPCConfig::defaultConfig, hasAccuracy);
				RenderStaffOrScrollRows(kScrollCategory, Options::NPC, Options::NPCConfig::defaultConfig, hasAccuracy);
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
				RenderStaffOrScrollRows(kStaffCategory, Options::Player, Options::PlayerConfig::defaultConfig, hasAccuracy);
				RenderSpellRows(Options::Player, Options::PlayerConfig::defaultConfig, hasAccuracy);
				RenderConcentrationSpellRows(Options::Player, Options::PlayerConfig::defaultConfig, hasAccuracy);
				RenderStaffOrScrollRows(kScrollCategory, Options::Player, Options::PlayerConfig::defaultConfig, hasAccuracy);
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
