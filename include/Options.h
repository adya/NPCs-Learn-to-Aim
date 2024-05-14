#pragma once

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

namespace NLA::Options
{
	namespace General
	{
		inline bool spellAiming = true;
		inline bool staffAiming = true;
		inline bool bowAiming = true;
		inline bool crossbowAiming = true;
	}

	namespace NPC
	{
		inline std::vector<std::string> excludeKeywords{};
		inline std::vector<std::string> includeKeywords{ "ActorTypeNPC" };

		bool ShouldLearn(RE::Actor* a_actor);
	}

	namespace Skills
	{
		inline float crossbowSkillMultiplier = 1.25f;  // make crossbows slightly less demanding.

		inline float staffSkillMultiplier = 1.25f;  // make staffs slightly less demanding.

		inline bool staffsUseEnchantingSkill = false;
	}

	void Load();
}
