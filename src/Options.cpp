#include "Options.h"
#include "CLIBUtil/string.hpp"

namespace NLA::Options
{
	inline void LogSkillMultiplier(std::string_view actor, std::string_view weaponName, float mult) {
		if (mult > 1) {
			auto percent = (mult - 1) * 100;
			logger::info("\t{} are {:.2f}% easier to use for {}", weaponName, percent, actor);
		} else if (mult < 1) {
			auto percent = (1 - mult) * 100;
			logger::info("\t{} are {:.2f}% harder to use for {}", weaponName, percent, actor);
		} else {
			logger::info("\t{} are at standard difficuly for {}", weaponName, actor);
		}
	}

	inline void LogCrossbowsShootStraight(std::string_view actor, bool crossbowsShootStraight) {
		if (crossbowsShootStraight) {
			logger::info("\tCrossbows help {} shoot straight", actor);
		}
		else {
			logger::info("\tCrossbows require {} to know how to use it", actor);
		}
	}

	inline void LogStavesUseEnchantingSkill(std::string_view actor, bool stavesUseEnchantingSkill) {
		if (stavesUseEnchantingSkill) {
			logger::info("\t{}'s accuracy with staves is based on Enchanting skill", actor);
		}
		else {
			logger::info("\t{}'s accuracy with staves is based on {}'s skill in spell's magic school", actor);
		}
	}

	inline void LogSpellsUseHighestMagicSkill(std::string_view actor, bool spellsUseHighestMagicSkill, bool stavesUseEnchantingSkill) {
		if (spellsUseHighestMagicSkill) {
			logger::info("\t{}'s accuracy with spells is based on the highest {}'s magic skill regardless of spell", actor);
		}
		else {
			logger::info("\t{}'s accuracy with spells is based on {}'s skill in spell's magic school", actor);
		}
	}

	inline void LogConcentrationSpellsRequireContinuousAim(std::string_view actor, bool concetrationSpellsRequireContinuousAim) {
		if (concetrationSpellsRequireContinuousAim) {
			logger::info("\t{} must aim continuously to maintain concentration spells", actor);
		}
		else {
			logger::info("\t{} will always hit with concentration spells", actor);
		}
	}

	void Load() {
		logger::info("{:*^40}", "OPTIONS");
		std::filesystem::path options = R"(Data\SKSE\Plugins\NPCsLearnToAim.ini)";
		CSimpleIniA           ini{};
		ini.SetUnicode();
		if (ini.LoadFile(options.string().c_str()) >= 0) {
			NPC = NPCConfig(ini);
			Player = PlayerConfig(ini);
		} else {
			logger::info(R"(Data\SKSE\Plugins\NPCsLearnToAim.ini not found. Default options will be used.)");
			logger::info("");
		}

		logger::info("NPC:");
		logger::info("\tArchers {}", NPC.bowAiming ? "will learn to aim" : "have perfect aim");
		logger::info("\tCrossbowmen {}", NPC.crossbowAiming ? "will learn to aim" : "have perfect aim");
		logger::info("\tMages {} with spells", NPC.spellAiming ? "will learn to aim" : "have perfect aim");
		logger::info("\tMages {} with staves", NPC.staffAiming ? "will learn to aim" : "have perfect aim");

		logger::info("");
		LogCrossbowsShootStraight("NPCs", NPC.crossbowsAlwaysShootStraight);
		LogStavesUseEnchantingSkill("NPCs", NPC.stavesUseEnchantingSkill);
		LogSpellsUseHighestMagicSkill("NPCs", NPC.spellsUseHighestMagicSkill, NPC.stavesUseEnchantingSkill);
		LogConcentrationSpellsRequireContinuousAim("NPCs", NPC.concetrationSpellsRequireContinuousAim);

		logger::info("");
		LogSkillMultiplier("NPCs", "Bows", NPC.bowSkillMultiplier);
		LogSkillMultiplier("NPCs", "Crossbows", NPC.crossbowSkillMultiplier);
		LogSkillMultiplier("NPCs", "Spells", NPC.spellSkillMultiplier);
		LogSkillMultiplier("NPCs", "Staves", NPC.staffSkillMultiplier);

		logger::info("");
		if (NPC.excludeKeywords.empty() && NPC.includeKeywords.empty()) {
			logger::info("\tAll NPCs will learn to aim");
		} else {
			bool include = !NPC.includeKeywords.empty();
			bool exclude = !NPC.excludeKeywords.empty();
			bool both = include && exclude;

			if (both) {
				logger::info("\tOnly NPCs that have at least one of these keywords: [{}] and none of these: [{}] will learn to aim", fmt::join(NPC.includeKeywords, ", "), fmt::join(NPC.excludeKeywords, ", "));
			} else if (include) {
				logger::info("\tOnly NPCs that have at least one of these keywords: [{}] will learn to aim", fmt::join(NPC.includeKeywords, ", "));
			} else {
				logger::info("\tOnly NPCs that have none of these keywords: [{}] will learn to aim", fmt::join(NPC.excludeKeywords, ", "));
			}
		}

		logger::info("");
		logger::info("Player:");
		logger::info("\t{} with bows", Player.bowAiming ? "Will learn to aim" : "Has perfect aim");
		logger::info("\t{} with crossbows", Player.crossbowAiming ? "Will learn to aim" : "Has perfect aim");
		logger::info("\t{} with spells", Player.spellAiming ? "Will learn to aim" : "Has perfect aim");
		logger::info("\t{} with staves", Player.staffAiming ? "Will learn to aim" : "Has perfect aim");
		
		logger::info("");
		LogCrossbowsShootStraight("Player", Player.crossbowsAlwaysShootStraight);
		LogStavesUseEnchantingSkill("Player", Player.stavesUseEnchantingSkill);
		LogSpellsUseHighestMagicSkill("Player", Player.spellsUseHighestMagicSkill, Player.stavesUseEnchantingSkill);

		logger::info("");
		LogSkillMultiplier("Player", "Bows", Player.bowSkillMultiplier);
		LogSkillMultiplier("Player", "Crossbows", Player.crossbowSkillMultiplier);
		LogSkillMultiplier("Player", "Spells", Player.spellSkillMultiplier);
		LogSkillMultiplier("Player", "Staves", Player.staffSkillMultiplier);
		

		logger::info("{:*^40}", "");
	}

	bool NPCConfig::ShouldLearn(RE::Actor* a_actor) {
		if (!a_actor || a_actor->IsPlayerRef()) {
			return true; // ShouldLearn only concerns NPCs, so we ignore players.
		}
		if (!a_actor->HasAnyKeywordByEditorID(includeKeywords) && (!a_actor->GetRace() || !a_actor->GetRace()->HasAnyKeywordByEditorID(includeKeywords))) {
			return false;
		}

		return !a_actor->HasAnyKeywordByEditorID(excludeKeywords) && (!a_actor->GetRace() || !a_actor->GetRace()->HasAnyKeywordByEditorID(excludeKeywords));
	}

	Config::Config(CSimpleIniA& ini, const char* a_section) {
		spellAiming = ini.GetBoolValue(a_section, "bEnableSpellAiming", spellAiming);
		staffAiming = ini.GetBoolValue(a_section, "bEnableStaffAiming", staffAiming);
		bowAiming = ini.GetBoolValue(a_section, "bEnableBowAiming", bowAiming);
		crossbowAiming = ini.GetBoolValue(a_section, "bEnableCrossbowAiming", crossbowAiming);

		bowSkillMultiplier = ini.GetDoubleValue(a_section, "fBowSkillMultiplier", bowSkillMultiplier);
		crossbowSkillMultiplier = ini.GetDoubleValue(a_section, "fCrossbowSkillMultiplier", crossbowSkillMultiplier);
		spellSkillMultiplier = ini.GetDoubleValue(a_section, "fSpellSkillMultiplier", spellSkillMultiplier);
		staffSkillMultiplier = ini.GetDoubleValue(a_section, "fStaffSkillMultiplier", staffSkillMultiplier);

		crossbowsAlwaysShootStraight = ini.GetBoolValue(a_section, "bCrossbowsAlwaysShootStraight", crossbowsAlwaysShootStraight);

		stavesUseEnchantingSkill = ini.GetBoolValue(a_section, "bStavesUseEnchantingSkill", stavesUseEnchantingSkill);

		spellsUseHighestMagicSkill = ini.GetBoolValue(a_section, "bSpellsUseHighestMagicSkill", spellsUseHighestMagicSkill);
	}

	PlayerConfig::PlayerConfig(CSimpleIniA& ini) :
		Config(ini, "Player") {
		concetrationSpellsRequireContinuousAim = false;
	}

	NPCConfig::NPCConfig(CSimpleIniA& ini) :
		Config(ini, "NPC") {

		concetrationSpellsRequireContinuousAim = ini.GetBoolValue("NPC", "bConcentrationSpellsRequireContinuousAim", concetrationSpellsRequireContinuousAim);

		excludeKeywords = clib_util::string::split(std::string(ini.GetValue("NPC", "sExcludeKeywords", "")), ",");
		includeKeywords = clib_util::string::split(std::string(ini.GetValue("NPC", "sIncludeKeywords", "ActorTypeNPC")), ",");
	}
}
