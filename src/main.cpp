#include "ChargeTime.h"
#include "Hooks.h"
#include "Menu.h"
#include "Options.h"

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void MessageHandler(SKSE::MessagingInterface::Message* a_message) {
	switch (a_message->type) {
	case SKSE::MessagingInterface::kPostLoad:
		NLA::Install();
		NLA::Options::Load();
		NLA::Menu::Register();
		break;
	case SKSE::MessagingInterface::kPreLoadGame:
	case SKSE::MessagingInterface::kNewGame:
		NLA::ChargeTime::Reset();
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

void InitializeLog() {
	auto path = logger::log_directory();
	if (!path) {
		SKSE::stl::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
}

SKSE_EXPORT bool SKSEPlugin_Load(const SKSE::LoadInterface* a_skse) {
	InitializeLog();

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	logger::info("Game version : {}", a_skse->RuntimeVersion().string());

#ifdef ENABLE_SKYRIM_VR
	if (REL::Module::IsVR()) {
		REL::IDDB::get().IsVRAddressLibraryAtLeastVersion("0.134.0", true);
	}
#endif

	SKSE::Init(a_skse);

	// Pre-allocate trampoline space upfront. Each write_call<5> hook needs 14 bytes.
	SKSE::AllocTrampoline(14 * 16);

	SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);

	return true;
}
