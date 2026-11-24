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

		/// `withToggle` is false for Accuracy, which has no switch of its own: `Attack::aiming` turns it off.
		void SaveAim(CSimpleIniA& ini, const char* a_section, const Keys::Aim& keys, const Config::Aim& value, const Config::Aim& defaultValue, bool writeDefaults = false, bool withToggle = true) {
			if (withToggle) {
				SetOrDeleteValue(ini, a_section, keys.enabled.c_str(), value.enabled, defaultValue.enabled, writeDefaults);
			}
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

		/// `retarget` and `skillShot` say whether the kind has them: concentration spells and staves retarget, and only NPCs take skill shots.
		void SaveSkillSources(CSimpleIniA& ini, const char* a_section, const Keys::SkillSources& keys, const Config::SkillSources& value, const Config::SkillSources& defaultValue, bool retarget, bool skillShot, bool writeDefaults = false) {
			SaveSkillSource(ini, a_section, keys.accuracy, value.accuracy, defaultValue.accuracy, writeDefaults);
			SaveSkillSource(ini, a_section, keys.precision, value.precision, defaultValue.precision, writeDefaults);
			if (retarget) {
				SaveSkillSource(ini, a_section, keys.retarget, value.retarget, defaultValue.retarget, writeDefaults);
			}
			if (skillShot) {
				SaveSkillSource(ini, a_section, keys.skillShot, value.skillShot, defaultValue.skillShot, writeDefaults);
			}
			SaveSkillSource(ini, a_section, keys.speed, value.speed, defaultValue.speed, writeDefaults);
			if (defaultValue.range != SkillSource::kTotal) {
				SaveSkillSource(ini, a_section, keys.range, value.range, defaultValue.range, writeDefaults);
			}
		}

		/// Whether the config has skill shots at all: the Player aims by hand, so it doesn't.
		bool SupportsSkillShots(const Config& config) {
			return std::ranges::any_of(AttackKind::kAll, [&](const AttackKind& kind) { return (config.*kind.config).skillShot != Config::Scaling::kNone; });
		}

		/// Options that are `kNone` in the defaults are not supported and are skipped.
		void SaveConfig(CSimpleIniA& ini, const char* a_section, const Config& custom, const Config& defaults, bool writeDefaults = false) {
			SetOrDeleteValue(ini, a_section, "bEnabled", custom.enabled, defaults.enabled, writeDefaults);

			if (SupportsSkillShots(defaults)) {
				SetOrDeleteValue(ini, a_section, "bHeadshots", custom.headshots, defaults.headshots, writeDefaults);
				SetOrDeleteValue(ini, a_section, "bArmshots", custom.armshots, defaults.armshots, writeDefaults);
				SetOrDeleteValue(ini, a_section, "bLegshots", custom.legshots, defaults.legshots, writeDefaults);
			}

			for (const AttackKind& kind : AttackKind::kAll) {
				const Keys& keys = Keys::For(kind);
				const auto& attack = custom.*kind.config;
				const auto& defaultAttack = defaults.*kind.config;

				SetOrDeleteValue(ini, a_section, keys.enabled.c_str(), attack.enabled, defaultAttack.enabled, writeDefaults);
				SetOrDeleteValue(ini, a_section, keys.aiming.c_str(), attack.aiming, defaultAttack.aiming, writeDefaults);

				SaveAim(ini, a_section, keys.accuracy, attack.accuracy, defaultAttack.accuracy, writeDefaults, false);
				SaveAim(ini, a_section, keys.precision, attack.precision, defaultAttack.precision, writeDefaults);
				SetOrDeleteValue(ini, a_section, keys.accuracyBlindness.c_str(), attack.accuracyBlindness, defaultAttack.accuracyBlindness, writeDefaults);
				SetOrDeleteValue(ini, a_section, keys.precisionBlindness.c_str(), attack.precisionBlindness, defaultAttack.precisionBlindness, writeDefaults);
				if (kind.concentration) {
					SaveAim(ini, a_section, keys.retarget, attack.retarget, defaultAttack.retarget, writeDefaults);
					SetOrDeleteValue(ini, a_section, keys.retargetBlindness.c_str(), attack.retargetBlindness, defaultAttack.retargetBlindness, writeDefaults);
				}

				SaveSkillSources(ini, a_section, keys.skills, attack.skills, defaultAttack.skills, kind.concentration, defaultAttack.skillShot != Config::Scaling::kNone, writeDefaults);
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

		constexpr std::array kColumnNames{ "Bow", "Crossbow", "Spell", "Conc. Spell", "Staff", "Conc. Staff", "Scroll" };
		static_assert(kColumnNames.size() == AttackKind::kAll.size());

		constexpr int kLabelWidth = 16;
		constexpr int kColumnWidth = 13;
		/// Each column is padded by a space on both sides and preceded by a border.
		constexpr int kTableWidth = kLabelWidth + static_cast<int>(kColumnNames.size()) * (kColumnWidth + 3);

		/// A cell of an option that the kind doesn't have, or that has no effect with its other options.
		constexpr const char* kNotApplicable = "-";

		/// A line of the options table: what it shows for each attack kind.
		struct Row
		{
			const char* label;
			std::string (*cell)(const AttackKind&, const Config::Attack&);
		};

		std::string OnOff(bool value) {
			return value ? "On" : "Off";
		}

		std::string Multiplier(float value) {
			return fmt::format("x{:.2f}", value);
		}

		std::string Percent(float value) {
			return fmt::format("{:.0f}%", value * 100);
		}

		std::string Seconds(float value) {
			return fmt::format("{:.2f}s", value);
		}

		/// The values at 0 and at 100 skill.
		std::string FromTo(const Bounds& bounds) {
			return fmt::format("{:.2f} -> {:.2f}", bounds.from, bounds.to);
		}

		std::string SourceName(const AttackKind& kind, SkillSource source) {
			switch (source) {
			case SkillSource::kSpellSkill:
				// Weapons have no magic school, see `SkillForSource`.
				return kind.magic ? "School" : "Archery";
			case SkillSource::kHighest:
				return "Highest";
			case SkillSource::kEnchanting:
				return "Enchanting";
			case SkillSource::kArchery:
				return "Archery";
			default:
				return kNotApplicable;
			}
		}

		bool Precise(const Config::Attack& attack) {
			return attack.Aims() && attack.precision.enabled;
		}

		/// Retargeting only exists for concentration kinds.
		bool Retargets(const AttackKind& kind, const Config::Attack& attack) {
			return kind.concentration && attack.Aims() && attack.retarget.enabled;
		}

		bool Ranged(const AttackKind& kind, const Config::Attack& attack) {
			return kind.range && attack.enabled && attack.range.enabled;
		}

		constexpr std::array kGeneralRows{
			Row{ "Enabled", [](const AttackKind&, const Config::Attack& a) -> std::string { return OnOff(a.enabled); } },
			Row{ "Aiming", [](const AttackKind&, const Config::Attack& a) -> std::string { return a.enabled ? OnOff(a.aiming) : kNotApplicable; } },
		};

		constexpr std::array kAccuracyRows{
			Row{ "Accuracy", [](const AttackKind&, const Config::Attack& a) -> std::string { return a.Aims() ? FromTo(a.accuracy.bounds) : kNotApplicable; } },
			Row{ "  Skill", [](const AttackKind& k, const Config::Attack& a) -> std::string { return a.Aims() ? SourceName(k, a.skills.accuracy) : kNotApplicable; } },
			Row{ "  Skill Mult", [](const AttackKind&, const Config::Attack& a) -> std::string { return a.Aims() ? Multiplier(a.accuracy.skillMultiplier) : kNotApplicable; } },
			Row{ "  Blindness", [](const AttackKind&, const Config::Attack& a) -> std::string { return a.Aims() ? Multiplier(a.accuracyBlindness) : kNotApplicable; } },
		};

		constexpr std::array kPrecisionRows{
			Row{ "Precision", [](const AttackKind&, const Config::Attack& a) -> std::string { return !a.Aims() ? kNotApplicable : a.precision.enabled ? FromTo(a.precision.bounds) :
			                                                                                                                                            "Even"; } },
			Row{ "  Skill", [](const AttackKind& k, const Config::Attack& a) -> std::string { return Precise(a) ? SourceName(k, a.skills.precision) : kNotApplicable; } },
			Row{ "  Skill Mult", [](const AttackKind&, const Config::Attack& a) -> std::string { return Precise(a) ? Multiplier(a.precision.skillMultiplier) : kNotApplicable; } },
			Row{ "  Blindness", [](const AttackKind&, const Config::Attack& a) -> std::string { return Precise(a) ? Multiplier(a.precisionBlindness) : kNotApplicable; } },
		};

		constexpr std::array kRetargetRows{
			Row{ "Retargeting", [](const AttackKind& k, const Config::Attack& a) -> std::string { return !k.concentration || !a.Aims() ? kNotApplicable : a.retarget.enabled ? FromTo(a.retarget.bounds) :
			                                                                                                                                                                   "No limit"; } },
			Row{ "  Skill", [](const AttackKind& k, const Config::Attack& a) -> std::string { return Retargets(k, a) ? SourceName(k, a.skills.retarget) : kNotApplicable; } },
			Row{ "  Skill Mult", [](const AttackKind& k, const Config::Attack& a) -> std::string { return Retargets(k, a) ? Multiplier(a.retarget.skillMultiplier) : kNotApplicable; } },
			Row{ "  Blindness", [](const AttackKind& k, const Config::Attack& a) -> std::string { return Retargets(k, a) ? Multiplier(a.retargetBlindness) : kNotApplicable; } },
		};

		constexpr std::array kSpeedRows{
			Row{ "Charge Speed", [](const AttackKind&, const Config::Attack& a) -> std::string { return !a.enabled ? kNotApplicable : a.speed.enabled ? FromTo(a.speed.bounds) :
			                                                                                                                                            "Vanilla"; } },
			Row{ "  Skill", [](const AttackKind& k, const Config::Attack& a) -> std::string { return a.enabled && a.speed.enabled ? SourceName(k, a.skills.speed) : kNotApplicable; } },
			Row{ "  Skill Mult", [](const AttackKind&, const Config::Attack& a) -> std::string { return a.enabled && a.speed.enabled ? Multiplier(a.speed.skillMultiplier) : kNotApplicable; } },
		};

		constexpr std::array kRangeRows{
			Row{ "Range", [](const AttackKind& k, const Config::Attack& a) -> std::string { return !k.range || !a.enabled ? kNotApplicable : a.range.enabled ? FromTo(a.range.bounds) :
			                                                                                                                                                   "Vanilla"; } },
			Row{ "  Skill", [](const AttackKind& k, const Config::Attack& a) -> std::string { return Ranged(k, a) ? SourceName(k, a.skills.range) : kNotApplicable; } },
			Row{ "  Skill Mult", [](const AttackKind& k, const Config::Attack& a) -> std::string { return Ranged(k, a) ? Multiplier(a.range.skillMultiplier) : kNotApplicable; } },
		};

		constexpr std::array kSkillShotRows{
			Row{ "Skill Shots", [](const AttackKind&, const Config::Attack& a) -> std::string { return !a.enabled ? kNotApplicable : a.skillShot.enabled ? fmt::format("{} -> {}", Percent(a.skillShot.bounds.from), Percent(a.skillShot.bounds.to)) :
			                                                                                                                                               "Off"; } },
			Row{ "  Skill", [](const AttackKind& k, const Config::Attack& a) -> std::string { return a.enabled && a.skillShot.enabled ? SourceName(k, a.skills.skillShot) : kNotApplicable; } },
			Row{ "  Skill Mult", [](const AttackKind&, const Config::Attack& a) -> std::string { return a.enabled && a.skillShot.enabled ? Multiplier(a.skillShot.skillMultiplier) : kNotApplicable; } },
		};

		constexpr std::array kSteadyingRows{
			Row{ "Steadying", [](const AttackKind&, const Config::Attack& a) -> std::string { return !a.Aims() ? kNotApplicable : a.steadying.enabled ? "-" + Percent(a.steadying.maxSpreadMultiplier) :
			                                                                                                                                            "Off"; } },
			Row{ "  Half Time", [](const AttackKind&, const Config::Attack& a) -> std::string { return a.Aims() && a.steadying.enabled ? Seconds(a.steadying.halfDuration) : kNotApplicable; } },
		};

		constexpr std::array kFatigueRows{
			Row{ "Fatigue After", [](const AttackKind&, const Config::Attack& a) -> std::string { return !a.Aims() ? kNotApplicable : a.fatigue.enabled ? fmt::format("{:.1f}s -> {:.1f}s", a.fatigue.threshold.from, a.fatigue.threshold.to) :
			                                                                                                                                              "Off"; } },
			Row{ "  Penalty", [](const AttackKind&, const Config::Attack& a) -> std::string { return a.Aims() && a.fatigue.enabled ? "+" + Percent(a.fatigue.maxSpreadMultiplier) : kNotApplicable; } },
			Row{ "  Half Time", [](const AttackKind&, const Config::Attack& a) -> std::string { return a.Aims() && a.fatigue.enabled ? Seconds(a.fatigue.halfDuration) : kNotApplicable; } },
		};

		void LogSeparator() {
			std::string line = fmt::format("{:-<{}}", "", kLabelWidth);
			for (std::size_t i = 0; i < kColumnNames.size(); ++i) {
				line += fmt::format("+{:-<{}}", "", kColumnWidth + 2);
			}
			logger::info("{}", line);
		}

		/// Logs a group of rows below a separator, with a column per attack kind.
		void LogRows(const Config& cfg, std::span<const Row> rows) {
			LogSeparator();
			for (const auto& row : rows) {
				std::string line = fmt::format("{:<{}}", row.label, kLabelWidth);
				for (const AttackKind& kind : AttackKind::kAll) {
					line += fmt::format("| {:^{}} ", row.cell(kind, cfg.*kind.config), kColumnWidth);
				}
				logger::info("{}", line);
			}
		}

		void LogSkillShotTargets(const Config& cfg) {
			std::vector<std::string_view> parts;
			if (cfg.headshots) {
				parts.emplace_back("head");
			}
			if (cfg.armshots) {
				parts.emplace_back("hands");
			}
			if (cfg.legshots) {
				parts.emplace_back("legs");
			}
			if (parts.empty()) {
				logger::info("Skill shot targets: none, NPCs always aim for the chest");
			} else {
				logger::info("Skill shot targets: {}", fmt::join(parts, ", "));
			}
		}

		/// Logs every option of every attack kind as a table. Skill shots are NPC-only: the Player aims by hand.
		void LogConfig(std::string_view actor, const Config& cfg, bool isNPC) {
			std::string header = fmt::format("{:<{}}", actor, kLabelWidth);
			for (const auto* name : kColumnNames) {
				header += fmt::format("| {:^{}} ", name, kColumnWidth);
			}
			logger::info("{}", header);

			LogRows(cfg, kGeneralRows);
			LogRows(cfg, kAccuracyRows);
			LogRows(cfg, kPrecisionRows);
			LogRows(cfg, kRetargetRows);
			LogRows(cfg, kSpeedRows);
			LogRows(cfg, kRangeRows);
			if (isNPC) {
				LogRows(cfg, kSkillShotRows);
			}
			LogRows(cfg, kSteadyingRows);
			LogRows(cfg, kFatigueRows);
			LogSeparator();

			if (isNPC) {
				LogSkillShotTargets(cfg);
			}
		}

		void LogKeywordFilters(const NPCConfig& npc) {
			const bool include = !npc.includeKeywords.empty();
			const bool exclude = !npc.excludeKeywords.empty();

			if (include && exclude) {
				logger::info("Only NPCs that have at least one of these keywords: [{}] and none of these: [{}] will learn to aim", fmt::join(npc.includeKeywords, ", "), fmt::join(npc.excludeKeywords, ", "));
			} else if (include) {
				logger::info("Only NPCs that have at least one of these keywords: [{}] will learn to aim", fmt::join(npc.includeKeywords, ", "));
			} else if (exclude) {
				logger::info("Only NPCs that have none of these keywords: [{}] will learn to aim", fmt::join(npc.excludeKeywords, ", "));
			} else {
				logger::info("All NPCs will learn to aim");
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

		logger::info("{:*^{}}", "OPTIONS", kTableWidth);
		CSimpleIniA ini{};
		ini.SetUnicode();
		if (ini.LoadFile(kIniPath.string().c_str()) >= 0) {
			NPC = NPCConfig::FromIni(ini);
			Player = PlayerConfig::FromIni(ini);
		} else {
			logger::info(R"(Data\SKSE\Plugins\NPCsLearnToAim.ini not found. Default options will be used.)");
			logger::info("");
		}

		if (NPC.enabled) {
			LogConfig("NPC", NPC, true);
			LogKeywordFilters(NPC);
		} else {
			logger::info("Mod is disabled for NPCs");
		}

		logger::info("");
		if (Player.enabled) {
			LogConfig("Player", Player, false);
		} else {
			logger::info("Mod is disabled for Player");
		}

		logger::info("{:*^{}}", "", kTableWidth);
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

		if (SupportsSkillShots(defaults)) {
			result.headshots = ini.GetBoolValue(a_section, "bHeadshots", result.headshots);
			result.armshots = ini.GetBoolValue(a_section, "bArmshots", result.armshots);
			result.legshots = ini.GetBoolValue(a_section, "bLegshots", result.legshots);
		} else {
			WarnUnsupported(ini, a_section, std::array{ Key{ "bHeadshots" }, Key{ "bArmshots" }, Key{ "bLegshots" } }, kUnsupported);
		}

		for (const AttackKind& kind : AttackKind::kAll) {
			const Keys& keys = Keys::For(kind);
			auto&       attack = result.*kind.config;
			const auto& defaultAttack = defaults.*kind.config;

			attack.enabled = ini.GetBoolValue(a_section, keys.enabled.c_str(), attack.enabled);
			attack.aiming = ini.GetBoolValue(a_section, keys.aiming.c_str(), attack.aiming);

			attack.accuracy = Aim::FromIni(ini, a_section, keys.accuracy, attack.accuracy, Limits::kAccuracyFactor, false);
			attack.precision = Aim::FromIni(ini, a_section, keys.precision, attack.precision, Limits::kFocus);
			attack.accuracyBlindness = ReadClamped(ini, a_section, keys.accuracyBlindness, attack.accuracyBlindness, Limits::kAccuracyBlindness);
			attack.precisionBlindness = ReadClamped(ini, a_section, keys.precisionBlindness, attack.precisionBlindness, Limits::kPrecisionBlindness);
			if (kind.concentration) {
				attack.retarget = Aim::FromIni(ini, a_section, keys.retarget, attack.retarget, Limits::kRetarget);
				attack.retargetBlindness = ReadClamped(ini, a_section, keys.retargetBlindness, attack.retargetBlindness, Limits::kRetargetBlindness);
			} else {
				WarnUnsupported(ini, a_section, keys.retarget.All(), "only concentration spells and staves support retargeting");
				WarnUnsupported(ini, a_section, std::array{ keys.retargetBlindness }, "only concentration spells and staves support retargeting");
			}

			attack.skills = SkillSources::FromIni(ini, a_section, keys.skills, attack.skills, kind.concentration, defaultAttack.skillShot != Config::Scaling::kNone);

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

	Config::SkillSources Config::SkillSources::FromIni(CSimpleIniA& ini, const char* a_section, const Keys::SkillSources& keys, const SkillSources& defaults, bool retarget, bool skillShot) {
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
		result.accuracy = read(keys.accuracy, result.accuracy);
		result.precision = read(keys.precision, result.precision);
		if (retarget) {
			result.retarget = read(keys.retarget, result.retarget);
		} else {
			WarnUnsupported(ini, a_section, std::array{ keys.retarget }, "only concentration spells and staves support retargeting");
		}
		if (skillShot) {
			result.skillShot = read(keys.skillShot, result.skillShot);
		} else {
			WarnUnsupported(ini, a_section, std::array{ keys.skillShot }, kUnsupported);
		}
		result.speed = read(keys.speed, result.speed);
		if (defaults.range != SkillSource::kTotal) {
			result.range = read(keys.range, result.range);
		} else {
			WarnUnsupported(ini, a_section, std::array{ keys.range }, kNoRange);
		}
		return result;
	}

	Config::Aim Config::Aim::FromIni(CSimpleIniA& ini, const char* a_section, const Keys::Aim& keys, const Aim& defaults, Limit limit, bool withToggle) {
		Aim result = defaults;
		if (withToggle) {
			result.enabled = ini.GetBoolValue(a_section, keys.enabled.c_str(), result.enabled);
		}
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
