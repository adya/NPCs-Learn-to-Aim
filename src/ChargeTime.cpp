#include "ChargeTime.h"
#include "Helpers.h"
#include "Hooking.h"
#include "SkillUsage.h"

namespace NLA::ChargeTime
{
	float SpeedMultiplier(RE::Actor* actor, const RE::MagicItem* spell) {
		if (auto skillUsage = SkillUsage(spell, actor, kChargeTime); skillUsage) {
			return skillUsage.SkillFactor(actor);
		}
		return 1.0f;
	}
}

namespace NLA::ChargeTime
{
	struct MagicCaster_Update
	{
		using Target = RE::ActorMagicCaster;

		static inline constexpr VariantIndex index{ 0x1D, 0x1D, 0x1F };

		static void thunk(RE::ActorMagicCaster* caster, float delta) {
			if (caster && Caster::IsCharging(caster->state.get())) {
				delta *= SpeedMultiplier(caster->actor, caster->currentSpell);
			}

			func(caster, delta);
		}

		static inline REL::Relocation<decltype(thunk)> func;

		static inline void pre_hook() { logger::debug("Installing ChargeTime hook..."); }
		static inline void post_hook() { logger::info("🪝 Installed ChargeTime hook."); }
	};

	void Install() {
		stl::install_hook<MagicCaster_Update>();
	}
}
