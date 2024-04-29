#include "Hooks.h"
#include "CombatProjectileAimController.h"
#include "render/DrawHandler.h"
#include "Utils.h"
#include <numbers>

namespace NLA
{

	namespace UpdateHooks
	{
		struct Nullsub
		{
			static void thunk() {
				func();
				DrawHandler::GetSingleton()->Update(*g_deltaTime);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		inline void Install() {
			const REL::Relocation<uintptr_t> update{ RELOCATION_ID(35565, 36564) };  // 5B2FF0, 5D9F50, main update
			stl::write_thunk_call<Nullsub>(update.address() + OFFSET(0x748, 0xC26));
		}
	}

	namespace Combat
	{
		struct Color
		{
			using color = glm::vec4;

			static constexpr color red = { 1, 0, 0, 1 };
			static constexpr color blue = { 0, 0, 1, 1 };
			static constexpr color green = { 0, 1, 0, 1 };
			static constexpr color yellow = { 1, 1, 0, 1 };
			static constexpr color purple = { 0.5, 0, 0.5, 1 };
			static constexpr color cyan = { 0, 1, 1, 1 };
			static constexpr color orange = { 1, 0.5, 0, 1 };
			static constexpr color magenta = { 1, 0, 1, 1 };
			static constexpr color lime = { 0.75, 1, 0, 1 };
			static constexpr color teal = { 0, 0.5, 0.5, 1 };

			static constexpr color pale_red = { 1, 0.5, 0.5, 1 };
			static constexpr color pale_blue = { 0.7, 0.7, 1, 1 };
			static constexpr color pale_green = { 0.5, 1, 0.5, 1 };
			static constexpr color pale_yellow = { 1, 1, 0.5, 1 };
			static constexpr color pale_purple = { 0.75, 0.5, 0.75, 1 };
			static constexpr color pale_cyan = { 0.5, 1, 1, 1 };
			static constexpr color pale_orange = { 1, 0.75, 0.5, 1 };
			static constexpr color pale_magenta = { 1, 0.5, 1, 1 };
			static constexpr color pale_lime = { 0.85, 1, 0.5, 1 };
			static constexpr color pale_teal = { 0.5, 0.75, 0.75, 1 };


			// Green reserved for player
			static constexpr std::array<color, 9> colors = { red, blue, /*green,*/ yellow, purple, cyan, orange, magenta, lime };

			static constexpr std::array<color, 9> pale_colors = { pale_red, pale_blue, /*pale_green,*/ pale_yellow, pale_purple, pale_cyan, pale_orange, pale_magenta, pale_lime };

			static color GetRandomColor(const RE::TESObjectREFR* obj, bool pale = false) {
				if (obj) {
					if (obj == RE::PlayerCharacter::GetSingleton()) {
						return pale ? Color::pale_green : Color::green;
					}

					return pale ? pale_colors[obj->formID % colors.size()] : colors[obj->formID % colors.size()];
				}
				
				return pale ? pale_red : red;
			}
		};

		static void Mark(const RE::NiPoint3& point, const Color::color& color) {
			DrawHandler::DrawDebugSphere(point, 5, 0.2, color);
		}

		static void Mark(const RE::NiBound& bound, const Color::color& color) {
			DrawHandler::DrawDebugSphere(bound.center, bound.radius, 0.2, color);
		}

		static void Mark(const RE::NiPoint3& from, const RE::NiPoint3& to, const Color::color& color) {
			DrawHandler::DrawDebugLine(from, to, 0.2, color);
		}

		static void Mark(const RE::NiPoint3& point, const RE::TESObjectREFR* owner, bool pale = false) {
			Mark(point, Color::GetRandomColor(owner, pale));
		}

		static void Mark(const RE::NiBound& bound, const RE::TESObjectREFR* owner, bool pale = false) {
			Mark(bound, Color::GetRandomColor(owner, pale));
		}

		static void Mark(const RE::NiPoint3& from, const RE::NiPoint3& to, const RE::TESObjectREFR* owner, bool pale = false) {
			Mark(from, to, Color::GetRandomColor(owner, pale));
		}

		struct CalculateAim
		{
			static void thunk(RE::CombatProjectileAimController* controller, RE::Actor* target) 
			{		
				logger::info("{:*^50}", "Calculating Projectile Aim");
				float aimOffset = 0;
				float aimVariance = 0;

				if (auto aimOffsetSetting = RE::GameSettingCollection::GetSingleton()->GetSetting("fCombatAimProjectileRandomOffset")) {
					aimOffset = aimOffsetSetting->GetFloat();
				}
				if (auto aimVarianceSetting = RE::GameSettingCollection::GetSingleton()->GetSetting("fCombatRangedAimVariance")) {
					aimVariance = aimVarianceSetting->GetFloat();
					controller->aimVariance = aimVariance;
				}

				float           width = 0;
				RE::NiAVObject* bounds;

				if (target->IsPlayerRef() && !RE::PlayerCharacter::GetSingleton()->playerFlags.isInThirdPersonMode) {
					width = target->IsSneaking() ? 89.3 : 93.5;
				} else if (auto bounds = target->Get3D()) {
					width = bounds->worldBound.radius;
					Mark(bounds->worldBound, Color::GetRandomColor(target));
				}

				auto distance = controller->attackerLocation.GetDistance(controller->aimPoint);

				logger::info("fCombatAimProjectileRandomOffset: {:.4f}", aimOffset);
				logger::info("fCombatRangedAimVariance: {:.4f}", aimVariance);
				logger::info("{} width: {:.4f}", target->GetName(), width);
				logger::info("\taimVariance: {:.4f}", controller->aimVariance);
				logger::info("width/offset: {:.4f}", width / aimOffset);
				logger::info("distance to target: {:.4f}", distance);

				// TODO: Figure out the shot complexity formula and adjustable offset.
				controller->aimVariance = width / aimOffset;

				func(controller, target);
				DrawHandler::GetSingleton()->Update(*g_deltaTime);
				
				RE::Actor* attacker = nullptr;
				if (attacker = controller->combatController->attackerHandle.get().get()) {
					logger::info("{} is shooting at {}", attacker->GetName(), target->GetName());
				} else {
					return;
				}

				

				

			
				Mark(controller->actualAimPoint + controller->aimOffset, attacker);
				Mark(controller->actualAimPoint + controller->aimOffset, controller->projectileLaunchPoint, attacker, true);
				Mark(controller->projectileLaunchPoint, attacker, true);

				
				logger::info("\taimOffset: {}", controller->aimOffset);

			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct WeapFireAmmo
		{
			static RE::ProjectileHandle* thunk(RE::ProjectileHandle* projectile, RE::Projectile::LaunchData& data) {
				const auto pitch = data.angleX;
				const auto yaw = -data.angleZ + std::numbers::pi_v<float> / 2;

				using namespace std;

				const auto xt = cosf(yaw) * cosf(pitch);
				const auto yt = sinf(yaw) * cosf(pitch);
				const auto zt = -sin(pitch);

				const float D = 1000;

				RE::NiPoint3 target = {
					data.origin.x + D * xt,
					data.origin.y + D * yt,
					data.origin.z + D * zt
				};

				DrawHandler::GetSingleton()->Update(*g_deltaTime);
				Mark(data.origin, data.shooter);
				Mark(data.origin, target, data.shooter);
			
				return func(projectile, data);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct WeapFireAmmoRangomizeArrowDirection
		{
			static float thunk(float min, float max) {
				return 0;
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		inline void Install() {
			const REL::Relocation<std::uintptr_t> aim{ RELOCATION_ID(0, 44396) };
			const REL::Relocation<std::uintptr_t> update{ RELOCATION_ID(0, 44384) };
			const REL::Relocation<std::uintptr_t> weaponFire{ RELOCATION_ID(0, 18102) };
		
			stl::write_thunk_call<CalculateAim>(update.address() + OFFSET(0, 0x97));

			stl::write_thunk_call<WeapFireAmmo>(weaponFire.address() + OFFSET(0, 0xE60));

			stl::write_thunk_call<WeapFireAmmoRangomizeArrowDirection>(weaponFire.address() + OFFSET(0, 0xCD5));


			logger::info("Installed PerfectAim hooks");

			/*auto settings = RE::GameSettingCollection::GetSingleton();
			if (auto aimVariance = settings->GetSetting("fCombatRangedAimVariance")) {
				aimVariance->data.f = 0.9;
				settings->WriteSetting(aimVariance);
				logger::info("Set fCombatRangedAimVariance to 0.9");
			}

			if (auto aimOffset = settings->GetSetting("fCombatAimProjectileRandomOffset")) {
				aimOffset->data.f = 0;
				settings->WriteSetting(aimOffset);
				logger::info("Set fCombatAimProjectileRandomOffset to 16");
			}*/

			 
		}
	}
	

	void Install() {
		logger::info("{:*^30}", "HOOKS");
		UpdateHooks::Install();
		Combat::Install();
	}

}
