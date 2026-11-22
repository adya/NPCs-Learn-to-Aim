#include "AimDebug.h"
#include "Menu+Internal.h"
#include <SKSEMCP/SKSEMenuFramework.hpp>
#include <fmt/ranges.h>

using namespace ImGuiMCP;

namespace NLA::Menu
{
	namespace
	{
		using KeywordBuffer = char[512];

		void RenderSkillShotParts(Config& cfg) {
			PushID("SkillShotParts");
			SeparatorText("Skill Shots");
			TextWrapped("Body parts skilled NPCs may aim for on a skill shot. Turn off any that don't matter in your game.");
			Spacing();

			bool changed = false;
			changed |= Checkbox("Headshots", &cfg.headshots);
			ShowTooltip("Skill shots may go for the head.");
			changed |= Checkbox("Arm Shots", &cfg.armshots);
			ShowTooltip("Skill shots may go for a hand. Only used on targets that have hands.");
			changed |= Checkbox("Leg Shots", &cfg.legshots);
			ShowTooltip("Skill shots may go for a leg. Only used on targets that have legs.");
			if (changed) {
				Options::Save();
			}
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
			ShowTooltip("How much detail goes in the log file. Use 'debug' when troubleshooting.");
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
			TextWrapped("Only NPCs that match these keyword rules learn to aim. Separate keywords with commas.");
			Spacing();

			if (IsWindowAppearing() || buffersGeneration != Options::currentID) {
				syncBuffers();
			}

			static const std::string includeHint = fmt::format("{}", fmt::join(Options::NPCConfig::defaultConfig.includeKeywords, ","));

			KeywordsInput("Include Keywords", "##IncludeKeywords", includeHint.c_str(), includeBuffer, npc.includeKeywords, "NPCs need at least one of these keywords (on the NPC or its race). Leave empty to allow everyone.");
			Spacing();
			KeywordsInput("Exclude Keywords", "##ExcludeKeywords", "", excludeBuffer, npc.excludeKeywords, "NPCs with any of these keywords never learn to aim.");

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
		RenderSkillShotParts(ctx.cfg);
		Spacing();
		RenderKeywordFilters();
	}

	void RenderGeneralSectionPage() {
		SeparatorText("Actions");
		if (Button("Reload Settings", ImVec2(250, 0))) {
			Options::Load();
			logger::info("Settings reloaded via menu");
		}
		ShowTooltip("Load settings from the INI file again, dropping unsaved changes.");

		if (Button("Export Settings", ImVec2(250, 0))) {
			Options::Export();
			logger::info("Settings exported via menu");
		}
		ShowTooltip("Save all current settings to the INI file, including ones left at default.");

		if (Button("Reset All to Defaults", ImVec2(250, 0))) {
			Options::ResetNPC();
			Options::ResetPlayer();
			logger::info("All settings reset to defaults via menu");
		}
		ShowTooltip("Reset all NPC and Player settings to defaults.");

		Spacing();
		RenderLogLevel();
	}
}
