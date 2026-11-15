#pragma once

#ifndef NDEBUG

namespace NLA::AimDebug
{
	/// What an aim point is aimed at.
	enum class Mark : std::uint8_t
	{
		kBody,
		kHead,
		kHand,
		kLeg
	};

	inline bool enabled = false;

	void Install();

	void RecordAimPoint(RE::Actor* attacker, const RE::NiPoint3& point, Mark mark);
}

#endif
