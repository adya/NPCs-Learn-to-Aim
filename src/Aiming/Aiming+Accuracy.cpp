#include "Aiming.h"
#include "Hooking.h"
#include "Options.h"
#include "RE/C/CombatProjectileAimController.h"
#include "SkillUsage.h"


namespace NLA
{
	namespace
	{
		using MCasterSource = std::variant<RE::TESObjectWEAP*, RE::MagicItem*>;

		/// Attempts to retrieve the source of the projectile that is being launched.
		/// There were reports of access violation which bypass the usual nullptr checks,
		/// so this is an attempt to catch such exception and handle gracefully.
		std::optional<MCasterSource> GetSource(RE::MagicCaster* mcaster) {
			if (!mcaster)
				return std::nullopt;

			__try {
				if (auto* weapon = reinterpret_cast<RE::TESObjectWEAP*>(mcaster); weapon->formType == RE::FormType::Weapon) {
					return weapon;
				}

				return mcaster->currentSpell;
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				logger::warn("Caught an access violation while probing CombatProjectileAimController's caster.");
				return std::nullopt;
			}
		}

	}

	namespace Accuracy
	{
		namespace
		{
			RE::NiAVObject* GetHeadNode(RE::Actor* target) {
				auto* process = target->GetActorRuntimeData().currentProcess;
				return process && process->middleHigh ? process->middleHigh->headNode : nullptr;
			}

			/// Headshot decision made for the attacker's current target, kept until the shot is fired.
			struct HeadshotDecision
			{
				RE::FormID target;
				bool       headshot;
			};

			RE::BSSpinLock                                   decisionsLock;
			std::unordered_map<RE::FormID, HeadshotDecision> headshotDecisions{};
		}

		float AimVariance(const SkillUsage& accuracyUsage, RE::Actor* attacker) {
			return 2.0f * Settings::fCombatRangedAimVariance() * accuracyUsage.SkillFactor(attacker);
		}

		float HeadshotChance(RE::Actor* attacker, const SkillUsage& headshotUsage) {
			return headshotUsage ? headshotUsage.SkillFactor(attacker) : 0.0f;
		}

		bool IsGoingForHeadshot(RE::Actor* attacker) {
			if (!attacker) {
				return false;
			}
			RE::BSSpinLockGuard lock{ decisionsLock };

			auto it = headshotDecisions.find(attacker->GetFormID());
			return it != headshotDecisions.end() && it->second.headshot;
		}

		void ClearHeadshot(RE::Actor* attacker) {
			if (!attacker) {
				return;
			}
			RE::BSSpinLockGuard lock{ decisionsLock };

			headshotDecisions.erase(attacker->GetFormID());
		}

		void Reset() {
			RE::BSSpinLockGuard lock{ decisionsLock };

			headshotDecisions.clear();
		}

		namespace
		{
			struct AimModifier
			{
				RE::Actor* attacker{ nullptr };
				SkillUsage accuracy{};
				bool       headshot{ false };

				AimModifier(RE::Actor* attacker, RE::Actor* target, RE::MagicCaster* mcaster) :
					attacker(attacker) {
					// This works for bows, crossbows, staves, spells and... guns :)
					auto source = GetSource(mcaster);
					if (!source)
						return;

					accuracy = std::visit([&](auto* form) { return SkillUsage(form, attacker, kAccuracy); }, *source);

					headshot = std::visit([&](auto* form) { return DecideHeadshot(attacker, target, SkillUsage(form, attacker, kHeadshot)); }, *source);
				}

				using UpdateFunc = void (*)(RE::CombatProjectileAimController*, RE::Actor*);

				void Apply(RE::CombatProjectileAimController* controller, RE::Actor* target, UpdateFunc func) const {
					// Often when NPC is a magic caster the currentSpell only gets set when NPC starts charging/casting it,
					// in such cases we can't determine the skill to use and should fallback to default logic.
					// Frankly, calls to aim in such cases are meaningless anyways, so we're not missing much.
					if (accuracy) {
						controller->radiusMode = AimVariance(accuracy, attacker);
					}

					func(controller, target);

					if (headshot) {
						AimAtHead(controller, target);
					}
				}

			private:
				static bool DecideHeadshot(RE::Actor* attacker, RE::Actor* target, const SkillUsage& headshotUsage) {
					if (!attacker || !target || !headshotUsage || attacker->IsPlayerRef()) {
						return false;
					}

					RE::BSSpinLockGuard lock{ decisionsLock };

					if (auto it = headshotDecisions.find(attacker->GetFormID()); it != headshotDecisions.end() && it->second.target == target->GetFormID()) {
						return it->second.headshot;
					}

					std::uniform_real_distribution<float> roll(0.0f, 1.0f);
					const bool                            headshot = roll(rnd) < HeadshotChance(attacker, headshotUsage);

					headshotDecisions[attacker->GetFormID()] = { target->GetFormID(), headshot };
					return headshot;
				}

				static void AimAtHead(RE::CombatProjectileAimController* controller, RE::Actor* target) {
					auto* head = target ? GetHeadNode(target) : nullptr;
					if (!head) {
						return;
					}

					RE::NiPoint3 headOffset = head->world.translate - target->GetPosition();
					RE::NiPoint3 delta = headOffset - controller->aimOffset;

					controller->aim_location += delta;
					controller->idealAimPosition += delta;
					controller->aimOffset = headOffset;
				}
			};
		}

		// The full mod should be a combination of both hooks: weapon/spell fire and aiming in CombatProjectileAimController.
		// While the former provides a random deviation of projectile angle, the latter still randomizes the aim point:
		// * Controller determines general ability of NPC to aim at a specific point accurately.
		// * Firing function determines NPCs ability to actually make the projectile go in the desired direction.
		struct CombatProjectileAimController_Update
		{
			static inline constexpr REL::RelocationID  relocation{ 43162, 44384 };
			static inline constexpr REL::VariantOffset offset{ 0x103, 0x97, 0x103 };
			static inline constexpr Signature          signature{ "E8 ?? ?? ?? ?? 48 8B CB E8 ?? ?? ?? ?? 84 C0 74" };

			static inline void pre_hook() { logger::debug("Installing Accuracy logic..."); }
			static inline void post_hook() { logger::info("🪝 Installed Accuracy logic"); }

			static void thunk(RE::CombatProjectileAimController* controller, RE::Actor* target) {
				if (!controller || !controller->combat_control) {
					func(controller, target);
					return;
				}

				RE::Actor* attacker = controller->combat_control->attackerHandle.get().get();
				if (!attacker || !Options::ShouldLearn(attacker)) {
					func(controller, target);
					return;
				}

				AimModifier(attacker, target, controller->mcaster).Apply(controller, target, func.get());
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		void Install() {
			stl::install_hook<CombatProjectileAimController_Update>();
		}
	}
}
