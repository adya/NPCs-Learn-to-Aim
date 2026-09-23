#include "Options.h"
#include "CLIBUtil/string.hpp"
#include <cmath>
#include <fmt/ranges.h>

namespace NLA::Options
{
	namespace
	{
		template <typename T>
		void SetOrDeleteValue(CSimpleIniA&, const char*, const char*, const T&, const T&) {}

		template <>
		void SetOrDeleteValue<bool>(CSimpleIniA& ini, const char* section, const char* key, const bool& value, const bool& defaultValue) {
			if (value != defaultValue) {
				ini.SetBoolValue(section, key, value);
			} else {
				ini.Delete(section, key, true);
			}
		}

		template <>
		void SetOrDeleteValue<float>(CSimpleIniA& ini, const char* section, const char* key, const float& value, const float& defaultValue) {
			constexpr float kEpsilon = 0.001f;
			if (std::abs(value - defaultValue) > kEpsilon) {
				ini.SetDoubleValue(section, key, value);
			} else {
				ini.Delete(section, key, true);
			}
		}

		void SetOrDeleteKeywords(CSimpleIniA& ini, const char* section, const char* key, const std::vector<std::string>& value, const std::vector<std::string>& defaultValue) {
			if (value != defaultValue) {
				ini.SetValue(section, key, fmt::format("{}", fmt::join(value, ",")).c_str());
			} else {
				ini.Delete(section, key, true);
			}
		}

		void SaveSkillMultiplier(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const Config::SkillMultiplier& value, const Config::SkillMultiplier& defaultValue) {
			SetOrDeleteValue(ini, a_section, fmt::format("fBow{}SkillMultiplier", prefix).c_str(), value.bow, defaultValue.bow);
			SetOrDeleteValue(ini, a_section, fmt::format("fCrossbow{}SkillMultiplier", prefix).c_str(), value.crossbow, defaultValue.crossbow);
			SetOrDeleteValue(ini, a_section, fmt::format("fSpell{}SkillMultiplier", prefix).c_str(), value.spell, defaultValue.spell);
			SetOrDeleteValue(ini, a_section, fmt::format("fConcentrationSpell{}SkillMultiplier", prefix).c_str(), value.concentrationSpell, defaultValue.concentrationSpell);
			SetOrDeleteValue(ini, a_section, fmt::format("fStaff{}SkillMultiplier", prefix).c_str(), value.staff, defaultValue.staff);
			SetOrDeleteValue(ini, a_section, fmt::format("fScroll{}SkillMultiplier", prefix).c_str(), value.scroll, defaultValue.scroll);
			SetOrDeleteValue(ini, a_section, fmt::format("fBlindness{}SkillMultiplier", prefix).c_str(), value.blindness, defaultValue.blindness);
		}

		void SaveHeadshotSettings(CSimpleIniA& ini, const char* a_section, const NPCConfig::HeadshotSettings& value, const NPCConfig::HeadshotSettings& defaultValue) {
			SetOrDeleteValue(ini, a_section, "bBowHeadshots", value.bow, defaultValue.bow);
			SetOrDeleteValue(ini, a_section, "fBowMinHeadshotChance", value.bowMinChance, defaultValue.bowMinChance);
			SetOrDeleteValue(ini, a_section, "fBowMaxHeadshotChance", value.bowMaxChance, defaultValue.bowMaxChance);
			SetOrDeleteValue(ini, a_section, "bCrossbowHeadshots", value.crossbow, defaultValue.crossbow);
			SetOrDeleteValue(ini, a_section, "fCrossbowMinHeadshotChance", value.crossbowMinChance, defaultValue.crossbowMinChance);
			SetOrDeleteValue(ini, a_section, "fCrossbowMaxHeadshotChance", value.crossbowMaxChance, defaultValue.crossbowMaxChance);
		}

		void SaveConfig(CSimpleIniA& ini, const char* a_section, const Config& custom, const Config& defaults) {
			SetOrDeleteValue(ini, a_section, "bEnabled", custom.enabled, defaults.enabled);

			SetOrDeleteValue(ini, a_section, "bLearnSpells", custom.spellAiming, defaults.spellAiming);
			SetOrDeleteValue(ini, a_section, "bLearnStaves", custom.staffAiming, defaults.staffAiming);
			SetOrDeleteValue(ini, a_section, "bLearnScrolls", custom.scrollAiming, defaults.scrollAiming);
			SetOrDeleteValue(ini, a_section, "bLearnBows", custom.bowAiming, defaults.bowAiming);
			SetOrDeleteValue(ini, a_section, "bLearnCrossbows", custom.crossbowAiming, defaults.crossbowAiming);

			SaveSkillMultiplier(ini, a_section, "Accuracy", custom.accuracyMultipliers, defaults.accuracyMultipliers);
			SaveSkillMultiplier(ini, a_section, "Precision", custom.precisionMultipliers, defaults.precisionMultipliers);

			SetOrDeleteValue(ini, a_section, "bBlindnessAffectsAim", custom.blindnessAffectsAiming, defaults.blindnessAffectsAiming);
			SetOrDeleteValue(ini, a_section, "bUseEnchantingSkill", custom.stavesUseEnchantingSkill, defaults.stavesUseEnchantingSkill);
			SetOrDeleteValue(ini, a_section, "bStavesUseHighestMagicSkill", custom.stavesUseHighestMagicSkill, defaults.stavesUseHighestMagicSkill);
			SetOrDeleteValue(ini, a_section, "bUseHighestMagicSkill", custom.spellsUseHighestMagicSkill, defaults.spellsUseHighestMagicSkill);
			SetOrDeleteValue(ini, a_section, "bConcentrationSpellsUseHighestMagicSkill", custom.concentrationSpellsUseHighestMagicSkill, defaults.concentrationSpellsUseHighestMagicSkill);
			SetOrDeleteValue(ini, a_section, "bScrollsUseEnchantingSkill", custom.scrollsUseEnchantingSkill, defaults.scrollsUseEnchantingSkill);
			SetOrDeleteValue(ini, a_section, "bScrollsUseHighestMagicSkill", custom.scrollsUseHighestMagicSkill, defaults.scrollsUseHighestMagicSkill);

			SetOrDeleteValue(ini, a_section, "bLearnConcentrationSpells", custom.concentrationSpellsAiming, defaults.concentrationSpellsAiming);
			SetOrDeleteValue(ini, a_section, "bScaleRangeWithSkill", custom.concentrationSpellRangeScaling, defaults.concentrationSpellRangeScaling);
			SetOrDeleteValue(ini, a_section, "fConcentrationSpellMinRangeMultiplier", custom.concentrationSpellMinRangeMultiplier, defaults.concentrationSpellMinRangeMultiplier);
			SetOrDeleteValue(ini, a_section, "fConcentrationSpellMaxRangeMultiplier", custom.concentrationSpellMaxRangeMultiplier, defaults.concentrationSpellMaxRangeMultiplier);

			SetOrDeleteValue(ini, a_section, "bScaleSpellChargeTimeWithSkill", custom.spellChargeTimeScaling, defaults.spellChargeTimeScaling);
			SetOrDeleteValue(ini, a_section, "fSpellChargeSpeedMinMultiplier", custom.spellChargeSpeedMinMultiplier, defaults.spellChargeSpeedMinMultiplier);
			SetOrDeleteValue(ini, a_section, "fSpellChargeSpeedMaxMultiplier", custom.spellChargeSpeedMaxMultiplier, defaults.spellChargeSpeedMaxMultiplier);

			SetOrDeleteValue(ini, a_section, "bScaleConcentrationSpellChargeTimeWithSkill", custom.concentrationSpellChargeTimeScaling, defaults.concentrationSpellChargeTimeScaling);
			SetOrDeleteValue(ini, a_section, "fConcentrationSpellChargeSpeedMinMultiplier", custom.concentrationSpellChargeSpeedMinMultiplier, defaults.concentrationSpellChargeSpeedMinMultiplier);
			SetOrDeleteValue(ini, a_section, "fConcentrationSpellChargeSpeedMaxMultiplier", custom.concentrationSpellChargeSpeedMaxMultiplier, defaults.concentrationSpellChargeSpeedMaxMultiplier);

			SetOrDeleteValue(ini, a_section, "bScaleStaffChargeTimeWithSkill", custom.staffChargeTimeScaling, defaults.staffChargeTimeScaling);
			SetOrDeleteValue(ini, a_section, "fStaffChargeSpeedMinMultiplier", custom.staffChargeSpeedMinMultiplier, defaults.staffChargeSpeedMinMultiplier);
			SetOrDeleteValue(ini, a_section, "fStaffChargeSpeedMaxMultiplier", custom.staffChargeSpeedMaxMultiplier, defaults.staffChargeSpeedMaxMultiplier);

			SetOrDeleteValue(ini, a_section, "bScaleScrollChargeTimeWithSkill", custom.scrollChargeTimeScaling, defaults.scrollChargeTimeScaling);
			SetOrDeleteValue(ini, a_section, "fScrollChargeSpeedMinMultiplier", custom.scrollChargeSpeedMinMultiplier, defaults.scrollChargeSpeedMinMultiplier);
			SetOrDeleteValue(ini, a_section, "fScrollChargeSpeedMaxMultiplier", custom.scrollChargeSpeedMaxMultiplier, defaults.scrollChargeSpeedMaxMultiplier);
		}
	}

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

	inline void LogStavesUseEnchantingSkill(std::string_view actor, bool stavesUseEnchantingSkill, bool stavesUseHighestMagicSkill) {
		if (stavesUseEnchantingSkill) {
			logger::info("\t{}'s accuracy with staves is based on Enchanting skill", actor);
		} else if (stavesUseHighestMagicSkill) {
			logger::info("\t{}'s accuracy with staves is based on the highest {}'s magic skill regardless of spell", actor, actor);
		} else {
			logger::info("\t{}'s accuracy with staves is based on {}'s skill in spell's magic school", actor, actor);
		}
	}

	inline void LogSpellsUseHighestMagicSkill(std::string_view actor, bool spellsUseHighestMagicSkill) {
		if (spellsUseHighestMagicSkill) {
			logger::info("\t{}'s accuracy with spells is based on the highest {}'s magic skill regardless of spell", actor, actor);
		} else {
			logger::info("\t{}'s accuracy with spells is based on {}'s skill in spell's magic school", actor, actor);
		}
	}

	inline void LogScrollsUseEnchantingSkill(std::string_view actor, bool scrollsUseEnchantingSkill, bool scrollsUseHighestMagicSkill) {
		if (scrollsUseEnchantingSkill) {
			logger::info("\t{}'s accuracy with scrolls is based on Enchanting skill", actor);
		} else if (scrollsUseHighestMagicSkill) {
			logger::info("\t{}'s accuracy with scrolls is based on the highest {}'s magic skill regardless of spell", actor, actor);
		} else {
			logger::info("\t{}'s accuracy with scrolls is based on {}'s skill in scroll's magic school", actor, actor);
		}
	}

	inline void LogConcentrationSpellsUseHighestMagicSkill(std::string_view actor, bool concentrationSpellsUseHighestMagicSkill) {
		if (concentrationSpellsUseHighestMagicSkill) {
			logger::info("\t{}'s accuracy with concentration spells is based on the highest {}'s magic skill regardless of spell", actor, actor);
		} else {
			logger::info("\t{}'s accuracy with concentration spells is based on {}'s skill in spell's magic school", actor, actor);
		}
	}

	inline void LogConcentrationSpellRangeMultiplier(std::string_view actor, bool enabled, float minMult, float maxMult) {
		if (!enabled) {
			logger::info("	{} always have vanilla reach with concentration spells", actor);
		} else {
			logger::info("	{} reach concentration spells at {:.2f}x to {:.2f}x vanilla range, scaling with skill", actor, minMult, maxMult);
		}
	}

	inline std::string SpeedPercentPhrase(float mult) {
		if (mult > 1) {
			return fmt::format("{:.2f}% faster", (mult - 1) * 100);
		} else if (mult < 1) {
			return fmt::format("{:.2f}% slower", (1 - mult) * 100);
		} else {
			return "at vanilla speed";
		}
	}

	inline void LogChargeSpeedMultiplier(std::string_view actor, std::string_view categoryName, bool enabled, float minMult, float maxMult) {
		if (!enabled) {
			logger::info("\t{} always charge/cast {} at vanilla speed", actor, categoryName);
		} else if (minMult == maxMult) {
			logger::info("\t{} charge/cast {} {}, regardless of skill", actor, categoryName, SpeedPercentPhrase(minMult));
		} else if (minMult == 1.0f) {
			logger::info("\t{} charge/cast {} up to {}, scaling with skill", actor, categoryName, SpeedPercentPhrase(maxMult));
		} else {
			logger::info("\t{} charge/cast {} {} at 0 skill, {} at 100 skill", actor, categoryName, SpeedPercentPhrase(minMult), SpeedPercentPhrase(maxMult));
		}
	}

	void Load() {
		++currentID;

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
		if (!NPC.enabled) {
			logger::info("\tMod is disabled for NPCs");
			return;
		}
		logger::info("\tArchers {}", NPC.bowAiming ? "will learn to aim" : "have perfect aim");
		logger::info("\tCrossbowmen {}", NPC.crossbowAiming ? "will learn to aim" : "have perfect aim");
		logger::info("\tMages {} with spells", NPC.spellAiming ? "will learn to aim" : "have perfect aim");
		logger::info("\tMages {} with staves", NPC.staffAiming ? "will learn to aim" : "have perfect aim");
		logger::info("\tMages {} with scrolls", NPC.scrollAiming ? "will learn to aim" : "have perfect aim");
		logger::info("\tMages {} with concentration spells", NPC.concentrationSpellsAiming ? "will learn to aim" : "have perfect aim");
		if (NPC.headshots.bow) {
			logger::info("\tSkilled archers may go for headshots ({:.0f}%-{:.0f}% chance)", NPC.headshots.bowMinChance * 100, NPC.headshots.bowMaxChance * 100);
		} else {
			logger::info("\tArchers always aim for the chest");
		}
		if (NPC.headshots.crossbow) {
			logger::info("\tSkilled crossbowmen may go for headshots ({:.0f}%-{:.0f}% chance)", NPC.headshots.crossbowMinChance * 100, NPC.headshots.crossbowMaxChance * 100);
		} else {
			logger::info("\tCrossbowmen always aim for the chest");
		}

		logger::info("");
		LogSkillMultiplier("NPCs", "Bows", "Accuracy", NPC.accuracyMultipliers.bow);
		LogSkillMultiplier("NPCs", "Crossbows", "Accuracy", NPC.accuracyMultipliers.crossbow);
		LogSkillMultiplier("NPCs", "Spells", "Accuracy", NPC.accuracyMultipliers.spell);
		LogSkillMultiplier("NPCs", "Concentration Spells", "Accuracy", NPC.accuracyMultipliers.concentrationSpell);
		LogSkillMultiplier("NPCs", "Staves", "Accuracy", NPC.accuracyMultipliers.staff);
		LogSkillMultiplier("NPCs", "Scrolls", "Accuracy", NPC.accuracyMultipliers.scroll);
		LogBlindnessSkillMultiplier("NPCs", "accuracy", NPC.accuracyMultipliers.blindness);

		logger::info("");
		LogSkillMultiplier("NPCs", "Bows", "Precision", NPC.precisionMultipliers.bow);
		LogSkillMultiplier("NPCs", "Crossbows", "Precision", NPC.precisionMultipliers.crossbow);
		LogSkillMultiplier("NPCs", "Spells", "Precision", NPC.precisionMultipliers.spell);
		LogSkillMultiplier("NPCs", "Concentration Spells", "Precision", NPC.precisionMultipliers.concentrationSpell);
		LogSkillMultiplier("NPCs", "Staves", "Precision", NPC.precisionMultipliers.staff);
		LogSkillMultiplier("NPCs", "Scrolls", "Precision", NPC.precisionMultipliers.scroll);
		LogBlindnessSkillMultiplier("NPCs", "precision", NPC.precisionMultipliers.blindness);

		logger::info("");
		LogStavesUseEnchantingSkill("NPCs", NPC.stavesUseEnchantingSkill, NPC.stavesUseHighestMagicSkill);
		LogSpellsUseHighestMagicSkill("NPCs", NPC.spellsUseHighestMagicSkill);
		LogScrollsUseEnchantingSkill("NPCs", NPC.scrollsUseEnchantingSkill, NPC.scrollsUseHighestMagicSkill);
		LogConcentrationSpellsUseHighestMagicSkill("NPCs", NPC.concentrationSpellsUseHighestMagicSkill);
		LogConcentrationSpellRangeMultiplier("NPCs", NPC.concentrationSpellRangeScaling, NPC.concentrationSpellMinRangeMultiplier, NPC.concentrationSpellMaxRangeMultiplier);

		logger::info("");
		LogChargeSpeedMultiplier("NPCs", "spells", NPC.spellChargeTimeScaling, NPC.spellChargeSpeedMinMultiplier, NPC.spellChargeSpeedMaxMultiplier);
		LogChargeSpeedMultiplier("NPCs", "concentration spells", NPC.concentrationSpellChargeTimeScaling, NPC.concentrationSpellChargeSpeedMinMultiplier, NPC.concentrationSpellChargeSpeedMaxMultiplier);
		LogChargeSpeedMultiplier("NPCs", "staves", NPC.staffChargeTimeScaling, NPC.staffChargeSpeedMinMultiplier, NPC.staffChargeSpeedMaxMultiplier);
		LogChargeSpeedMultiplier("NPCs", "scrolls", NPC.scrollChargeTimeScaling, NPC.scrollChargeSpeedMinMultiplier, NPC.scrollChargeSpeedMaxMultiplier);

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
		if (!Player.enabled) {
			logger::info("\tMod is disabled for Player");
			return;
		}
		logger::info("\t{} with bows", Player.bowAiming ? "Will learn to aim" : "Has perfect aim");
		logger::info("\t{} with crossbows", Player.crossbowAiming ? "Will learn to aim" : "Has perfect aim");
		logger::info("\t{} with spells", Player.spellAiming ? "Will learn to aim" : "Has perfect aim");
		logger::info("\t{} with staves", Player.staffAiming ? "Will learn to aim" : "Has perfect aim");
		logger::info("\t{} with scrolls", Player.scrollAiming ? "Will learn to aim" : "Has perfect aim");
		logger::info("\t{} with concentration spells", Player.concentrationSpellsAiming ? "Will learn to aim" : "Has perfect aim");

		logger::info("");
		LogSkillMultiplier("Player", "Bows", "Precision", Player.precisionMultipliers.bow);
		LogSkillMultiplier("Player", "Crossbows", "Precision", Player.precisionMultipliers.crossbow);
		LogSkillMultiplier("Player", "Spells", "Precision", Player.precisionMultipliers.spell);
		LogSkillMultiplier("Player", "Concentration Spells", "Precision", Player.precisionMultipliers.concentrationSpell);
		LogSkillMultiplier("Player", "Staves", "Precision", Player.precisionMultipliers.staff);
		LogSkillMultiplier("Player", "Scrolls", "Precision", Player.precisionMultipliers.scroll);

		logger::info("");
		LogStavesUseEnchantingSkill("Player", Player.stavesUseEnchantingSkill, Player.stavesUseHighestMagicSkill);
		LogSpellsUseHighestMagicSkill("Player", Player.spellsUseHighestMagicSkill);
		LogScrollsUseEnchantingSkill("Player", Player.scrollsUseEnchantingSkill, Player.scrollsUseHighestMagicSkill);
		LogConcentrationSpellsUseHighestMagicSkill("Player", Player.concentrationSpellsUseHighestMagicSkill);
		LogConcentrationSpellRangeMultiplier("Player", Player.concentrationSpellRangeScaling, Player.concentrationSpellMinRangeMultiplier, Player.concentrationSpellMaxRangeMultiplier);

		logger::info("");
		LogChargeSpeedMultiplier("Player", "spells", Player.spellChargeTimeScaling, Player.spellChargeSpeedMinMultiplier, Player.spellChargeSpeedMaxMultiplier);
		LogChargeSpeedMultiplier("Player", "concentration spells", Player.concentrationSpellChargeTimeScaling, Player.concentrationSpellChargeSpeedMinMultiplier, Player.concentrationSpellChargeSpeedMaxMultiplier);
		LogChargeSpeedMultiplier("Player", "staves", Player.staffChargeTimeScaling, Player.staffChargeSpeedMinMultiplier, Player.staffChargeSpeedMaxMultiplier);
		LogChargeSpeedMultiplier("Player", "scrolls", Player.scrollChargeTimeScaling, Player.scrollChargeSpeedMinMultiplier, Player.scrollChargeSpeedMaxMultiplier);

		logger::info("{:*^40}", "");
	}

	void Save() {
		const std::filesystem::path options = R"(Data\SKSE\Plugins\NPCsLearnToAim.ini)";
		CSimpleIniA                 ini{};
		ini.SetUnicode();
		ini.LoadFile(options.string().c_str());

		SaveConfig(ini, "NPC", NPC, NPCConfig::defaultConfig);
		SaveConfig(ini, "Player", Player, PlayerConfig::defaultConfig);

		SaveHeadshotSettings(ini, "NPC", NPC.headshots, NPCConfig::defaultConfig.headshots);

		SetOrDeleteKeywords(ini, "NPC", "sExcludeKeywords", NPC.excludeKeywords, NPCConfig::defaultConfig.excludeKeywords);
		SetOrDeleteKeywords(ini, "NPC", "sIncludeKeywords", NPC.includeKeywords, NPCConfig::defaultConfig.includeKeywords);

		ini.SaveFile(options.string().c_str());
	}

	bool NPCConfig::ShouldLearn(RE::Actor* a_actor) {
		if (!a_actor) {
			return false;
		}
		if (a_actor->IsPlayerRef()) {
			return Player.enabled;
		}

		if (!enabled) {
			return false;
		}
		if (!a_actor->HasAnyKeywordByEditorID(includeKeywords) && (!a_actor->GetRace() || !a_actor->GetRace()->HasAnyKeywordByEditorID(includeKeywords))) {
			return false;
		}

		return !a_actor->HasAnyKeywordByEditorID(excludeKeywords) && (!a_actor->GetRace() || !a_actor->GetRace()->HasAnyKeywordByEditorID(excludeKeywords));
	}

	Config::Config(CSimpleIniA& ini, const char* a_section, const Config& defaults) :
		Config(defaults) {
		enabled = ini.GetBoolValue(a_section, "bEnabled", enabled);

		spellAiming = ini.GetBoolValue(a_section, "bLearnSpells", spellAiming);
		staffAiming = ini.GetBoolValue(a_section, "bLearnStaves", staffAiming);
		scrollAiming = ini.GetBoolValue(a_section, "bLearnScrolls", scrollAiming);
		bowAiming = ini.GetBoolValue(a_section, "bLearnBows", bowAiming);
		crossbowAiming = ini.GetBoolValue(a_section, "bLearnCrossbows", crossbowAiming);

		accuracyMultipliers = SkillMultiplier(ini, a_section, "Accuracy", defaults.accuracyMultipliers);
		precisionMultipliers = SkillMultiplier(ini, a_section, "Precision", defaults.precisionMultipliers);

		blindnessAffectsAiming = ini.GetBoolValue(a_section, "bBlindnessAffectsAim", blindnessAffectsAiming);

		stavesUseEnchantingSkill = ini.GetBoolValue(a_section, "bUseEnchantingSkill", stavesUseEnchantingSkill);
		stavesUseHighestMagicSkill = ini.GetBoolValue(a_section, "bStavesUseHighestMagicSkill", stavesUseHighestMagicSkill);

		spellsUseHighestMagicSkill = ini.GetBoolValue(a_section, "bUseHighestMagicSkill", spellsUseHighestMagicSkill);
		concentrationSpellsUseHighestMagicSkill = ini.GetBoolValue(a_section, "bConcentrationSpellsUseHighestMagicSkill", concentrationSpellsUseHighestMagicSkill);
		scrollsUseEnchantingSkill = ini.GetBoolValue(a_section, "bScrollsUseEnchantingSkill", scrollsUseEnchantingSkill);
		scrollsUseHighestMagicSkill = ini.GetBoolValue(a_section, "bScrollsUseHighestMagicSkill", scrollsUseHighestMagicSkill);

		concentrationSpellRangeScaling = ini.GetBoolValue(a_section, "bScaleRangeWithSkill", concentrationSpellRangeScaling);
		concentrationSpellMinRangeMultiplier = ini.GetDoubleValue(a_section, "fConcentrationSpellMinRangeMultiplier", concentrationSpellMinRangeMultiplier);
		concentrationSpellMaxRangeMultiplier = ini.GetDoubleValue(a_section, "fConcentrationSpellMaxRangeMultiplier", concentrationSpellMaxRangeMultiplier);

		spellChargeTimeScaling = ini.GetBoolValue(a_section, "bScaleSpellChargeTimeWithSkill", spellChargeTimeScaling);
		spellChargeSpeedMinMultiplier = ini.GetDoubleValue(a_section, "fSpellChargeSpeedMinMultiplier", spellChargeSpeedMinMultiplier);
		spellChargeSpeedMaxMultiplier = ini.GetDoubleValue(a_section, "fSpellChargeSpeedMaxMultiplier", spellChargeSpeedMaxMultiplier);

		concentrationSpellChargeTimeScaling = ini.GetBoolValue(a_section, "bScaleConcentrationSpellChargeTimeWithSkill", concentrationSpellChargeTimeScaling);
		concentrationSpellChargeSpeedMinMultiplier = ini.GetDoubleValue(a_section, "fConcentrationSpellChargeSpeedMinMultiplier", concentrationSpellChargeSpeedMinMultiplier);
		concentrationSpellChargeSpeedMaxMultiplier = ini.GetDoubleValue(a_section, "fConcentrationSpellChargeSpeedMaxMultiplier", concentrationSpellChargeSpeedMaxMultiplier);

		staffChargeTimeScaling = ini.GetBoolValue(a_section, "bScaleStaffChargeTimeWithSkill", staffChargeTimeScaling);
		staffChargeSpeedMinMultiplier = ini.GetDoubleValue(a_section, "fStaffChargeSpeedMinMultiplier", staffChargeSpeedMinMultiplier);
		staffChargeSpeedMaxMultiplier = ini.GetDoubleValue(a_section, "fStaffChargeSpeedMaxMultiplier", staffChargeSpeedMaxMultiplier);

		scrollChargeTimeScaling = ini.GetBoolValue(a_section, "bScaleScrollChargeTimeWithSkill", scrollChargeTimeScaling);
		scrollChargeSpeedMinMultiplier = ini.GetDoubleValue(a_section, "fScrollChargeSpeedMinMultiplier", scrollChargeSpeedMinMultiplier);
		scrollChargeSpeedMaxMultiplier = ini.GetDoubleValue(a_section, "fScrollChargeSpeedMaxMultiplier", scrollChargeSpeedMaxMultiplier);
	}

	PlayerConfig::PlayerConfig(CSimpleIniA& ini) :
		Config(ini, "Player", PlayerConfig::defaultConfig) {
		concentrationSpellsAiming = ini.GetBoolValue("Player", "bLearnConcentrationSpells", concentrationSpellsAiming);
	}

	NPCConfig::NPCConfig(CSimpleIniA& ini) :
		Config(ini, "NPC", NPCConfig::defaultConfig) {
		concentrationSpellsAiming = ini.GetBoolValue("NPC", "bLearnConcentrationSpells", concentrationSpellsAiming);

		headshots = HeadshotSettings(ini, "NPC", headshots);

		excludeKeywords = clib_util::string::split(std::string(ini.GetValue("NPC", "sExcludeKeywords", "")), ",");
		includeKeywords = clib_util::string::split(std::string(ini.GetValue("NPC", "sIncludeKeywords", "ActorTypeNPC")), ",");
	}

	NPCConfig::HeadshotSettings::HeadshotSettings(CSimpleIniA& ini, const char* a_section, const HeadshotSettings& defaults) :
		HeadshotSettings(defaults) {
		bow = ini.GetBoolValue(a_section, "bBowHeadshots", bow);
		bowMinChance = ini.GetDoubleValue(a_section, "fBowMinHeadshotChance", bowMinChance);
		bowMaxChance = ini.GetDoubleValue(a_section, "fBowMaxHeadshotChance", bowMaxChance);
		crossbow = ini.GetBoolValue(a_section, "bCrossbowHeadshots", crossbow);
		crossbowMinChance = ini.GetDoubleValue(a_section, "fCrossbowMinHeadshotChance", crossbowMinChance);
		crossbowMaxChance = ini.GetDoubleValue(a_section, "fCrossbowMaxHeadshotChance", crossbowMaxChance);
	}

	Config::SkillMultiplier::SkillMultiplier(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const SkillMultiplier& defaults) :
		SkillMultiplier(defaults) {
		std::string bowOption = fmt::format("fBow{}SkillMultiplier", prefix);
		std::string crossbowOption = fmt::format("fCrossbow{}SkillMultiplier", prefix);
		std::string spellOption = fmt::format("fSpell{}SkillMultiplier", prefix);
		std::string concentrationSpellOption = fmt::format("fConcentrationSpell{}SkillMultiplier", prefix);
		std::string staffOption = fmt::format("fStaff{}SkillMultiplier", prefix);
		std::string scrollOption = fmt::format("fScroll{}SkillMultiplier", prefix);
		std::string blindnessOption = fmt::format("fBlindness{}SkillMultiplier", prefix);

		bow = ini.GetDoubleValue(a_section, bowOption.c_str(), bow);
		crossbow = ini.GetDoubleValue(a_section, crossbowOption.c_str(), crossbow);
		spell = ini.GetDoubleValue(a_section, spellOption.c_str(), spell);
		concentrationSpell = ini.GetDoubleValue(a_section, concentrationSpellOption.c_str(), concentrationSpell);
		staff = ini.GetDoubleValue(a_section, staffOption.c_str(), staff);
		scroll = ini.GetDoubleValue(a_section, scrollOption.c_str(), scroll);
		blindness = ini.GetDoubleValue(a_section, blindnessOption.c_str(), blindness);
	}
}
