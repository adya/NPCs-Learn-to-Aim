#include "Options.h"
#include "CLIBUtil/string.hpp"

namespace NLA::Options
{
	inline void LogSkillMultiplier(std::string_view actor, std::string_view weaponName, std::string_view stage, float mult) {
		if (mult > 1) {
			auto percent = (mult - 1) * 100;
			logger::info("\t{} with {} is {:.2f}% easier to use for {}", stage, weaponName, percent, actor);
		} else if (mult < 1) {
			auto percent = (1 - mult) * 100;
			logger::info("\t{} with {} is {:.2f}% harder to use for {}", stage, weaponName, percent, actor);
		} else {
			logger::info("\t{} with {} is at standard difficuly for {}", stage, weaponName, actor);
		}
	}

	inline void LogBlindnessSkillMultiplier(std::string_view actor, std::string_view stage, float mult) {
		if (mult > 1) {
			auto percent = (mult - 1) * 100;
			logger::info("\tBlind {} have {:.2f}% easier {}", actor, percent, stage);
		} else if (mult < 1) {
			auto percent = (1 - mult) * 100;
			logger::info("\tBlind {} have {:.2f}% harder {}", actor, percent, stage);
		} else {
			logger::info("\tBlind {} have standard {}", actor, stage);
		}
	}

	inline void LogStavesUseEnchantingSkill(std::string_view actor, bool stavesUseEnchantingSkill) {
		if (stavesUseEnchantingSkill) {
			logger::info("\t{}'s accuracy with staves is based on Enchanting skill", actor);
		}
		else {
			logger::info("\t{}'s accuracy with staves is based on {}'s skill in spell's magic school", actor, actor);
		}
	}

	inline void LogSpellsUseHighestMagicSkill(std::string_view actor, bool spellsUseHighestMagicSkill, bool stavesUseEnchantingSkill) {
		if (spellsUseHighestMagicSkill) {
			logger::info("\t{}'s accuracy with spells is based on the highest {}'s magic skill regardless of spell", actor, actor);
		}
		else {
			logger::info("\t{}'s accuracy with spells is based on {}'s skill in spell's magic school", actor, actor);
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
		LogSkillMultiplier("NPCs", "Bows", "Aiming", NPC.aimMultipliers.bow);
		LogSkillMultiplier("NPCs", "Crossbows", "Aiming", NPC.aimMultipliers.crossbow);
		LogSkillMultiplier("NPCs", "Spells", "Aiming", NPC.aimMultipliers.spell);
		LogSkillMultiplier("NPCs", "Staves", "Aiming", NPC.aimMultipliers.staff);
		LogBlindnessSkillMultiplier("NPCs", "aiming", NPC.aimMultipliers.blindness);

		logger::info("");
		LogSkillMultiplier("NPCs", "Bows", "Release", NPC.releaseMultipliers.bow);
		LogSkillMultiplier("NPCs", "Crossbows", "Release", NPC.releaseMultipliers.crossbow);
		LogSkillMultiplier("NPCs", "Spells", "Release", NPC.releaseMultipliers.spell);
		LogSkillMultiplier("NPCs", "Staves", "Release", NPC.releaseMultipliers.staff);
		LogBlindnessSkillMultiplier("NPCs", "release", NPC.releaseMultipliers.blindness);

		logger::info("");
		LogStavesUseEnchantingSkill("NPCs", NPC.stavesUseEnchantingSkill);
		LogSpellsUseHighestMagicSkill("NPCs", NPC.spellsUseHighestMagicSkill, NPC.stavesUseEnchantingSkill);
		LogConcentrationSpellsRequireContinuousAim("NPCs", NPC.concetrationSpellsRequireContinuousAim);
		

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
		LogSkillMultiplier("Player", "Bows", "Aiming", Player.aimMultipliers.bow);
		LogSkillMultiplier("Player", "Crossbows", "Aiming", Player.aimMultipliers.crossbow);
		LogSkillMultiplier("Player", "Spells", "Aiming", Player.aimMultipliers.spell);
		LogSkillMultiplier("Player", "Staves", "Aiming", Player.aimMultipliers.staff);

		logger::info("");
		LogSkillMultiplier("Player", "Bows", "Release", Player.releaseMultipliers.bow);
		LogSkillMultiplier("Player", "Crossbows", "Release", Player.releaseMultipliers.crossbow);
		LogSkillMultiplier("Player", "Spells", "Release", Player.releaseMultipliers.spell);
		LogSkillMultiplier("Player", "Staves", "Release", Player.releaseMultipliers.staff);
		
		logger::info("");
		LogStavesUseEnchantingSkill("Player", Player.stavesUseEnchantingSkill);
		LogSpellsUseHighestMagicSkill("Player", Player.spellsUseHighestMagicSkill, Player.stavesUseEnchantingSkill);

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
		spellAiming = ini.GetBoolValue(a_section, "bEnableSpellAim", spellAiming);
		staffAiming = ini.GetBoolValue(a_section, "bEnableStaffAim", staffAiming);
		bowAiming = ini.GetBoolValue(a_section, "bEnableBowAim", bowAiming);
		crossbowAiming = ini.GetBoolValue(a_section, "bEnableCrossbowAim", crossbowAiming);

		aimMultipliers = SkillMultiplier(ini, a_section, "Aim");
		releaseMultipliers = SkillMultiplier(ini, a_section, "Release");

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


	Config::SkillMultiplier::SkillMultiplier(CSimpleIniA& ini, const char* a_section, std::string_view prefix) {
		std::string bowOption = fmt::format("fBow{}SkillMultiplier", prefix);
		std::string crossbowOption = fmt::format("fCrossbow{}SkillMultiplier", prefix);
		std::string spellOption = fmt::format("fSpell{}SkillMultiplier", prefix);
		std::string staffOption = fmt::format("fStaff{}SkillMultiplier", prefix);
		std::string blindnessOption = fmt::format("fBlindness{}SkillMultiplier", prefix);

		bow = ini.GetDoubleValue(a_section, bowOption.c_str(), bow);
		crossbow = ini.GetDoubleValue(a_section, crossbowOption.c_str(), crossbow);
		spell = ini.GetDoubleValue(a_section, spellOption.c_str(), spell);
		staff = ini.GetDoubleValue(a_section, staffOption.c_str(), staff);
		blindness = ini.GetDoubleValue(a_section, blindnessOption.c_str(), blindness);
	}
}
