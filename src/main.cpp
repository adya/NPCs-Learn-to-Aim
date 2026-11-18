#include "AimDebug.h"
#include "Hooks.h"
#include "Menu/Menu.h"
#include "Options.h"

void MessageHandler(SKSE::MessagingInterface::Message* a_message) {
	switch (a_message->type) {
	case SKSE::MessagingInterface::kPostLoad:
		NLA::Install();
		NLA::Options::Load();
		NLA::Menu::Register();
#ifndef NDEBUG
		NLA::AimDebug::Install();
#endif
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		NLA::ListenForEvents();
		break;
	case SKSE::MessagingInterface::kPreLoadGame:
	case SKSE::MessagingInterface::kNewGame:
		NLA::Reset();
		break;
	default:
		break;
	}
}

SKSE_EXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Version::MAJOR);
	v.PluginName(Version::PROJECT.data());
	v.AuthorName("sasnikol");
	v.UsesAddressLibrary();
	v.UsesUpdatedStructs();
	v.CompatibleVersions({ SKSE::RUNTIME_SSE_LATEST, SKSE::RUNTIME_LATEST_VR });

	return v;
}();

SKSE_EXPORT bool SKSEPlugin_Query(SKSE::QueryInterface*, SKSE::PluginInfo* a_info) {
	a_info->name = SKSEPlugin_Version.pluginName;
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->version = SKSEPlugin_Version.pluginVersion;
	return true;
}

SKSE_EXPORT bool SKSEPlugin_Load(const SKSE::LoadInterface* a_skse) {
	const auto logLevel = NLA::Options::LogLevel();

	SKSE::Init(a_skse, { .logLevel = logLevel });

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
	logger::info("Log level: {}", spdlog::level::to_string_view(logLevel));

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	logger::info("Game version : {}", a_skse->RuntimeVersion().string());

#ifdef ENABLE_SKYRIM_VR
	if (REL::Module::IsVR()) {
		REL::IDDB::get().IsVRAddressLibraryAtLeastVersion("0.305.0", true);
	}
#endif

	// Pre-allocate trampoline space upfront. Each write_call<5> hook needs 14 bytes.
	SKSE::AllocTrampoline(14 * 25);

	SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);

	return true;
}
