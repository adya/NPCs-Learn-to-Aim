#include "Options.h"
#include "CLIBUtil/string.hpp"
#include <cmath>
#include <fmt/ranges.h>

namespace NLA::Options
{
	namespace
	{
		template <typename T>
		void SetOrDeleteValue(CSimpleIniA&, const char*, const char*, const T&, const T&, bool writeDefaults = false) {}

		template <>
		void SetOrDeleteValue<bool>(CSimpleIniA& ini, const char* section, const char* key, const bool& value, const bool& defaultValue, bool writeDefaults) {
			if (writeDefaults || value != defaultValue) {
				ini.SetBoolValue(section, key, value);
			} else {
				ini.Delete(section, key, true);
			}
		}

		template <>
		void SetOrDeleteValue<float>(CSimpleIniA& ini, const char* section, const char* key, const float& value, const float& defaultValue, bool writeDefaults) {
			constexpr float kEpsilon = 0.001f;
			if (writeDefaults || std::abs(value - defaultValue) > kEpsilon) {
				ini.SetDoubleValue(section, key, value);
			} else {
				ini.Delete(section, key, true);
			}
		}

		void SetOrDeleteKeywords(CSimpleIniA& ini, const char* section, const char* key, const std::vector<std::string>& value, const std::vector<std::string>& defaultValue, bool writeDefaults = false) {
			if (writeDefaults || value != defaultValue) {
				ini.SetValue(section, key, fmt::format("{}", fmt::join(value, ",")).c_str());
			} else {
				ini.Delete(section, key, true);
			}
		}

		void SaveSkillMultiplier(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const Config::SkillMultiplier& value, const Config::SkillMultiplier& defaultValue, bool writeDefaults = false) {
			SetOrDeleteValue(ini, a_section, fmt::format("fBow{}SkillMultiplier", prefix).c_str(), value.bow, defaultValue.bow, writeDefaults);
			SetOrDeleteValue(ini, a_section, fmt::format("fCrossbow{}SkillMultiplier", prefix).c_str(), value.crossbow, defaultValue.crossbow, writeDefaults);
			SetOrDeleteValue(ini, a_section, fmt::format("fSpell{}SkillMultiplier", prefix).c_str(), value.spell, defaultValue.spell, writeDefaults);
			SetOrDeleteValue(ini, a_section, fmt::format("fConcentrationSpell{}SkillMultiplier", prefix).c_str(), value.concentrationSpell, defaultValue.concentrationSpell, writeDefaults);
			SetOrDeleteValue(ini, a_section, fmt::format("fStaff{}SkillMultiplier", prefix).c_str(), value.staff, defaultValue.staff, writeDefaults);
			SetOrDeleteValue(ini, a_section, fmt::format("fConcentrationStaff{}SkillMultiplier", prefix).c_str(), value.concentrationStaff, defaultValue.concentrationStaff, writeDefaults);
			SetOrDeleteValue(ini, a_section, fmt::format("fScroll{}SkillMultiplier", prefix).c_str(), value.scroll, defaultValue.scroll, writeDefaults);
			SetOrDeleteValue(ini, a_section, fmt::format("fBlindness{}SkillMultiplier", prefix).c_str(), value.blindness, defaultValue.blindness, writeDefaults);
		}

		void SaveHeadshotSettings(CSimpleIniA& ini, const char* a_section, const NPCConfig::HeadshotSettings& value, const NPCConfig::HeadshotSettings& defaultValue, bool writeDefaults = false) {
			SetOrDeleteValue(ini, a_section, "bBowHeadshots", value.bow, defaultValue.bow, writeDefaults);
			SetOrDeleteValue(ini, a_section, "fBowMinHeadshotChance", value.bowMinChance, defaultValue.bowMinChance, writeDefaults);
			SetOrDeleteValue(ini, a_section, "fBowMaxHeadshotChance", value.bowMaxChance, defaultValue.bowMaxChance, writeDefaults);
			SetOrDeleteValue(ini, a_section, "bCrossbowHeadshots", value.crossbow, defaultValue.crossbow, writeDefaults);
			SetOrDeleteValue(ini, a_section, "fCrossbowMinHeadshotChance", value.crossbowMinChance, defaultValue.crossbowMinChance, writeDefaults);
			SetOrDeleteValue(ini, a_section, "fCrossbowMaxHeadshotChance", value.crossbowMaxChance, defaultValue.crossbowMaxChance, writeDefaults);
		}

		void SaveSteadying(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const Config::Steadying& value, const Config::Steadying& defaultValue, bool writeDefaults = false) {
			SetOrDeleteValue(ini, a_section, fmt::format("b{}", prefix).c_str(), value.enabled, defaultValue.enabled, writeDefaults);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}MaxSpreadReduction", prefix).c_str(), value.maxSpreadMultiplier, defaultValue.maxSpreadMultiplier, writeDefaults);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}HalfDuration", prefix).c_str(), value.halfDuration, defaultValue.halfDuration, writeDefaults);
		}

		void SaveScalingOptions(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const Config::ScalingOptions& value, const Config::ScalingOptions& defaultValue, bool writeDefaults = false) {
			SetOrDeleteValue(ini, a_section, fmt::format("bScale{}WithSkill", prefix).c_str(), value.enabled, defaultValue.enabled, writeDefaults);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}MinMultiplier", prefix).c_str(), value.minMultiplier, defaultValue.minMultiplier, writeDefaults);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}MaxMultiplier", prefix).c_str(), value.maxMultiplier, defaultValue.maxMultiplier, writeDefaults);
		}

		void SaveFatigue(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const Config::FatigueOptions& value, const Config::FatigueOptions& defaultValue, bool writeDefaults = false) {
			SetOrDeleteValue(ini, a_section, fmt::format("b{}", prefix).c_str(), value.enabled, defaultValue.enabled, writeDefaults);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}MinThreshold", prefix).c_str(), value.minThreshold, defaultValue.minThreshold, writeDefaults);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}MaxThreshold", prefix).c_str(), value.maxThreshold, defaultValue.maxThreshold, writeDefaults);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}MaxPenalty", prefix).c_str(), value.maxSpreadMultiplier, defaultValue.maxSpreadMultiplier, writeDefaults);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}HalfDuration", prefix).c_str(), value.halfDuration, defaultValue.halfDuration, writeDefaults);
		}

		constexpr std::array<const char*, 3> kSkillSourceNames{ "SpellSkill", "Highest", "Enchanting" };

		const char* SkillSourceName(SkillSource source) {
			return kSkillSourceNames[std::min<std::size_t>(static_cast<std::size_t>(source), kSkillSourceNames.size() - 1)];
		}

		void SaveSkillSource(CSimpleIniA& ini, const char* a_section, const std::string& key, SkillSource value, SkillSource defaultValue, bool writeDefaults) {
			if (writeDefaults || value != defaultValue) {
				ini.SetValue(a_section, key.c_str(), SkillSourceName(value));
			} else {
				ini.Delete(a_section, key.c_str(), true);
			}
		}

		void SaveSkillSources(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const Config::SkillSources& value, const Config::SkillSources& defaultValue, bool writeDefaults = false) {
			SaveSkillSource(ini, a_section, fmt::format("s{}AimingSkill", prefix), value.aiming, defaultValue.aiming, writeDefaults);
			SaveSkillSource(ini, a_section, fmt::format("s{}ChargeSpeedSkill", prefix), value.chargeSpeed, defaultValue.chargeSpeed, writeDefaults);
			if (prefix.starts_with("Concentration")) {
				SaveSkillSource(ini, a_section, fmt::format("s{}RangeSkill", prefix), value.range, defaultValue.range, writeDefaults);
			}
		}

		void SaveConfig(CSimpleIniA& ini, const char* a_section, const Config& custom, const Config& defaults, bool writeDefaults = false) {
			SetOrDeleteValue(ini, a_section, "bEnabled", custom.enabled, defaults.enabled, writeDefaults);

			SetOrDeleteValue(ini, a_section, "bSpellAiming", custom.spellAiming, defaults.spellAiming, writeDefaults);
			SetOrDeleteValue(ini, a_section, "bStaffAiming", custom.staffAiming, defaults.staffAiming, writeDefaults);
			SetOrDeleteValue(ini, a_section, "bScrollAiming", custom.scrollAiming, defaults.scrollAiming, writeDefaults);
			SetOrDeleteValue(ini, a_section, "bBowAiming", custom.bowAiming, defaults.bowAiming, writeDefaults);
			SetOrDeleteValue(ini, a_section, "bCrossbowAiming", custom.crossbowAiming, defaults.crossbowAiming, writeDefaults);

			SaveSkillMultiplier(ini, a_section, "Accuracy", custom.accuracyMultipliers, defaults.accuracyMultipliers, writeDefaults);
			SaveSkillMultiplier(ini, a_section, "Precision", custom.precisionMultipliers, defaults.precisionMultipliers, writeDefaults);

			SetOrDeleteValue(ini, a_section, "bBlindnessAffectsAiming", custom.blindnessAffectsAiming, defaults.blindnessAffectsAiming, writeDefaults);
			SaveSkillSources(ini, a_section, "Spell", custom.spellSkills, defaults.spellSkills, writeDefaults);
			SaveSkillSources(ini, a_section, "ConcentrationSpell", custom.concentrationSpellSkills, defaults.concentrationSpellSkills, writeDefaults);
			SaveSkillSources(ini, a_section, "Staff", custom.staffSkills, defaults.staffSkills, writeDefaults);
			SaveSkillSources(ini, a_section, "ConcentrationStaff", custom.concentrationStaffSkills, defaults.concentrationStaffSkills, writeDefaults);
			SaveSkillSources(ini, a_section, "Scroll", custom.scrollSkills, defaults.scrollSkills, writeDefaults);

			SetOrDeleteValue(ini, a_section, "bConcentrationSpellsAiming", custom.concentrationSpellsAiming, defaults.concentrationSpellsAiming, writeDefaults);
			SaveScalingOptions(ini, a_section, "ConcentrationSpellRange", custom.concentrationSpellRange, defaults.concentrationSpellRange, writeDefaults);
			SaveScalingOptions(ini, a_section, "ConcentrationStaffRange", custom.concentrationStaffRange, defaults.concentrationStaffRange, writeDefaults);

			SetOrDeleteValue(ini, a_section, "bConcentrationStaffAiming", custom.concentrationStaffAiming, defaults.concentrationStaffAiming, writeDefaults);

			SaveScalingOptions(ini, a_section, "SpellChargeSpeed", custom.spellChargeSpeed, defaults.spellChargeSpeed, writeDefaults);

			SaveSteadying(ini, a_section, "SpellSteadying", custom.spellSteadying, defaults.spellSteadying, writeDefaults);
			SaveSteadying(ini, a_section, "StaffSteadying", custom.staffSteadying, defaults.staffSteadying, writeDefaults);
			SaveSteadying(ini, a_section, "ScrollSteadying", custom.scrollSteadying, defaults.scrollSteadying, writeDefaults);

			SaveFatigue(ini, a_section, "SpellFatigue", custom.spellFatigue, defaults.spellFatigue, writeDefaults);
			SaveFatigue(ini, a_section, "StaffFatigue", custom.staffFatigue, defaults.staffFatigue, writeDefaults);
			SaveFatigue(ini, a_section, "ScrollFatigue", custom.scrollFatigue, defaults.scrollFatigue, writeDefaults);
			SaveFatigue(ini, a_section, "ConcentrationSpellFatigue", custom.concentrationSpellFatigue, defaults.concentrationSpellFatigue, writeDefaults);
			SaveFatigue(ini, a_section, "ConcentrationStaffFatigue", custom.concentrationStaffFatigue, defaults.concentrationStaffFatigue, writeDefaults);

			SaveScalingOptions(ini, a_section, "ConcentrationSpellChargeSpeed", custom.concentrationSpellChargeSpeed, defaults.concentrationSpellChargeSpeed, writeDefaults);
			SaveScalingOptions(ini, a_section, "StaffChargeSpeed", custom.staffChargeSpeed, defaults.staffChargeSpeed, writeDefaults);
			SaveScalingOptions(ini, a_section, "ConcentrationStaffChargeSpeed", custom.concentrationStaffChargeSpeed, defaults.concentrationStaffChargeSpeed, writeDefaults);
			SaveScalingOptions(ini, a_section, "ScrollChargeSpeed", custom.scrollChargeSpeed, defaults.scrollChargeSpeed, writeDefaults);

			SaveScalingOptions(ini, a_section, "BowDrawSpeed", custom.bowDrawSpeed, defaults.bowDrawSpeed, writeDefaults);
			SaveScalingOptions(ini, a_section, "CrossbowDrawSpeed", custom.crossbowDrawSpeed, defaults.crossbowDrawSpeed, writeDefaults);
			SaveSteadying(ini, a_section, "BowSteadying", custom.bowSteadying, defaults.bowSteadying, writeDefaults);
			SaveFatigue(ini, a_section, "BowFatigue", custom.bowFatigue, defaults.bowFatigue, writeDefaults);
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

	inline void LogSkillSources(std::string_view actor, std::string_view name, const Config::SkillSources& sources, bool hasRange) {
		const auto describe = [&](SkillSource source) -> std::string {
			switch (source) {
			case SkillSource::kEnchanting:
				return "Enchanting skill";
			case SkillSource::kHighest:
				return fmt::format("the highest {}'s magic skill regardless of spell", actor);
			default:
				return "skill in spell's magic school";
			}
		};
		logger::info("	{}'s aiming with {} is based on {}", actor, name, describe(sources.aiming));
		logger::info("	{}'s charge speed with {} is based on {}", actor, name, describe(sources.chargeSpeed));
		if (hasRange) {
			logger::info("	{}'s range with {} is based on {}", actor, name, describe(sources.range));
		}
	}

	inline void LogConcentrationSpellRangeMultiplier(std::string_view actor, bool enabled, float minMult, float maxMult) {
		if (!enabled) {
			logger::info("	{} always have vanilla reach with concentration spells", actor);
		} else {
			logger::info("	{} reach concentration spells at {:.2f}x to {:.2f}x vanilla range, scaling with skill", actor, minMult, maxMult);
		}
	}

	inline void LogConcentrationStaffRangeMultiplier(std::string_view actor, bool enabled, float minMult, float maxMult) {
		if (!enabled) {
			logger::info("	{} always have vanilla reach with concentration staves", actor);
		} else {
			logger::info("	{} reach concentration staves at {:.2f}x to {:.2f}x vanilla range, scaling with skill", actor, minMult, maxMult);
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

	inline void LogSpellSteadying(std::string_view actor, std::string_view categoryName, const Config::Steadying& steadying) {
		if (!steadying.enabled) {
			logger::info("\tHolding {} charged doesn't affect {}'s precision", categoryName, actor);
		} else {
			logger::info("\tHolding {} charged reduces {}'s spread by up to {:.0f}% (half of it after {:.2f}s)", categoryName, actor, steadying.maxSpreadMultiplier * 100, steadying.halfDuration);
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

	inline void LogBowFatigue(std::string_view actor, const Config::FatigueOptions& fatigue) {
		if (!fatigue.enabled) {
			logger::info("\tHolding a bow too long doesn't fatigue {}'s aim", actor);
		} else {
			logger::info("\t{} can hold a full draw for {:.2f}s at 0 max Stamina to {:.2f}s at 100 max Stamina before fatigue sets in, "
			             "growing spread back by up to {:.0f}% (half of that after another {:.2f}s)",
			             actor, fatigue.minThreshold, fatigue.maxThreshold, fatigue.maxSpreadMultiplier * 100, fatigue.halfDuration);
		}
	}

	inline void LogCastFatigue(std::string_view actor, std::string_view categoryName, const Config::FatigueOptions& fatigue) {
		if (!fatigue.enabled) {
			logger::info("\tHolding {} charged too long doesn't fatigue {}'s precision", categoryName, actor);
		} else {
			logger::info("\t{} can hold {} charged for {:.2f}s at 0 max Magicka to {:.2f}s at 100 max Magicka before fatigue sets in, "
			             "growing spread back by up to {:.0f}% (half of that after another {:.2f}s)",
			             actor, categoryName, fatigue.minThreshold, fatigue.maxThreshold, fatigue.maxSpreadMultiplier * 100, fatigue.halfDuration);
		}
	}

	inline void LogConcentrationFatigue(std::string_view actor, std::string_view categoryName, const Config::FatigueOptions& fatigue) {
		if (!fatigue.enabled) {
			logger::info("\tCasting {} for too long doesn't fatigue {}'s precision", categoryName, actor);
		} else {
			logger::info("\t{} can cast {} for {:.2f}s at 0 max Magicka to {:.2f}s at 100 max Magicka before fatigue sets in, "
			             "growing spread by up to {:.0f}% (half of that after another {:.2f}s)",
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
		logger::info("\tMages {} with concentration staves", NPC.concentrationStaffAiming ? "will learn to aim" : "have perfect aim");
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
		LogSkillMultiplier("NPCs", "Concentration Staves", "Accuracy", NPC.accuracyMultipliers.concentrationStaff);
		LogSkillMultiplier("NPCs", "Scrolls", "Accuracy", NPC.accuracyMultipliers.scroll);
		LogBlindnessSkillMultiplier("NPCs", "accuracy", NPC.accuracyMultipliers.blindness);

		logger::info("");
		LogSkillMultiplier("NPCs", "Bows", "Precision", NPC.precisionMultipliers.bow);
		LogSkillMultiplier("NPCs", "Crossbows", "Precision", NPC.precisionMultipliers.crossbow);
		LogSkillMultiplier("NPCs", "Spells", "Precision", NPC.precisionMultipliers.spell);
		LogSkillMultiplier("NPCs", "Concentration Spells", "Precision", NPC.precisionMultipliers.concentrationSpell);
		LogSkillMultiplier("NPCs", "Staves", "Precision", NPC.precisionMultipliers.staff);
		LogSkillMultiplier("NPCs", "Concentration Staves", "Precision", NPC.precisionMultipliers.concentrationStaff);
		LogSkillMultiplier("NPCs", "Scrolls", "Precision", NPC.precisionMultipliers.scroll);
		LogBlindnessSkillMultiplier("NPCs", "precision", NPC.precisionMultipliers.blindness);

		logger::info("");
		LogSkillSources("NPCs", "spells", NPC.spellSkills, false);
		LogSkillSources("NPCs", "concentration spells", NPC.concentrationSpellSkills, true);
		LogSkillSources("NPCs", "staves", NPC.staffSkills, false);
		LogSkillSources("NPCs", "concentration staves", NPC.concentrationStaffSkills, true);
		LogSkillSources("NPCs", "scrolls", NPC.scrollSkills, false);
		LogConcentrationSpellRangeMultiplier("NPCs", NPC.concentrationSpellRange.enabled, NPC.concentrationSpellRange.minMultiplier, NPC.concentrationSpellRange.maxMultiplier);
		LogConcentrationStaffRangeMultiplier("NPCs", NPC.concentrationStaffRange.enabled, NPC.concentrationStaffRange.minMultiplier, NPC.concentrationStaffRange.maxMultiplier);

		logger::info("");
		LogChargeSpeedMultiplier("NPCs", "spells", NPC.spellChargeSpeed.enabled, NPC.spellChargeSpeed.minMultiplier, NPC.spellChargeSpeed.maxMultiplier);
		LogChargeSpeedMultiplier("NPCs", "concentration spells", NPC.concentrationSpellChargeSpeed.enabled, NPC.concentrationSpellChargeSpeed.minMultiplier, NPC.concentrationSpellChargeSpeed.maxMultiplier);
		LogChargeSpeedMultiplier("NPCs", "staves", NPC.staffChargeSpeed.enabled, NPC.staffChargeSpeed.minMultiplier, NPC.staffChargeSpeed.maxMultiplier);
		LogChargeSpeedMultiplier("NPCs", "concentration staves", NPC.concentrationStaffChargeSpeed.enabled, NPC.concentrationStaffChargeSpeed.minMultiplier, NPC.concentrationStaffChargeSpeed.maxMultiplier);
		LogChargeSpeedMultiplier("NPCs", "scrolls", NPC.scrollChargeSpeed.enabled, NPC.scrollChargeSpeed.minMultiplier, NPC.scrollChargeSpeed.maxMultiplier);
		LogDrawSpeedMultiplier("NPCs", "bows", NPC.bowDrawSpeed.enabled, NPC.bowDrawSpeed.minMultiplier, NPC.bowDrawSpeed.maxMultiplier);
		LogDrawSpeedMultiplier("NPCs", "crossbows", NPC.crossbowDrawSpeed.enabled, NPC.crossbowDrawSpeed.minMultiplier, NPC.crossbowDrawSpeed.maxMultiplier);
		LogSpellSteadying("NPCs", "spells", NPC.spellSteadying);
		LogSpellSteadying("NPCs", "staves", NPC.staffSteadying);
		LogSpellSteadying("NPCs", "scrolls", NPC.scrollSteadying);
		LogCastFatigue("NPCs", "spells", NPC.spellFatigue);
		LogCastFatigue("NPCs", "staves", NPC.staffFatigue);
		LogCastFatigue("NPCs", "scrolls", NPC.scrollFatigue);
		LogConcentrationFatigue("NPCs", "concentration spells", NPC.concentrationSpellFatigue);
		LogConcentrationFatigue("NPCs", "concentration staves", NPC.concentrationStaffFatigue);
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
		logger::info("\t{} with concentration staves", Player.concentrationStaffAiming ? "Will learn to aim" : "Has perfect aim");

		logger::info("");
		LogSkillMultiplier("Player", "Bows", "Precision", Player.precisionMultipliers.bow);
		LogSkillMultiplier("Player", "Crossbows", "Precision", Player.precisionMultipliers.crossbow);
		LogSkillMultiplier("Player", "Spells", "Precision", Player.precisionMultipliers.spell);
		LogSkillMultiplier("Player", "Concentration Spells", "Precision", Player.precisionMultipliers.concentrationSpell);
		LogSkillMultiplier("Player", "Staves", "Precision", Player.precisionMultipliers.staff);
		LogSkillMultiplier("Player", "Concentration Staves", "Precision", Player.precisionMultipliers.concentrationStaff);
		LogSkillMultiplier("Player", "Scrolls", "Precision", Player.precisionMultipliers.scroll);

		logger::info("");
		LogSkillSources("Player", "spells", Player.spellSkills, false);
		LogSkillSources("Player", "concentration spells", Player.concentrationSpellSkills, true);
		LogSkillSources("Player", "staves", Player.staffSkills, false);
		LogSkillSources("Player", "concentration staves", Player.concentrationStaffSkills, true);
		LogSkillSources("Player", "scrolls", Player.scrollSkills, false);
		LogConcentrationSpellRangeMultiplier("Player", Player.concentrationSpellRange.enabled, Player.concentrationSpellRange.minMultiplier, Player.concentrationSpellRange.maxMultiplier);
		LogConcentrationStaffRangeMultiplier("Player", Player.concentrationStaffRange.enabled, Player.concentrationStaffRange.minMultiplier, Player.concentrationStaffRange.maxMultiplier);

		logger::info("");
		LogChargeSpeedMultiplier("Player", "spells", Player.spellChargeSpeed.enabled, Player.spellChargeSpeed.minMultiplier, Player.spellChargeSpeed.maxMultiplier);
		LogChargeSpeedMultiplier("Player", "concentration spells", Player.concentrationSpellChargeSpeed.enabled, Player.concentrationSpellChargeSpeed.minMultiplier, Player.concentrationSpellChargeSpeed.maxMultiplier);
		LogChargeSpeedMultiplier("Player", "staves", Player.staffChargeSpeed.enabled, Player.staffChargeSpeed.minMultiplier, Player.staffChargeSpeed.maxMultiplier);
		LogChargeSpeedMultiplier("Player", "concentration staves", Player.concentrationStaffChargeSpeed.enabled, Player.concentrationStaffChargeSpeed.minMultiplier, Player.concentrationStaffChargeSpeed.maxMultiplier);
		LogChargeSpeedMultiplier("Player", "scrolls", Player.scrollChargeSpeed.enabled, Player.scrollChargeSpeed.minMultiplier, Player.scrollChargeSpeed.maxMultiplier);
		LogDrawSpeedMultiplier("Player", "bows", Player.bowDrawSpeed.enabled, Player.bowDrawSpeed.minMultiplier, Player.bowDrawSpeed.maxMultiplier);
		LogDrawSpeedMultiplier("Player", "crossbows", Player.crossbowDrawSpeed.enabled, Player.crossbowDrawSpeed.minMultiplier, Player.crossbowDrawSpeed.maxMultiplier);
		LogSpellSteadying("Player", "spells", Player.spellSteadying);
		LogSpellSteadying("Player", "staves", Player.staffSteadying);
		LogSpellSteadying("Player", "scrolls", Player.scrollSteadying);
		LogCastFatigue("Player", "spells", Player.spellFatigue);
		LogCastFatigue("Player", "staves", Player.staffFatigue);
		LogCastFatigue("Player", "scrolls", Player.scrollFatigue);
		LogConcentrationFatigue("Player", "concentration spells", Player.concentrationSpellFatigue);
		LogConcentrationFatigue("Player", "concentration staves", Player.concentrationStaffFatigue);
		LogBowSteadying("Player", Player.bowSteadying);
		LogBowFatigue("Player", Player.bowFatigue);

		logger::info("{:*^40}", "");
	}

	namespace
	{
		void SaveAll(CSimpleIniA& ini, bool writeDefaults = false) {
			SaveConfig(ini, "NPC", NPC, NPCConfig::defaultConfig, writeDefaults);
			SaveConfig(ini, "Player", Player, PlayerConfig::defaultConfig, writeDefaults);

			SaveHeadshotSettings(ini, "NPC", NPC.headshots, NPCConfig::defaultConfig.headshots, writeDefaults);

			SetOrDeleteKeywords(ini, "NPC", "sExcludeKeywords", NPC.excludeKeywords, NPCConfig::defaultConfig.excludeKeywords, writeDefaults);
			SetOrDeleteKeywords(ini, "NPC", "sIncludeKeywords", NPC.includeKeywords, NPCConfig::defaultConfig.includeKeywords, writeDefaults);
		}
	}

	void Save() {
		const std::filesystem::path options = R"(Data\SKSE\Plugins\NPCsLearnToAim.ini)";
		CSimpleIniA                 ini{};
		ini.SetUnicode();
		ini.LoadFile(options.string().c_str());

		SaveAll(ini);

		ini.SaveFile(options.string().c_str());
	}

	void Export() {
		const std::filesystem::path options = R"(Data\SKSE\Plugins\NPCsLearnToAim.ini)";
		CSimpleIniA                 ini{};
		ini.SetUnicode();
		ini.LoadFile(options.string().c_str());

		SaveAll(ini, true);

		ini.SaveFile(options.string().c_str());
	}

	void ResetNPC() {
		NPC = NPCConfig::defaultConfig;
		++currentID;
		Save();
	}

	void ResetPlayer() {
		Player = PlayerConfig::defaultConfig;
		++currentID;
		Save();
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

		result.spellSkills = SkillSources::FromIni(ini, a_section, "Spell", result.spellSkills);
		result.concentrationSpellSkills = SkillSources::FromIni(ini, a_section, "ConcentrationSpell", result.concentrationSpellSkills);
		result.staffSkills = SkillSources::FromIni(ini, a_section, "Staff", result.staffSkills);
		result.concentrationStaffSkills = SkillSources::FromIni(ini, a_section, "ConcentrationStaff", result.concentrationStaffSkills);
		result.scrollSkills = SkillSources::FromIni(ini, a_section, "Scroll", result.scrollSkills);



		result.concentrationSpellRange = ScalingOptions::FromIni(ini, a_section, "ConcentrationSpellRange", result.concentrationSpellRange);
		result.concentrationStaffRange = ScalingOptions::FromIni(ini, a_section, "ConcentrationStaffRange", result.concentrationStaffRange);

		result.concentrationStaffAiming = ini.GetBoolValue(a_section, "bConcentrationStaffAiming", result.concentrationStaffAiming);

		result.spellChargeSpeed = ScalingOptions::FromIni(ini, a_section, "SpellChargeSpeed", result.spellChargeSpeed);
		result.concentrationSpellChargeSpeed = ScalingOptions::FromIni(ini, a_section, "ConcentrationSpellChargeSpeed", result.concentrationSpellChargeSpeed);
		result.staffChargeSpeed = ScalingOptions::FromIni(ini, a_section, "StaffChargeSpeed", result.staffChargeSpeed);
		result.concentrationStaffChargeSpeed = ScalingOptions::FromIni(ini, a_section, "ConcentrationStaffChargeSpeed", result.concentrationStaffChargeSpeed);
		result.scrollChargeSpeed = ScalingOptions::FromIni(ini, a_section, "ScrollChargeSpeed", result.scrollChargeSpeed);

		result.spellSteadying = SpreadOptions::FromIni(ini, a_section, "SpellSteadying", result.spellSteadying);
		result.staffSteadying = SpreadOptions::FromIni(ini, a_section, "StaffSteadying", result.staffSteadying);
		result.scrollSteadying = SpreadOptions::FromIni(ini, a_section, "ScrollSteadying", result.scrollSteadying);

		result.spellFatigue = FatigueOptions::FromIni(ini, a_section, "SpellFatigue", result.spellFatigue);
		result.staffFatigue = FatigueOptions::FromIni(ini, a_section, "StaffFatigue", result.staffFatigue);
		result.scrollFatigue = FatigueOptions::FromIni(ini, a_section, "ScrollFatigue", result.scrollFatigue);
		result.concentrationSpellFatigue = FatigueOptions::FromIni(ini, a_section, "ConcentrationSpellFatigue", result.concentrationSpellFatigue);
		result.concentrationStaffFatigue = FatigueOptions::FromIni(ini, a_section, "ConcentrationStaffFatigue", result.concentrationStaffFatigue);

		result.bowDrawSpeed = ScalingOptions::FromIni(ini, a_section, "BowDrawSpeed", result.bowDrawSpeed);
		result.crossbowDrawSpeed = ScalingOptions::FromIni(ini, a_section, "CrossbowDrawSpeed", result.crossbowDrawSpeed);
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

	Config::SkillSources Config::SkillSources::FromIni(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const SkillSources& defaults) {
		const auto read = [&](const std::string& key, SkillSource fallback) {
			const char* value = ini.GetValue(a_section, key.c_str(), nullptr);
			if (!value) {
				return fallback;
			}
			for (std::size_t i = 0; i < kSkillSourceNames.size(); ++i) {
				if (clib_util::string::iequals(value, kSkillSourceNames[i])) {
					return static_cast<SkillSource>(i);
				}
			}
			logger::warn("Unknown skill source '{}' for {}, using the default", value, key);
			return fallback;
		};

		SkillSources result = defaults;
		result.aiming = read(fmt::format("s{}AimingSkill", prefix), result.aiming);
		result.chargeSpeed = read(fmt::format("s{}ChargeSpeedSkill", prefix), result.chargeSpeed);
		result.range = read(fmt::format("s{}RangeSkill", prefix), result.range);
		return result;
	}

	Config::ScalingOptions Config::ScalingOptions::FromIni(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const ScalingOptions& defaults) {
		ScalingOptions result = defaults;
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
		std::string concentrationStaffOption = fmt::format("fConcentrationStaff{}SkillMultiplier", prefix);
		std::string scrollOption = fmt::format("fScroll{}SkillMultiplier", prefix);
		std::string blindnessOption = fmt::format("fBlindness{}SkillMultiplier", prefix);

		result.bow = ini.GetDoubleValue(a_section, bowOption.c_str(), result.bow);
		result.crossbow = ini.GetDoubleValue(a_section, crossbowOption.c_str(), result.crossbow);
		result.spell = ini.GetDoubleValue(a_section, spellOption.c_str(), result.spell);
		result.concentrationSpell = ini.GetDoubleValue(a_section, concentrationSpellOption.c_str(), result.concentrationSpell);
		result.staff = ini.GetDoubleValue(a_section, staffOption.c_str(), result.staff);
		result.concentrationStaff = ini.GetDoubleValue(a_section, concentrationStaffOption.c_str(), result.concentrationStaff);
		result.scroll = ini.GetDoubleValue(a_section, scrollOption.c_str(), result.scroll);
		result.blindness = ini.GetDoubleValue(a_section, blindnessOption.c_str(), result.blindness);

		return result;
	}
}
