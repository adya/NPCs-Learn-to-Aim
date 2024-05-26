#pragma once

namespace NLA
{
	enum SkillUsageContext
	{
		kAim,
		kRelease
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
			float staff;

			float blindness;

			SkillMultiplier() = default;
			SkillMultiplier(CSimpleIniA& ini, const char* section, std::string_view prefix);
			SkillMultiplier(float bow, float crossbow, float spell, float staff, float blindness) :
				bow(bow), crossbow(crossbow), spell(spell), staff(staff), blindness(blindness) {}
		};

		bool spellAiming = true;
		bool staffAiming = true;
		bool bowAiming = true;
		bool crossbowAiming = true;

		SkillMultiplier aimMultipliers;
		SkillMultiplier releaseMultipliers;

		bool stavesUseEnchantingSkill = false;  // if true, staves use the enchanting skill instead of the spell's primary effect's school skill.

		bool spellsUseHighestMagicSkill = false;  // if true, the highest NPC's magic skill is used to aim spells, instead of the spell's primary effect's school skill.

		bool concetrationSpellsRequireContinuousAim = true;  // if true, concentration spells require continuous aiming, breaking aim point will interrupt the spell. Only works for NPCs.

		Config() = default;
		Config(CSimpleIniA& ini, const char* section);
		Config(bool            spellAiming,
		       bool            staffAiming,
		       bool            bowAiming,
		       bool            crossbowAiming,
		       SkillMultiplier aimMultipliers,
		       SkillMultiplier releaseMultipliers,
		       bool            stavesUseEnchantingSkill,
		       bool            spellsUseHighestMagicSkill,
		       bool            concetrationSpellsRequireContinuousAim) :
			spellAiming(spellAiming),
			staffAiming(staffAiming),
			bowAiming(bowAiming),
			crossbowAiming(crossbowAiming),
			aimMultipliers(aimMultipliers),
			releaseMultipliers(releaseMultipliers),
			stavesUseEnchantingSkill(stavesUseEnchantingSkill),
			spellsUseHighestMagicSkill(spellsUseHighestMagicSkill),
			concetrationSpellsRequireContinuousAim(concetrationSpellsRequireContinuousAim) {}

		inline const SkillMultiplier& MultiplierFor(SkillUsageContext context) const {
			switch (context) {
			case kAim:
				return aimMultipliers;
			case kRelease:
				return releaseMultipliers;
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
		             SkillMultiplier aimMultipliers,
		             SkillMultiplier releaseMultipliers,
		             bool            stavesUseEnchantingSkill,
		             bool            spellsUseHighestMagicSkill) :
			Config(spellAiming,
		           staffAiming,
		           bowAiming,
		           crossbowAiming,
		           aimMultipliers,
		           releaseMultipliers,
		           stavesUseEnchantingSkill,
		           spellsUseHighestMagicSkill,
		           false) {}
	};

	struct NPCConfig : Config
	{
		std::vector<std::string> excludeKeywords{};
		std::vector<std::string> includeKeywords{};

		NPCConfig() = default;
		NPCConfig(CSimpleIniA& ini);
		NPCConfig(bool                     spellAiming,
		          bool                     staffAiming,
		          bool                     bowAiming,
		          bool                     crossbowAiming,
		          SkillMultiplier          aimMultipliers,
		          SkillMultiplier          releaseMultipliers,
		          bool                     stavesUseEnchantingSkill,
		          bool                     spellsUseHighestMagicSkill,
		          bool                     concetrationSpellsRequireContinuousAim,
		          std::vector<std::string> excludeKeywords,
		          std::vector<std::string> includeKeywords) :
			Config(spellAiming,
		           staffAiming,
		           bowAiming,
		           crossbowAiming,
		           aimMultipliers,
		           releaseMultipliers,
		           stavesUseEnchantingSkill,
		           spellsUseHighestMagicSkill,
		           concetrationSpellsRequireContinuousAim),
			excludeKeywords(excludeKeywords),
			includeKeywords(includeKeywords) {}

		bool ShouldLearn(RE::Actor* actor);
	};

	inline NPCConfig    NPC{ true, true, true, true, { 1.0f, 0.85f, 1.15f, 0.85f, 0.2f }, { 1.0f, 7.0f, 0.85f, 1.15f, 1.5f }, false, false, true, {}, { "ActorTypeNPC" } };
	inline PlayerConfig Player{ true, true, true, true, {}, { 1.4f, 7.0f, 1.25f, 1.55f, 0.0f }, false, false };

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

	static float fBowNPCSpreadAngle() {
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("fBowNPCSpreadAngle")) {
			return setting->data.f;
		}
		return 4.0f;
	}
}
