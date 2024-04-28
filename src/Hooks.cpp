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
			DrawHandler::DrawDebugSphere(point, 5, 0.3f, color);
		}

		static void Mark(const RE::NiPoint3& from, const RE::NiPoint3& to, const Color::color& color) {
			DrawHandler::DrawDebugLine(from, to, 0.3f, color);
		}

		static void Mark(const RE::NiPoint3& point, const RE::TESObjectREFR* owner, bool pale = false) {
			Mark(point, Color::GetRandomColor(owner, pale));
		}

		static void Mark(const RE::NiPoint3& from, const RE::NiPoint3& to, const RE::TESObjectREFR* owner, bool pale = false) {
			Mark(from, to, Color::GetRandomColor(owner, pale));
		}

		struct CalculateAim
		{
			static void thunk(RE::CombatProjectileAimController* controller, RE::Actor* target) 
			{
				func(controller, target);

				logger::info("{:*^50}", "Calculating Projectile Aim");

				RE::Actor* attacker = nullptr;
				if (attacker = controller->combatController->attackerHandle.get().get()) {
					logger::info("{} is shooting at {}", attacker->GetName(), target->GetName());
				} else {
					return;
				}

				if (auto aimVarianceSetting = RE::GameSettingCollection::GetSingleton()->GetSetting("fCombatRangedAimVariance")) {
					auto aimVariance = aimVarianceSetting->GetFloat();
					logger::info("Aim variance Setting: {:.4f}", aimVariance);
				}

				Mark(target->data.location, target);
				auto point = target->data.location + controller->targetBodyPartOffset;
				Mark(target->data.location, point, target, true);
				Mark(point, target, true);
				
				Mark(controller->aimPoint, attacker, true);
				Mark(controller->aimPoint, controller->projectileLaunchPoint, attacker, true);
				Mark(controller->attackerLocation, attacker);
				Mark(controller->projectileLaunchPoint, attacker);

				logger::info("\taimVariance: {:.4f}", controller->aimVariance);
				logger::info("\taimOffset: {}", controller->aimOffset);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct GetBodyPartLocation
		{
			static void thunk(RE::NiPoint3& point, RE::Actor* target, std::uint32_t bodyPart) {
				func(point, target, bodyPart);

				logger::info("\tGetBodyPartLocation: {}: {}", bodyPart, point);
				Mark(point, Color::red);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};
		struct GetBodyPartLocationVariance
		{
			static void thunk(RE::NiPoint3& point, RE::Actor* target, std::uint32_t bodyPart) {
				func(point, target, bodyPart);

				logger::info("\tGetBodyPartLocationVariance {}: {}", bodyPart, point);
				Mark(point, Color::teal);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct GetAnticipatedLocation
		{
			static void thunk(RE::Actor* target, float timePassed, RE::NiPoint3& point) {
				func(target, timePassed, point);

				logger::info("\tGetAnticipatedLocation: {}", point);
				Mark(point, Color::lime);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct GetProjectileLaunchPoint
		{
			static bool thunk(RE::CombatProjectileAimController* controller, RE::NiPoint3& point) {
				bool res = func(controller, point);

				logger::info("\tGetProjectileLaunchPoint: {}", point);
				auto projectilePoint = controller->attackerLocation + point;
				Mark(controller->attackerLocation, projectilePoint, Color::red);
				return res;
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct WeapFireAmmo
		{
			static RE::ProjectileHandle* thunk(RE::ProjectileHandle* projectile, RE::Projectile::LaunchData& data) {
				
				logger::info("{:*^50}", "Firing Projectile");

				logger::info("\tOrigin: {}", data.origin);
				logger::info("\tangleX: {}", data.angleX);
				logger::info("\tangleZ: {}", data.angleZ);
				logger::info("\tcontactNormal: {}", data.contactNormal);
				if (auto desiredTarget = data.desiredTarget) {
					logger::info("\tdesiredTarget: {}", desiredTarget->GetName());
				} else {
					logger::info("\tdesiredTarget: NONE");
				}
				logger::info("\tunk60: {}", data.unk60);
				logger::info("\tunk64: {}", data.unk64);
				Mark(data.origin, Color::magenta);

				
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

				Mark(data.origin, target, Color::magenta);

				return func(projectile, data);
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct WeapFireAmmoRANDOMSHIT
		{
			static float thunk(float min, float max) {
				return 0;
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		inline void Install() {
			const REL::Relocation<std::uintptr_t> aim{ RELOCATION_ID(0, 44396) };
			const REL::Relocation<std::uintptr_t> update{ RELOCATION_ID(0, 44384) };
			const REL::Relocation<std::uintptr_t> something{ RELOCATION_ID(0, 44397) };
			const REL::Relocation<std::uintptr_t> weaponFire{ RELOCATION_ID(0, 18102) };

			stl::write_thunk_call<GetBodyPartLocation>(aim.address() + OFFSET(0, 0x211));
			stl::write_thunk_call<GetBodyPartLocationVariance>(aim.address() + OFFSET(0, 0x5C2));
			stl::write_thunk_call<GetAnticipatedLocation>(aim.address() + OFFSET(0, 0x381));

			stl::write_thunk_call<CalculateAim>(update.address() + OFFSET(0, 0x97));

			stl::write_thunk_call<GetProjectileLaunchPoint>(something.address() + OFFSET(0, 0x2C));
			
			stl::write_thunk_call<WeapFireAmmo>(weaponFire.address() + OFFSET(0, 0xE60));

			stl::write_thunk_call<WeapFireAmmoRANDOMSHIT>(weaponFire.address() + OFFSET(0, 0xCD5));


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
		UpdateHooks::Install();
		Combat::Install();
	}

}
