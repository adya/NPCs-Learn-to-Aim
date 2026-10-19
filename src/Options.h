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
		kHeadshot,
		kFatigueThreshold
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
	struct Config
	{
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

		/// Controls how much holding a bow at full draw reduces spread.
		using Steadying = SpreadOptions;

		struct SpeedOptions
		{
			bool enabled = true;

			/// Speed multiplier at 0 skill.
			float minMultiplier = 1.0f;

			/// Speed multiplier at 100 skill.
			float maxMultiplier = 1.5f;

			static SpeedOptions FromIni(CSimpleIniA& ini, const char* section, std::string_view prefix, const SpeedOptions& defaults);
		};

		/// Controls how fast a spell/staff/scroll charges.
		using ChargeSpeed = SpeedOptions;

		/// Controls how fast a bow is drawn and crossbow is loaded.
		using DrawSpeed = SpeedOptions;

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
		bool concentrationSpellsAiming = true;
		bool concentrationStaffAiming = true;
		bool scrollAiming = true;
		bool bowAiming = true;
		bool crossbowAiming = true;

		SkillMultiplier accuracyMultipliers;
		SkillMultiplier precisionMultipliers;

		/// If true, being blinded further affects aiming according to the blindness multipliers.
		bool blindnessAffectsAiming = true;

		/// If true, staves use the enchanting skill instead of the spell's primary effect's school skill.
		/// Mutually exclusive with stavesUseHighestMagicSkill.
		bool stavesUseEnchantingSkill = false;

		/// If true, staves use the highest NPC's magic skill instead of the spell's primary effect's school skill.
		/// Mutually exclusive with stavesUseEnchantingSkill.
		bool stavesUseHighestMagicSkill = false;

		/// If true, concentration staves use the enchanting skill instead of the spell's primary effect's school skill.
		/// Mutually exclusive with concentrationStavesUseHighestMagicSkill.
		bool concentrationStavesUseEnchantingSkill = false;

		/// If true, concentration staves use the highest NPC's magic skill instead of the spell's primary effect's school skill.
		/// Mutually exclusive with concentrationStavesUseEnchantingSkill.
		bool concentrationStavesUseHighestMagicSkill = false;

		/// If true, the highest NPC's magic skill is used to aim fire-and-forget spells, instead of the spell's primary effect's school skill.
		bool spellsUseHighestMagicSkill = false;

		/// If true, the highest NPC's magic skill is used to aim concentration spells, instead of the spell's primary effect's school skill.
		bool concentrationSpellsUseHighestMagicSkill = false;

		/// If true, scrolls use the enchanting skill instead of the scroll spell's primary effect's school skill.
		/// Mutually exclusive with scrollsUseHighestMagicSkill.
		bool scrollsUseEnchantingSkill = false;

		/// If true, the highest NPC's magic skill is used to aim scrolls, instead of the scroll spell's primary effect's school skill.
		/// Mutually exclusive with scrollsUseEnchantingSkill.
		bool scrollsUseHighestMagicSkill = false;

		/// If true, concentration spells would reach farther as the skill in corresponding school increases.
		bool concentrationSpellRangeScaling = true;
		float concentrationSpellMinRangeMultiplier = 1.2f;
		float concentrationSpellMaxRangeMultiplier = 2.5f;

		bool  concentrationStaffRangeScaling = true;
		float concentrationStaffMinRangeMultiplier = 1.2f;
		float concentrationStaffMaxRangeMultiplier = 2.5f;

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
			.concentrationSpellsAiming = true,
			.bowAiming = true,
			.crossbowAiming = true,
			.accuracyMultipliers = { .bow = 1.0f, .crossbow = 1.0f, .spell = 1.0f, .concentrationSpell = 1.0f, .staff = 1.0f, .concentrationStaff = 1.0f, .scroll = 1.0f, .blindness = 1.0f },
			.precisionMultipliers = { .bow = 1.4f, .crossbow = 7.0f, .spell = 1.25f, .concentrationSpell = 1.25f, .staff = 1.55f, .concentrationStaff = 1.55f, .scroll = 1.25f, .blindness = 0.0f },
			.stavesUseEnchantingSkill = false,
			.spellsUseHighestMagicSkill = false,
			.spellSteadying = { .maxSpreadMultiplier = 0.5f, .halfDuration = 1.0f },
			.staffSteadying = { .maxSpreadMultiplier = 0.5f, .halfDuration = 1.0f },
			.scrollSteadying = { .maxSpreadMultiplier = 0.5f, .halfDuration = 1.0f },
			.spellFatigue = { { .enabled = true, .maxSpreadMultiplier = 1.0f, .halfDuration = 1.5f }, 1.0f, 4.0f },
			.staffFatigue = { { .enabled = false, .maxSpreadMultiplier = 1.0f, .halfDuration = 1.5f }, 1.0f, 4.0f },
			.scrollFatigue = { { .enabled = false, .maxSpreadMultiplier = 1.0f, .halfDuration = 1.5f }, 1.0f, 4.0f },
			.concentrationSpellFatigue = { { .enabled = true, .maxSpreadMultiplier = 1.0f, .halfDuration = 3.0f }, 2.0f, 6.0f },
			.concentrationStaffFatigue = { { .enabled = false, .maxSpreadMultiplier = 1.0f, .halfDuration = 3.0f }, 2.0f, 6.0f },
			.bowSteadying = { .maxSpreadMultiplier = 0.4f, .halfDuration = 1.0f },
			.bowFatigue = { { .maxSpreadMultiplier = 1.0f, .halfDuration = 1.5f }, 0.5f, 2.5f },
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
			.concentrationSpellsAiming = true,
			.bowAiming = true,
			.crossbowAiming = true,
			.accuracyMultipliers = { .bow = 1.0f, .crossbow = 0.85f, .spell = 1.15f, .concentrationSpell = 1.15f, .staff = 0.85f, .concentrationStaff = 0.85f, .scroll = 1.15f, .blindness = 0.2f },
			.precisionMultipliers = { .bow = 1.0f, .crossbow = 7.0f, .spell = 0.85f, .concentrationSpell = 0.85f, .staff = 1.15f, .concentrationStaff = 1.15f, .scroll = 0.85f, .blindness = 1.5f },
			.stavesUseEnchantingSkill = false,
			.spellsUseHighestMagicSkill = false,
			.spellSteadying = { .maxSpreadMultiplier = 0.5f, .halfDuration = 1.0f },
			.staffSteadying = { .maxSpreadMultiplier = 0.5f, .halfDuration = 1.0f },
			.scrollSteadying = { .maxSpreadMultiplier = 0.5f, .halfDuration = 1.0f },
			.spellFatigue = { { .enabled = true, .maxSpreadMultiplier = 1.0f, .halfDuration = 1.5f }, 1.0f, 4.0f },
			.staffFatigue = { { .enabled = false, .maxSpreadMultiplier = 1.0f, .halfDuration = 1.5f }, 1.0f, 4.0f },
			.scrollFatigue = { { .enabled = false, .maxSpreadMultiplier = 1.0f, .halfDuration = 1.5f }, 1.0f, 4.0f },
			.concentrationSpellFatigue = { { .enabled = true, .maxSpreadMultiplier = 1.0f, .halfDuration = 3.0f }, 2.0f, 6.0f },
			.concentrationStaffFatigue = { { .enabled = false, .maxSpreadMultiplier = 1.0f, .halfDuration = 3.0f }, 2.0f, 6.0f },
			.bowSteadying = { .maxSpreadMultiplier = 0.4f, .halfDuration = 0.5f },
			.bowFatigue = { { .maxSpreadMultiplier = 1.0f, .halfDuration = 1.5f }, 0.5f, 2.5f },
		},
		{},
		{},
		{ "ActorTypeNPC" },
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
	static float fCombatAimProjectileRandomOffset() {
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("fCombatAimProjectileRandomOffset")) {
			return setting->data.f;
		}
		return 16.0f;
	}

	static float fCombatRangedAimVariance() {
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("fCombatRangedAimVariance")) {
			return setting->data.f;
		}
		return 0.9f;
	}

	static float fBowNPCSpreadAngle() {
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("fBowNPCSpreadAngle")) {
			return setting->data.f;
		}
		return 4.0f;
	}
}
