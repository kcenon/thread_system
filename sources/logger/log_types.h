/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions of source code must reproduce the above copyright notice,
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

#include "../utilities/formatter.h"
#include "../utilities/convert_string.h"

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
	 * @brief Enumerates different categories of log messages.
	 *
	 * Each enumerator represents a distinct log category (e.g., error, debug) used
	 * within the logging system. These values can be mapped to human-readable strings
	 * or used programmatically for filtering and routing log messages.
	 */
	enum class log_types : uint8_t
	{
		None,		 ///< No specific category for the log message.
		Exception,	 ///< Indicates an exception-related log message.
		Error,		 ///< Represents an error-level log message.
		Information, ///< General informational log message.
		Debug,		 ///< Diagnostic or debug-level log message.
		Sequence,	 ///< Log message related to a particular sequence or flow.
		Parameter	 ///< Log message describing parameters or configurations.
	};

	namespace log_detail
	{
		/**
		 * @brief String representations corresponding to each enumerator in @ref log_types.
		 *
		 * The index in this array matches the integer value of each enumerator in
		 * @ref log_types, thus ensuring direct indexing.
		 */
		constexpr std::array log_type_strings
			= { "NONE", "EXCEPTION", "ERROR", "INFORMATION", "DEBUG", "SEQUENCE", "PARAMETER" };

		/**
		 * @brief Total number of log types defined in @ref log_type_strings.
		 *
		 * Used to validate that the @ref log_type_strings array remains in sync
		 * with the @ref log_types enum.
		 */
		constexpr size_t log_type_count = log_type_strings.size();

		/**
		 * @brief Compile-time assertion to ensure that @ref log_type_strings aligns with @ref
		 * log_types.
		 *
		 * If the sizes do not match, this static_assert triggers a compile-time error,
		 * indicating that the strings array or the enum is out of date.
		 */
		static_assert(log_type_count == static_cast<size_t>(log_types::Parameter) + 1,
					  "log_type_strings and log_types enum are out of sync");
	}

	/**
	 * @brief Retrieves the string representation of a given @ref log_types value.
	 *
	 * Provides a human-readable form of the @ref log_types enum value. If the provided
	 * value is out of range, it returns "UNKNOWN".
	 *
	 * @param log_type A @ref log_types enumerator to be converted.
	 * @return std::string_view The corresponding string representation of the log type.
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
 * @brief Specialization of std::formatter for @ref log_module::log_types.
 *
 * Allows @ref log_module::log_types values to be formatted as strings using
 * the C++20 standard <format> library.
 */
template <> struct std::formatter<log_module::log_types> : std::formatter<std::string_view>
{
	/**
	 * @tparam FormatContext Type of the format context used for output.
	 * @brief Formats a @ref log_module::log_types enumerator as a string.
	 *
	 * @param log_type A @ref log_module::log_types value to be formatted.
	 * @param ctx The formatting context where the output is written.
	 * @return FormatContext::iterator An iterator pointing to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const log_module::log_types& log_type, FormatContext& ctx) const
	{
		return std::formatter<std::string_view>::format(log_module::to_string(log_type), ctx);
	}
};

/**
 * @brief Specialization of std::formatter for wide-character @ref log_module::log_types.
 *
 * Enables @ref log_module::log_types values to be formatted as wide strings
 * using the C++20 standard <format> library.
 */
template <>
struct std::formatter<log_module::log_types, wchar_t> : std::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @tparam FormatContext Type of the format context used for output.
	 * @brief Formats a @ref log_module::log_types enumerator as a wide string.
	 *
	 * @param log_type A @ref log_module::log_types value to be formatted.
	 * @param ctx The formatting context where the output is written.
	 * @return FormatContext::iterator An iterator pointing to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const log_module::log_types& log_type, FormatContext& ctx) const
	{
		auto str = log_module::to_string(log_type);
		auto wstr = convert_string::to_wstring(str);
		return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};

#else  // Fallback to fmt library

/**
 * @brief Specialization of fmt::formatter for @ref log_module::log_types.
 *
 * Allows @ref log_module::log_types values to be formatted as strings using
 * the {fmt} library.
 */
template <> struct fmt::formatter<log_module::log_types> : fmt::formatter<std::string_view>
{
	/**
	 * @tparam FormatContext Type of the format context used for output.
	 * @brief Formats a @ref log_module::log_types enumerator as a string.
	 *
	 * @param log_type A @ref log_module::log_types value to be formatted.
	 * @param ctx The formatting context where the output is written.
	 * @return FormatContext::iterator An iterator pointing to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const log_module::log_types& log_type, FormatContext& ctx) const
	{
		return fmt::formatter<std::string_view>::format(log_module::to_string(log_type), ctx);
	}
};

/**
 * @brief Specialization of fmt::formatter for wide-character @ref log_module::log_types.
 *
 * Enables @ref log_module::log_types values to be formatted as wide strings
 * using the {fmt} library.
 */
template <>
struct fmt::formatter<log_module::log_types, wchar_t> : fmt::formatter<std::wstring_view, wchar_t>
{
	/**
	 * @tparam FormatContext Type of the format context used for output.
	 * @brief Formats a @ref log_module::log_types enumerator as a wide string.
	 *
	 * @param log_type A @ref log_module::log_types value to be formatted.
	 * @param ctx The formatting context where the output is written.
	 * @return FormatContext::iterator An iterator pointing to the end of the formatted output.
	 */
	template <typename FormatContext>
	auto format(const log_module::log_types& log_type, FormatContext& ctx) const
	{
		auto str = log_module::to_string(log_type);
		auto wstr = convert_string::to_wstring(str);
		return fmt::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
	}
};

#endif // USE_STD_FORMAT
