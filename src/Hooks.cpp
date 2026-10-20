#include "Hooks.h"
#include "Aiming/Aiming.h"
#include "ChargeTime.h"
#include "ConcentrationSpells.h"
#include "DrawSpeed.h"
#include "Options.h"

namespace NLA
{
	void Install() {
		logger::info("{:*^40}", "HOOKS");

		Accuracy::Install();
		Precision::Install();
		ConcentrationSpells::Install();
		ChargeTime::Install();
		Draw::Install();
	}

	void Reset() {
		Options::InvalidateActorCache();
		Accuracy::Reset();
		ConcentrationSpells::Reset();
		ChargeTime::Reset();
		Draw::Reset();
	}
}
