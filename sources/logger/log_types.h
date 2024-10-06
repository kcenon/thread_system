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

#ifdef USE_STD_FORMAT
#include <format>
#else
#include "fmt/format.h"
#endif

#include <string>
#include <array>
#include <cstdint>

namespace log_module
{
	/**
	 * @enum log_types
	 * @brief Enumeration of different log types.
	 *
	 * This enum class defines various types of log entries that can be used
	 * in the logging system. It is based on uint8_t for efficient storage.
	 */
	enum class log_types : uint8_t
	{
		None,		 ///< No specific log type
		Exception,	 ///< Exception log type
		Error,		 ///< Error log type
		Information, ///< Information log type
		Debug,		 ///< Debug log type
		Sequence,	 ///< Sequence log type
		Parameter	 ///< Parameter log type
	};

	namespace log_detail
	{
		/** @brief Array of string representations for log types */
		constexpr std::array log_type_strings
			= { "NONE", "EXCEPTION", "ERROR", "INFORMATION", "DEBUG", "SEQUENCE", "PARAMETER" };

		/** @brief Number of log types */
		constexpr size_t log_type_count = log_type_strings.size();

		// Compile-time check to ensure log_type_strings and log_types are in sync
		static_assert(log_type_count == static_cast<size_t>(log_types::Parameter) + 1,
					  "log_type_strings and log_types enum are out of sync");
	}

	/**
	 * @brief Converts a log_types value to its string representation.
	 * @param log_type The log_types value to convert.
	 * @return std::string_view A string representation of the log type.
	 */
	[[nodiscard]] constexpr std::string_view to_string(log_types log_type)
	{
		auto index = static_cast<size_t>(log_type);
		return (index < log_detail::log_type_count) ? log_detail::log_type_strings[index]
													: "UNKNOWN";
	}
} // namespace log_module

#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for log_module::log_types.
 *
 * This formatter allows log_types to be used with std::format.
 * It converts the log_types enum values to their string representations.
 */
template <> struct std::formatter<log_module::log_types>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const log_module::log_types& log_type, FormatContext& ctx) const
	{
		return std::format_to(ctx.out(), "{}", log_module::to_string(log_type));
	}
};
#else
/**
 * @brief Specialization of fmt::formatter for log_module::log_types.
 *
 * This formatter allows log_types to be used with fmt::format.
 * It converts the log_types enum values to their string representations.
 */
template <> struct fmt::formatter<log_module::log_types>
{
	constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const log_module::log_types& log_type, FormatContext& ctx) const
	{
		return fmt::format_to(ctx.out(), "{}", log_module::to_string(log_type));
	}
};
#endif