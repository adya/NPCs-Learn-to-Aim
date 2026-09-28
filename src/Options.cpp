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

		void SaveOvercharge(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const Config::Overcharge& value, const Config::Overcharge& defaultValue) {
			SetOrDeleteValue(ini, a_section, fmt::format("b{}", prefix).c_str(), value.enabled, defaultValue.enabled);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}MaxSpreadReduction", prefix).c_str(), value.maxSpreadMultiplier, defaultValue.maxSpreadMultiplier);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}HalfDuration", prefix).c_str(), value.halfDuration, defaultValue.halfDuration);
		}

		void SaveSpeedOptions(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const Config::SpeedOptions& value, const Config::SpeedOptions& defaultValue) {
			SetOrDeleteValue(ini, a_section, fmt::format("bScale{}WithSkill", prefix).c_str(), value.enabled, defaultValue.enabled);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}MinMultiplier", prefix).c_str(), value.minMultiplier, defaultValue.minMultiplier);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}MaxMultiplier", prefix).c_str(), value.maxMultiplier, defaultValue.maxMultiplier);
		}

		void SaveChargeFatigue(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const Config::FatigueOptions& value, const Config::FatigueOptions& defaultValue) {
			SetOrDeleteValue(ini, a_section, fmt::format("b{}", prefix).c_str(), value.enabled, defaultValue.enabled);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}MinThreshold", prefix).c_str(), value.minThreshold, defaultValue.minThreshold);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}MaxThreshold", prefix).c_str(), value.maxThreshold, defaultValue.maxThreshold);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}MaxPenalty", prefix).c_str(), value.maxSpreadMultiplier, defaultValue.maxSpreadMultiplier);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}HalfDuration", prefix).c_str(), value.halfDuration, defaultValue.halfDuration);
		}

		void SaveConfig(CSimpleIniA& ini, const char* a_section, const Config& custom, const Config& defaults) {
			SetOrDeleteValue(ini, a_section, "bEnabled", custom.enabled, defaults.enabled);

			SetOrDeleteValue(ini, a_section, "bSpellAiming", custom.spellAiming, defaults.spellAiming);
			SetOrDeleteValue(ini, a_section, "bStaffAiming", custom.staffAiming, defaults.staffAiming);
			SetOrDeleteValue(ini, a_section, "bScrollAiming", custom.scrollAiming, defaults.scrollAiming);
			SetOrDeleteValue(ini, a_section, "bBowAiming", custom.bowAiming, defaults.bowAiming);
			SetOrDeleteValue(ini, a_section, "bCrossbowAiming", custom.crossbowAiming, defaults.crossbowAiming);

			SaveSkillMultiplier(ini, a_section, "Accuracy", custom.accuracyMultipliers, defaults.accuracyMultipliers);
			SaveSkillMultiplier(ini, a_section, "Precision", custom.precisionMultipliers, defaults.precisionMultipliers);

			SetOrDeleteValue(ini, a_section, "bBlindnessAffectsAiming", custom.blindnessAffectsAiming, defaults.blindnessAffectsAiming);
			SetOrDeleteValue(ini, a_section, "bStavesUseEnchantingSkill", custom.stavesUseEnchantingSkill, defaults.stavesUseEnchantingSkill);
			SetOrDeleteValue(ini, a_section, "bStavesUseHighestMagicSkill", custom.stavesUseHighestMagicSkill, defaults.stavesUseHighestMagicSkill);
			SetOrDeleteValue(ini, a_section, "bSpellsUseHighestMagicSkill", custom.spellsUseHighestMagicSkill, defaults.spellsUseHighestMagicSkill);
			SetOrDeleteValue(ini, a_section, "bConcentrationSpellsUseHighestMagicSkill", custom.concentrationSpellsUseHighestMagicSkill, defaults.concentrationSpellsUseHighestMagicSkill);
			SetOrDeleteValue(ini, a_section, "bScrollsUseEnchantingSkill", custom.scrollsUseEnchantingSkill, defaults.scrollsUseEnchantingSkill);
			SetOrDeleteValue(ini, a_section, "bScrollsUseHighestMagicSkill", custom.scrollsUseHighestMagicSkill, defaults.scrollsUseHighestMagicSkill);

			SetOrDeleteValue(ini, a_section, "bConcentrationSpellsAiming", custom.concentrationSpellsAiming, defaults.concentrationSpellsAiming);
			SetOrDeleteValue(ini, a_section, "bConcentrationSpellRangeScaling", custom.concentrationSpellRangeScaling, defaults.concentrationSpellRangeScaling);
			SetOrDeleteValue(ini, a_section, "fConcentrationSpellMinRangeMultiplier", custom.concentrationSpellMinRangeMultiplier, defaults.concentrationSpellMinRangeMultiplier);
			SetOrDeleteValue(ini, a_section, "fConcentrationSpellMaxRangeMultiplier", custom.concentrationSpellMaxRangeMultiplier, defaults.concentrationSpellMaxRangeMultiplier);

			SaveSpeedOptions(ini, a_section, "SpellChargeSpeed", custom.spellChargeSpeed, defaults.spellChargeSpeed);

			SaveOvercharge(ini, a_section, "SpellOvercharge", custom.spellOvercharge, defaults.spellOvercharge);
			SaveOvercharge(ini, a_section, "StaffOvercharge", custom.staffOvercharge, defaults.staffOvercharge);
			SaveOvercharge(ini, a_section, "ScrollOvercharge", custom.scrollOvercharge, defaults.scrollOvercharge);

			SaveChargeFatigue(ini, a_section, "SpellFatigue", custom.spellFatigue, defaults.spellFatigue);
			SaveChargeFatigue(ini, a_section, "StaffFatigue", custom.staffFatigue, defaults.staffFatigue);
			SaveChargeFatigue(ini, a_section, "ScrollFatigue", custom.scrollFatigue, defaults.scrollFatigue);

			SaveSpeedOptions(ini, a_section, "ConcentrationSpellChargeSpeed", custom.concentrationSpellChargeSpeed, defaults.concentrationSpellChargeSpeed);
			SaveSpeedOptions(ini, a_section, "StaffChargeSpeed", custom.staffChargeSpeed, defaults.staffChargeSpeed);
			SaveSpeedOptions(ini, a_section, "ScrollChargeSpeed", custom.scrollChargeSpeed, defaults.scrollChargeSpeed);

			SaveSpeedOptions(ini, a_section, "BowDrawSpeed", custom.bowDrawSpeed, defaults.bowDrawSpeed);
			SaveSpeedOptions(ini, a_section, "CrossbowDrawSpeed", custom.crossbowDrawSpeed, defaults.crossbowDrawSpeed);
			SaveOvercharge(ini, a_section, "BowSteadying", custom.bowSteadying, defaults.bowSteadying);
			SaveChargeFatigue(ini, a_section, "BowFatigue", custom.bowFatigue, defaults.bowFatigue);
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

	inline void LogSpellOvercharge(std::string_view actor, std::string_view categoryName, const Config::Overcharge& overcharge) {
		if (!overcharge.enabled) {
			logger::info("\tOvercharging {} doesn't affect {}'s precision", categoryName, actor);
		} else {
			logger::info("\tOvercharging {} reduces {}'s spread by up to {:.0f}% (half of it after {:.2f}s)", categoryName, actor, overcharge.maxSpreadMultiplier * 100, overcharge.halfDuration);
		}
	}

	inline void LogBowSteadying(std::string_view actor, const Config::Steadying& steadying) {
		if (!steadying.enabled) {
			logger::info("\tHolding a bow at full draw doesn't affect {}'s precision", actor);
		} else {
			logger::info("\tHolding a bow at full draw reduces {}'s spread by up to {:.0f}% (half of it after {:.2f}s)",
			             actor, steadying.maxSpreadMultiplier * 100, steadying.halfDuration);
		}
	}

	inline void LogBowFatigue(std::string_view actor, const Config::DrawFatigue& fatigue) {
		if (!fatigue.enabled) {
			logger::info("\tHolding a bow too long doesn't fatigue {}'s aim", actor);
		} else {
			logger::info("\t{} can hold a full draw for {:.2f}s at 0 skill to {:.2f}s at 100 skill before fatigue sets in, "
			             "growing spread back by up to {:.0f}% (half of that after another {:.2f}s)",
			             actor, fatigue.minThreshold, fatigue.maxThreshold, fatigue.maxSpreadMultiplier * 100, fatigue.halfDuration);
		}
	}

	inline void LogCastFatigue(std::string_view actor, std::string_view categoryName, const Config::CastFatigue& fatigue) {
		if (!fatigue.enabled) {
			logger::info("\tOvercharging {} too long doesn't fatigue {}'s precision", categoryName, actor);
		} else {
			logger::info("\t{} can overcharge {} for {:.2f}s at 0 skill to {:.2f}s at 100 skill before fatigue sets in, "
			             "growing spread back by up to {:.0f}% (half of that after another {:.2f}s)",
			             actor, categoryName, fatigue.minThreshold, fatigue.maxThreshold, fatigue.maxSpreadMultiplier * 100, fatigue.halfDuration);
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

	inline void LogDrawSpeedMultiplier(std::string_view actor, std::string_view categoryName, bool enabled, float minMult, float maxMult) {
		if (!enabled) {
			logger::info("\t{} always draw {} at vanilla speed", actor, categoryName);
		} else if (minMult == maxMult) {
			logger::info("\t{} draw {} {}, regardless of skill", actor, categoryName, SpeedPercentPhrase(minMult));
		} else if (minMult == 1.0f) {
			logger::info("\t{} draw {} up to {}, scaling with skill", actor, categoryName, SpeedPercentPhrase(maxMult));
		} else {
			logger::info("\t{} draw {} {} at 0 skill, {} at 100 skill", actor, categoryName, SpeedPercentPhrase(minMult), SpeedPercentPhrase(maxMult));
		}
	}

	void Load() {
		++currentID;

		logger::info("{:*^40}", "OPTIONS");
		std::filesystem::path options = R"(Data\SKSE\Plugins\NPCsLearnToAim.ini)";
		CSimpleIniA           ini{};
		ini.SetUnicode();
		if (ini.LoadFile(options.string().c_str()) >= 0) {
			NPC = NPCConfig::FromIni(ini);
			Player = PlayerConfig::FromIni(ini);
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
		LogChargeSpeedMultiplier("NPCs", "spells", NPC.spellChargeSpeed.enabled, NPC.spellChargeSpeed.minMultiplier, NPC.spellChargeSpeed.maxMultiplier);
		LogChargeSpeedMultiplier("NPCs", "concentration spells", NPC.concentrationSpellChargeSpeed.enabled, NPC.concentrationSpellChargeSpeed.minMultiplier, NPC.concentrationSpellChargeSpeed.maxMultiplier);
		LogChargeSpeedMultiplier("NPCs", "staves", NPC.staffChargeSpeed.enabled, NPC.staffChargeSpeed.minMultiplier, NPC.staffChargeSpeed.maxMultiplier);
		LogChargeSpeedMultiplier("NPCs", "scrolls", NPC.scrollChargeSpeed.enabled, NPC.scrollChargeSpeed.minMultiplier, NPC.scrollChargeSpeed.maxMultiplier);
		LogDrawSpeedMultiplier("NPCs", "bows", NPC.bowDrawSpeed.enabled, NPC.bowDrawSpeed.minMultiplier, NPC.bowDrawSpeed.maxMultiplier);
		LogDrawSpeedMultiplier("NPCs", "crossbows", NPC.crossbowDrawSpeed.enabled, NPC.crossbowDrawSpeed.minMultiplier, NPC.crossbowDrawSpeed.maxMultiplier);
		LogSpellOvercharge("NPCs", "spells", NPC.spellOvercharge);
		LogSpellOvercharge("NPCs", "staves", NPC.staffOvercharge);
		LogSpellOvercharge("NPCs", "scrolls", NPC.scrollOvercharge);
		LogCastFatigue("NPCs", "spells", NPC.spellFatigue);
		LogCastFatigue("NPCs", "staves", NPC.staffFatigue);
		LogCastFatigue("NPCs", "scrolls", NPC.scrollFatigue);
		LogBowSteadying("NPCs", NPC.bowSteadying);
		LogBowFatigue("NPCs", NPC.bowFatigue);

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
		LogChargeSpeedMultiplier("Player", "spells", Player.spellChargeSpeed.enabled, Player.spellChargeSpeed.minMultiplier, Player.spellChargeSpeed.maxMultiplier);
		LogChargeSpeedMultiplier("Player", "concentration spells", Player.concentrationSpellChargeSpeed.enabled, Player.concentrationSpellChargeSpeed.minMultiplier, Player.concentrationSpellChargeSpeed.maxMultiplier);
		LogChargeSpeedMultiplier("Player", "staves", Player.staffChargeSpeed.enabled, Player.staffChargeSpeed.minMultiplier, Player.staffChargeSpeed.maxMultiplier);
		LogChargeSpeedMultiplier("Player", "scrolls", Player.scrollChargeSpeed.enabled, Player.scrollChargeSpeed.minMultiplier, Player.scrollChargeSpeed.maxMultiplier);
		LogDrawSpeedMultiplier("Player", "bows", Player.bowDrawSpeed.enabled, Player.bowDrawSpeed.minMultiplier, Player.bowDrawSpeed.maxMultiplier);
		LogDrawSpeedMultiplier("Player", "crossbows", Player.crossbowDrawSpeed.enabled, Player.crossbowDrawSpeed.minMultiplier, Player.crossbowDrawSpeed.maxMultiplier);
		LogSpellOvercharge("Player", "spells", Player.spellOvercharge);
		LogSpellOvercharge("Player", "staves", Player.staffOvercharge);
		LogSpellOvercharge("Player", "scrolls", Player.scrollOvercharge);
		LogCastFatigue("Player", "spells", Player.spellFatigue);
		LogCastFatigue("Player", "staves", Player.staffFatigue);
		LogCastFatigue("Player", "scrolls", Player.scrollFatigue);
		LogBowSteadying("Player", Player.bowSteadying);
		LogBowFatigue("Player", Player.bowFatigue);

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

	Config Config::FromIni(CSimpleIniA& ini, const char* a_section, const Config& defaults) {
		Config result = defaults;

		result.enabled = ini.GetBoolValue(a_section, "bEnabled", result.enabled);

		result.spellAiming = ini.GetBoolValue(a_section, "bSpellAiming", result.spellAiming);
		result.staffAiming = ini.GetBoolValue(a_section, "bStaffAiming", result.staffAiming);
		result.scrollAiming = ini.GetBoolValue(a_section, "bScrollAiming", result.scrollAiming);
		result.bowAiming = ini.GetBoolValue(a_section, "bBowAiming", result.bowAiming);
		result.crossbowAiming = ini.GetBoolValue(a_section, "bCrossbowAiming", result.crossbowAiming);

		result.accuracyMultipliers = SkillMultiplier::FromIni(ini, a_section, "Accuracy", defaults.accuracyMultipliers);
		result.precisionMultipliers = SkillMultiplier::FromIni(ini, a_section, "Precision", defaults.precisionMultipliers);

		result.blindnessAffectsAiming = ini.GetBoolValue(a_section, "bBlindnessAffectsAiming", result.blindnessAffectsAiming);

		result.stavesUseEnchantingSkill = ini.GetBoolValue(a_section, "bStavesUseEnchantingSkill", result.stavesUseEnchantingSkill);
		result.stavesUseHighestMagicSkill = ini.GetBoolValue(a_section, "bStavesUseHighestMagicSkill", result.stavesUseHighestMagicSkill);

		result.spellsUseHighestMagicSkill = ini.GetBoolValue(a_section, "bSpellsUseHighestMagicSkill", result.spellsUseHighestMagicSkill);
		result.concentrationSpellsUseHighestMagicSkill = ini.GetBoolValue(a_section, "bConcentrationSpellsUseHighestMagicSkill", result.concentrationSpellsUseHighestMagicSkill);
		result.scrollsUseEnchantingSkill = ini.GetBoolValue(a_section, "bScrollsUseEnchantingSkill", result.scrollsUseEnchantingSkill);
		result.scrollsUseHighestMagicSkill = ini.GetBoolValue(a_section, "bScrollsUseHighestMagicSkill", result.scrollsUseHighestMagicSkill);

		result.concentrationSpellRangeScaling = ini.GetBoolValue(a_section, "bConcentrationSpellRangeScaling", result.concentrationSpellRangeScaling);
		result.concentrationSpellMinRangeMultiplier = ini.GetDoubleValue(a_section, "fConcentrationSpellMinRangeMultiplier", result.concentrationSpellMinRangeMultiplier);
		result.concentrationSpellMaxRangeMultiplier = ini.GetDoubleValue(a_section, "fConcentrationSpellMaxRangeMultiplier", result.concentrationSpellMaxRangeMultiplier);

		result.spellChargeSpeed = SpeedOptions::FromIni(ini, a_section, "SpellChargeSpeed", result.spellChargeSpeed);
		result.concentrationSpellChargeSpeed = SpeedOptions::FromIni(ini, a_section, "ConcentrationSpellChargeSpeed", result.concentrationSpellChargeSpeed);
		result.staffChargeSpeed = SpeedOptions::FromIni(ini, a_section, "StaffChargeSpeed", result.staffChargeSpeed);
		result.scrollChargeSpeed = SpeedOptions::FromIni(ini, a_section, "ScrollChargeSpeed", result.scrollChargeSpeed);

		result.spellOvercharge = SpreadOptions::FromIni(ini, a_section, "SpellOvercharge", result.spellOvercharge);
		result.staffOvercharge = SpreadOptions::FromIni(ini, a_section, "StaffOvercharge", result.staffOvercharge);
		result.scrollOvercharge = SpreadOptions::FromIni(ini, a_section, "ScrollOvercharge", result.scrollOvercharge);

		result.spellFatigue = FatigueOptions::FromIni(ini, a_section, "SpellFatigue", result.spellFatigue);
		result.staffFatigue = FatigueOptions::FromIni(ini, a_section, "StaffFatigue", result.staffFatigue);
		result.scrollFatigue = FatigueOptions::FromIni(ini, a_section, "ScrollFatigue", result.scrollFatigue);

		result.bowDrawSpeed = SpeedOptions::FromIni(ini, a_section, "BowDrawSpeed", result.bowDrawSpeed);
		result.crossbowDrawSpeed = SpeedOptions::FromIni(ini, a_section, "CrossbowDrawSpeed", result.crossbowDrawSpeed);
		result.bowSteadying = SpreadOptions::FromIni(ini, a_section, "BowSteadying", result.bowSteadying);
		result.bowFatigue = FatigueOptions::FromIni(ini, a_section, "BowFatigue", result.bowFatigue);

		return result;
	}

	PlayerConfig PlayerConfig::FromIni(CSimpleIniA& ini) {
		PlayerConfig result{ Config::FromIni(ini, "Player", PlayerConfig::defaultConfig) };
		result.concentrationSpellsAiming = ini.GetBoolValue("Player", "bConcentrationSpellsAiming", result.concentrationSpellsAiming);
		return result;
	}

	NPCConfig NPCConfig::FromIni(CSimpleIniA& ini) {
		NPCConfig result{ Config::FromIni(ini, "NPC", NPCConfig::defaultConfig) };
		result.concentrationSpellsAiming = ini.GetBoolValue("NPC", "bConcentrationSpellsAiming", result.concentrationSpellsAiming);

		result.headshots = HeadshotSettings::FromIni(ini, "NPC", NPCConfig::defaultConfig.headshots);

		result.excludeKeywords = clib_util::string::split(std::string(ini.GetValue("NPC", "sExcludeKeywords", "")), ",");
		result.includeKeywords = clib_util::string::split(std::string(ini.GetValue("NPC", "sIncludeKeywords", "ActorTypeNPC")), ",");
		return result;
	}

	NPCConfig::HeadshotSettings NPCConfig::HeadshotSettings::FromIni(CSimpleIniA& ini, const char* a_section, const HeadshotSettings& defaults) {
		HeadshotSettings result = defaults;
		result.bow = ini.GetBoolValue(a_section, "bBowHeadshots", result.bow);
		result.bowMinChance = ini.GetDoubleValue(a_section, "fBowMinHeadshotChance", result.bowMinChance);
		result.bowMaxChance = ini.GetDoubleValue(a_section, "fBowMaxHeadshotChance", result.bowMaxChance);
		result.crossbow = ini.GetBoolValue(a_section, "bCrossbowHeadshots", result.crossbow);
		result.crossbowMinChance = ini.GetDoubleValue(a_section, "fCrossbowMinHeadshotChance", result.crossbowMinChance);
		result.crossbowMaxChance = ini.GetDoubleValue(a_section, "fCrossbowMaxHeadshotChance", result.crossbowMaxChance);
		return result;
	}

	Config::SpeedOptions Config::SpeedOptions::FromIni(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const SpeedOptions& defaults) {
		SpeedOptions result = defaults;
		result.enabled = ini.GetBoolValue(a_section, fmt::format("bScale{}WithSkill", prefix).c_str(), result.enabled);
		result.minMultiplier = static_cast<float>(ini.GetDoubleValue(a_section, fmt::format("f{}MinMultiplier", prefix).c_str(), result.minMultiplier));
		result.maxMultiplier = static_cast<float>(ini.GetDoubleValue(a_section, fmt::format("f{}MaxMultiplier", prefix).c_str(), result.maxMultiplier));
		return result;
	}

	Config::SpreadOptions Config::SpreadOptions::FromIni(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const SpreadOptions& defaults) {
		SpreadOptions result = defaults;
		result.enabled = ini.GetBoolValue(a_section, fmt::format("b{}", prefix).c_str(), result.enabled);
		result.maxSpreadMultiplier = std::clamp(static_cast<float>(ini.GetDoubleValue(a_section, fmt::format("f{}MaxSpreadReduction", prefix).c_str(), result.maxSpreadMultiplier)), 0.0f, 1.0f);
		result.halfDuration = std::max(static_cast<float>(ini.GetDoubleValue(a_section, fmt::format("f{}HalfDuration", prefix).c_str(), result.halfDuration)), 0.01f);
		return result;
	}

	Config::FatigueOptions Config::FatigueOptions::FromIni(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const FatigueOptions& defaults) {
		FatigueOptions result = defaults;
		result.enabled = ini.GetBoolValue(a_section, fmt::format("b{}", prefix).c_str(), result.enabled);
		result.minThreshold = std::max(static_cast<float>(ini.GetDoubleValue(a_section, fmt::format("f{}MinThreshold", prefix).c_str(), result.minThreshold)), 0.0f);
		result.maxThreshold = std::max(static_cast<float>(ini.GetDoubleValue(a_section, fmt::format("f{}MaxThreshold", prefix).c_str(), result.maxThreshold)), 0.0f);
		result.maxSpreadMultiplier = std::max(static_cast<float>(ini.GetDoubleValue(a_section, fmt::format("f{}MaxPenalty", prefix).c_str(), result.maxSpreadMultiplier)), 0.0f);
		result.halfDuration = std::max(static_cast<float>(ini.GetDoubleValue(a_section, fmt::format("f{}HalfDuration", prefix).c_str(), result.halfDuration)), 0.01f);
		return result;
	}

	Config::SkillMultiplier Config::SkillMultiplier::FromIni(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const SkillMultiplier& defaults) {
		SkillMultiplier result = defaults;

		std::string bowOption = fmt::format("fBow{}SkillMultiplier", prefix);
		std::string crossbowOption = fmt::format("fCrossbow{}SkillMultiplier", prefix);
		std::string spellOption = fmt::format("fSpell{}SkillMultiplier", prefix);
		std::string concentrationSpellOption = fmt::format("fConcentrationSpell{}SkillMultiplier", prefix);
		std::string staffOption = fmt::format("fStaff{}SkillMultiplier", prefix);
		std::string scrollOption = fmt::format("fScroll{}SkillMultiplier", prefix);
		std::string blindnessOption = fmt::format("fBlindness{}SkillMultiplier", prefix);

		result.bow = ini.GetDoubleValue(a_section, bowOption.c_str(), result.bow);
		result.crossbow = ini.GetDoubleValue(a_section, crossbowOption.c_str(), result.crossbow);
		result.spell = ini.GetDoubleValue(a_section, spellOption.c_str(), result.spell);
		result.concentrationSpell = ini.GetDoubleValue(a_section, concentrationSpellOption.c_str(), result.concentrationSpell);
		result.staff = ini.GetDoubleValue(a_section, staffOption.c_str(), result.staff);
		result.scroll = ini.GetDoubleValue(a_section, scrollOption.c_str(), result.scroll);
		result.blindness = ini.GetDoubleValue(a_section, blindnessOption.c_str(), result.blindness);

		return result;
	}
}
