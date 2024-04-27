#include "Hooks.h"
#include "CombatProjectileAimController.h"
#include "render/DrawHandler.h"

namespace NLA
{
	namespace Combat
	{
		struct CalculateAim
		{
			static void thunk(RE::CombatProjectileAimController* controller, RE::Actor* target) 
			{
				logger::info("{:*^50}", "Calculating Projectile Aim");
				auto t = controller->combatController->attackerHandle.get();
				if (auto attacker = t.get(); attacker) {
					logger::info("{} is shooting at {}", attacker->GetName(), target->GetName());

				}

				if (auto aimVarianceSetting = RE::GameSettingCollection::GetSingleton()->GetSetting("fCombatRangedAimVariance")) {
					auto aimVariance = aimVarianceSetting->GetFloat();
					logger::info("Aim variance Setting: {:.4f}", aimVariance);
				}

				
				func(controller, target);

				logger::info("Target location: {}", target->data.location);

				logger::info("Controller stats:");
				logger::info("\tturningAngle: {:.4f}", controller->turningAngle);
				logger::info("\tpredictedAimPoint: {}", controller->predictedAimPoint);
				logger::info("\tunk68: {}", controller->unk68);
				logger::info("\tunk74: {}", controller->unk74);
				logger::info("\tunk80: {}", controller->unk80);
				logger::info("\tunkB4: {}", controller->unkB4);
				logger::info("\taimVariance: {:.4f}", controller->aimVariance);
				logger::info("\taimOffset: {}", controller->aimOffset);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct GetBodyPartLocation
		{
			static void thunk(RE::NiPoint3& point, RE::Actor* target, std::uint32_t bodyPart) {
				func(point, target, 3);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct GetBodyPartLocationVariance
		{
			static void thunk(RE::NiPoint3& point, RE::Actor* target, std::uint32_t bodyPart) {
				func(point, target, 3);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		inline void Install() {
			const REL::Relocation<std::uintptr_t> aim{ RELOCATION_ID(0, 44396) };
			const REL::Relocation<std::uintptr_t> update{ RELOCATION_ID(0, 44384) };

			/*stl::write_thunk_call<GetBodyPartLocation>(aim.address() + OFFSET(0, 0x211));
			stl::write_thunk_call<GetBodyPartLocationVariance>(aim.address() + OFFSET(0, 0x5C2));*/
			stl::write_thunk_call<CalculateAim>(update.address() + OFFSET(0, 0x97));

			logger::info("Installed PerfectAim hooks");

			auto settings = RE::GameSettingCollection::GetSingleton();
			if (auto aimVariance = settings->GetSetting("fCombatRangedAimVariance")) {
				aimVariance->data.f = 0;
				settings->WriteSetting(aimVariance);
				logger::info("Set fCombatRangedAimVariance to 0");
			}

			if (auto aimOffset = settings->GetSetting("fCombatAimProjectileRandomOffset")) {
				aimOffset->data.f = 0;
				settings->WriteSetting(aimOffset);
				logger::info("Set fCombatAimProjectileRandomOffset to 0");
			}

			 
		}
	}
	

	void Install() {
		logger::info("{:*^30}", "HOOKS");

		Combat::Install();
	}

}
