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
}

namespace NLA::Options
{
	namespace General
	{
		inline bool magicAiming = true;
		inline bool archeryAiming = true;

		inline bool forceAimForAll = false;
	}

	namespace Complexity
	{
		inline bool useComplexity = true;
		inline float maxDistance = 3072; // I didn't find the actual setting that controls at which max range NPCs can shoot. But this is roughly twice the distance that I observed in-game :)
		inline float maxTargetSize = 500; // For reference: dragon ~ 700-1000 (depending on whether they spread their wings); giant ~ 180; human ~ 90; rabbit - 30
	}

	void Load();
}
