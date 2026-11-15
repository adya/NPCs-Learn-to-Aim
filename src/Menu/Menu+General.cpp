#include "Menu+Internal.h"
#include <SKSEMCP/SKSEMenuFramework.hpp>
#include <fmt/ranges.h>

using namespace ImGuiMCP;

namespace NLA::Menu
{
	namespace
	{
		using KeywordBuffer = char[512];

		void RenderBlindnessRows(Config& cfg, const Config& defaults, bool isNPC) {
			PushID("Blindness");
			// The Player aims by hand, so aim blindness only applies to NPCs.
			if (isNPC) {
				HeaderRow("Aim", "Accuracy", "Precision");
			} else {
				HeaderRow("Accuracy", "Precision");
			}
			TableNextRow();
			ToggleColumn("Blindness Affects Aim", cfg.blindnessAffectsAiming, "When enabled, being blinded further affects aiming.");
			{
				DisabledIf off{ !cfg.blindnessAffectsAiming };
				if (isNPC) {
					SliderColumn("##Aim", cfg.aimBlindness, "How much the blindness affects the NPC's aim. The higher this multiplier is the easier it is to aim.", Options::Limits::kBlindnessMultiplier);
				}
				SliderColumn("##Accuracy", cfg.accuracyBlindness, "How much the blindness affects accuracy. The higher this multiplier is the smaller the widest miss can be.", Options::Limits::kBlindnessMultiplier);
				SliderColumn("##Precision", cfg.precisionBlindness, "How much the blindness affects precision. The higher this multiplier is the more shots land close to the aim.", Options::Limits::kBlindnessMultiplier);
			}

			if (ResetButton("Reset Blindness to Default")) {
				cfg.blindnessAffectsAiming = defaults.blindnessAffectsAiming;
				cfg.aimBlindness = defaults.aimBlindness;
				cfg.accuracyBlindness = defaults.accuracyBlindness;
				cfg.precisionBlindness = defaults.precisionBlindness;
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
			InputTextWithHint(id, hint, buffer, sizeof(buffer));

			if (IsItemDeactivatedAfterEdit()) {
				keywords = Options::ParseKeywords(buffer);
				Options::Save();
			}
			ShowTooltip(tooltip);
		}

		void RenderLogLevel() {
			static constexpr std::array levels{ spdlog::level::trace, spdlog::level::debug, spdlog::level::info, spdlog::level::warn, spdlog::level::err, spdlog::level::critical };
			static constexpr std::array labels{ "trace", "debug", "info", "warn", "error", "critical" };

			const auto current = spdlog::default_logger()->level();
			int        index = static_cast<int>(std::ranges::find(levels, current) - levels.begin());
			index = std::min(index, static_cast<int>(levels.size()) - 1);

			SeparatorText("Logging");
			AlignTextToFramePadding();
			Text("Log Level");
			SameLine();
			SetNextItemWidth(180);
			if (Combo("##LogLevel", &index, labels.data(), static_cast<int>(labels.size()))) {
				const auto logger = spdlog::default_logger();
				logger->set_level(levels[index]);
				logger->flush_on(levels[index]);
				logger::info("Log level changed to '{}'", labels[index]);
				Options::Save();
			}
			ShowTooltip("Controls verbosity of the log file. Use 'debug' to also log which hooks are being installed.");
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

			static const std::string includeHint = fmt::format("{}", fmt::join(Options::NPCConfig::defaultConfig.includeKeywords, ","));

			KeywordsInput("Include Keywords", "##IncludeKeywords", includeHint.c_str(), includeBuffer, npc.includeKeywords, "Only NPCs with at least one of these keywords (on the NPC or its race) will learn to aim. Leave empty to skip this filter.");
			Spacing();
			KeywordsInput("Exclude Keywords", "##ExcludeKeywords", "", excludeBuffer, npc.excludeKeywords, "NPCs with any of these keywords (on the NPC or its race) will never learn to aim.");

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
		RenderTable("Table", [&] {
			RenderBlindnessRows(ctx.cfg, ctx.defaults, ctx.isNPC);
		});
		Spacing();
		RenderKeywordFilters();
		Spacing();
		RenderLogLevel();
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
