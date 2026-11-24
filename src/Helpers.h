#pragma once

#include "Options.h"

namespace NLA
{
	struct TrackingKey
	{
		std::uint32_t formID;
		std::uint32_t source: 4;  // spells only.
		std::uint32_t weapon: 1;
		std::uint32_t reserved: 27;  // named, so that aggregate initialization zeroes it and the whole key can be hashed as one integer.

		bool operator==(const TrackingKey&) const = default;

		static TrackingKey ForWeapon(RE::Actor* actor) { return { .formID = actor->GetFormID(), .weapon = 1 }; }

		static TrackingKey ForCaster(RE::Actor* actor, RE::MagicSystem::CastingSource source) { return { .formID = actor->GetFormID(), .source = static_cast<std::uint32_t>(std::to_underlying(source)) }; }
	};
	static_assert(sizeof(TrackingKey) == sizeof(std::uint64_t));
}

template <>
struct std::hash<NLA::TrackingKey>
{
	std::size_t operator()(const NLA::TrackingKey& key) const noexcept { return std::hash<std::uint64_t>{}(std::bit_cast<std::uint64_t>(key)); }
};

namespace NLA::Weapon
{
	inline const Options::AttackKind* KindOf(const RE::TESObjectWEAP* weapon) {
		if (!weapon)
			return nullptr;

		switch (weapon->GetWeaponType()) {
		case RE::WEAPON_TYPE::kBow:
			return &Options::AttackKind::kBow;
		case RE::WEAPON_TYPE::kCrossbow:
			return &Options::AttackKind::kCrossbow;
		default:
			return nullptr;
		}
	}

	inline bool IsBowOrCrossbow(RE::WEAPON_TYPE type) {
		return type == RE::WEAPON_TYPE::kBow || type == RE::WEAPON_TYPE::kCrossbow;
	}

	inline bool IsFullyDrawn(RE::ATTACK_STATE_ENUM state) {
		return state == RE::ATTACK_STATE_ENUM::kBowDrawn;
	}

	/// Whether the shot has been launched or the draw interrupted.
	inline bool IsReleased(RE::ATTACK_STATE_ENUM state) {
		return state == RE::ATTACK_STATE_ENUM::kBowReleased || state == RE::ATTACK_STATE_ENUM::kNone;
	}
}

namespace NLA::Actor
{
	inline RE::Actor* FromActorValueOwner(RE::ActorValueOwner* owner) {
		return owner ? &REL::RelocateMemberIfNewer<RE::Actor>(SKSE::RUNTIME_SSE_1_6_629, owner, -0xB0, -0xB8) : nullptr;
	}
}

namespace NLA::Spell
{
	inline bool IsConcentration(const RE::MagicItem* spell) {
		return spell && spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration;
	}
}

namespace NLA::Caster
{
	inline bool IsReady(RE::MagicCaster::State state) {
		return state == RE::MagicCaster::State::kReady;
	}

	inline bool IsCharging(RE::MagicCaster::State state) {
		return state == RE::MagicCaster::State::kUnk02;  // Actually, the state during charging is 2, unlike enum's suggested kCharging = 5.
	}

	inline bool IsInterrupted(RE::MagicCaster::State state) {
		return state == RE::MagicCaster::State::kUnk08 || state == RE::MagicCaster::State::kUnk09;
	}

	inline bool IsConcentrating(const RE::MagicCaster* caster, RE::MagicCaster::State state) {
		return Spell::IsConcentration(caster->currentSpell) && state == RE::MagicCaster::State::kCasting;
	}
}
