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
		 * @brief Alias for format string type using `char` character type.
		 * @tparam Args Variadic template for format argument types.
		 *
		 * Uses `std::format_string` or `fmt::format_string` depending on compilation.
		 */
		template <typename... Args>
#ifdef USE_STD_FORMAT
		using format_string = std::format_string<Args...>;
#else
		using format_string = fmt::format_string<Args...>;
#endif

		/**
		 * @brief Alias for format string type using `wchar_t` character type.
		 * @tparam Args Variadic template for format argument types.
		 */
		template <typename... Args>
#ifdef USE_STD_FORMAT
		using wformat_string = std::wformat_string<Args...>;
#else
		using wformat_string = fmt::format_string<Args...>;
#endif

		/**
		 * @brief Formats a string using `char` type and the provided format string and arguments.
		 *
		 * @tparam Args Variadic template parameter for argument types.
		 * @param format_string The format string of type `char`.
		 * @param args The arguments to be formatted according to `format_string`.
		 * @return std::string The formatted string.
		 */
		template <typename... Args>
		static auto format(format_string<Args...> format_string, Args&&... args) -> std::string
		{
#ifdef USE_STD_FORMAT
			return std::format(format_string, std::forward<Args>(args)...);
#else
			return fmt::format(format_string, std::forward<Args>(args)...);
#endif
		}

		/**
		 * @brief Formats a string using `wchar_t` type.
		 *
		 * @tparam Args Variadic template parameter for argument types.
		 * @param format_string The format string of type `wchar_t`.
		 * @param args The arguments to be formatted according to `format_string`.
		 * @return std::wstring The formatted wide string.
		 */
		template <typename... Args>
		static auto format(wformat_string<Args...> format_string, Args&&... args) -> std::wstring
		{
#ifdef USE_STD_FORMAT
			return std::format(format_string, std::forward<Args>(args)...);
#else
			return fmt::format(format_string, std::forward<Args>(args)...);
#endif
		}

		/**
		 * @brief Formats a string and writes the result to an output iterator, using `char` type.
		 *
		 * @tparam OutputIt The type of the output iterator.
		 * @tparam Args Variadic template parameter for argument types.
		 * @param out The output iterator to write the formatted string to.
		 * @param format_string The format string of type `char`.
		 * @param args The arguments to be formatted.
		 */
		template <typename OutputIt, typename... Args>
		static auto format_to(OutputIt out,
							  format_string<Args...> format_string,
							  Args&&... args) -> void
		{
#ifdef USE_STD_FORMAT
			std::format_to(out, format_string, std::forward<Args>(args)...);
#else
			fmt::format_to(out, format_string, std::forward<Args>(args)...);
#endif
		}

		/**
		 * @brief Formats a string and writes the result to an output iterator, using `wchar_t`
		 * type.
		 *
		 * @tparam OutputIt The type of the output iterator.
		 * @tparam Args Variadic template parameter for argument types.
		 * @param out The output iterator to write the formatted wide string to.
		 * @param format_string The format string of type `wchar_t`.
		 * @param args The arguments to be formatted.
		 */
		template <typename OutputIt, typename... Args>
		static auto format_to(OutputIt out,
							  wformat_string<Args...> format_string,
							  Args&&... args) -> void
		{
#ifdef USE_STD_FORMAT
			std::format_to(out, format_string, std::forward<Args>(args)...);
#else
			fmt::format_to(out, format_string, std::forward<Args>(args)...);
#endif
		}
	};
} // namespace utility_module