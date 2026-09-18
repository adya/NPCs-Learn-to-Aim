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

inline std::default_random_engine rnd{};

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
