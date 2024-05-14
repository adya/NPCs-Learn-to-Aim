#include "Options.h"
#include "CLIBUtil/string.hpp"

namespace NLA::Options
{
	void Load() {
		logger::info("{:*^30}", "OPTIONS");
		std::filesystem::path options = R"(Data\SKSE\Plugins\NPCsLearnToAim.ini)";
		CSimpleIniA           ini{};
		ini.SetUnicode();
		if (ini.LoadFile(options.string().c_str()) >= 0) {
			General::bowAiming = ini.GetBoolValue("General", "bEnableBowAim", General::bowAiming);
			General::crossbowAiming = ini.GetBoolValue("General", "bEnableCrossbowAim", General::crossbowAiming);
			General::spellAiming = ini.GetBoolValue("General", "bEnableSpellAim", General::spellAiming);
			General::staffAiming = ini.GetBoolValue("General", "bEnableStaffAim", General::staffAiming);

			NPC::excludeKeywords = clib_util::string::split(std::string(ini.GetValue("NPC", "sExcludeKeywords", "")), ",");
			NPC::includeKeywords = clib_util::string::split(std::string(ini.GetValue("NPC", "sIncludeKeywords", "ActorTypeNPC")), ",");
			
		} else {
			logger::info(R"(Data\SKSE\Plugins\NPCsLearnToAim.ini not found. Default options will be used.)");
			logger::info("");
		}

		logger::info("General:");
		logger::info("\tArchers {} learn to aim", General::bowAiming ? "will" : "won't");
		logger::info("\tCrossbowmen {} learn to aim", General::crossbowAiming ? "will" : "won't");
		logger::info("\tMages {} learn to aim spells", General::spellAiming ? "will" : "won't");
		logger::info("\tMages {} learn to aim staffs", General::staffAiming ? "will" : "won't");

		logger::info("");
		logger::info("NPC:");
		if (NPC::excludeKeywords.empty() && NPC::includeKeywords.empty()) {
			logger::info("\tAll NPCs will learn to aim");
		} else {
			logger::info("\tOnly NPCs that have at least one of these keywords: [{}] and none of these: [{}] will learn to aim", fmt::join(NPC::includeKeywords, ", "), fmt::join(NPC::excludeKeywords, ", "));
		}
	}

	bool NPC::ShouldLearn(RE::Actor* a_actor) {
		if (!a_actor->HasAnyKeywordByEditorID(includeKeywords) && (!a_actor->GetRace() || !a_actor->GetRace()->HasAnyKeywordByEditorID(includeKeywords))) {
			return false;
		}

		return !a_actor->HasAnyKeywordByEditorID(excludeKeywords) && (!a_actor->GetRace() || !a_actor->GetRace()->HasAnyKeywordByEditorID(excludeKeywords));
	}
}
