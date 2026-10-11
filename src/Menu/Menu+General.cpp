#include "Menu+Internal.h"
#include "CLIBUtil/string.hpp"
#include <fmt/ranges.h>
#include <SKSEMCP/SKSEMenuFramework.hpp>

using namespace ImGuiMCP;

namespace NLA::Menu
{
	namespace
	{
		using KeywordBuffer = char[512];

		void RenderBlindnessRows(Config& cfg, const Config& defaults) {
			PushID("Blindness");
			TableNextRow();
			ToggleColumn("Blindness Affects Aim", cfg.blindnessAffectsAiming, "When enabled, being blinded further affects aiming.");
			{
				DisabledIf off{ !cfg.blindnessAffectsAiming };
				SliderColumn("##Accuracy", cfg.accuracyMultipliers.blindness, "How much the blindness affects general accuracy. The higher this multiplier is the easier it is to aim.");
				SliderColumn("##Precision", cfg.precisionMultipliers.blindness, "How much the blindness affects general precision. The higher this multiplier is the easier it is to hit the target.");
			}

			if (ResetButton("Reset Blindness to Default")) {
				cfg.blindnessAffectsAiming = defaults.blindnessAffectsAiming;
				cfg.accuracyMultipliers.blindness = defaults.accuracyMultipliers.blindness;
				cfg.precisionMultipliers.blindness = defaults.precisionMultipliers.blindness;
				Options::Save();
			}
			EmptyRow();
			PopID();
		}

		void FillBuffer(KeywordBuffer& buffer, const std::vector<std::string>& keywords) {
			strcpy_s(buffer, fmt::format("{}", fmt::join(keywords, ",")).c_str());
		}

		void KeywordsInput(const char* label, const char* id, const char* hint, KeywordBuffer& buffer, std::vector<std::string>& keywords, const char* tooltip) {
			Text("%s", label);
			if (InputTextWithHint(id, hint, buffer, sizeof(buffer))) {
				keywords = clib_util::string::split(std::string(buffer), ",");
				Options::Save();
			}
			ShowTooltip(tooltip);
		}

		void RenderKeywordFilters() {
			static KeywordBuffer includeBuffer = "";
			static KeywordBuffer excludeBuffer = "";
			static unsigned int  buffersGeneration = 0;

			auto& npc = Options::NPC;
			auto  syncBuffers = [&] {
				FillBuffer(includeBuffer, npc.includeKeywords);
				FillBuffer(excludeBuffer, npc.excludeKeywords);
				buffersGeneration = Options::currentID;
			};

			PushID("Filters");
			SeparatorText("Filters");
			TextWrapped("Only NPCs matching these keyword rules will learn to aim. Comma-separated keyword EditorIDs.");
			Spacing();

			if (IsWindowAppearing() || buffersGeneration != Options::currentID) {
				syncBuffers();
			}

			KeywordsInput("Include Keywords", "##IncludeKeywords", "ActorTypeNPC", includeBuffer, npc.includeKeywords,
				"Only NPCs with at least one of these keywords (on the NPC or its race) will learn to aim. Leave empty to skip this filter.");
			Spacing();
			KeywordsInput("Exclude Keywords", "##ExcludeKeywords", "", excludeBuffer, npc.excludeKeywords,
				"NPCs with any of these keywords (on the NPC or its race) will never learn to aim.");

			Spacing();
			if (Button("Reset Filters to Default")) {
				npc.includeKeywords = Options::NPCConfig::defaultConfig.includeKeywords;
				npc.excludeKeywords = Options::NPCConfig::defaultConfig.excludeKeywords;
				syncBuffers();
				Options::Save();
			}
			PopID();
		}
	}

	void RenderGeneralPage(const PageContext& ctx) {
		if (!ctx.isNPC) {
			return;
		}
		RenderTable("Table", true, [&] {
			RenderBlindnessRows(ctx.cfg, ctx.defaults);
		});
		Spacing();
		RenderKeywordFilters();
	}

	void RenderActionsPage() {
		if (Button("Reload Settings", ImVec2(250, 0))) {
			Options::Load();
			logger::info("Settings reloaded via menu");
		}
		ShowTooltip("Reload all settings from the INI file, discarding any unsaved in-memory changes.");

		if (Button("Export Settings", ImVec2(250, 0))) {
			Options::Export();
			logger::info("Settings exported via menu");
		}
		ShowTooltip("Write all current settings, including the ones equal to their defaults, to the INI file.");

		if (Button("Reset All to Defaults", ImVec2(250, 0))) {
			Options::ResetNPC();
			Options::ResetPlayer();
			logger::info("All settings reset to defaults via menu");
		}
		ShowTooltip("Reset all NPC and Player settings to their defaults.");
	}
}
