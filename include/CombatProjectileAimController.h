#pragma once

#include "RE/A/AITimer.h"
#include "RE/B/BSCoreTypes.h"
#include "RE/B/BSPointerHandle.h"
#include "CombatAimController.h"
#include "RE/N/NiPoint3.h"

namespace RE
{
	class CombatController;
	class MagicCaster;

	class CombatProjectileAimController : public CombatAimController
	{
	public:
	
		~CombatProjectileAimController();  // 00

		// override (CombatAimController)
		std::uint32_t        GetObjectType() override;                                     // 02
		void                 SaveGame(BGSSaveGameBuffer* a_buf) override;                  // 03
		void                 LoadGame(BGSLoadGameBuffer* a_buf) override;                  // 04
		bool                 CheckAim(const NiPoint3& from, const NiPoint3& to) override;  // 05
		bool                 CheckAim(const NiPoint3& P) override;                         // 06
		bool                 CheckAim(float cone) override;                                // 07
		void                 Update() override;                                            // 08
		CombatAimController* Clone() const override;                                       // 09
		void                 FinishLoadGame() override;                                    // 0A

		// members
		std::uint64_t unk48;                      // 48
		std::uint32_t unk50;                      // 50
		std::uint32_t unk54;                      // 54
		float         turningAngle;               // 58
		RE::NiPoint3  predictedAimPoint;          // 5C
		RE::NiPoint3  unk68;                      // 68
		RE::NiPoint3  unk74;                      // 74
		RE::NiPoint3  unk80;                      // 80
		RE::NiPoint3  aimOffset;                  // 8C
		std::uint32_t releaseTimestamp;           // 98
		std::uint32_t unk9C;                      // 9C
		float         aimVariance;                // A0
		std::uint32_t unkA4;                      // A4
		std::uint64_t unkA8;                      // A8
		float         unkB0;                      // B0
		RE::NiPoint3  unkB4;                      // B4
		std::uint32_t lastUpdatedUnkB4Timestamp;  // C0
		std::uint64_t unkC8;                      // C8
	};
	static_assert(sizeof(CombatProjectileAimController) == 0xD0);
}
