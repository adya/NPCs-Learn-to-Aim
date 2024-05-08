#include "Options.h"

namespace NLA::Options
{
	void Options::Load() {
		logger::info("{:*^30}", "OPTIONS");
		std::filesystem::path options = R"(Data\SKSE\Plugins\NPCsLearnToAim.ini)";
		CSimpleIniA           ini{};
		ini.SetUnicode();
		if (ini.LoadFile(options.string().c_str()) >= 0) {
			General::archeryAiming = ini.GetBoolValue("General", "bEnableArcheryAim", General::archeryAiming);
			General::magicAiming = ini.GetBoolValue("General", "bEnableMagicAim", General::magicAiming);
			General::forceAimForAll = ini.GetBoolValue("General", "bForceAimForAll", General::forceAimForAll);

			Complexity::useComplexity = ini.GetBoolValue("ShotComplexity", "bUseShotComplexity", Complexity::useComplexity);
			float distance = ini.GetDoubleValue("ShotComplexity", "fMaxDistance", Complexity::maxDistance);
			if (distance > 0.0f)
				Complexity::maxDistance = distance;
			else {
				logger::warn("fMaxDistance must be greater than 0. Using default value.");
			}

			float targetSize = ini.GetDoubleValue("ShotComplexity", "fMaxTargetSize", Complexity::maxTargetSize);
			if (targetSize > 0.0f)
				Complexity::maxTargetSize = targetSize;
			else {
				logger::warn("fMaxTargetSize must be greater than 0. Using default value.");
			}
		} else {
			logger::info(R"(Data\SKSE\Plugins\NPCsLearnToAim.ini not found. Default options will be used.)");
			logger::info("");
		}

		logger::info("General:");
		logger::info("\tArchers {} learn to aim", General::archeryAiming ? "will" : "won't");
		logger::info("\tMages {} learn to aim", General::magicAiming ? "will" : "won't");

		if (General::forceAimForAll) {
			logger::info("\tAll NPCs will learn to aim (except those marked with NLA_Ignored keyword)");
		} else {
			logger::info("\tOnly ActorTypeNPC will learn to aim (except those marked with NLA_Ignored keyword)");
		}

		logger::info("Shot Complexity:");
		if (Complexity::useComplexity) {
			logger::info("\tShots complexity will be based on distance to the target and its size.");
			logger::info("\tMax Distance: {}", Complexity::maxDistance);
			logger::info("\tMax Target Size: {}", Complexity::maxTargetSize);
		} else {
			logger::info("\tAll shots will be trivial :) only NPC's skill determines their accuracy.");
		}
	}
}
