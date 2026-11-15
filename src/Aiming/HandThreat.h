#pragma once

namespace NLA::Skillshots::HandThreat
{
	enum class Hand : std::uint8_t
	{
		kEven = 0,
		kLeft = 1,
		kRight = 2
	};

	/// The hand that is clearly more dangerous than the other.
	/// For one-handed weapons and spells a threat is based on the damage that each hand can do.
	/// For two-handed weapons both hands are evenly dangerous.
	Hand StrongerHand(RE::Actor* actor);
}
