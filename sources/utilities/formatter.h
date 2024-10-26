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

#ifdef USE_STD_FORMAT
#include <format>
#else
#include <fmt/format.h>
#include <fmt/xchar.h>
#endif

namespace utility_module
{
	/**
	 * @brief Alias for format string type using `char` character type.
	 * @tparam Args Variadic template for format argument types.
	 *
	 * Uses `std::format_string` or `fmt::format_string` depending on compilation.
	 */
	template <typename... Args>
	using format_string =
#ifdef USE_STD_FORMAT
		std::format_string<Args...>;
#else
		fmt::format_string<Args...>;
#endif

#ifdef USE_STD_FORMAT
	/**
	 * @brief Alias for format string type using `wchar_t` character type.
	 * @tparam Args Variadic template for format argument types.
	 */
	template <typename... Args> using wformat_string = std::wformat_string<Args...>;
#endif

	/**
	 * @class formatter
	 * @brief A utility class for string formatting.
	 *
	 * This class provides static methods for string formatting using either
	 * the C++20 `std::format` or the `{fmt}` library, depending on the compilation flag.
	 * Supports formatting for various character types, including `char`, `wchar_t`,
	 * `char16_t`, and `char32_t`.
	 */
	class formatter
	{
	public:
		/**
		 * @brief Formats a string using `char` type and the provided format string and arguments.
		 *
		 * @tparam FormatArgs Variadic template parameter for argument types.
		 * @param format_str The format string of type `char`.
		 * @param args The arguments to be formatted according to `format_str`.
		 * @return std::string The formatted string.
		 */
		template <typename... FormatArgs>
		static auto format(const format_string<FormatArgs...>& format_str,
						   FormatArgs&&... args) -> std::string
		{
#ifdef USE_STD_FORMAT
			return std::format(format_str, std::forward<FormatArgs>(args)...);
#else
			return fmt::format(format_str, std::forward<FormatArgs>(args)...);
#endif
		}

#ifdef USE_STD_FORMAT
		/**
		 * @brief Formats a string using `wchar_t` type.
		 *
		 * @tparam WideFormatArgs Variadic template parameter for argument types.
		 * @param format_str The format string of type `wchar_t`.
		 * @param args The arguments to be formatted according to `format_str`.
		 * @return std::wstring The formatted wide string.
		 */
		template <typename... WideFormatArgs>
		static auto format(const wformat_string<WideFormatArgs...>& format_str,
						   WideFormatArgs&&... args) -> std::wstring
		{
			return std::format(format_str, std::forward<WideFormatArgs>(args)...);
		}
#endif

		/**
		 * @brief Formats a string and writes the result to an output iterator, using `char` type.
		 *
		 * @tparam OutputIt The type of the output iterator.
		 * @tparam FormatArgs Variadic template parameter for argument types.
		 * @param out The output iterator to write the formatted string to.
		 * @param format_str The format string of type `char`.
		 * @param args The arguments to be formatted.
		 */
		template <typename OutputIt, typename... FormatArgs>
		static auto format_to(OutputIt out,
							  const format_string<FormatArgs...>& format_str,
							  FormatArgs&&... args) -> void
		{
#ifdef USE_STD_FORMAT
			std::format_to(out, format_str, std::forward<FormatArgs>(args)...);
#else
			fmt::format_to(out, format_str, std::forward<FormatArgs>(args)...);
#endif
		}

#ifdef USE_STD_FORMAT
		/**
		 * @brief Formats a string and writes the result to an output iterator, using `wchar_t`
		 * type.
		 *
		 * @tparam OutputIt The type of the output iterator.
		 * @tparam WideFormatArgs Variadic template parameter for argument types.
		 * @param out The output iterator to write the formatted wide string to.
		 * @param format_str The format string of type `wchar_t`.
		 * @param args The arguments to be formatted.
		 */
		template <typename OutputIt, typename... WideFormatArgs>
		static auto format_to(OutputIt out,
							  const wformat_string<WideFormatArgs...>& format_str,
							  WideFormatArgs&&... args) -> void
		{
			std::format_to(out, format_str, std::forward<WideFormatArgs>(args)...);
		}
#endif
	};
} // namespace utility_module
