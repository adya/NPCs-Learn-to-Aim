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

		float ReadClamped(CSimpleIniA& ini, const char* section, const Key& key, float fallback, Limit limit) {
			const auto  raw = static_cast<float>(ini.GetDoubleValue(section, key.c_str(), fallback));
			const float clamped = std::clamp(raw, limit.min, limit.max);
			if (clamped != raw) {
				logger::warn("[{}] {} = {} is outside the allowed range {}..{}, using {}", section, key.c_str(), raw, limit.min, limit.max, clamped);
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

		constexpr std::string_view kUnsupported = "it is not supported here";
		constexpr std::string_view kNoRange = "only concentration spells and staves support range";

		/// Warns about every key that is present in the INI although the option is not supported.
		void WarnUnsupported(CSimpleIniA& ini, const char* section, std::span<const Key> keys, std::string_view reason) {
			for (const auto& key : keys) {
				if (ini.GetValue(section, key.c_str(), nullptr)) {
					logger::warn("[{}] {} is ignored: {}", section, key.c_str(), reason);
				}
			}
		}

		void SaveAim(CSimpleIniA& ini, const char* a_section, const Keys::Aim& keys, const Config::Aim& value, const Config::Aim& defaultValue, bool writeDefaults = false) {
			SetOrDeleteValue(ini, a_section, keys.skillMultiplier.c_str(), value.skillMultiplier, defaultValue.skillMultiplier, writeDefaults);
			SetOrDeleteValue(ini, a_section, keys.from.c_str(), value.bounds.from, defaultValue.bounds.from, writeDefaults);
			SetOrDeleteValue(ini, a_section, keys.to.c_str(), value.bounds.to, defaultValue.bounds.to, writeDefaults);
		}

		void SaveSteadying(CSimpleIniA& ini, const char* a_section, const Keys::Steadying& keys, const Config::Steadying& value, const Config::Steadying& defaultValue, bool writeDefaults = false) {
			SetOrDeleteValue(ini, a_section, keys.enabled.c_str(), value.enabled, defaultValue.enabled, writeDefaults);
			SetOrDeleteValue(ini, a_section, keys.maxSpreadMultiplier.c_str(), value.maxSpreadMultiplier, defaultValue.maxSpreadMultiplier, writeDefaults);
			SetOrDeleteValue(ini, a_section, keys.halfDuration.c_str(), value.halfDuration, defaultValue.halfDuration, writeDefaults);
		}

		void SaveScaling(CSimpleIniA& ini, const char* a_section, const Keys::Scaling& keys, const Config::Scaling& value, const Config::Scaling& defaultValue, bool writeDefaults = false) {
			SetOrDeleteValue(ini, a_section, keys.enabled.c_str(), value.enabled, defaultValue.enabled, writeDefaults);
			SetOrDeleteValue(ini, a_section, keys.from.c_str(), value.bounds.from, defaultValue.bounds.from, writeDefaults);
			SetOrDeleteValue(ini, a_section, keys.to.c_str(), value.bounds.to, defaultValue.bounds.to, writeDefaults);
			SetOrDeleteValue(ini, a_section, keys.skillMultiplier.c_str(), value.skillMultiplier, defaultValue.skillMultiplier, writeDefaults);
		}

		void SaveFatigue(CSimpleIniA& ini, const char* a_section, const Keys::Fatigue& keys, const Config::Fatigue& value, const Config::Fatigue& defaultValue, bool writeDefaults = false) {
			SetOrDeleteValue(ini, a_section, keys.enabled.c_str(), value.enabled, defaultValue.enabled, writeDefaults);
			SetOrDeleteValue(ini, a_section, keys.from.c_str(), value.threshold.from, defaultValue.threshold.from, writeDefaults);
			SetOrDeleteValue(ini, a_section, keys.to.c_str(), value.threshold.to, defaultValue.threshold.to, writeDefaults);
			SetOrDeleteValue(ini, a_section, keys.maxSpreadMultiplier.c_str(), value.maxSpreadMultiplier, defaultValue.maxSpreadMultiplier, writeDefaults);
			SetOrDeleteValue(ini, a_section, keys.halfDuration.c_str(), value.halfDuration, defaultValue.halfDuration, writeDefaults);
		}

		constexpr std::array kSkillSourceNames{ "SpellSkill", "Highest", "Enchanting", "Archery" };
		static_assert(kSkillSourceNames.size() == static_cast<std::size_t>(SkillSource::kTotal));

		const char* SkillSourceName(SkillSource source) {
			return kSkillSourceNames[std::min<std::size_t>(static_cast<std::size_t>(source), kSkillSourceNames.size() - 1)];
		}

		void SaveSkillSource(CSimpleIniA& ini, const char* a_section, const Key& key, SkillSource value, SkillSource defaultValue, bool writeDefaults) {
			if (writeDefaults || value != defaultValue) {
				ini.SetValue(a_section, key.c_str(), SkillSourceName(value));
			} else {
				ini.Delete(a_section, key.c_str(), true);
			}
		}

		void SaveSkillSources(CSimpleIniA& ini, const char* a_section, const Keys::SkillSources& keys, const Config::SkillSources& value, const Config::SkillSources& defaultValue, bool writeDefaults = false) {
			SaveSkillSource(ini, a_section, keys.aiming, value.aiming, defaultValue.aiming, writeDefaults);
			SaveSkillSource(ini, a_section, keys.speed, value.speed, defaultValue.speed, writeDefaults);
			if (defaultValue.range != SkillSource::kTotal) {
				SaveSkillSource(ini, a_section, keys.range, value.range, defaultValue.range, writeDefaults);
			}
		}

		/// Whether the config has aim at all: the Player aims by hand, so it doesn't.
		bool SupportsAim(const Config& config) {
			return std::ranges::any_of(AttackKind::kAll, [&](const AttackKind& kind) { return (config.*kind.config).aim != Config::Aim::kNone; });
		}

		/// Options that are `kNone` in the defaults are not supported and are skipped.
		void SaveConfig(CSimpleIniA& ini, const char* a_section, const Config& custom, const Config& defaults, bool writeDefaults = false) {
			SetOrDeleteValue(ini, a_section, "bEnabled", custom.enabled, defaults.enabled, writeDefaults);

			SetOrDeleteValue(ini, a_section, "bBlindnessAffectsAiming", custom.blindnessAffectsAiming, defaults.blindnessAffectsAiming, writeDefaults);
			if (SupportsAim(defaults)) {
				SetOrDeleteValue(ini, a_section, "fBlindnessAimSkillMultiplier", custom.aimBlindness, defaults.aimBlindness, writeDefaults);
				SetOrDeleteValue(ini, a_section, "bHeadshots", custom.headshots, defaults.headshots, writeDefaults);
				SetOrDeleteValue(ini, a_section, "bArmshots", custom.armshots, defaults.armshots, writeDefaults);
				SetOrDeleteValue(ini, a_section, "bLegshots", custom.legshots, defaults.legshots, writeDefaults);
			}
			SetOrDeleteValue(ini, a_section, "fBlindnessAccuracySkillMultiplier", custom.accuracyBlindness, defaults.accuracyBlindness, writeDefaults);
			SetOrDeleteValue(ini, a_section, "fBlindnessPrecisionSkillMultiplier", custom.precisionBlindness, defaults.precisionBlindness, writeDefaults);

			for (const AttackKind& kind : AttackKind::kAll) {
				const Keys&            keys = Keys::For(kind);
				const auto&            attack = custom.*kind.config;
				const auto&            defaultAttack = defaults.*kind.config;

				SetOrDeleteValue(ini, a_section, keys.aiming.c_str(), attack.aiming, defaultAttack.aiming, writeDefaults);

				if (defaultAttack.aim != Config::Aim::kNone) {
					SaveAim(ini, a_section, keys.aim, attack.aim, defaultAttack.aim, writeDefaults);
				}
				SaveAim(ini, a_section, keys.accuracy, attack.accuracy, defaultAttack.accuracy, writeDefaults);
				SaveAim(ini, a_section, keys.precision, attack.precision, defaultAttack.precision, writeDefaults);

				SaveSkillSources(ini, a_section, keys.skills, attack.skills, defaultAttack.skills, writeDefaults);
				if (kind.range) {
					SaveScaling(ini, a_section, keys.range, attack.range, defaultAttack.range, writeDefaults);
				}
				SaveScaling(ini, a_section, keys.speed, attack.speed, defaultAttack.speed, writeDefaults);
				if (defaultAttack.skillShot != Config::Scaling::kNone) {
					SaveScaling(ini, a_section, keys.skillShot, attack.skillShot, defaultAttack.skillShot, writeDefaults);
				}
				SaveSteadying(ini, a_section, keys.steadying, attack.steadying, defaultAttack.steadying, writeDefaults);
				SaveFatigue(ini, a_section, keys.fatigue, attack.fatigue, defaultAttack.fatigue, writeDefaults);
			}
		}

		struct Wording
		{
			std::string_view kind;

			std::string_view plural;

			std::string_view speedVerb;

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
			Wording{ "ConcentrationSpell", "concentration spells", "charge/cast", "concentration spells being cast", "Casting concentration spells", "cast concentration spells", "Magicka" },
			Wording{ "Staff", "staves", "charge/cast", "staves charged", "Holding staves charged", "hold staves charged", "Magicka" },
			Wording{ "ConcentrationStaff", "concentration staves", "charge/cast", "concentration staves being cast", "Casting concentration staves", "cast concentration staves", "Magicka" },
			Wording{ "Scroll", "scrolls", "charge/cast", "scrolls charged", "Holding scrolls charged", "hold scrolls charged", "Magicka" },
		};
		static_assert(kWordings.size() == AttackKind::kAll.size());

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

		/// Logs how every weapon type's multiplier changes the difficulty of `stage` (Aim/Accuracy/Precision).
		void LogSkillMultipliers(std::string_view actor, std::string_view stage, const Config& cfg, SkillUsageContext context) {
			for (const AttackKind& kind : AttackKind::kAll) {
				LogSkillMultiplier(actor, Wording::For(kind).plural, stage, (cfg.*kind.config).AimFor(context).skillMultiplier);
			}
			const char* noun = context == kAim ? "aim" : (context == kAccuracy ? "accuracy" : "precision");
			if (cfg.blindnessAffectsAiming) {
				LogBlindnessSkillMultiplier(actor, noun, cfg.BlindnessFor(context));
			} else {
				logger::info("\tBlindness doesn't affect {}'s {}", actor, noun);
			}
		}

		void LogSkillSources(std::string_view actor, std::string_view name, const Config::SkillSources& sources) {
			const auto describe = [&](SkillSource source) -> std::string {
				switch (source) {
				case SkillSource::kEnchanting:
					return "Enchanting skill";
				case SkillSource::kArchery:
					return "Archery skill";
				case SkillSource::kHighest:
					return fmt::format("the highest {}'s magic skill regardless of spell", actor);
				default:
					return "skill in spell's magic school";
				}
			};
			logger::info("\t{}'s aiming with {} is based on {}", actor, name, describe(sources.aiming));
			logger::info("\t{}'s charge speed with {} is based on {}", actor, name, describe(sources.speed));
			if (sources.range != SkillSource::kTotal) {
				logger::info("\t{}'s range with {} is based on {}", actor, name, describe(sources.range));
			}
		}

		void LogRange(std::string_view actor, std::string_view kindName, const Config::RangeScaling& range) {
			if (!range.enabled) {
				logger::info("\t{} always have vanilla reach with {}", actor, kindName);
			} else {
				logger::info("\t{} reach {} at {:.2f}x to {:.2f}x vanilla range, scaling with skill", actor, kindName, range.bounds.from, range.bounds.to);
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
		void LogSpeed(std::string_view actor, std::string_view verb, std::string_view kindName, const Config::Speed& speed) {
			const auto [enabled, minMult, maxMult] = std::tie(speed.enabled, speed.bounds.from, speed.bounds.to);
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
					fatigue.threshold.from,
					attribute,
					fatigue.threshold.to,
					attribute,
					fatigue.maxSpreadMultiplier * 100,
					fatigue.halfDuration);
			}
		}

		void LogSkillShots(std::string_view actor, const Config& cfg) {
			std::vector<std::string_view> parts;
			if (cfg.headshots) {
				parts.emplace_back("the head");
			}
			if (cfg.armshots) {
				parts.emplace_back("the hands");
			}
			if (cfg.legshots) {
				parts.emplace_back("the legs");
			}

			bool any = false;
			for (const AttackKind& kind : AttackKind::kAll) {
				if (const auto& skillShot = (cfg.*kind.config).skillShot; skillShot.enabled && !parts.empty()) {
					any = true;
					logger::info("	Skilled {} with {} take skill shots at {} ({:.0f}%-{:.0f}% of shots)", actor, Wording::For(kind).plural, fmt::join(parts, ", "), skillShot.bounds.from * 100, skillShot.bounds.to * 100);
				}
			}
			if (!any) {
				logger::info("	{} always aim for the chest", actor);
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

		/// Logs everything the config changes. Aim and skill shots are NPC-only: the Player aims by hand.
		void LogConfig(std::string_view actor, const Config& cfg, bool isNPC) {
			const auto aiming = [&](std::string_view weapon, bool enabled) {
				logger::info("\t{} {} with {}", actor, enabled ? "will learn to aim" : (isNPC ? "have perfect aim" : "has perfect aim"), weapon);
			};
			for (const AttackKind& kind : AttackKind::kAll) {
				aiming(Wording::For(kind).plural, (cfg.*kind.config).aiming);
			}
			if (isNPC) {
				LogSkillShots(actor, cfg);

				logger::info("");
				LogSkillMultipliers(actor, "Aim", cfg, kAim);
				}
				logger::info("");
				LogSkillMultipliers(actor, "Accuracy", cfg, kAccuracy);
				logger::info("");
				LogSkillMultipliers(actor, "Precision", cfg, kPrecision);

			logger::info("");
			for (const AttackKind& kind : AttackKind::kAll) {
				LogSkillSources(actor, Wording::For(kind).plural, (cfg.*kind.config).skills);
			}
			for (const AttackKind& kind : AttackKind::kAll) {
				if (kind.range) {
					LogRange(actor, Wording::For(kind).plural, (cfg.*kind.config).range);
				}
			}

			logger::info("");
			for (const AttackKind& kind : AttackKind::kAll) {
				const auto& wording = Wording::For(kind);
				LogSpeed(actor, wording.speedVerb, wording.plural, (cfg.*kind.config).speed);
			}

			for (const AttackKind& kind : AttackKind::kAll) {
				LogSteadying(actor, Wording::For(kind).held, (cfg.*kind.config).steadying);
			}

			for (const AttackKind& kind : AttackKind::kAll) {
				const auto& wording = Wording::For(kind);
				LogFatigue(actor, wording.fatigueGerund, wording.fatigueInfinitive, wording.attribute, (cfg.*kind.config).fatigue);
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
			return isConcentration ? &kConcentrationSpell : &kSpell;
		case RE::MagicSystem::SpellType::kStaffEnchantment:
			return isConcentration ? &kConcentrationStaff : &kStaff;
		case RE::MagicSystem::SpellType::kScroll:
			return &kScroll;
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
			LogConfig("NPCs", NPC, true);
			logger::info("");
			LogKeywordFilters(NPC);
		} else {
			logger::info("\tMod is disabled for NPCs");
		}

		logger::info("");
		logger::info("Player:");
		if (Player.enabled) {
			LogConfig("Player", Player, false);
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

		result.blindnessAffectsAiming = ini.GetBoolValue(a_section, "bBlindnessAffectsAiming", result.blindnessAffectsAiming);
		if (SupportsAim(defaults)) {
			result.aimBlindness = ReadClamped(ini, a_section, "fBlindnessAimSkillMultiplier", result.aimBlindness, Limits::kBlindnessMultiplier);
			result.headshots = ini.GetBoolValue(a_section, "bHeadshots", result.headshots);
			result.armshots = ini.GetBoolValue(a_section, "bArmshots", result.armshots);
			result.legshots = ini.GetBoolValue(a_section, "bLegshots", result.legshots);
		} else {
			WarnUnsupported(ini, a_section, std::array{ Key{ "fBlindnessAimSkillMultiplier" }, Key{ "bHeadshots" }, Key{ "bArmshots" }, Key{ "bLegshots" } }, kUnsupported);
		}
		result.accuracyBlindness = ReadClamped(ini, a_section, "fBlindnessAccuracySkillMultiplier", result.accuracyBlindness, Limits::kBlindnessMultiplier);
		result.precisionBlindness = ReadClamped(ini, a_section, "fBlindnessPrecisionSkillMultiplier", result.precisionBlindness, Limits::kBlindnessMultiplier);

		for (const AttackKind& kind : AttackKind::kAll) {
			const Keys& keys = Keys::For(kind);
			auto&       attack = result.*kind.config;
			const auto& defaultAttack = defaults.*kind.config;

			attack.aiming = ini.GetBoolValue(a_section, keys.aiming.c_str(), attack.aiming);

			if (defaultAttack.aim != Config::Aim::kNone) {
				attack.aim = Aim::FromIni(ini, a_section, keys.aim, attack.aim, Limits::kAimFactor);
			} else {
				WarnUnsupported(ini, a_section, keys.aim.All(), kUnsupported);
			}
			attack.accuracy = Aim::FromIni(ini, a_section, keys.accuracy, attack.accuracy, Limits::kAimFactor);
			attack.precision = Aim::FromIni(ini, a_section, keys.precision, attack.precision, Limits::kFocus);

			attack.skills = SkillSources::FromIni(ini, a_section, keys.skills, attack.skills);

			if (kind.range) {
				attack.range = Scaling::FromIni(ini, a_section, keys.range, attack.range, Limits::kRangeMultiplier);
			} else {
				WarnUnsupported(ini, a_section, keys.range.All(), kNoRange);
			}
			attack.speed = Scaling::FromIni(ini, a_section, keys.speed, attack.speed, Limits::kSpeedMultiplier);

			if (defaultAttack.skillShot != Config::Scaling::kNone) {
				attack.skillShot = Scaling::FromIni(ini, a_section, keys.skillShot, attack.skillShot, Limits::kSkillShotChance);
			} else {
				WarnUnsupported(ini, a_section, keys.skillShot.All(), kUnsupported);
			}

			attack.steadying = Spread::FromIni(ini, a_section, keys.steadying, attack.steadying);
			attack.fatigue = Fatigue::FromIni(ini, a_section, keys.fatigue, attack.fatigue);
		}

		return result;
	}

	PlayerConfig PlayerConfig::FromIni(CSimpleIniA& ini) {
		return PlayerConfig{ Config::FromIni(ini, "Player", PlayerConfig::defaultConfig) };
	}

	NPCConfig NPCConfig::FromIni(CSimpleIniA& ini) {
		NPCConfig result{ Config::FromIni(ini, "NPC", NPCConfig::defaultConfig) };

		// A key that is present but empty means "no keywords", whereas a missing one means the defaults.
		const auto readKeywords = [&](const char* key, const std::vector<std::string>& fallback) {
			const char* value = ini.GetValue("NPC", key, nullptr);
			return value ? ParseKeywords(value) : fallback;
		};
		result.excludeKeywords = readKeywords("sExcludeKeywords", NPCConfig::defaultConfig.excludeKeywords);
		result.includeKeywords = readKeywords("sIncludeKeywords", NPCConfig::defaultConfig.includeKeywords);
		return result;
	}

	Config::SkillSources Config::SkillSources::FromIni(CSimpleIniA& ini, const char* a_section, const Keys::SkillSources& keys, const SkillSources& defaults) {
		const auto read = [&](const Key& key, SkillSource fallback) {
			const char* value = ini.GetValue(a_section, key.c_str(), nullptr);
			if (!value) {
				return fallback;
			}
			for (std::size_t i = 0; i < kSkillSourceNames.size(); ++i) {
				if (clib_util::string::iequals(value, kSkillSourceNames[i])) {
					return static_cast<SkillSource>(i);
				}
			}
			logger::warn("Unknown skill source '{}' for {}, using the default", value, key.c_str());
			return fallback;
		};

		SkillSources result = defaults;
		result.aiming = read(keys.aiming, result.aiming);
		result.speed = read(keys.speed, result.speed);
		if (defaults.range != SkillSource::kTotal) {
			result.range = read(keys.range, result.range);
		} else {
			WarnUnsupported(ini, a_section, std::array{ keys.range }, kNoRange);
		}
		return result;
	}

	Config::Aim Config::Aim::FromIni(CSimpleIniA& ini, const char* a_section, const Keys::Aim& keys, const Aim& defaults, Limit limit) {
 	Aim result = defaults;
 	result.skillMultiplier = ReadClamped(ini, a_section, keys.skillMultiplier, result.skillMultiplier, Limits::kSkillMultiplier);
 	result.bounds.from = ReadClamped(ini, a_section, keys.from, result.bounds.from, limit);
 	result.bounds.to = ReadClamped(ini, a_section, keys.to, result.bounds.to, Limits::ToLimit(limit, result.bounds.from));
		return result;
	}

	Config::Scaling Config::Scaling::FromIni(CSimpleIniA& ini, const char* a_section, const Keys::Scaling& keys, const Scaling& defaults, Limit limit) {
		Scaling result = defaults;
		result.enabled = ini.GetBoolValue(a_section, keys.enabled.c_str(), result.enabled);
		result.bounds.from = ReadClamped(ini, a_section, keys.from, result.bounds.from, limit);
		result.bounds.to = ReadClamped(ini, a_section, keys.to, result.bounds.to, limit);
		result.skillMultiplier = ReadClamped(ini, a_section, keys.skillMultiplier, result.skillMultiplier, Limits::kSkillMultiplier);
		return result;
	}

	Config::Spread Config::Spread::FromIni(CSimpleIniA& ini, const char* a_section, const Keys::Steadying& keys, const Spread& defaults) {
		Spread result = defaults;
		result.enabled = ini.GetBoolValue(a_section, keys.enabled.c_str(), result.enabled);
		result.maxSpreadMultiplier = ReadClamped(ini, a_section, keys.maxSpreadMultiplier, result.maxSpreadMultiplier, Limits::kSteadyingReduction);
		result.halfDuration = ReadClamped(ini, a_section, keys.halfDuration, result.halfDuration, Limits::kHalfDuration);
		return result;
	}

	Config::Fatigue Config::Fatigue::FromIni(CSimpleIniA& ini, const char* a_section, const Keys::Fatigue& keys, const Fatigue& defaults) {
		Fatigue result = defaults;
		result.enabled = ini.GetBoolValue(a_section, keys.enabled.c_str(), result.enabled);
		result.threshold.from = ReadClamped(ini, a_section, keys.from, result.threshold.from, Limits::kFatigueThreshold);
		result.threshold.to = ReadClamped(ini, a_section, keys.to, result.threshold.to, Limits::kFatigueThreshold);
		result.maxSpreadMultiplier = ReadClamped(ini, a_section, keys.maxSpreadMultiplier, result.maxSpreadMultiplier, Limits::kFatiguePenalty);
		result.halfDuration = ReadClamped(ini, a_section, keys.halfDuration, result.halfDuration, Limits::kHalfDuration);
		return result;
	}
}
