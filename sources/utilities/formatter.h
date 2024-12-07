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
	/**
	 * @class enum_formatter
	 * @brief A formatter template for converting enums to strings using a custom converter functor.
	 * @tparam T Enum type to be formatted.
	 * @tparam Converter A functor type that, when invoked, returns a string representation of the
	 * enum value.
	 */
	template <typename T, typename Converter> class enum_formatter
	{
	private:
		/**
		 * @brief Internal helper function that performs the formatting operation.
		 * @tparam CharT Character type (char or wchar_t).
		 * @param out Output iterator where the formatted output will be written.
		 * @param value The enum value to format.
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
		 * @brief Parse function required by formatting library, does nothing here.
		 * @param ctx The format parse context.
		 * @return Iterator pointing to the end of the parse context.
		 */
		constexpr auto parse(auto& ctx) { return ctx.begin(); }

		/**
		 * @brief Formats the enum value into the provided format context.
		 * @tparam FormatContext The type of the format context.
		 * @param value The enum value to be formatted.
		 * @param ctx The format context.
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
	 * @brief Utility class for formatting strings using either std::format or the fmt library.
	 *
	 * This class provides static methods to format strings and write the result either to a
	 * returned string or directly to an output iterator.
	 */
	class formatter
	{
	public:
		/**
		 * @brief Format a string using the provided arguments.
		 * @tparam FormatArgs Parameter pack of arguments for formatting.
		 * @param formats The format string (e.g., "Hello {}").
		 * @param args The arguments to replace the placeholders.
		 * @return A formatted std::string.
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
		 * @brief Format a wide string using the provided arguments.
		 * @tparam FormatArgs Parameter pack of arguments for formatting.
		 * @param formats The wide format string.
		 * @param args The arguments to replace the placeholders.
		 * @return A formatted std::wstring.
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
		 * @brief Format a string directly to an output iterator.
		 * @tparam OutputIt Output iterator type.
		 * @tparam FormatArgs Parameter pack of arguments for formatting.
		 * @param out The output iterator.
		 * @param formats The format string.
		 * @param args The arguments to replace the placeholders.
		 * @return OutputIt The output iterator at the end of the formatted output.
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
		 * @brief Format a wide string directly to an output iterator.
		 * @tparam OutputIt Output iterator type.
		 * @tparam FormatArgs Parameter pack of arguments for formatting.
		 * @param out The output iterator.
		 * @param formats The wide format string.
		 * @param args The arguments to replace the placeholders.
		 * @return OutputIt The output iterator at the end of the formatted output.
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