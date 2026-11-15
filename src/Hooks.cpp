#include "Hooks.h"
#include "Aiming/Aiming.h"
#include "ChargeTime.h"
#include "ConcentrationSpells.h"
#include "DrawSpeed.h"
#include "Options.h"
#include "Steadying.h"

namespace NLA
{
	void Install() {
		logger::info("{:*^40}", "HOOKS");

		Aim::Install();
		Skillshots::Install();
		ConcentrationSpells::Install();
		ChargeTime::Install();
		Draw::Install();
		Steadying::Install();
	}

	void Reset() {
		Options::InvalidateActorCache();
		Accuracy::Reset();
		ConcentrationSpells::Reset();
		Steadying::Reset();
	}
}
