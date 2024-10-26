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
	template <typename... Args>
	using format_string =
#ifdef USE_STD_FORMAT
		std::format_string<Args...>;
#else
		fmt::format_string<Args...>;
#endif

	template <typename... Args>
	using wformat_string =
#ifdef USE_STD_FORMAT
		std::wformat_string<Args...>;
#else
		fmt::basic_format_string<wchar_t, Args...>;
#endif

	class formatter
	{
	public:
		template <typename... FormatArgs>
		static auto format(format_string<FormatArgs...> formats,
						   FormatArgs&&... args) -> std::string
		{
#ifdef USE_STD_FORMAT
			return std::format(std::move(formats), std::forward<FormatArgs>(args)...);
#else
			return fmt::format(std::move(formats), std::forward<FormatArgs>(args)...);
#endif
		}

		template <typename... WideFormatArgs>
		static auto format(wformat_string<WideFormatArgs...> formats,
						   WideFormatArgs&&... args) -> std::wstring
		{
#ifdef USE_STD_FORMAT
			return std::format(std::move(formats), std::forward<WideFormatArgs>(args)...);
#else
			return fmt::format(std::move(formats), std::forward<WideFormatArgs>(args)...);
#endif
		}

		template <typename OutputIt, typename... FormatArgs>
		static auto format_to(OutputIt out,
							  format_string<FormatArgs...> formats,
							  FormatArgs&&... args) -> void
		{
#ifdef USE_STD_FORMAT
			std::format_to(out, std::move(formats), std::forward<FormatArgs>(args)...);
#else
			fmt::format_to(out, std::move(formats), std::forward<FormatArgs>(args)...);
#endif
		}

		template <typename OutputIt, typename... WideFormatArgs>
		static auto format_to(OutputIt out,
							  wformat_string<WideFormatArgs...> formats,
							  WideFormatArgs&&... args) -> void
		{
#ifdef USE_STD_FORMAT
			std::format_to(out, std::move(formats), std::forward<WideFormatArgs>(args)...);
#else
			fmt::format_to(out, std::move(formats), std::forward<WideFormatArgs>(args)...);
#endif
		}
	};
} // namespace utility_module
