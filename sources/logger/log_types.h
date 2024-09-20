#pragma once

#ifdef USE_STD_FORMAT
#include <format>
#else
#include "fmt/format.h"
#endif

#include <string>
#include <array>
#include <cstdint>

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
	Parameter,	 ///< Parameter log type
	COUNT		 ///< Used to get the number of enum values
};

namespace log_type_utils
{
	constexpr std::array<const char*, static_cast<size_t>(log_types::COUNT)> log_type_strings
		= { "NONE", "EXCEPTION", "ERROR", "INFORMATION", "DEBUG", "SEQUENCE", "PARAMETER" };

	/**
	 * @brief Converts a log_types value to its string representation.
	 * @param log_type The log_types value to convert.
	 * @return std::string A string representation of the log type.
	 */
	inline std::string to_string(log_types log_type)
	{
		if (static_cast<size_t>(log_type) < log_type_strings.size())
		{
			return std::string(log_type_strings[static_cast<size_t>(log_type)]);
		}
		return "UNKNOWN";
	}
} // namespace log_type_utils

#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for log_types.
 *
 * This formatter allows log_types to be used with std::format.
 * It converts the log_types enum values to their string representations.
 */
template <> struct std::formatter<log_types>
{
	constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const log_types& log_type, FormatContext& ctx) const
	{
		return std::format_to(ctx.out(), "{}", log_type_utils::to_string(log_type));
	}
};
#else
/**
 * @brief Specialization of fmt::formatter for log_types.
 *
 * This formatter allows log_types to be used with fmt::format.
 * It converts the log_types enum values to their string representations.
 */
template <> struct fmt::formatter<log_types>
{
	constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const log_types& log_type, FormatContext& ctx) const
	{
		return fmt::format_to(ctx.out(), "{}", log_type_utils::to_string(log_type));
	}
};
#endif