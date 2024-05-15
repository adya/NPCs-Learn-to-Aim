#include "Options.h"
#include "CLIBUtil/string.hpp"

namespace NLA::Options
{
#ifndef NDEBUG
	void Load(bool silent) {
#else
#define silent false
	void Load() {
#endif
		if (!silent) {
			logger::info("{:*^40}", "OPTIONS");
		}
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
			
			Skills::crossbowSkillMultiplier = ini.GetDoubleValue("Skills", "fCrossbowSkillMultiplier", Skills::crossbowSkillMultiplier);
			Skills::staffSkillMultiplier = ini.GetDoubleValue("Skills", "fStaffSkillMultiplier", Skills::staffSkillMultiplier);
			
			Skills::staffsUseEnchantingSkill = ini.GetBoolValue("Skills", "bStaffsUseEnchantingSkill", Skills::staffsUseEnchantingSkill);

		} else if (!silent) {
			logger::info(R"(Data\SKSE\Plugins\NPCsLearnToAim.ini not found. Default options will be used.)");
			logger::info("");
		}

		if (silent) {
			return;
		}

		logger::info("General:");
		logger::info("\tArchers {} learn to aim", General::bowAiming ? "will" : "won't");
		logger::info("\tCrossbowmen {} learn to aim", General::crossbowAiming ? "will" : "won't");
		logger::info("\tMages {} learn to aim with spells", General::spellAiming ? "will" : "won't");
		logger::info("\tMages {} learn to aim with staffs", General::staffAiming ? "will" : "won't");

		logger::info("Skills:");
		if (Skills::crossbowSkillMultiplier > 1) {
			logger::info("\tCrossbows are {:.2f}% easier to use than bows", (Skills::crossbowSkillMultiplier - 1) * 100);
		}
		else if (Skills::crossbowSkillMultiplier < 1) {
			logger::info("\tCrossbows are {:.2f}% harder to use than bows", (1 - Skills::crossbowSkillMultiplier) * 100);
		} else {
			logger::info("\tCrossbows are the same as bows");
		}

		if (Skills::staffSkillMultiplier > 1) {
			logger::info("\tStaffs are {:.2f}% easier to use than spells", (Skills::staffSkillMultiplier - 1) * 100);
		}
		else if (Skills::staffSkillMultiplier < 1) {
			logger::info("\tStaffs are {:.2f}% harder to use than spells", (1 - Skills::staffSkillMultiplier) * 100);
		}
		else {
			logger::info("\tStaffs are the same as spells");
		}

		logger::info("");
		logger::info("NPC:");
		if (NPC::excludeKeywords.empty() && NPC::includeKeywords.empty()) {
			logger::info("\tAll NPCs will learn to aim");
		} else {
			bool include = !NPC::includeKeywords.empty();
			bool exclude = !NPC::excludeKeywords.empty();
			bool both = include && exclude;

			if (both) {
				logger::info("\tOnly NPCs that have at least one of these keywords: [{}] and none of these: [{}] will learn to aim", fmt::join(NPC::includeKeywords, ", "), fmt::join(NPC::excludeKeywords, ", "));
			}
			else if (include) {
				logger::info("\tOnly NPCs that have at least one of these keywords: [{}] will learn to aim", fmt::join(NPC::includeKeywords, ", "));
			}
			else {
				logger::info("\tOnly NPCs that have none of these keywords: [{}] will learn to aim", fmt::join(NPC::excludeKeywords, ", "));
			}
		}
		logger::info("{:*^40}", "");
	}

	bool NPC::ShouldLearn(RE::Actor* a_actor) {
		if (!a_actor->HasAnyKeywordByEditorID(includeKeywords) && (!a_actor->GetRace() || !a_actor->GetRace()->HasAnyKeywordByEditorID(includeKeywords))) {
			return false;
		}

		return !a_actor->HasAnyKeywordByEditorID(excludeKeywords) && (!a_actor->GetRace() || !a_actor->GetRace()->HasAnyKeywordByEditorID(excludeKeywords));
	}
}
