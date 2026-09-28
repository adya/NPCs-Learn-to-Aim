#include "Hooks.h"
#include "Aiming.h"
#include "RE/C/CombatProjectileAimController.h"
#include "ConcentrationSpells.h"
#include "ChargeTime.h"
#include "DrawSpeed.h"

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
}
