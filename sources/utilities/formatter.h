/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#pragma once

#include <string>
#include <string_view>
#include <type_traits>

#ifdef USE_STD_FORMAT
#include <format>
#else
#include <fmt/format.h>
#include <fmt/xchar.h>
#endif

namespace utility_module
{
	// Forward declaration of formatter class
	class formatter;

	/**
	 * @brief Base formatter implementation for formatting enum types with a custom converter.
	 * @tparam T Enum type to be formatted.
	 * @tparam Converter Function object type that converts T to a string representation.
	 */
	template <typename T, typename Converter> class enum_formatter
	{
	private:
		/**
		 * @brief Helper function to format the enum value to a string or wide string.
		 * @tparam CharT Character type (either char or wchar_t).
		 * @param out Output iterator for the formatted result.
		 * @param value Enum value to be formatted.
		 * @return Output iterator pointing to the end of the formatted output.
		 */
		template <typename CharT> static auto do_format(auto& out, const T& value)
		{
#ifdef USE_STD_FORMAT
			if constexpr (std::is_same_v<CharT, wchar_t>)
			{
				return std::format_to(out, L"{}", Converter{}(value));
			}
			else
			{
				return std::format_to(out, "{}", Converter{}(value));
			}
#else
			if constexpr (std::is_same_v<CharT, wchar_t>)
			{
				return fmt::format_to(out, L"{}", Converter{}(value));
			}
			else
			{
				return fmt::format_to(out, "{}", Converter{}(value));
			}
#endif
		}

	public:
		/**
		 * @brief Parses the format context.
		 * @param ctx Format context.
		 * @return Iterator pointing to the end of the parsing context.
		 */
		constexpr auto parse(auto& ctx) { return ctx.begin(); }

		/**
		 * @brief Formats the provided enum value into the given format context.
		 * @tparam FormatContext Type of the format context.
		 * @param value Enum value to be formatted.
		 * @param ctx Format context to output the result.
		 * @return Output iterator pointing to the end of the formatted output.
		 */
		template <typename FormatContext> auto format(const T& value, FormatContext& ctx) const
		{
			using char_type =
				typename std::iterator_traits<typename FormatContext::iterator>::value_type;
			return do_format<char_type>(ctx.out(), value);
		}
	};

	/**
	 * @class formatter
	 * @brief Provides utilities for formatting strings and wide strings.
	 *
	 * Supports both std::format (when USE_STD_FORMAT is defined) and the fmt library
	 * as a fallback, ensuring compatibility and flexibility for formatting.
	 */
	class formatter
	{
	public:
		/**
		 * @brief Formats a string using the provided arguments.
		 * @tparam FormatArgs Types of the format arguments.
		 * @param formats C-style format string.
		 * @param args Values to format into the placeholders.
		 * @return Formatted std::string.
		 */
		template <typename... FormatArgs>
		static auto format(const char* formats, const FormatArgs&... args) -> std::string
		{
#ifdef USE_STD_FORMAT
			return std::vformat(formats, std::make_format_args(args...));
#else
			return fmt::format(fmt::runtime(formats), args...);
#endif
		}

		/**
		 * @brief Formats a wide string using the provided arguments.
		 * @tparam FormatArgs Types of the format arguments.
		 * @param formats Wide C-style format string.
		 * @param args Values to format into the placeholders.
		 * @return Formatted std::wstring.
		 */
		template <typename... FormatArgs>
		static auto format(const wchar_t* formats, const FormatArgs&... args) -> std::wstring
		{
#ifdef USE_STD_FORMAT
			return std::vformat(formats, std::make_wformat_args(args...));
#else
			return fmt::format(fmt::runtime(formats), args...);
#endif
		}

		/**
		 * @brief Formats a string directly to an output iterator.
		 * @tparam OutputIt Type of the output iterator.
		 * @tparam FormatArgs Types of the format arguments.
		 * @param out Output iterator for the formatted result.
		 * @param formats C-style format string.
		 * @param args Values to format into the placeholders.
		 * @return Output iterator pointing to the end of the formatted output.
		 */
		template <typename OutputIt, typename... FormatArgs>
		static auto format_to(OutputIt out,
							  const char* formats,
							  const FormatArgs&... args) -> OutputIt
		{
#ifdef USE_STD_FORMAT
			return std::vformat_to(out, formats, std::make_format_args(args...));
#else
			return fmt::format_to(out, fmt::runtime(formats), args...);
#endif
		}

		/**
		 * @brief Formats a wide string directly to an output iterator.
		 * @tparam OutputIt Type of the output iterator.
		 * @tparam FormatArgs Types of the format arguments.
		 * @param out Output iterator for the formatted result.
		 * @param formats Wide C-style format string.
		 * @param args Values to format into the placeholders.
		 * @return Output iterator pointing to the end of the formatted output.
		 */
		template <typename OutputIt, typename... FormatArgs>
		static auto format_to(OutputIt out,
							  const wchar_t* formats,
							  const FormatArgs&... args) -> OutputIt
		{
#ifdef USE_STD_FORMAT
			return std::vformat_to(out, formats, std::make_wformat_args(args...));
#else
			return fmt::format_to(out, fmt::runtime(formats), args...);
#endif
		}
	};
} // namespace utility_module
