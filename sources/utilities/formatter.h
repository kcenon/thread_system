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
#endif

namespace utility_module
{
	/**
	 * @class formatter
	 * @brief A utility class for string formatting.
	 *
	 * This class provides static methods for string formatting using either
	 * the C++20 std::format or the {fmt} library, depending on the compilation flag.
	 */
	class formatter
	{
	public:
		/**
		 * @brief Formats a string using the provided format string and arguments.
		 *
		 * @tparam Args Variadic template parameter for the argument types.
		 * @param format_str The format string.
		 * @param args The arguments to be formatted.
		 * @return std::string The formatted string.
		 *
		 * @note This method uses std::format if USE_STD_FORMAT is defined, otherwise it uses
		 * fmt::format.
		 */
		template <typename... Args>
		static auto format(
#ifdef USE_STD_FORMAT
			std::format_string<Args...> format_str,
#else
			fmt::format_string<Args...> format_str,
#endif
			Args&&... args) -> std::string
		{
			return
#ifdef USE_STD_FORMAT
				std::format
#else
				fmt::format
#endif
				(format_str, std::forward<Args>(args)...);
		}

		/**
		 * @brief Formats a string and writes the result to the provided output iterator.
		 *
		 * @tparam OutputIt The type of the output iterator.
		 * @tparam Args Variadic template parameter for the argument types.
		 * @param out The output iterator to write the formatted string to.
		 * @param format_str The format string.
		 * @param args The arguments to be formatted.
		 *
		 * @note This method uses std::format_to if USE_STD_FORMAT is defined, otherwise it uses
		 * fmt::format_to.
		 */
		template <typename OutputIt, typename... Args>
		static auto format_to(
#ifdef USE_STD_FORMAT
			OutputIt out,
			std::format_string<Args...> format_str,
#else
			OutputIt out,
			fmt::format_string<Args...> format_str,
#endif
			Args&&... args) -> void
		{
#ifdef USE_STD_FORMAT
			std::format_to(out, format_str, std::forward<Args>(args)...);
#else
			fmt::format_to(out, format_str, std::forward<Args>(args)...);
#endif
		}
	};
} // namespace utility_module