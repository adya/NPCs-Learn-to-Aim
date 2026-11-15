#include "AimDebug.h"
#include "Aiming.h"
#include "HandThreat.h"
#include "Hooking.h"
#include "Options.h"
#include "RE/C/CombatProjectileAimController.h"
#include "SkillUsage.h"

#include <randomness.h>

#include <chrono>

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

	namespace Skillshots
	{
		namespace
		{
			/// A part of the target that a skilled shooter aims at on purpose, instead of the usual chest.
			enum class SkillShot : std::uint8_t
			{
				kNone,
				kHead,
				kLeftHand,
				kRightHand,
				kLeftLeg,
				kRightLeg
			};

			const char* Name(SkillShot shot) {
				switch (shot) {
				case SkillShot::kHead:
					return "head";
				case SkillShot::kLeftHand:
					return "left hand";
				case SkillShot::kRightHand:
					return "right hand";
				case SkillShot::kLeftLeg:
					return "left leg";
				case SkillShot::kRightLeg:
					return "right leg";
				default:
					return "body";
				}
			}

			/// Skeleton nodes that make up one side's hand and leg. Humanoid skeletons have them, many creatures do not.
			struct LimbNodes
			{
				RE::BSFixedString hand;
				RE::BSFixedString calf;
			};

			const LimbNodes& NodesOf(bool left) {
				static const LimbNodes leftNodes{ "NPC L Hand [LHnd]", "NPC L Calf [LClf]" };
				static const LimbNodes rightNodes{ "NPC R Hand [RHnd]", "NPC R Calf [RClf]" };
				return left ? leftNodes : rightNodes;
			}

			RE::NiAVObject* GetHeadNode(RE::Actor* target) {
				auto* process = target->GetActorRuntimeData().currentProcess;
				return process && process->middleHigh ? process->middleHigh->headNode : nullptr;
			}

			/// Where in the world the target's given part is, if it has one.
			/// Legs are aimed at the calf (the knee joint), as the foot itself is on the ground. A skeleton without one has no leg shots.
			std::optional<RE::NiPoint3> PointOf(RE::Actor* target, SkillShot shot) {
				if (!target) {
					return std::nullopt;
				}

				switch (shot) {
				case SkillShot::kHead:
					if (auto* head = GetHeadNode(target); head) {
						return head->world.translate;
					}
					return std::nullopt;

				case SkillShot::kLeftHand:
				case SkillShot::kRightHand:
					if (auto* hand = target->GetNodeByName(NodesOf(shot == SkillShot::kLeftHand).hand); hand) {
						return hand->world.translate;
					}
					return std::nullopt;

				case SkillShot::kLeftLeg:
				case SkillShot::kRightLeg:
					if (auto* calf = target->GetNodeByName(NodesOf(shot == SkillShot::kLeftLeg).calf); calf) {
						return calf->world.translate;
					}
					return std::nullopt;

				default:
					return std::nullopt;
				}
			}

			bool Has(RE::Actor* target, SkillShot shot) {
				return PointOf(target, shot).has_value();
			}

#ifndef NDEBUG
			/// Logs which of the aimable nodes the target has, to spot creatures that lack some (e.g. no calf, so no leg shots).
			void LogNodes(RE::Actor* attacker, RE::Actor* target, SkillShot picked) {
				auto has = [&](SkillShot part) { return Has(target, part) ? "yes" : "no"; };
				logger::debug("{} aims at {}: head={} L hand={} R hand={} L calf={} R calf={} -> {}",
				              attacker->GetName(),
				              target->GetName(),
				              has(SkillShot::kHead),
				              has(SkillShot::kLeftHand),
				              has(SkillShot::kRightHand),
				              has(SkillShot::kLeftLeg),
				              has(SkillShot::kRightLeg),
				              Name(picked));
			}
#endif

			/// What a marble in the bag stands for. Arms and legs turn into a left or a right one when drawn.
			enum class Category : std::uint8_t
			{
				kRegular,
				kHead,
				kArm,
				kLeg
			};

			/// The categories that may be taken, from the settings and the target.
			struct Allowed
			{
				bool head{ false };
				bool arm{ false };
				bool leg{ false };

				explicit operator bool() const { return head || arm || leg; }
			};

			/// What the target actually has to aim at, out of the parts the settings allow.
			/// A part is on offer if the target has at least one of its sides.
			Allowed PartsOf(RE::Actor* target, const Allowed& allowed) {
				return {
					allowed.head && Has(target, SkillShot::kHead),
					allowed.arm && (Has(target, SkillShot::kLeftHand) || Has(target, SkillShot::kRightHand)),
					allowed.leg && (Has(target, SkillShot::kLeftLeg) || Has(target, SkillShot::kRightLeg))
				};
			}

			using Shots = randomness::marbles::bag<Category>;
			using Marble = randomness::marbles::marble<Category>;

			/// A batch of shots at one target, some of them skill shots, drawn one by one.
			/// The skill shots sit in a bag of their own, nested in the main one, so that every pass through it has an exact split between the parts.
			Shots MakeBag(float chance) {
				constexpr std::size_t kBagSize = 20;

				const auto skillShots = static_cast<std::size_t>(std::lround(std::clamp(chance, 0.0f, 1.0f) * static_cast<float>(kBagSize)));

				std::vector<Marble> slots{ { kBagSize - skillShots, Category::kRegular } };
				if (skillShots > 0) {
					std::vector<Marble> parts{
						{ 6, Category::kHead },
						{ 2, Category::kArm },
						{ 2, Category::kLeg }
					};
					slots.emplace_back(skillShots, std::move(parts));
				}
				return Shots(std::move(slots));
			}

			/// Swaps the drawn category for the nearest one that is on offer: the other limb, then the head, then a regular shot.
			/// A head goes to a limb first. This way the skill shots happen as often as the chance says, as long as there is anything to aim at.
			Category Resolve(Category drawn, const Allowed& offered) {
				switch (drawn) {
				case Category::kHead:
					if (offered.head) {
						return Category::kHead;
					}
					if (offered.arm && offered.leg) {
						return std::uniform_int_distribution<int>(0, 1)(rnd) ? Category::kArm : Category::kLeg;
					}
					if (offered.arm) {
						return Category::kArm;
					}
					return offered.leg ? Category::kLeg : Category::kRegular;

				case Category::kArm:
					if (offered.arm) {
						return Category::kArm;
					}
					if (offered.leg) {
						return Category::kLeg;
					}
					return offered.head ? Category::kHead : Category::kRegular;

				case Category::kLeg:
					if (offered.leg) {
						return Category::kLeg;
					}
					if (offered.arm) {
						return Category::kArm;
					}
					return offered.head ? Category::kHead : Category::kRegular;

				default:
					return Category::kRegular;
				}
			}

			/// A random side of the two, among the ones the target has.
			SkillShot RandomSide(RE::Actor* target, SkillShot left, SkillShot right) {
				const bool hasLeft = Has(target, left);
				const bool hasRight = Has(target, right);
				if (hasLeft && hasRight) {
					return std::uniform_int_distribution<int>(0, 1)(rnd) ? left : right;
				}
				return hasLeft ? left : hasRight ? right :
				                                   SkillShot::kNone;
			}

			/// Prefers the hand with a weapon or an offensive spell, as aiming at a hand with a shield or nothing is pointless.
			SkillShot ArmSide(RE::Actor* target) {
				if (const auto hand = HandThreat::StrongerHand(target); hand != HandThreat::Hand::kEven) {
					const SkillShot preferred = hand == HandThreat::Hand::kRight ? SkillShot::kRightHand : SkillShot::kLeftHand;
					if (Has(target, preferred)) {
						return preferred;
					}
				}
				return RandomSide(target, SkillShot::kLeftHand, SkillShot::kRightHand);
			}

			SkillShot ToSkillShot(RE::Actor* target, Category category) {
				switch (category) {
				case Category::kHead:
					return SkillShot::kHead;
				case Category::kArm:
					return ArmSide(target);
				case Category::kLeg:
					return RandomSide(target, SkillShot::kLeftLeg, SkillShot::kRightLeg);
				default:
					return SkillShot::kNone;
				}
			}

			/// The skill shot the attacker is going for with its current target, kept until the shot is fired.
			struct Pick
			{
				RE::FormID                            target;
				SkillShot                             shot;
				std::chrono::steady_clock::time_point seen;
			};

			/// A pick that wasn't asked for this long is a leftover of an attack that stopped, and the next one draws anew.
			constexpr auto kPickTimeout = std::chrono::seconds(3);

			RE::BSSpinLock decisionsLock;

			/// By shooter.
			std::unordered_map<RE::FormID, Pick> picks{};

			/// A bag belongs to a shooter and the target it is aiming at, for each kind of attack, as the chance differs between them.
			/// It lives until the next reset (e.g. a cell load), as a bag is too small to bother with finer cleanup.
			struct BagKey
			{
				RE::FormID                 attacker;
				RE::FormID                 target;
				const Options::AttackKind* kind;

				bool operator==(const BagKey&) const = default;
			};

			struct BagKeyHash
			{
				std::size_t operator()(const BagKey& key) const {
					const std::size_t pair = (static_cast<std::size_t>(key.attacker) << 32) | key.target;
					return pair ^ (std::hash<const void*>()(key.kind) * 0x9E3779B97F4A7C15ull);
				}
			};

			std::unordered_map<BagKey, Shots, BagKeyHash> bags{};
		}

		float SkillShotChance(RE::Actor* attacker, const SkillUsage& skillShotUsage) {
			return skillShotUsage ? skillShotUsage.SkillFactor(attacker) : 0.0f;
		}

		const char* CurrentSkillShot(RE::Actor* attacker) {
			if (!attacker) {
				return Name(SkillShot::kNone);
			}
			RE::BSSpinLockGuard lock{ decisionsLock };

			auto it = picks.find(attacker->GetFormID());
			return Name(it != picks.end() ? it->second.shot : SkillShot::kNone);
		}

		void ClearSkillShot(RE::Actor* attacker) {
			if (!attacker) {
				return;
			}
			RE::BSSpinLockGuard lock{ decisionsLock };

			picks.erase(attacker->GetFormID());
		}

		void Reset() {
			RE::BSSpinLockGuard lock{ decisionsLock };

			picks.clear();
			bags.clear();
		}

		namespace
		{
			/// Bags are not persisted anywhere, so a shooter's are let go of when it leaves combat (it dies, wins or gives up the search).
			/// If it is back at it later, it simply gets new ones.
			class CombatSink : public RE::BSTEventSink<RE::TESCombatEvent>
			{
				RE::BSEventNotifyControl ProcessEvent(const RE::TESCombatEvent* event, RE::BSTEventSource<RE::TESCombatEvent>*) override {
					if (!event || !event->actor || event->newState.all(RE::ACTOR_COMBAT_STATE::kCombat)) {
						return RE::BSEventNotifyControl::kContinue;
					}

					const auto shooter = event->actor->GetFormID();

					logger::debug("{} ({:X}) left combat (state {}), dropping its skill shot bags", event->actor->GetName(), shooter, static_cast<std::uint32_t>(event->newState.get()));

					RE::BSSpinLockGuard lock{ decisionsLock };

					picks.erase(shooter);
					std::erase_if(bags, [shooter](const auto& entry) { return entry.first.attacker == shooter; });
					return RE::BSEventNotifyControl::kContinue;
				}
			};
		}

		void ListenForCombat() {
			static CombatSink sink;
			if (auto* events = RE::ScriptEventSourceHolder::GetSingleton(); events) {
				events->AddEventSink<RE::TESCombatEvent>(&sink);
			}
		}

		struct AimModifier
		{
			RE::Actor* attacker{ nullptr };
			SkillUsage aim{};
			SkillShot  skillShot{ SkillShot::kNone };

			AimModifier(RE::Actor* attacker, RE::Actor* target, RE::MagicCaster* mcaster) :
				attacker(attacker) {
				// This works for bows, crossbows, staves, spells and... guns :)
				auto source = GetSource(mcaster);
				if (!source)
					return;

				aim = std::visit([&](auto* form) { return SkillUsage(form, attacker, kAim); }, *source);

				skillShot = std::visit([&](auto* form) { return DecideSkillShot(attacker, target, SkillUsage(form, attacker, kSkillShot)); }, *source);
			}

			using UpdateFunc = void (*)(RE::CombatProjectileAimController*, RE::Actor*);

			void Apply(RE::CombatProjectileAimController* controller, RE::Actor* target, UpdateFunc func) const {
				// Often when NPC is a magic caster the currentSpell only gets set when NPC starts charging/casting it,
				// in such cases we can't determine the skill to use and should fallback to default logic.
				// Frankly, calls to aim in such cases are meaningless anyways, so we're not missing much.
				if (aim) {
					controller->radiusMode = Aim::AimVariance(aim, attacker);
				}

				func(controller, target);

				// A part that can't be found (e.g. its 3D is not loaded) leaves the aim at the chest.
				bool aimedAtPart = false;
				if (skillShot != SkillShot::kNone) {
					if (auto point = PointOf(target, skillShot); point) {
						AimAt(controller, target, *point);
						aimedAtPart = true;
					}
				}

#ifndef NDEBUG
				AimDebug::RecordAimPoint(attacker, controller->aim_location, aimedAtPart ? MarkOf(skillShot) : AimDebug::Mark::kBody);
#endif
			}

		private:
			/// Takes the next shot out of the shooter's bag when it starts aiming at a target, and sticks with it until the shot is fired.
			static SkillShot DecideSkillShot(RE::Actor* attacker, RE::Actor* target, const SkillUsage& skillShotUsage) {
				if (!attacker || !target || attacker->IsPlayerRef() || !skillShotUsage) {
					return SkillShot::kNone;
				}

				const auto&   options = Options::For(attacker);
				const Allowed allowed{ options.headshots, options.armshots, options.legshots };
				if (!allowed) {
					return SkillShot::kNone;
				}

				const auto now = std::chrono::steady_clock::now();

				{
					RE::BSSpinLockGuard lock{ decisionsLock };

					if (auto it = picks.find(attacker->GetFormID()); it != picks.end() && it->second.target == target->GetFormID() && now - it->second.seen < kPickTimeout) {
						it->second.seen = now;
						return it->second.shot;
					}
				}

				// Only the parts the target really has are aimed at, e.g. a creature without hands or legs only gets the head.
				// The skeleton lookups are done outside of the lock.
				const Allowed offered = PartsOf(target, allowed);
				const float   chance = SkillShotChance(attacker, skillShotUsage);

				RE::BSSpinLockGuard lock{ decisionsLock };

				SkillShot shot = SkillShot::kNone;
				if (offered) {
					const BagKey key{ attacker->GetFormID(), target->GetFormID(), skillShotUsage.kind };
					auto         it = bags.find(key);
					if (it == bags.end()) {
						it = bags.emplace(key, MakeBag(chance)).first;
					}
					shot = ToSkillShot(target, Resolve(it->second.draw(rnd), offered));
				}

#ifndef NDEBUG
				LogNodes(attacker, target, shot);
#endif
				picks[attacker->GetFormID()] = { target->GetFormID(), shot, now };
				return shot;
			}

			/// Moves the aim so that it points at the world position, keeping the randomness the controller already added to it.
			static void AimAt(RE::CombatProjectileAimController* controller, RE::Actor* target, const RE::NiPoint3& point) {
				RE::NiPoint3 offset = point - target->GetPosition();
				RE::NiPoint3 delta = offset - controller->aimOffset;

				controller->aim_location += delta;
				controller->idealAimPosition += delta;
				controller->aimOffset = offset;
			}

#ifndef NDEBUG
			static AimDebug::Mark MarkOf(SkillShot shot) {
				switch (shot) {
				case SkillShot::kHead:
					return AimDebug::Mark::kHead;
				case SkillShot::kLeftHand:
				case SkillShot::kRightHand:
					return AimDebug::Mark::kHand;
				case SkillShot::kLeftLeg:
				case SkillShot::kRightLeg:
					return AimDebug::Mark::kLeg;
				default:
					return AimDebug::Mark::kBody;
				}
			}
#endif
		};

		struct CombatProjectileAimController_Update
		{
			static inline constexpr REL::RelocationID  relocation{ 43162, 44384 };
			static inline constexpr REL::VariantOffset offset{ 0x103, 0x97, 0x103 };
			static inline constexpr Signature          signature{ "E8 ?? ?? ?? ?? 48 8B CB E8 ?? ?? ?? ?? 84 C0 74" };

			static inline void pre_hook() { logger::debug("Installing Skill shots logic..."); }
			static inline void post_hook() { logger::info("🪝 Installed Skill shots logic"); }

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
