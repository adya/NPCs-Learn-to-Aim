#include "ChargeTime.h"
#include "Hooking.h"
#include "Options.h"
#include "SkillUsage.h"

namespace NLA::ChargeTime
{
	struct MagicCaster_Update
	{
		using Target = RE::ActorMagicCaster;

		static inline constexpr VariantIndex index{ 0x1D, 0x1D, 0x1F };

		static void thunk(RE::ActorMagicCaster* caster, float delta) {
			if (caster && caster->state == RE::MagicCaster::State::kUnk02) {  // Actualy, the state during charging is 2, unlike enum's suggested kCharging = 5.
				if (auto skillUsage = SkillUsage(caster->currentSpell, caster->actor, kChargeTime); skillUsage) {
					delta *= skillUsage.LerpSkillFactor(caster->actor);
				}
			}

			func(caster, delta);
		}

		static inline REL::Relocation<decltype(thunk)> func;

		static inline void pre_hook() { logger::info("Installing ChargeTime hook..."); }
		static inline void post_hook() { logger::info("Installed ChargeTime hook."); }
	};

	void Install() {
		stl::install_hook<MagicCaster_Update>();
	}
}
