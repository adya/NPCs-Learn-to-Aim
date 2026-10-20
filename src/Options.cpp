#include "Options.h"
#include "CLIBUtil/string.hpp"
#include <cmath>
#include <fmt/ranges.h>
#include <unordered_map>

namespace NLA::Options
{
	namespace
	{
		template <typename T>
		void SetOrDeleteValue(CSimpleIniA&, const char*, const char*, const T&, const T&, bool writeDefaults = false) = delete;

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

		float ReadClamped(CSimpleIniA& ini, const char* section, const std::string& key, float fallback, Limit limit) {
			const auto  raw = static_cast<float>(ini.GetDoubleValue(section, key.c_str(), fallback));
			const float clamped = std::clamp(raw, limit.min, limit.max);
			if (clamped != raw) {
				logger::warn("[{}] {} = {} is outside the allowed range {}..{}, using {}", section, key, raw, limit.min, limit.max, clamped);
			}
			return clamped;
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

		// The order of options here and in Config::FromIni follows the order of Config's members.
		void SaveConfig(CSimpleIniA& ini, const char* a_section, const Config& custom, const Config& defaults, bool writeDefaults = false) {
			SetOrDeleteValue(ini, a_section, "bEnabled", custom.enabled, defaults.enabled, writeDefaults);

			SetOrDeleteValue(ini, a_section, "bSpellAiming", custom.spellAiming, defaults.spellAiming, writeDefaults);
			SetOrDeleteValue(ini, a_section, "bStaffAiming", custom.staffAiming, defaults.staffAiming, writeDefaults);
			SetOrDeleteValue(ini, a_section, "bConcentrationSpellsAiming", custom.concentrationSpellAiming, defaults.concentrationSpellAiming, writeDefaults);
			SetOrDeleteValue(ini, a_section, "bConcentrationStaffAiming", custom.concentrationStaffAiming, defaults.concentrationStaffAiming, writeDefaults);
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

			SaveScalingOptions(ini, a_section, "ConcentrationSpellRange", custom.concentrationSpellRange, defaults.concentrationSpellRange, writeDefaults);
			SaveScalingOptions(ini, a_section, "ConcentrationStaffRange", custom.concentrationStaffRange, defaults.concentrationStaffRange, writeDefaults);

			SaveScalingOptions(ini, a_section, "SpellChargeSpeed", custom.spellChargeSpeed, defaults.spellChargeSpeed, writeDefaults);
			SaveScalingOptions(ini, a_section, "ConcentrationSpellChargeSpeed", custom.concentrationSpellChargeSpeed, defaults.concentrationSpellChargeSpeed, writeDefaults);
			SaveScalingOptions(ini, a_section, "StaffChargeSpeed", custom.staffChargeSpeed, defaults.staffChargeSpeed, writeDefaults);
			SaveScalingOptions(ini, a_section, "ConcentrationStaffChargeSpeed", custom.concentrationStaffChargeSpeed, defaults.concentrationStaffChargeSpeed, writeDefaults);
			SaveScalingOptions(ini, a_section, "ScrollChargeSpeed", custom.scrollChargeSpeed, defaults.scrollChargeSpeed, writeDefaults);

			SaveSteadying(ini, a_section, "SpellSteadying", custom.spellSteadying, defaults.spellSteadying, writeDefaults);
			SaveSteadying(ini, a_section, "StaffSteadying", custom.staffSteadying, defaults.staffSteadying, writeDefaults);
			SaveSteadying(ini, a_section, "ScrollSteadying", custom.scrollSteadying, defaults.scrollSteadying, writeDefaults);

			SaveFatigue(ini, a_section, "SpellFatigue", custom.spellFatigue, defaults.spellFatigue, writeDefaults);
			SaveFatigue(ini, a_section, "StaffFatigue", custom.staffFatigue, defaults.staffFatigue, writeDefaults);
			SaveFatigue(ini, a_section, "ScrollFatigue", custom.scrollFatigue, defaults.scrollFatigue, writeDefaults);
			SaveFatigue(ini, a_section, "ConcentrationSpellFatigue", custom.concentrationSpellFatigue, defaults.concentrationSpellFatigue, writeDefaults);
			SaveFatigue(ini, a_section, "ConcentrationStaffFatigue", custom.concentrationStaffFatigue, defaults.concentrationStaffFatigue, writeDefaults);

			SaveScalingOptions(ini, a_section, "BowDrawSpeed", custom.bowDrawSpeed, defaults.bowDrawSpeed, writeDefaults);
			SaveScalingOptions(ini, a_section, "CrossbowDrawSpeed", custom.crossbowDrawSpeed, defaults.crossbowDrawSpeed, writeDefaults);
			SaveSteadying(ini, a_section, "BowSteadying", custom.bowSteadying, defaults.bowSteadying, writeDefaults);
			SaveFatigue(ini, a_section, "BowFatigue", custom.bowFatigue, defaults.bowFatigue, writeDefaults);
			SaveSteadying(ini, a_section, "CrossbowSteadying", custom.crossbowSteadying, defaults.crossbowSteadying, writeDefaults);
			SaveFatigue(ini, a_section, "CrossbowFatigue", custom.crossbowFatigue, defaults.crossbowFatigue, writeDefaults);
		}

		void LogSkillMultiplier(std::string_view actor, std::string_view weaponName, std::string_view stage, float mult) {
			if (mult > 1) {
				auto percent = (mult - 1) * 100;
				logger::info("\t{} with {} is {:.2f}% easier for {}", stage, weaponName, percent, actor);
			} else if (mult < 1) {
				auto percent = (1 - mult) * 100;
				logger::info("\t{} with {} is {:.2f}% harder for {}", stage, weaponName, percent, actor);
			} else {
				logger::info("\t{} with {} is at standard difficulty for {}", stage, weaponName, actor);
			}
		}

		void LogBlindnessSkillMultiplier(std::string_view actor, std::string_view stage, float mult) {
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

		/// Logs how every weapon type's multiplier changes the difficulty of `stage` (Accuracy/Precision).
		void LogSkillMultipliers(std::string_view actor, std::string_view stage, const Config::SkillMultiplier& multipliers, bool blindnessAffectsAiming) {
			LogSkillMultiplier(actor, "Bows", stage, multipliers.bow);
			LogSkillMultiplier(actor, "Crossbows", stage, multipliers.crossbow);
			LogSkillMultiplier(actor, "Spells", stage, multipliers.spell);
			LogSkillMultiplier(actor, "Concentration Spells", stage, multipliers.concentrationSpell);
			LogSkillMultiplier(actor, "Staves", stage, multipliers.staff);
			LogSkillMultiplier(actor, "Concentration Staves", stage, multipliers.concentrationStaff);
			LogSkillMultiplier(actor, "Scrolls", stage, multipliers.scroll);
			const char* noun = stage == "Accuracy" ? "accuracy" : "precision";
			if (blindnessAffectsAiming) {
				LogBlindnessSkillMultiplier(actor, noun, multipliers.blindness);
			} else {
				logger::info("\tBlindness doesn't affect {}'s {}", actor, noun);
			}
		}

		void LogSkillSources(std::string_view actor, std::string_view name, const Config::SkillSources& sources, bool hasRange) {
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
			logger::info("\t{}'s aiming with {} is based on {}", actor, name, describe(sources.aiming));
			logger::info("\t{}'s charge speed with {} is based on {}", actor, name, describe(sources.chargeSpeed));
			if (hasRange) {
				logger::info("\t{}'s range with {} is based on {}", actor, name, describe(sources.range));
			}
		}

		void LogRange(std::string_view actor, std::string_view categoryName, const Config::RangeScaling& range) {
			if (!range.enabled) {
				logger::info("\t{} always have vanilla reach with {}", actor, categoryName);
			} else {
				logger::info("\t{} reach {} at {:.2f}x to {:.2f}x vanilla range, scaling with skill", actor, categoryName, range.minMultiplier, range.maxMultiplier);
			}
		}

		std::string SpeedPercentPhrase(float mult) {
			if (mult > 1) {
				return fmt::format("{:.2f}% faster", (mult - 1) * 100);
			} else if (mult < 1) {
				return fmt::format("{:.2f}% slower", (1 - mult) * 100);
			} else {
				return "at vanilla speed";
			}
		}

		/// `verb` is what the actor does with the category, e.g. "draw" or "charge/cast".
		void LogSpeed(std::string_view actor, std::string_view verb, std::string_view categoryName, const Config::ScalingOptions& speed) {
			const auto [enabled, minMult, maxMult] = std::tie(speed.enabled, speed.minMultiplier, speed.maxMultiplier);
			if (!enabled) {
				logger::info("\t{} always {} {} at vanilla speed", actor, verb, categoryName);
			} else if (minMult == maxMult) {
				logger::info("\t{} {} {} {}, regardless of skill", actor, verb, categoryName, SpeedPercentPhrase(minMult));
			} else if (minMult == 1.0f) {
				logger::info("\t{} {} {} up to {}, scaling with skill", actor, verb, categoryName, SpeedPercentPhrase(maxMult));
			} else {
				logger::info("\t{} {} {} {} at 0 skill, {} at 100 skill", actor, verb, categoryName, SpeedPercentPhrase(minMult), SpeedPercentPhrase(maxMult));
			}
		}

		/// `held` is what is being held, e.g. "a bow at full draw" or "spells charged".
		void LogSteadying(std::string_view actor, std::string_view held, const Config::Steadying& steadying) {
			if (!steadying.enabled) {
				logger::info("\tHolding {} doesn't affect {}'s precision", held, actor);
			} else {
				logger::info("\tHolding {} reduces {}'s spread by up to {:.0f}% (half of it after {:.2f}s)", held, actor, steadying.maxSpreadMultiplier * 100, steadying.halfDuration);
			}
		}

		/// `gerund` and `infinitive` describe the fatigued activity, e.g. "Holding a bow at full draw" and "hold a full draw".
		void LogFatigue(std::string_view actor, std::string_view gerund, std::string_view infinitive, std::string_view attribute, const Config::FatigueOptions& fatigue) {
			if (!fatigue.enabled) {
				logger::info("\t{} for too long doesn't fatigue {}'s aim", gerund, actor);
			} else {
				logger::info(
					"\t{} can {} for {:.2f}s at 0 max {} to {:.2f}s at 100 max {} before fatigue sets in, "
					"adding up to {:.0f}% spread (half of that after another {:.2f}s)",
					actor,
					infinitive,
					fatigue.minThreshold,
					attribute,
					fatigue.maxThreshold,
					attribute,
					fatigue.maxSpreadMultiplier * 100,
					fatigue.halfDuration);
			}
		}

		void LogHeadshots(const NPCConfig::HeadshotSettings& headshots) {
			if (headshots.bow) {
				logger::info("\tSkilled archers may go for headshots ({:.0f}%-{:.0f}% chance)", headshots.bowMinChance * 100, headshots.bowMaxChance * 100);
			} else {
				logger::info("\tArchers always aim for the chest");
			}
			if (headshots.crossbow) {
				logger::info("\tSkilled crossbowmen may go for headshots ({:.0f}%-{:.0f}% chance)", headshots.crossbowMinChance * 100, headshots.crossbowMaxChance * 100);
			} else {
				logger::info("\tCrossbowmen always aim for the chest");
			}
		}

		void LogKeywordFilters(const NPCConfig& npc) {
			const bool include = !npc.includeKeywords.empty();
			const bool exclude = !npc.excludeKeywords.empty();

			if (include && exclude) {
				logger::info("\tOnly NPCs that have at least one of these keywords: [{}] and none of these: [{}] will learn to aim", fmt::join(npc.includeKeywords, ", "), fmt::join(npc.excludeKeywords, ", "));
			} else if (include) {
				logger::info("\tOnly NPCs that have at least one of these keywords: [{}] will learn to aim", fmt::join(npc.includeKeywords, ", "));
			} else if (exclude) {
				logger::info("\tOnly NPCs that have none of these keywords: [{}] will learn to aim", fmt::join(npc.excludeKeywords, ", "));
			} else {
				logger::info("\tAll NPCs will learn to aim");
			}
		}

		/// Logs everything the config changes. Accuracy and headshots are NPC-only: the Player aims by hand.
		void LogConfig(std::string_view actor, const Config& cfg, const NPCConfig* npc = nullptr) {
			const auto aiming = [&](std::string_view weapon, bool enabled) {
				logger::info("\t{} {} with {}", actor, enabled ? "will learn to aim" : (npc ? "have perfect aim" : "has perfect aim"), weapon);
			};
			aiming("bows", cfg.bowAiming);
			aiming("crossbows", cfg.crossbowAiming);
			aiming("spells", cfg.spellAiming);
			aiming("staves", cfg.staffAiming);
			aiming("scrolls", cfg.scrollAiming);
			aiming("concentration spells", cfg.concentrationSpellAiming);
			aiming("concentration staves", cfg.concentrationStaffAiming);
			if (npc) {
				LogHeadshots(npc->headshots);
			}

			if (npc) {
				logger::info("");
				LogSkillMultipliers(actor, "Accuracy", cfg.accuracyMultipliers, cfg.blindnessAffectsAiming);
			}
			logger::info("");
			LogSkillMultipliers(actor, "Precision", cfg.precisionMultipliers, cfg.blindnessAffectsAiming);

			logger::info("");
			LogSkillSources(actor, "spells", cfg.spellSkills, false);
			LogSkillSources(actor, "concentration spells", cfg.concentrationSpellSkills, true);
			LogSkillSources(actor, "staves", cfg.staffSkills, false);
			LogSkillSources(actor, "concentration staves", cfg.concentrationStaffSkills, true);
			LogSkillSources(actor, "scrolls", cfg.scrollSkills, false);
			LogRange(actor, "concentration spells", cfg.concentrationSpellRange);
			LogRange(actor, "concentration staves", cfg.concentrationStaffRange);

			logger::info("");
			LogSpeed(actor, "charge/cast", "spells", cfg.spellChargeSpeed);
			LogSpeed(actor, "charge/cast", "concentration spells", cfg.concentrationSpellChargeSpeed);
			LogSpeed(actor, "charge/cast", "staves", cfg.staffChargeSpeed);
			LogSpeed(actor, "charge/cast", "concentration staves", cfg.concentrationStaffChargeSpeed);
			LogSpeed(actor, "charge/cast", "scrolls", cfg.scrollChargeSpeed);
			LogSpeed(actor, "draw", "bows", cfg.bowDrawSpeed);
			LogSpeed(actor, "draw", "crossbows", cfg.crossbowDrawSpeed);

			LogSteadying(actor, "spells charged", cfg.spellSteadying);
			LogSteadying(actor, "staves charged", cfg.staffSteadying);
			LogSteadying(actor, "scrolls charged", cfg.scrollSteadying);
			LogSteadying(actor, "a bow at full draw", cfg.bowSteadying);
			LogSteadying(actor, "a loaded crossbow", cfg.crossbowSteadying);

			LogFatigue(actor, "Holding spells charged", "hold spells charged", "Magicka", cfg.spellFatigue);
			LogFatigue(actor, "Holding staves charged", "hold staves charged", "Magicka", cfg.staffFatigue);
			LogFatigue(actor, "Holding scrolls charged", "hold scrolls charged", "Magicka", cfg.scrollFatigue);
			LogFatigue(actor, "Casting concentration spells", "cast concentration spells", "Magicka", cfg.concentrationSpellFatigue);
			LogFatigue(actor, "Casting concentration staves", "cast concentration staves", "Magicka", cfg.concentrationStaffFatigue);
			LogFatigue(actor, "Holding a bow at full draw", "hold a full draw", "Stamina", cfg.bowFatigue);
			LogFatigue(actor, "Holding a loaded crossbow", "hold a loaded crossbow", "Stamina", cfg.crossbowFatigue);
		}

		/// Whether each NPC passed the keyword filters, so that hooks running every frame don't look up keywords each time.
		RE::BSSpinLock                       learnersLock;
		std::unordered_map<RE::FormID, bool> learners;

		const std::filesystem::path kIniPath = R"(Data\SKSE\Plugins\NPCsLearnToAim.ini)";

		/// Updates the INI file with the current options, or all of them when `writeDefaults` is set.
		void Write(bool writeDefaults) {
			CSimpleIniA ini{};
			ini.SetUnicode();
			ini.LoadFile(kIniPath.string().c_str());

			SaveConfig(ini, "NPC", NPC, NPCConfig::defaultConfig, writeDefaults);
			SaveConfig(ini, "Player", Player, PlayerConfig::defaultConfig, writeDefaults);

			SaveHeadshotSettings(ini, "NPC", NPC.headshots, NPCConfig::defaultConfig.headshots, writeDefaults);

			SetOrDeleteKeywords(ini, "NPC", "sExcludeKeywords", NPC.excludeKeywords, NPCConfig::defaultConfig.excludeKeywords, writeDefaults);
			SetOrDeleteKeywords(ini, "NPC", "sIncludeKeywords", NPC.includeKeywords, NPCConfig::defaultConfig.includeKeywords, writeDefaults);

			ini.SaveFile(kIniPath.string().c_str());

			// Every settings edit ends up here, including the master switch and keyword filters.
			InvalidateActorCache();
		}
	}

	std::vector<std::string> ParseKeywords(std::string_view list) {
		std::vector<std::string> keywords;
		for (auto& entry : clib_util::string::split(std::string(list), ",")) {
			const auto first = entry.find_first_not_of(" \t");
			if (first == std::string::npos) {
				continue;
			}
			const auto last = entry.find_last_not_of(" \t");
			keywords.push_back(entry.substr(first, last - first + 1));
		}
		return keywords;
	}

	void InvalidateActorCache() {
		RE::BSSpinLockGuard lock{ learnersLock };
		learners.clear();
	}

	void Load() {
		++currentID;
		InvalidateActorCache();

		logger::info("{:*^40}", "OPTIONS");
		CSimpleIniA ini{};
		ini.SetUnicode();
		if (ini.LoadFile(kIniPath.string().c_str()) >= 0) {
			NPC = NPCConfig::FromIni(ini);
			Player = PlayerConfig::FromIni(ini);
		} else {
			logger::info(R"(Data\SKSE\Plugins\NPCsLearnToAim.ini not found. Default options will be used.)");
			logger::info("");
		}

		logger::info("NPC:");
		if (NPC.enabled) {
			LogConfig("NPCs", NPC, &NPC);
			logger::info("");
			LogKeywordFilters(NPC);
		} else {
			logger::info("\tMod is disabled for NPCs");
		}

		logger::info("");
		logger::info("Player:");
		if (Player.enabled) {
			LogConfig("Player", Player);
		} else {
			logger::info("\tMod is disabled for Player");
		}

		logger::info("{:*^40}", "");
	}

	void Save() {
		Write(false);
	}

	void Export() {
		Write(true);
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

		const auto formID = a_actor->GetFormID();
		{
			RE::BSSpinLockGuard lock{ learnersLock };
			if (auto it = learners.find(formID); it != learners.end()) {
				return it->second;
			}
		}

		const auto* race = a_actor->GetRace();
		const auto  hasAny = [&](const std::vector<std::string>& keywords) {
            return a_actor->HasAnyKeywordByEditorID(keywords) || (race && race->HasAnyKeywordByEditorID(keywords));
		};

		// An empty include list means no filtering: HasAnyKeywordByEditorID would never match it.
		const bool learns = (includeKeywords.empty() || hasAny(includeKeywords)) && (excludeKeywords.empty() || !hasAny(excludeKeywords));

		RE::BSSpinLockGuard lock{ learnersLock };
		learners[formID] = learns;
		return learns;
	}

	Config Config::FromIni(CSimpleIniA& ini, const char* a_section, const Config& defaults) {
		Config result = defaults;

		result.enabled = ini.GetBoolValue(a_section, "bEnabled", result.enabled);

		result.spellAiming = ini.GetBoolValue(a_section, "bSpellAiming", result.spellAiming);
		result.staffAiming = ini.GetBoolValue(a_section, "bStaffAiming", result.staffAiming);
		result.concentrationSpellAiming = ini.GetBoolValue(a_section, "bConcentrationSpellsAiming", result.concentrationSpellAiming);
		result.concentrationStaffAiming = ini.GetBoolValue(a_section, "bConcentrationStaffAiming", result.concentrationStaffAiming);
		result.scrollAiming = ini.GetBoolValue(a_section, "bScrollAiming", result.scrollAiming);
		result.bowAiming = ini.GetBoolValue(a_section, "bBowAiming", result.bowAiming);
		result.crossbowAiming = ini.GetBoolValue(a_section, "bCrossbowAiming", result.crossbowAiming);

		result.accuracyMultipliers = SkillMultiplier::FromIni(ini, a_section, "Accuracy", result.accuracyMultipliers);
		result.precisionMultipliers = SkillMultiplier::FromIni(ini, a_section, "Precision", result.precisionMultipliers);

		result.blindnessAffectsAiming = ini.GetBoolValue(a_section, "bBlindnessAffectsAiming", result.blindnessAffectsAiming);

		result.spellSkills = SkillSources::FromIni(ini, a_section, "Spell", result.spellSkills);
		result.concentrationSpellSkills = SkillSources::FromIni(ini, a_section, "ConcentrationSpell", result.concentrationSpellSkills);
		result.staffSkills = SkillSources::FromIni(ini, a_section, "Staff", result.staffSkills);
		result.concentrationStaffSkills = SkillSources::FromIni(ini, a_section, "ConcentrationStaff", result.concentrationStaffSkills);
		result.scrollSkills = SkillSources::FromIni(ini, a_section, "Scroll", result.scrollSkills);

		result.concentrationSpellRange = ScalingOptions::FromIni(ini, a_section, "ConcentrationSpellRange", result.concentrationSpellRange, Limits::kRangeMultiplier);
		result.concentrationStaffRange = ScalingOptions::FromIni(ini, a_section, "ConcentrationStaffRange", result.concentrationStaffRange, Limits::kRangeMultiplier);

		result.spellChargeSpeed = ScalingOptions::FromIni(ini, a_section, "SpellChargeSpeed", result.spellChargeSpeed, Limits::kSpeedMultiplier);
		result.concentrationSpellChargeSpeed = ScalingOptions::FromIni(ini, a_section, "ConcentrationSpellChargeSpeed", result.concentrationSpellChargeSpeed, Limits::kSpeedMultiplier);
		result.staffChargeSpeed = ScalingOptions::FromIni(ini, a_section, "StaffChargeSpeed", result.staffChargeSpeed, Limits::kSpeedMultiplier);
		result.concentrationStaffChargeSpeed = ScalingOptions::FromIni(ini, a_section, "ConcentrationStaffChargeSpeed", result.concentrationStaffChargeSpeed, Limits::kSpeedMultiplier);
		result.scrollChargeSpeed = ScalingOptions::FromIni(ini, a_section, "ScrollChargeSpeed", result.scrollChargeSpeed, Limits::kSpeedMultiplier);

		result.spellSteadying = SpreadOptions::FromIni(ini, a_section, "SpellSteadying", result.spellSteadying);
		result.staffSteadying = SpreadOptions::FromIni(ini, a_section, "StaffSteadying", result.staffSteadying);
		result.scrollSteadying = SpreadOptions::FromIni(ini, a_section, "ScrollSteadying", result.scrollSteadying);

		result.spellFatigue = FatigueOptions::FromIni(ini, a_section, "SpellFatigue", result.spellFatigue);
		result.staffFatigue = FatigueOptions::FromIni(ini, a_section, "StaffFatigue", result.staffFatigue);
		result.scrollFatigue = FatigueOptions::FromIni(ini, a_section, "ScrollFatigue", result.scrollFatigue);
		result.concentrationSpellFatigue = FatigueOptions::FromIni(ini, a_section, "ConcentrationSpellFatigue", result.concentrationSpellFatigue);
		result.concentrationStaffFatigue = FatigueOptions::FromIni(ini, a_section, "ConcentrationStaffFatigue", result.concentrationStaffFatigue);

		result.bowDrawSpeed = ScalingOptions::FromIni(ini, a_section, "BowDrawSpeed", result.bowDrawSpeed, Limits::kSpeedMultiplier);
		result.crossbowDrawSpeed = ScalingOptions::FromIni(ini, a_section, "CrossbowDrawSpeed", result.crossbowDrawSpeed, Limits::kSpeedMultiplier);
		result.bowSteadying = SpreadOptions::FromIni(ini, a_section, "BowSteadying", result.bowSteadying);
		result.bowFatigue = FatigueOptions::FromIni(ini, a_section, "BowFatigue", result.bowFatigue);
		result.crossbowSteadying = SpreadOptions::FromIni(ini, a_section, "CrossbowSteadying", result.crossbowSteadying);
		result.crossbowFatigue = FatigueOptions::FromIni(ini, a_section, "CrossbowFatigue", result.crossbowFatigue);

		return result;
	}

	PlayerConfig PlayerConfig::FromIni(CSimpleIniA& ini) {
		return PlayerConfig{ Config::FromIni(ini, "Player", PlayerConfig::defaultConfig) };
	}

	NPCConfig NPCConfig::FromIni(CSimpleIniA& ini) {
		NPCConfig result{ Config::FromIni(ini, "NPC", NPCConfig::defaultConfig) };

		result.headshots = HeadshotSettings::FromIni(ini, "NPC", NPCConfig::defaultConfig.headshots);

		// A key that is present but empty means "no keywords", whereas a missing one means the defaults.
		const auto readKeywords = [&](const char* key, const std::vector<std::string>& fallback) {
			const char* value = ini.GetValue("NPC", key, nullptr);
			return value ? ParseKeywords(value) : fallback;
		};
		result.excludeKeywords = readKeywords("sExcludeKeywords", NPCConfig::defaultConfig.excludeKeywords);
		result.includeKeywords = readKeywords("sIncludeKeywords", NPCConfig::defaultConfig.includeKeywords);
		return result;
	}

	NPCConfig::HeadshotSettings NPCConfig::HeadshotSettings::FromIni(CSimpleIniA& ini, const char* a_section, const HeadshotSettings& defaults) {
		HeadshotSettings result = defaults;
		result.bow = ini.GetBoolValue(a_section, "bBowHeadshots", result.bow);
		result.bowMinChance = ReadClamped(ini, a_section, "fBowMinHeadshotChance", result.bowMinChance, Limits::kHeadshotChance);
		result.bowMaxChance = ReadClamped(ini, a_section, "fBowMaxHeadshotChance", result.bowMaxChance, Limits::kHeadshotChance);
		result.crossbow = ini.GetBoolValue(a_section, "bCrossbowHeadshots", result.crossbow);
		result.crossbowMinChance = ReadClamped(ini, a_section, "fCrossbowMinHeadshotChance", result.crossbowMinChance, Limits::kHeadshotChance);
		result.crossbowMaxChance = ReadClamped(ini, a_section, "fCrossbowMaxHeadshotChance", result.crossbowMaxChance, Limits::kHeadshotChance);
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

	Config::ScalingOptions Config::ScalingOptions::FromIni(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const ScalingOptions& defaults, Limit limit) {
		ScalingOptions result = defaults;
		result.enabled = ini.GetBoolValue(a_section, fmt::format("bScale{}WithSkill", prefix).c_str(), result.enabled);
		result.minMultiplier = ReadClamped(ini, a_section, fmt::format("f{}MinMultiplier", prefix), result.minMultiplier, limit);
		result.maxMultiplier = ReadClamped(ini, a_section, fmt::format("f{}MaxMultiplier", prefix), result.maxMultiplier, limit);
		return result;
	}

	Config::SpreadOptions Config::SpreadOptions::FromIni(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const SpreadOptions& defaults) {
		SpreadOptions result = defaults;
		result.enabled = ini.GetBoolValue(a_section, fmt::format("b{}", prefix).c_str(), result.enabled);
		result.maxSpreadMultiplier = ReadClamped(ini, a_section, fmt::format("f{}MaxSpreadReduction", prefix), result.maxSpreadMultiplier, Limits::kSteadyingReduction);
		result.halfDuration = ReadClamped(ini, a_section, fmt::format("f{}HalfDuration", prefix), result.halfDuration, Limits::kHalfDuration);
		return result;
	}

	Config::FatigueOptions Config::FatigueOptions::FromIni(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const FatigueOptions& defaults) {
		FatigueOptions result = defaults;
		result.enabled = ini.GetBoolValue(a_section, fmt::format("b{}", prefix).c_str(), result.enabled);
		result.minThreshold = ReadClamped(ini, a_section, fmt::format("f{}MinThreshold", prefix), result.minThreshold, Limits::kFatigueThreshold);
		result.maxThreshold = ReadClamped(ini, a_section, fmt::format("f{}MaxThreshold", prefix), result.maxThreshold, Limits::kFatigueThreshold);
		result.maxSpreadMultiplier = ReadClamped(ini, a_section, fmt::format("f{}MaxPenalty", prefix), result.maxSpreadMultiplier, Limits::kFatiguePenalty);
		result.halfDuration = ReadClamped(ini, a_section, fmt::format("f{}HalfDuration", prefix), result.halfDuration, Limits::kHalfDuration);
		return result;
	}

	Config::SkillMultiplier Config::SkillMultiplier::FromIni(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const SkillMultiplier& defaults) {
		SkillMultiplier result = defaults;

		const auto read = [&](std::string_view weapon, float fallback, Limit limit = Limits::kSkillMultiplier) {
			return ReadClamped(ini, a_section, fmt::format("f{}{}SkillMultiplier", weapon, prefix), fallback, limit);
		};

		result.bow = read("Bow", result.bow);
		result.crossbow = read("Crossbow", result.crossbow);
		result.spell = read("Spell", result.spell);
		result.concentrationSpell = read("ConcentrationSpell", result.concentrationSpell);
		result.staff = read("Staff", result.staff);
		result.concentrationStaff = read("ConcentrationStaff", result.concentrationStaff);
		result.scroll = read("Scroll", result.scroll);
		result.blindness = read("Blindness", result.blindness, Limits::kBlindnessMultiplier);

		return result;
	}
}
