#pragma once

namespace NLA::Options
{
	struct Config
	{
		bool spellAiming = true;
		bool staffAiming = true;
		bool bowAiming = true;
		bool crossbowAiming = true;

		float bowSkillMultiplier = 1.0f;       // bows are the standard.
		float crossbowSkillMultiplier = 0.5f;  // crossbows are harder to aim, but shoot directly at the aim point, so overall easier to use.
		float spellSkillMultiplier = 1.1f;     // spells are the standard.
		float staffSkillMultiplier = 0.85f;    // staffs are a bit harder to aim properly.
	
		float blindnessMultiplier = 0.2f;	   // if the actor is blind, their aiming skill is reduced by this factor.

		bool crossbowsAlwaysShootStraight = true;  // if true, crossbows launches bolts in a straight line without random spread.

		bool stavesUseEnchantingSkill = false;  // if true, staves use the enchanting skill instead of the spell's primary effect's school skill.

		bool spellsUseHighestMagicSkill = false;  // if true, the highest NPC's magic skill is used to aim spells, instead of the spell's primary effect's school skill.

		bool concetrationSpellsRequireContinuousAim = true;  // if true, concentration spells require continuous aiming, breaking aim point will interrupt the spell. Only works for NPCs.

		Config() = default;
		Config(CSimpleIniA& ini, const char* a_section);
	};

	struct PlayerConfig : Config
	{
		PlayerConfig() = default;
		PlayerConfig(CSimpleIniA& ini);
	};

	struct NPCConfig : Config
	{
		std::vector<std::string> excludeKeywords{};
		std::vector<std::string> includeKeywords{ "ActorTypeNPC" };

		NPCConfig() = default;
		NPCConfig(CSimpleIniA& ini);

		bool ShouldLearn(RE::Actor* a_actor);
	};

	inline NPCConfig    NPC;
	inline PlayerConfig Player;

	/// Returns configuration for given actor. Note, that only common configuration is returned.
	inline const Config& For(const RE::Actor* actor) {
		if (actor && actor->IsPlayerRef()) {
			return Player;
		} else {
			return NPC;
		}
	}
	void Load();
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

	static float fCombatAimProjectileGroundMinRadius() {
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("fCombatAimProjectileGroundMinRadius")) {
			return setting->data.f;
		}
		return 128.0f;
	}

	static float fBowNPCSpreadAngle() {
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("fBowNPCSpreadAngle")) {
			return setting->data.f;
		}
		return 4.0f;
	}
}
