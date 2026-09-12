#pragma once

namespace NLA
{
	void Install();
}

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
