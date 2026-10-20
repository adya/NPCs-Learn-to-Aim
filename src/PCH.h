#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <ClibUtil/simpleINI.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <xbyak/xbyak.h>

#define DLLEXPORT __declspec(dllexport)

namespace logger = SKSE::log;
using namespace std::literals;

#include "Version.h"

/// Each thread has its own engine, seeded from the OS, so sequences differ between launches and threads never share state.
inline thread_local std::mt19937 rnd{ std::random_device{}() };

namespace fmt
{
	template <>
	struct formatter<RE::Actor>
	{
		template <class ParseContext>
		constexpr auto parse(ParseContext& a_ctx) const {
			return a_ctx.begin();
		}

		template <class FormatContext>
		constexpr auto format(const RE::Actor& actor, FormatContext& a_ctx) const {
			return fmt::format_to(a_ctx.out(), "'{}' [0x{:X}]", actor.GetActorBase()->GetName(), actor.formID);
		}
	};
}
