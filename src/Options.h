#pragma once

namespace NLA
{
	enum SkillUsageContext
	{
		kAccuracy,
		kPrecision
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

			float blindness;

			SkillMultiplier() = default;
			SkillMultiplier(CSimpleIniA& ini, const char* section, std::string_view prefix, const SkillMultiplier& defaults);
			SkillMultiplier(float bow, float crossbow, float spell, float concentrationSpell, float staff, float blindness) :
				bow(bow), crossbow(crossbow), spell(spell), concentrationSpell(concentrationSpell), staff(staff), blindness(blindness) {}
		};

		bool spellAiming = true;
		bool staffAiming = true;
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

		/// If true, concentration spells require continuous accuracy, where the beam/flow will wander around the target.
		bool concentrationSpellsAiming = true;

		/// If true, concentration spells would reach farther as the skill in corresponding school increases.
		bool concentrationSpellRangeScaling = true;

		float concentrationSpellMinRangeMultiplier = 1.2f;
		float concentrationSpellMaxRangeMultiplier = 2.5f;

		Config() = default;

		Config(CSimpleIniA& ini, const char* section, const Config& defaults);
		Config(bool            spellAiming,
		       bool            staffAiming,
		       bool            bowAiming,
		       bool            crossbowAiming,
		       bool            concentrationSpellsAiming,
		       SkillMultiplier accuracyMultipliers,
		       SkillMultiplier precisionMultipliers,
		       bool            stavesUseEnchantingSkill,
		       bool            spellsUseHighestMagicSkill) :
			spellAiming(spellAiming),
			staffAiming(staffAiming),
			bowAiming(bowAiming),
			crossbowAiming(crossbowAiming),
			concentrationSpellsAiming(concentrationSpellsAiming),
			accuracyMultipliers(accuracyMultipliers),
			precisionMultipliers(precisionMultipliers),
			stavesUseEnchantingSkill(stavesUseEnchantingSkill),
			spellsUseHighestMagicSkill(spellsUseHighestMagicSkill) {}

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
	};

	struct PlayerConfig : Config
	{
		PlayerConfig() = default;
		PlayerConfig(CSimpleIniA& ini);
		PlayerConfig(bool            spellAiming,
		             bool            staffAiming,
		             bool            bowAiming,
		             bool            crossbowAiming,
		             bool            concentrationSpellsAiming,
		             SkillMultiplier accuracyMultipliers,
		             SkillMultiplier precisionMultipliers,
		             bool            stavesUseEnchantingSkill,
		             bool            spellsUseHighestMagicSkill) :
			Config(spellAiming,
		           staffAiming,
		           bowAiming,
		           crossbowAiming,
		           concentrationSpellsAiming,
		           accuracyMultipliers,
		           precisionMultipliers,
		           stavesUseEnchantingSkill,
		           spellsUseHighestMagicSkill) {}

		static const PlayerConfig defaultConfig;
	};
	inline const PlayerConfig PlayerConfig::defaultConfig{ true, true, true, true, true, { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f }, { 1.4f, 7.0f, 1.25f, 1.25f, 1.55f, 0.0f }, false, false };

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

		NPCConfig() = default;
		NPCConfig(CSimpleIniA& ini);
		NPCConfig(bool                     spellAiming,
		          bool                     staffAiming,
		          bool                     bowAiming,
		          bool                     crossbowAiming,
		          bool                     concentrationSpellsAiming,
		          SkillMultiplier          accuracyMultipliers,
		          SkillMultiplier          precisionMultipliers,
		          bool                     stavesUseEnchantingSkill,
		          bool                     spellsUseHighestMagicSkill,
		          std::vector<std::string> excludeKeywords,
		          std::vector<std::string> includeKeywords) :
			Config(spellAiming,
		           staffAiming,
		           bowAiming,
		           crossbowAiming,
		           concentrationSpellsAiming,
		           accuracyMultipliers,
		           precisionMultipliers,
		           stavesUseEnchantingSkill,
		           spellsUseHighestMagicSkill),
			excludeKeywords(excludeKeywords),
			includeKeywords(includeKeywords) {}

		bool ShouldLearn(RE::Actor* actor);

		static const NPCConfig defaultConfig;
	};
	inline const NPCConfig NPCConfig::defaultConfig{ true, true, true, true, true, { 1.0f, 0.85f, 1.15f, 1.15f, 0.85f, 0.2f }, { 1.0f, 7.0f, 0.85f, 0.85f, 1.15f, 1.5f }, false, false, {}, { "ActorTypeNPC" } };

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
