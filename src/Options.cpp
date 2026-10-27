#include "Options.h"
#include "CLIBUtil/string.hpp"
#include "Helpers.h"
#include <fmt/ranges.h>

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
			for (const auto& kind : kAttackKinds) {
				SetOrDeleteValue(ini, a_section, fmt::format("f{}{}SkillMultiplier", kind.name, prefix).c_str(), value.*kind.multiplier, defaultValue.*kind.multiplier, writeDefaults);
			}
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

		void SaveScalingOptions(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const Config::Scaling& value, const Config::Scaling& defaultValue, bool writeDefaults = false) {
			SetOrDeleteValue(ini, a_section, fmt::format("bScale{}WithSkill", prefix).c_str(), value.enabled, defaultValue.enabled, writeDefaults);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}MinMultiplier", prefix).c_str(), value.minMultiplier, defaultValue.minMultiplier, writeDefaults);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}MaxMultiplier", prefix).c_str(), value.maxMultiplier, defaultValue.maxMultiplier, writeDefaults);
		}

		void SaveFatigue(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const Config::Fatigue& value, const Config::Fatigue& defaultValue, bool writeDefaults = false) {
			SetOrDeleteValue(ini, a_section, fmt::format("b{}", prefix).c_str(), value.enabled, defaultValue.enabled, writeDefaults);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}MinThreshold", prefix).c_str(), value.minThreshold, defaultValue.minThreshold, writeDefaults);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}MaxThreshold", prefix).c_str(), value.maxThreshold, defaultValue.maxThreshold, writeDefaults);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}MaxPenalty", prefix).c_str(), value.maxSpreadMultiplier, defaultValue.maxSpreadMultiplier, writeDefaults);
			SetOrDeleteValue(ini, a_section, fmt::format("f{}HalfDuration", prefix).c_str(), value.halfDuration, defaultValue.halfDuration, writeDefaults);
		}

		constexpr std::array kSkillSourceNames{ "SpellSkill", "Highest", "Enchanting" };
		static_assert(kSkillSourceNames.size() == static_cast<std::size_t>(SkillSource::kTotal));

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

		void SaveSkillSources(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const Config::SkillSources& value, const Config::SkillSources& defaultValue, bool hasRange, bool writeDefaults = false) {
			SaveSkillSource(ini, a_section, fmt::format("s{}AimingSkill", prefix), value.aiming, defaultValue.aiming, writeDefaults);
			SaveSkillSource(ini, a_section, fmt::format("s{}ChargeSpeedSkill", prefix), value.chargeSpeed, defaultValue.chargeSpeed, writeDefaults);
			if (hasRange) {
				SaveSkillSource(ini, a_section, fmt::format("s{}RangeSkill", prefix), value.range, defaultValue.range, writeDefaults);
			}
		}

		std::string SpeedName(const AttackKind& kind) {
			return fmt::format("{}{}", kind.name, Weapon::IsBowOrCrossbow(kind.type) ? "DrawSpeed" : "ChargeSpeed");
		}

		void SaveConfig(CSimpleIniA& ini, const char* a_section, const Config& custom, const Config& defaults, bool writeDefaults = false) {
			SetOrDeleteValue(ini, a_section, "bEnabled", custom.enabled, defaults.enabled, writeDefaults);

			SaveSkillMultiplier(ini, a_section, "Accuracy", custom.accuracyMultipliers, defaults.accuracyMultipliers, writeDefaults);
			SaveSkillMultiplier(ini, a_section, "Precision", custom.precisionMultipliers, defaults.precisionMultipliers, writeDefaults);

			SetOrDeleteValue(ini, a_section, "bBlindnessAffectsAiming", custom.blindnessAffectsAiming, defaults.blindnessAffectsAiming, writeDefaults);

			for (const auto& kind : kAttackKinds) {
				const std::string_view name = kind.name;

				SetOrDeleteValue(ini, a_section, fmt::format("b{}Aiming", name).c_str(), custom.*kind.aiming, defaults.*kind.aiming, writeDefaults);

				if (kind.skills) {
					SaveSkillSources(ini, a_section, name, custom.*kind.skills, defaults.*kind.skills, kind.range != nullptr, writeDefaults);
				}
				if (kind.range) {
					SaveScalingOptions(ini, a_section, fmt::format("{}Range", name), custom.*kind.range, defaults.*kind.range, writeDefaults);
				}
				SaveScalingOptions(ini, a_section, SpeedName(kind), custom.*kind.speed, defaults.*kind.speed, writeDefaults);
				if (kind.steadying) {
					SaveSteadying(ini, a_section, fmt::format("{}Steadying", name), custom.*kind.steadying, defaults.*kind.steadying, writeDefaults);
				}
				SaveFatigue(ini, a_section, fmt::format("{}Fatigue", name), custom.*kind.fatigue, defaults.*kind.fatigue, writeDefaults);
			}
		}

		struct Wording
		{
			std::string_view kind;

			std::string_view plural;

			std::string_view speedVerb;

			/// Empty for kinds without steadying.
			std::string_view held;

			std::string_view fatigueGerund;
			std::string_view fatigueInfinitive;

			std::string_view attribute;

			static const Wording& For(const AttackKind& kind);
		};

		constexpr std::array kWordings{
			Wording{ "Bow", "bows", "draw", "a bow at full draw", "Holding a bow at full draw", "hold a full draw", "Stamina" },
			Wording{ "Crossbow", "crossbows", "reload", "a loaded crossbow", "Holding a loaded crossbow", "hold a loaded crossbow", "Stamina" },
			Wording{ "Spell", "spells", "charge/cast", "spells charged", "Holding spells charged", "hold spells charged", "Magicka" },
			Wording{ "ConcentrationSpell", "concentration spells", "charge/cast", "", "Casting concentration spells", "cast concentration spells", "Magicka" },
			Wording{ "Staff", "staves", "charge/cast", "staves charged", "Holding staves charged", "hold staves charged", "Magicka" },
			Wording{ "ConcentrationStaff", "concentration staves", "charge/cast", "", "Casting concentration staves", "cast concentration staves", "Magicka" },
			Wording{ "Scroll", "scrolls", "charge/cast", "scrolls charged", "Holding scrolls charged", "hold scrolls charged", "Magicka" },
		};
		static_assert(kWordings.size() == kAttackKinds.size());

		const Wording& Wording::For(const AttackKind& kind) {
			for (const auto& wording : kWordings) {
				if (wording.kind == kind.name) {
					return wording;
				}
			}
			assert(false && "No wording for the kind");
			return kWordings.front();
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
			for (const auto& kind : kAttackKinds) {
				LogSkillMultiplier(actor, Wording::For(kind).plural, stage, multipliers.*kind.multiplier);
			}
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

		void LogRange(std::string_view actor, std::string_view kindName, const Config::RangeScaling& range) {
			if (!range.enabled) {
				logger::info("\t{} always have vanilla reach with {}", actor, kindName);
			} else {
				logger::info("\t{} reach {} at {:.2f}x to {:.2f}x vanilla range, scaling with skill", actor, kindName, range.minMultiplier, range.maxMultiplier);
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

		/// `verb` is what the actor does with the kind, e.g. "draw" or "charge/cast".
		void LogSpeed(std::string_view actor, std::string_view verb, std::string_view kindName, const Config::Scaling& speed) {
			const auto [enabled, minMult, maxMult] = std::tie(speed.enabled, speed.minMultiplier, speed.maxMultiplier);
			if (!enabled) {
				logger::info("\t{} always {} {} at vanilla speed", actor, verb, kindName);
			} else if (minMult == maxMult) {
				logger::info("\t{} {} {} {}, regardless of skill", actor, verb, kindName, SpeedPercentPhrase(minMult));
			} else if (minMult == 1.0f) {
				logger::info("\t{} {} {} up to {}, scaling with skill", actor, verb, kindName, SpeedPercentPhrase(maxMult));
			} else {
				logger::info("\t{} {} {} {} at 0 skill, {} at 100 skill", actor, verb, kindName, SpeedPercentPhrase(minMult), SpeedPercentPhrase(maxMult));
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
		void LogFatigue(std::string_view actor, std::string_view gerund, std::string_view infinitive, std::string_view attribute, const Config::Fatigue& fatigue) {
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
			for (const auto& kind : kAttackKinds) {
				aiming(Wording::For(kind).plural, cfg.*kind.aiming);
			}
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
			for (const auto& kind : kAttackKinds) {
				if (kind.skills) {
					LogSkillSources(actor, Wording::For(kind).plural, cfg.*kind.skills, kind.range != nullptr);
				}
			}
			for (const auto& kind : kAttackKinds) {
				if (kind.range) {
					LogRange(actor, Wording::For(kind).plural, cfg.*kind.range);
				}
			}

			logger::info("");
			for (const auto& kind : kAttackKinds) {
				const auto& wording = Wording::For(kind);
				LogSpeed(actor, wording.speedVerb, wording.plural, cfg.*kind.speed);
			}

			for (const auto& kind : kAttackKinds) {
				if (kind.steadying) {
					LogSteadying(actor, Wording::For(kind).held, cfg.*kind.steadying);
				}
			}

			for (const auto& kind : kAttackKinds) {
				const auto& wording = Wording::For(kind);
				LogFatigue(actor, wording.fatigueGerund, wording.fatigueInfinitive, wording.attribute, cfg.*kind.fatigue);
			}
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

			const auto logLevel = spdlog::level::to_string_view(spdlog::default_logger()->level());
			if (writeDefaults || logLevel != spdlog::level::to_string_view(spdlog::level::info)) {
				ini.SetValue("Log", "sLogLevel", std::string(logLevel.data(), logLevel.size()).c_str());
			} else {
				ini.Delete("Log", "sLogLevel", true);
			}

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

	const AttackKind* AttackKind::For(const RE::MagicItem* spell) {
		const bool isConcentration = spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration;

		switch (spell->GetSpellType()) {
		case RE::MagicSystem::SpellType::kSpell:
		case RE::MagicSystem::SpellType::kLeveledSpell:
			return &For(kSpell, isConcentration);
		case RE::MagicSystem::SpellType::kStaffEnchantment:
			return &For(kStaff, isConcentration);
		case RE::MagicSystem::SpellType::kScroll:
			return &For(kScroll);
		default:
			return nullptr;
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

	spdlog::level::level_enum LogLevel() {
		CSimpleIniA ini{};
		ini.SetUnicode();
		if (ini.LoadFile(kIniPath.string().c_str()) < 0) {
			return spdlog::level::info;
		}

		const auto level = spdlog::level::from_str(ini.GetValue("Log", "sLogLevel", "info"));
		return level == spdlog::level::off ? spdlog::level::info : level;
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

	bool ShouldLearn(RE::Actor* a_actor) {
		if (!a_actor) {
			return false;
		}
		if (a_actor->IsPlayerRef()) {
			return Player.enabled;
		}

		if (!NPC.enabled) {
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
		const auto& include = NPC.includeKeywords;
		const auto& exclude = NPC.excludeKeywords;
		const bool  learns = (include.empty() || hasAny(include)) && (exclude.empty() || !hasAny(exclude));

		RE::BSSpinLockGuard lock{ learnersLock };
		learners[formID] = learns;
		return learns;
	}

	Config Config::FromIni(CSimpleIniA& ini, const char* a_section, const Config& defaults) {
		Config result = defaults;

		result.enabled = ini.GetBoolValue(a_section, "bEnabled", result.enabled);

		result.accuracyMultipliers = SkillMultiplier::FromIni(ini, a_section, "Accuracy", result.accuracyMultipliers);
		result.precisionMultipliers = SkillMultiplier::FromIni(ini, a_section, "Precision", result.precisionMultipliers);

		result.blindnessAffectsAiming = ini.GetBoolValue(a_section, "bBlindnessAffectsAiming", result.blindnessAffectsAiming);

		for (const auto& kind : kAttackKinds) {
			const std::string_view name = kind.name;

			result.*kind.aiming = ini.GetBoolValue(a_section, fmt::format("b{}Aiming", name).c_str(), result.*kind.aiming);

			if (kind.skills) {
				result.*kind.skills = SkillSources::FromIni(ini, a_section, name, result.*kind.skills);
			}
			if (kind.range) {
				result.*kind.range = Scaling::FromIni(ini, a_section, fmt::format("{}Range", name), result.*kind.range, Limits::kRangeMultiplier);
			}
			result.*kind.speed = Scaling::FromIni(ini, a_section, SpeedName(kind), result.*kind.speed, Limits::kSpeedMultiplier);
			if (kind.steadying) {
				result.*kind.steadying = Spread::FromIni(ini, a_section, fmt::format("{}Steadying", name), result.*kind.steadying);
			}
			result.*kind.fatigue = Fatigue::FromIni(ini, a_section, fmt::format("{}Fatigue", name), result.*kind.fatigue);
		}

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

	Config::Scaling Config::Scaling::FromIni(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const Scaling& defaults, Limit limit) {
		Scaling result = defaults;
		result.enabled = ini.GetBoolValue(a_section, fmt::format("bScale{}WithSkill", prefix).c_str(), result.enabled);
		result.minMultiplier = ReadClamped(ini, a_section, fmt::format("f{}MinMultiplier", prefix), result.minMultiplier, limit);
		result.maxMultiplier = ReadClamped(ini, a_section, fmt::format("f{}MaxMultiplier", prefix), result.maxMultiplier, limit);
		return result;
	}

	Config::Spread Config::Spread::FromIni(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const Spread& defaults) {
		Spread result = defaults;
		result.enabled = ini.GetBoolValue(a_section, fmt::format("b{}", prefix).c_str(), result.enabled);
		result.maxSpreadMultiplier = ReadClamped(ini, a_section, fmt::format("f{}MaxSpreadReduction", prefix), result.maxSpreadMultiplier, Limits::kSteadyingReduction);
		result.halfDuration = ReadClamped(ini, a_section, fmt::format("f{}HalfDuration", prefix), result.halfDuration, Limits::kHalfDuration);
		return result;
	}

	Config::Fatigue Config::Fatigue::FromIni(CSimpleIniA& ini, const char* a_section, std::string_view prefix, const Fatigue& defaults) {
		Fatigue result = defaults;
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

		for (const auto& kind : kAttackKinds) {
			result.*kind.multiplier = read(kind.name, result.*kind.multiplier);
		}
		result.blindness = read("Blindness", result.blindness, Limits::kBlindnessMultiplier);

		return result;
	}
}
