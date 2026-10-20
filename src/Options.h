#pragma once

#include <cassert>

namespace NLA
{
	enum SkillUsageContext
	{
		kAccuracy,
		kPrecision,

		kChargeTime,
		kRange,
		kHeadshot
	};

	enum WeaponType : std::uint8_t
	{
		kNone = 0,
		kBow = 1,
		kCrossbow = 2,
		kSpell = 3,
		kStaff = 4,
		kScroll = 5
	};
}

namespace NLA::Options
{
	/// The skill that scales a spell type's aiming, charge speed or range.
	enum class SkillSource : std::uint8_t
	{
		/// The skill of the magic school of the spell's costliest effect.
		kSpellSkill,
		/// The highest of the actor's magic skills, regardless of the spell.
		kHighest,
		kEnchanting
	};

	/// The allowed range of an option.
	struct Limit
	{
		float min;
		float max;
	};

	namespace Limits
	{
		inline constexpr Limit kSkillMultiplier{ 0.01f, 5.0f };
		inline constexpr Limit kBlindnessMultiplier{ 0.0f, 2.0f };
		inline constexpr Limit kHeadshotChance{ 0.0f, 1.0f };
		inline constexpr Limit kRangeMultiplier{ 0.5f, 5.0f };
		inline constexpr Limit kSpeedMultiplier{ 0.1f, 3.0f };
		inline constexpr Limit kSteadyingReduction{ 0.0f, 1.0f };
		inline constexpr Limit kFatigueThreshold{ 0.0f, 10.0f };
		inline constexpr Limit kFatiguePenalty{ 0.0f, 3.0f };
		inline constexpr Limit kHalfDuration{ 0.1f, 5.0f };
	}

	struct Config
	{
		struct SkillSources
		{
			SkillSource aiming = SkillSource::kSpellSkill;
			SkillSource chargeSpeed = SkillSource::kSpellSkill;

			/// Only used by concentration spells and staves.
			SkillSource range = SkillSource::kSpellSkill;

			static SkillSources FromIni(CSimpleIniA& ini, const char* section, std::string_view prefix, const SkillSources& defaults);
		};

		struct SkillMultiplier
		{
			float bow;
			float crossbow;
			float spell;
			float concentrationSpell;
			float staff;
			float concentrationStaff;
			float scroll;

			float blindness;

			static SkillMultiplier FromIni(CSimpleIniA& ini, const char* section, std::string_view prefix, const SkillMultiplier& defaults);
		};

		struct SpreadOptions
		{
			bool enabled = true;

			/// Max multiplier for the spread approached as hold time increases.
			float maxSpreadMultiplier;

			/// Time needed to gain half of the max spread multiplier.
			/// Note, the multiplier is not linear, so 2 * halfDuration does not give the full multiplier.
			float halfDuration;

			static SpreadOptions FromIni(CSimpleIniA& ini, const char* section, std::string_view prefix, const SpreadOptions& defaults);
		};

		/// Controls how much holding a bow at full draw, a crossbow loaded or a spell/staff/scroll charged reduces spread.
		using Steadying = SpreadOptions;

		/// A multiplier that grows with skill from `minMultiplier` to `maxMultiplier`.
		struct ScalingOptions
		{
			bool enabled = true;

			/// Multiplier at 0 skill.
			float minMultiplier = 1.0f;

			/// Multiplier at 100 skill.
			float maxMultiplier = 1.5f;

			static ScalingOptions FromIni(CSimpleIniA& ini, const char* section, std::string_view prefix, const ScalingOptions& defaults, Limit limit);
		};

		/// Controls how fast a spell/staff/scroll charges.
		using ChargeSpeed = ScalingOptions;

		/// Controls how fast a bow is drawn and crossbow is loaded.
		using DrawSpeed = ScalingOptions;

		/// Controls how far concentration spells/staves reach.
		using RangeScaling = ScalingOptions;

		struct FatigueOptions : SpreadOptions
		{
			/// Duration in seconds that must pass before fatigue sets in for an actor with 0 max Stamina (bows) or Magicka (spells).
			float minThreshold;

			/// Duration in seconds that must pass before fatigue sets in for an actor with 100 max Stamina (bows) or Magicka (spells).
			float maxThreshold;

			/// `prefix` is the full ini key prefix, verb included (e.g. "BowFatigue").
			static FatigueOptions FromIni(CSimpleIniA& ini, const char* section, std::string_view prefix, const FatigueOptions& defaults);
		};

		/// Controls whether the mod works for given characters.
		bool enabled = true;

		bool spellAiming = true;
		bool staffAiming = true;
		bool concentrationSpellAiming = true;
		bool concentrationStaffAiming = true;
		bool scrollAiming = true;
		bool bowAiming = true;
		bool crossbowAiming = true;

		SkillMultiplier accuracyMultipliers;
		SkillMultiplier precisionMultipliers;

		/// If true, being blinded further affects aiming according to the blindness multipliers.
		bool blindnessAffectsAiming = true;

		/// Which skill drives aiming, charge speed and range for each spell type.
		SkillSources spellSkills;
		SkillSources concentrationSpellSkills;
		SkillSources staffSkills;
		SkillSources concentrationStaffSkills;
		SkillSources scrollSkills;

		/// If enabled, concentration spells reach farther as the skill increases.
		RangeScaling concentrationSpellRange;
		RangeScaling concentrationStaffRange;

		ChargeSpeed spellChargeSpeed;
		ChargeSpeed concentrationSpellChargeSpeed;
		ChargeSpeed staffChargeSpeed;
		ChargeSpeed concentrationStaffChargeSpeed;
		ChargeSpeed scrollChargeSpeed;

		Steadying spellSteadying;
		Steadying staffSteadying;
		Steadying scrollSteadying;

		/// Controls how long a spell/staff/scroll can be held charged before fatigue starts to affect accuracy.
		FatigueOptions spellFatigue;
		FatigueOptions staffFatigue;
		FatigueOptions scrollFatigue;

		/// Controls how long a concentration spell/staff can be cast continuously before fatigue starts to affect accuracy.
		FatigueOptions concentrationSpellFatigue;
		FatigueOptions concentrationStaffFatigue;

		/// Controls how fast a bow reaches full draw.
		DrawSpeed bowDrawSpeed;

		/// Controls how fast a crossbow is reloaded.
		DrawSpeed crossbowDrawSpeed;

		Steadying bowSteadying;

		/// Controls how long a bow can be held fully drawn before fatigue starts to affect accuracy.
		FatigueOptions bowFatigue;

		/// Controls how much holding a loaded crossbow reduces spread.
		Steadying crossbowSteadying;

		/// Controls how long a crossbow can be held loaded before fatigue starts to affect accuracy.
		FatigueOptions crossbowFatigue;

		static Config FromIni(CSimpleIniA& ini, const char* section, const Config& defaults);

		inline const SkillMultiplier& MultiplierFor(SkillUsageContext context) const {
			switch (context) {
			case kAccuracy:
				return accuracyMultipliers;
			case kPrecision:
				return precisionMultipliers;
			default:
				return accuracyMultipliers;
			}
		}

		inline const Steadying& SpellSteadyingFor(WeaponType type) const {
			assert(type == kSpell || type == kStaff || type == kScroll);
			switch (type) {
			case kStaff:
				return staffSteadying;
			case kScroll:
				return scrollSteadying;
			default:
				return spellSteadying;
			}
		}

		inline const FatigueOptions& SpellFatigueFor(WeaponType type) const {
			assert(type == kSpell || type == kStaff || type == kScroll);
			switch (type) {
			case kStaff:
				return staffFatigue;
			case kScroll:
				return scrollFatigue;
			default:
				return spellFatigue;
			}
		}

		/// Steadying while holding a drawn bow or loaded crossbow.
		inline const Steadying& WeaponSteadyingFor(WeaponType type) const {
			assert(type == kBow || type == kCrossbow);
			return type == kCrossbow ? crossbowSteadying : bowSteadying;
		}

		/// Fatigue while holding a drawn bow or loaded crossbow.
		inline const FatigueOptions& WeaponFatigueFor(WeaponType type) const {
			assert(type == kBow || type == kCrossbow);
			return type == kCrossbow ? crossbowFatigue : bowFatigue;
		}

		inline const FatigueOptions& ConcentrationFatigueFor(WeaponType type) const {
			assert(type == kSpell || type == kStaff);
			return type == kStaff ? concentrationStaffFatigue : concentrationSpellFatigue;
		}
	};

	struct PlayerConfig : Config
	{
		static PlayerConfig FromIni(CSimpleIniA& ini);

		static const PlayerConfig defaultConfig;
	};
	inline const PlayerConfig PlayerConfig::defaultConfig{
		Config{
			.spellAiming = true,
			.staffAiming = true,
			.concentrationSpellAiming = true,
			.concentrationStaffAiming = true,
			.scrollAiming = true,
			.bowAiming = true,
			.crossbowAiming = true,
			.accuracyMultipliers = { .bow = 1.0f, .crossbow = 1.0f, .spell = 1.0f, .concentrationSpell = 1.0f, .staff = 1.0f, .concentrationStaff = 1.0f, .scroll = 1.0f, .blindness = 1.0f },
			.precisionMultipliers = { .bow = 1.4f, .crossbow = 3.0f, .spell = 1.4f, .concentrationSpell = 1.4f, .staff = 1.25f, .concentrationStaff = 1.25f, .scroll = 1.4f, .blindness = 0.5f },
			.spellSkills = { .aiming = SkillSource::kHighest, .chargeSpeed = SkillSource::kSpellSkill },
			.concentrationSpellSkills = { .aiming = SkillSource::kHighest, .chargeSpeed = SkillSource::kSpellSkill, .range = SkillSource::kSpellSkill },
			.staffSkills = { .aiming = SkillSource::kHighest, .chargeSpeed = SkillSource::kEnchanting },
			.concentrationStaffSkills = { .aiming = SkillSource::kHighest, .chargeSpeed = SkillSource::kEnchanting, .range = SkillSource::kEnchanting },
			.scrollSkills = { .aiming = SkillSource::kHighest, .chargeSpeed = SkillSource::kEnchanting },
			.concentrationSpellRange = { .minMultiplier = 0.8f, .maxMultiplier = 3.0f },
			.concentrationStaffRange = { .minMultiplier = 1.45f, .maxMultiplier = 2.0f },
			.spellChargeSpeed = { .minMultiplier = 0.35f, .maxMultiplier = 2.0f },
			.concentrationSpellChargeSpeed = { .minMultiplier = 0.35f, .maxMultiplier = 2.0f },
			.staffChargeSpeed = { .minMultiplier = 0.74f, .maxMultiplier = 1.4f },
			.concentrationStaffChargeSpeed = { .minMultiplier = 0.74f, .maxMultiplier = 1.4f },
			.scrollChargeSpeed = { .minMultiplier = 0.35f, .maxMultiplier = 2.0f },
			.spellSteadying = { .maxSpreadMultiplier = 0.5f, .halfDuration = 2.0f },
			.staffSteadying = { .maxSpreadMultiplier = 0.3f, .halfDuration = 3.0f },
			.scrollSteadying = { .maxSpreadMultiplier = 0.5f, .halfDuration = 2.0f },
			.spellFatigue = { { .enabled = false, .maxSpreadMultiplier = 1.0f, .halfDuration = 1.5f }, 1.0f, 4.0f },
			.staffFatigue = { { .enabled = false, .maxSpreadMultiplier = 1.0f, .halfDuration = 1.5f }, 1.0f, 4.0f },
			.scrollFatigue = { { .enabled = false, .maxSpreadMultiplier = 1.0f, .halfDuration = 1.5f }, 1.0f, 4.0f },
			.concentrationSpellFatigue = { { .enabled = true, .maxSpreadMultiplier = 1.0f, .halfDuration = 3.0f }, 2.0f, 6.0f },
			.concentrationStaffFatigue = { { .enabled = false, .maxSpreadMultiplier = 1.0f, .halfDuration = 3.0f }, 2.0f, 6.0f },
			.bowDrawSpeed = { .minMultiplier = 0.57f, .maxMultiplier = 2.0f },
			.crossbowDrawSpeed = { .minMultiplier = 0.87f, .maxMultiplier = 1.2f },
			.bowSteadying = { .maxSpreadMultiplier = 0.5f, .halfDuration = 2.0f },
			.bowFatigue = { { .maxSpreadMultiplier = 1.0f, .halfDuration = 1.5f }, 0.5f, 2.5f },
			.crossbowSteadying = { .maxSpreadMultiplier = 0.3f, .halfDuration = 3.0f },
			.crossbowFatigue = { { .enabled = false, .maxSpreadMultiplier = 1.0f, .halfDuration = 1.5f }, 0.5f, 2.5f },
		},
	};

	struct NPCConfig : Config
	{
		struct HeadshotSettings
		{
			bool  bow = true;
			float bowMinChance = 0.0f;
			float bowMaxChance = 0.35f;

			bool  crossbow = true;
			float crossbowMinChance = 0.0f;
			float crossbowMaxChance = 0.35f;

			static HeadshotSettings FromIni(CSimpleIniA& ini, const char* section, const HeadshotSettings& defaults);
		};
		HeadshotSettings headshots{};

		std::vector<std::string> excludeKeywords{};
		std::vector<std::string> includeKeywords{};

		static NPCConfig FromIni(CSimpleIniA& ini);

		bool ShouldLearn(RE::Actor* actor);

		static const NPCConfig defaultConfig;
	};
	inline const NPCConfig NPCConfig::defaultConfig{
		Config{
			.spellAiming = true,
			.staffAiming = true,
			.concentrationSpellAiming = true,
			.concentrationStaffAiming = true,
			.scrollAiming = true,
			.bowAiming = true,
			.crossbowAiming = true,
			.accuracyMultipliers = { .bow = 1.0f, .crossbow = 0.85f, .spell = 1.0f, .concentrationSpell = 1.0f, .staff = 0.85f, .concentrationStaff = 0.85f, .scroll = 1.0f, .blindness = 0.2f },
			.precisionMultipliers = { .bow = 1.0f, .crossbow = 3.0f, .spell = 1.0f, .concentrationSpell = 1.0f, .staff = 0.85f, .concentrationStaff = 0.85f, .scroll = 1.0f, .blindness = 1.5f },
			.spellSkills = { .aiming = SkillSource::kHighest, .chargeSpeed = SkillSource::kSpellSkill },
			.concentrationSpellSkills = { .aiming = SkillSource::kHighest, .chargeSpeed = SkillSource::kSpellSkill, .range = SkillSource::kSpellSkill },
			.staffSkills = { .aiming = SkillSource::kHighest, .chargeSpeed = SkillSource::kEnchanting },
			.concentrationStaffSkills = { .aiming = SkillSource::kHighest, .chargeSpeed = SkillSource::kEnchanting, .range = SkillSource::kEnchanting },
			.scrollSkills = { .aiming = SkillSource::kHighest, .chargeSpeed = SkillSource::kEnchanting },
			.concentrationSpellRange = { .minMultiplier = 0.8f, .maxMultiplier = 3.0f },
			.concentrationStaffRange = { .minMultiplier = 1.45f, .maxMultiplier = 2.0f },
			.spellChargeSpeed = { .minMultiplier = 0.35f, .maxMultiplier = 2.0f },
			.concentrationSpellChargeSpeed = { .minMultiplier = 0.35f, .maxMultiplier = 2.0f },
			.staffChargeSpeed = { .minMultiplier = 0.74f, .maxMultiplier = 1.4f },
			.concentrationStaffChargeSpeed = { .minMultiplier = 0.74f, .maxMultiplier = 1.4f },
			.scrollChargeSpeed = { .minMultiplier = 0.35f, .maxMultiplier = 2.0f },
			.spellSteadying = { .maxSpreadMultiplier = 0.5f, .halfDuration = 2.0f },
			.staffSteadying = { .maxSpreadMultiplier = 0.3f, .halfDuration = 3.0f },
			.scrollSteadying = { .maxSpreadMultiplier = 0.5f, .halfDuration = 2.0f },
			.spellFatigue = { { .enabled = false, .maxSpreadMultiplier = 1.0f, .halfDuration = 1.5f }, 1.0f, 4.0f },
			.staffFatigue = { { .enabled = false, .maxSpreadMultiplier = 1.0f, .halfDuration = 1.5f }, 1.0f, 4.0f },
			.scrollFatigue = { { .enabled = false, .maxSpreadMultiplier = 1.0f, .halfDuration = 1.5f }, 1.0f, 4.0f },
			.concentrationSpellFatigue = { { .enabled = true, .maxSpreadMultiplier = 1.0f, .halfDuration = 3.0f }, 2.0f, 6.0f },
			.concentrationStaffFatigue = { { .enabled = false, .maxSpreadMultiplier = 1.0f, .halfDuration = 3.0f }, 2.0f, 6.0f },
			.bowDrawSpeed = { .minMultiplier = 0.57f, .maxMultiplier = 2.0f },
			.crossbowDrawSpeed = { .minMultiplier = 0.87f, .maxMultiplier = 1.2f },
			.bowSteadying = { .maxSpreadMultiplier = 0.5f, .halfDuration = 2.0f },
			.bowFatigue = { { .maxSpreadMultiplier = 1.0f, .halfDuration = 1.5f }, 0.5f, 2.5f },
			.crossbowSteadying = { .maxSpreadMultiplier = 0.3f, .halfDuration = 3.0f },
			.crossbowFatigue = { { .enabled = false, .maxSpreadMultiplier = 1.0f, .halfDuration = 1.5f }, 0.5f, 2.5f },
		},
		{},
		{},
		{ "ActorTypeNPC", "UpdateDuringArchery" },
	};

	inline NPCConfig    NPC{ NPCConfig::defaultConfig };
	inline PlayerConfig Player{ PlayerConfig::defaultConfig };

	/// Returns configuration for given actor. Note, that only common configuration is returned.
	inline const Config& For(const RE::Actor* actor) {
		if (actor && actor->IsPlayerRef()) {
			return Player;
		} else {
			return NPC;
		}
	}
	/// Splits a comma-separated list of keyword EditorIDs, trimming whitespace around each and dropping empty entries.
	std::vector<std::string> ParseKeywords(std::string_view list);

	/// Forgets the cached keyword filter results of NPCs. Must be called whenever anything affecting `NPCConfig::ShouldLearn` changes.
	void InvalidateActorCache();

	void Load();
	void Save();

	/// Writes all options at their current values to the INI file.
	void Export();

	void ResetNPC();
	void ResetPlayer();

	/// An incremental identifier that can be used to track when Options are reloaded.
	inline unsigned int currentID = 0;
}

namespace NLA::Settings
{
	inline float fCombatAimProjectileRandomOffset() {
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("fCombatAimProjectileRandomOffset")) {
			return setting->data.f;
		}
		return 16.0f;
	}

	inline float fCombatRangedAimVariance() {
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("fCombatRangedAimVariance")) {
			return setting->data.f;
		}
		return 0.9f;
	}

	inline float fBowNPCSpreadAngle() {
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("fBowNPCSpreadAngle")) {
			return setting->data.f;
		}
		return 4.0f;
	}
}
