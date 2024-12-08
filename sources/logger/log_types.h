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

#include "formatter.h"
#include "convert_string.h"

#include <string>
#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>

using namespace utility_module;

namespace log_module
{
	/**
	 * @enum log_types
	 * @brief Enumeration of different log types.
	 *
	 * Defines various log categories that can be used within the logging system.
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
		/** @brief Array of string representations for each log type. */
		constexpr std::array log_type_strings
			= { "NONE", "EXCEPTION", "ERROR", "INFORMATION", "DEBUG", "SEQUENCE", "PARAMETER" };

		/** @brief Number of log types available in log_type_strings. */
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

// Formatter specializations for log_types
#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for log_module::log_types.
 * Enables formatting of log_types enum values as strings in the standard library format.
 */
template <> struct std::formatter<log_module::log_types> : std::formatter<std::string_view>
{
	/**
	 * @brief Formats a log_type value as a string.
	 * @tparam FormatContext Type of the format context.
	 * @param log_type The log_types enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const log_module::log_types& log_type, FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(log_module::to_string(log_type), ctx);
	}
};

/**
 * @brief Specialization of std::formatter for wide-character log_module::log_types.
 * Allows log_types enum values to be formatted as wide strings in the standard library format.
 */
template <>
struct std::formatter<log_module::log_types, wchar_t> : std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a log_type value as a wide string.
	 * @tparam FormatContext Type of the format context.
	 * @param log_type The log_types enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const log_module::log_types& log_type, FormatContext& ctx) const
	{
		auto str = log_module::to_string(log_type);
		auto wstr = convert_string::to_wstring(str);
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};
#else
/**
 * @brief Specialization of fmt::formatter for log_module::log_types.
 * Enables formatting of log_types enum values using the fmt library.
 */
template <> struct fmt::formatter<log_module::log_types> : fmt::formatter<std::string_view>
{
	/**
	 * @brief Formats a log_type value as a string.
	 * @tparam FormatContext Type of the format context.
	 * @param log_type The log_types enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const log_module::log_types& log_type, FormatContext& ctx) const
	{
		return fmt::formatter<std::string_view>::format(log_module::to_string(log_type), ctx);
	}
};

/**
 * @brief Specialization of fmt::formatter for wide-character log_module::log_types.
 * Allows log_types enum values to be formatted as wide strings in the standard library format.
 */
template <>
struct fmt::formatter<log_module::log_types, wchar_t> : fmt::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @brief Formats a log_type value as a wide string.
	 * @tparam FormatContext Type of the format context.
	 * @param log_type The log_types enum value to format.
	 * @param ctx Format context for the output.
	 * @return Iterator to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const log_module::log_types& log_type, FormatContext& ctx) const
	{
		auto str = log_module::to_string(log_type);
		auto wstr = convert_string::to_wstring(str);
		return fmt::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};
#endif
