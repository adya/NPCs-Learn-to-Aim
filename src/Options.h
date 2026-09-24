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
	struct Config
	{
		struct SkillMultiplier
		{
			float bow;
			float crossbow;
			float spell;
			float concentrationSpell;
			float staff;
			float scroll;

			float blindness;

			static SkillMultiplier FromIni(CSimpleIniA& ini, const char* section, std::string_view prefix, const SkillMultiplier& defaults);
		};

		/// Holding a fully charged spell steadies the caster's aim: the longer it's held, the smaller the spread.
		struct SpellOvercharge
		{
			bool enabled = true;

			/// The spread reduction (0-1) approached as overcharge time grows.
			float maxSpreadReduction = 0.5f;

			/// Seconds of overcharge to gain half of the max reduction.
			/// Note, the reduction is not linear, so 2 * halfDuration does not give the full reduction.
			float halfDuration = 1.0f;

			SpellOvercharge() = default;
			SpellOvercharge(CSimpleIniA& ini, const char* section, std::string_view name, const SpellOvercharge& defaults);
		};

		/// Controls how fast a spell/staff/scroll charges/casts, and whether that scales with skill.
		struct ChargeSpeed
		{
			bool  enabled = true;
			float minMultiplier = 1.0f;
			float maxMultiplier = 1.5f;

			ChargeSpeed() = default;
			ChargeSpeed(CSimpleIniA& ini, const char* section, std::string_view prefix, const ChargeSpeed& defaults);
		};

		/// Controls whether the mod works for given characters.
		bool enabled = true;

		bool spellAiming = true;
		bool staffAiming = true;
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

		/// If true, concentration spells require continuous accuracy, where the beam/flow will wander around the target.
		bool concentrationSpellsAiming = true;

		/// If true, concentration spells would reach farther as the skill in corresponding school increases.
		bool concentrationSpellRangeScaling = true;

		float concentrationSpellMinRangeMultiplier = 1.2f;
		float concentrationSpellMaxRangeMultiplier = 2.5f;

		ChargeSpeed spellChargeSpeed;
		ChargeSpeed concentrationSpellChargeSpeed;
		ChargeSpeed staffChargeSpeed;
		ChargeSpeed scrollChargeSpeed;

		SpellOvercharge spellOvercharge;
		SpellOvercharge staffOvercharge;
		SpellOvercharge scrollOvercharge;

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

		inline const SpellOvercharge& SpellOverchargeFor(WeaponType type) const {
			assert(type == kSpell || type == kStaff || type == kScroll);
			switch (type) {
			case kStaff:
				return staffOvercharge;
			case kScroll:
				return scrollOvercharge;
			default:
				return spellOvercharge;
			}
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
			.bowAiming = true,
			.crossbowAiming = true,
			.accuracyMultipliers = { .bow = 1.0f, .crossbow = 1.0f, .spell = 1.0f, .concentrationSpell = 1.0f, .staff = 1.0f, .scroll = 1.0f, .blindness = 1.0f },
			.precisionMultipliers = { .bow = 1.4f, .crossbow = 7.0f, .spell = 1.25f, .concentrationSpell = 1.25f, .staff = 1.55f, .scroll = 1.25f, .blindness = 0.0f },
			.stavesUseEnchantingSkill = false,
			.spellsUseHighestMagicSkill = false,
			.concentrationSpellsAiming = true,
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

			HeadshotSettings() = default;
			HeadshotSettings(CSimpleIniA& ini, const char* section, const HeadshotSettings& defaults);
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
			.bowAiming = true,
			.crossbowAiming = true,
			.accuracyMultipliers = { .bow = 1.0f, .crossbow = 0.85f, .spell = 1.15f, .concentrationSpell = 1.15f, .staff = 0.85f, .scroll = 1.15f, .blindness = 0.2f },
			.precisionMultipliers = { .bow = 1.0f, .crossbow = 7.0f, .spell = 0.85f, .concentrationSpell = 0.85f, .staff = 1.15f, .scroll = 0.85f, .blindness = 1.5f },
			.stavesUseEnchantingSkill = false,
			.spellsUseHighestMagicSkill = false,
			.concentrationSpellsAiming = true,
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
