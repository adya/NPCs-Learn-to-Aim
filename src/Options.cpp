#include "Options.h"
#include "CLIBUtil/string.hpp"
#include <fmt/ranges.h>

namespace NLA::Options
{
	inline void LogSkillMultiplier(std::string_view actor, std::string_view weaponName, std::string_view stage, float mult) {
		if (mult > 1) {
			auto percent = (mult - 1) * 100;
			logger::info("\t{} with {} is {:.2f}% easier for {}", stage, weaponName, percent, actor);
		} else if (mult < 1) {
			auto percent = (1 - mult) * 100;
			logger::info("\t{} with {} is {:.2f}% harder for {}", stage, weaponName, percent, actor);
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
		} else {
			logger::info("\t{}'s accuracy with staves is based on {}'s skill in spell's magic school", actor, actor);
		}
	}

	inline void LogSpellsUseHighestMagicSkill(std::string_view actor, bool spellsUseHighestMagicSkill, bool stavesUseEnchantingSkill) {
		if (spellsUseHighestMagicSkill) {
			logger::info("\t{}'s accuracy with spells is based on the highest {}'s magic skill regardless of spell", actor, actor);
		} else {
			logger::info("\t{}'s accuracy with spells is based on {}'s skill in spell's magic school", actor, actor);
		}
	}

	inline void LogConcentrationSpellRangeMultiplier(std::string_view actor, bool enabled, float minMult, float maxMult) {
		if (!enabled) {
			logger::info("	{} always have vanilla reach with concentration spells", actor);
		} else {
			logger::info("	{} reach concentration spells at {:.2f}x to {:.2f}x vanilla range, scaling with skill", actor, minMult, maxMult);
		}
	}

	inline void LogConcentrationSpellsRequireContinuousAim(std::string_view actor, bool concetrationSpellsRequireContinuousAim) {
		if (concetrationSpellsRequireContinuousAim) {
			logger::info("\t{} must aim continuously to maintain concentration spells", actor);
		} else {
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
		LogSkillMultiplier("NPCs", "Bows", "Accuracy", NPC.accuracyMultipliers.bow);
		LogSkillMultiplier("NPCs", "Crossbows", "Accuracy", NPC.accuracyMultipliers.crossbow);
		LogSkillMultiplier("NPCs", "Spells", "Accuracy", NPC.accuracyMultipliers.spell);
		LogSkillMultiplier("NPCs", "Staves", "Accuracy", NPC.accuracyMultipliers.staff);
		LogBlindnessSkillMultiplier("NPCs", "accuracy", NPC.accuracyMultipliers.blindness);

		logger::info("");
		LogSkillMultiplier("NPCs", "Bows", "Precision", NPC.precisionMultipliers.bow);
		LogSkillMultiplier("NPCs", "Crossbows", "Precision", NPC.precisionMultipliers.crossbow);
		LogSkillMultiplier("NPCs", "Spells", "Precision", NPC.precisionMultipliers.spell);
		LogSkillMultiplier("NPCs", "Staves", "Precision", NPC.precisionMultipliers.staff);
		LogBlindnessSkillMultiplier("NPCs", "precision", NPC.precisionMultipliers.blindness);

		logger::info("");
		LogStavesUseEnchantingSkill("NPCs", NPC.stavesUseEnchantingSkill);
		LogSpellsUseHighestMagicSkill("NPCs", NPC.spellsUseHighestMagicSkill, NPC.stavesUseEnchantingSkill);
		LogConcentrationSpellsRequireContinuousAim("NPCs", NPC.concetrationSpellsRequireContinuousAim);
		LogConcentrationSpellRangeMultiplier("NPCs", NPC.concentrationSpellRangeScaling, NPC.concentrationSpellMinRangeMultiplier, NPC.concentrationSpellMaxRangeMultiplier);

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
		LogSkillMultiplier("Player", "Bows", "Precision", Player.precisionMultipliers.bow);
		LogSkillMultiplier("Player", "Crossbows", "Precision", Player.precisionMultipliers.crossbow);
		LogSkillMultiplier("Player", "Spells", "Precision", Player.precisionMultipliers.spell);
		LogSkillMultiplier("Player", "Staves", "Precision", Player.precisionMultipliers.staff);

		logger::info("");
		LogStavesUseEnchantingSkill("Player", Player.stavesUseEnchantingSkill);
		LogSpellsUseHighestMagicSkill("Player", Player.spellsUseHighestMagicSkill, Player.stavesUseEnchantingSkill);
		LogConcentrationSpellsRequireContinuousAim("Player", Player.concetrationSpellsRequireContinuousAim);
		LogConcentrationSpellRangeMultiplier("Player", Player.concentrationSpellRangeScaling, Player.concentrationSpellMinRangeMultiplier, Player.concentrationSpellMaxRangeMultiplier);

		logger::info("{:*^40}", "");
	}

	bool NPCConfig::ShouldLearn(RE::Actor* a_actor) {
		if (!a_actor || a_actor->IsPlayerRef()) {
			return true;  // ShouldLearn only concerns NPCs, so we ignore players.
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

		accuracyMultipliers = SkillMultiplier(ini, a_section, "Accuracy");
		precisionMultipliers = SkillMultiplier(ini, a_section, "Precision");

		stavesUseEnchantingSkill = ini.GetBoolValue(a_section, "bStavesUseEnchantingSkill", stavesUseEnchantingSkill);

		spellsUseHighestMagicSkill = ini.GetBoolValue(a_section, "bSpellsUseHighestMagicSkill", spellsUseHighestMagicSkill);

		concentrationSpellRangeScaling = ini.GetBoolValue(a_section, "bEnableConcentrationSpellRangeScaling", concentrationSpellRangeScaling);
		concentrationSpellMinRangeMultiplier = static_cast<float>(ini.GetDoubleValue(a_section, "fConcentrationSpellMinRangeMultiplier", concentrationSpellMinRangeMultiplier));
		concentrationSpellMaxRangeMultiplier = static_cast<float>(ini.GetDoubleValue(a_section, "fConcentrationSpellMaxRangeMultiplier", concentrationSpellMaxRangeMultiplier));
	}

	PlayerConfig::PlayerConfig(CSimpleIniA& ini) :
		Config(ini, "Player") {
		concetrationSpellsRequireContinuousAim = ini.GetBoolValue("Player", "bConcentrationSpellsRequireContinuousAim", concetrationSpellsRequireContinuousAim);
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
