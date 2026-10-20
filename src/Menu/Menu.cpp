#include "Menu.h"
#include "Menu+Internal.h"
#include <SKSEMCP/SKSEMenuFramework.hpp>

using namespace ImGuiMCP;

namespace NLA::Menu
{
	namespace
	{
		PageContext ContextFor(bool isNPC) {
			if (isNPC) {
				return { Options::NPC, Options::NPCConfig::defaultConfig, true };
			}
			return { Options::Player, Options::PlayerConfig::defaultConfig, false };
		}

		/// Clicking the actor's own item shows its General page; "Actor/Page" paths nest under it.
		template <bool IsNPC>
		void AddActorItems(const char* actor) {
			SKSEMenuFramework::AddSectionItem(actor, ActorPage<IsNPC, RenderGeneralPage, true>);
			SKSEMenuFramework::AddSectionItem(fmt::format("{}/Bows", actor), ActorPage<IsNPC, RenderBowsPage>);
			SKSEMenuFramework::AddSectionItem(fmt::format("{}/Crossbows", actor), ActorPage<IsNPC, RenderCrossbowsPage>);
			SKSEMenuFramework::AddSectionItem(fmt::format("{}/Spells", actor), ActorPage<IsNPC, RenderSpellsPage>);
			SKSEMenuFramework::AddSectionItem(fmt::format("{}/Concentration Spells", actor), ActorPage<IsNPC, RenderConcentrationSpellsPage>);
			SKSEMenuFramework::AddSectionItem(fmt::format("{}/Staves", actor), ActorPage<IsNPC, RenderStavesPage>);
			SKSEMenuFramework::AddSectionItem(fmt::format("{}/Concentration Staves", actor), ActorPage<IsNPC, RenderConcentrationStavesPage>);
			SKSEMenuFramework::AddSectionItem(fmt::format("{}/Scrolls", actor), ActorPage<IsNPC, RenderScrollsPage>);
		}
	}

	void RenderActorPage(bool isNPC, bool masterSwitch, PageBody body) {
		const PageContext ctx = ContextFor(isNPC);
		auto&             enabled = ctx.cfg.enabled;

		PushID(isNPC ? "NPC" : "Player");
		if (masterSwitch) {
			if (Checkbox(isNPC ? "Enable for NPCs" : "Enable for Player", &enabled)) {
				Options::Save();
			}
			ShowTooltip(isNPC ?
			                "Master switch for NPCs. When disabled, NPCs have vanilla aiming and all settings below are ignored." :
			                "Master switch for the Player. When disabled, the Player has vanilla aiming and all settings below are ignored.");
			Spacing();
		} else if (!enabled) {
			TextDisabled("%s", isNPC ? "NPC settings are disabled. Enable them by clicking the NPC section." : "Player settings are disabled. Enable them by clicking the Player section.");
			Spacing();
		}
		{
			DisabledIf off{ !enabled };
			body(ctx);
		}

		if (masterSwitch) {
			Spacing();
			if (Button(isNPC ? "Reset NPC to Defaults" : "Reset Player to Defaults", ImVec2(250, 0))) {
				if (isNPC) {
					Options::ResetNPC();
				} else {
					Options::ResetPlayer();
				}
			}
			ShowTooltip(isNPC ?
			                "Reset all NPC settings, including filters and headshots, to their defaults." :
			                "Reset all Player settings to their defaults.");
		}
		PopID();
	}

	void Register() {
		if (!SKSEMenuFramework::IsInstalled()) {
			logger::warn("SKSEMenuFramework is not installed");
			return;
		}

		SKSEMenuFramework::SetSection("NPCs Learn to Aim");
		AddActorItems<true>("NPC");
		AddActorItems<false>("Player");
		SKSEMenuFramework::AddSectionItem("Actions", RenderActionsPage);

		logger::info("Added section to SKSEMenuFramework");
	}
}
