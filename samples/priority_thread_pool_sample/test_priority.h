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
 * @enum test_priority
 * @brief Enumeration of test priority levels.
 *
 * This enum class defines various priority levels for tests.
 * It is based on uint8_t for efficient storage.
 */
enum class test_priority : uint8_t
{
	Top = 0,
	Middle,
	Bottom,
	COUNT ///< Used to get the number of enum values
};

namespace test_priority_utils
{
	constexpr std::array<const char*, static_cast<size_t>(test_priority::COUNT)>
		test_priority_strings = { "Top", "Middle", "Bottom" };

	/**
	 * @brief Converts a test_priority value to its string representation.
	 * @param priority The test_priority value to convert.
	 * @return const char* A string representation of the test priority.
	 */
	inline const char* to_string(test_priority priority)
	{
		if (static_cast<size_t>(priority) < test_priority_strings.size())
		{
			return test_priority_strings[static_cast<size_t>(priority)];
		}
		return "Unknown";
	}
} // namespace test_priority_utils

#ifdef USE_STD_FORMAT
/**
 * @brief Specialization of std::formatter for test_priority.
 *
 * This formatter allows test_priority to be used with std::format.
 * It converts the test_priority enum values to their string representations.
 */
template <> struct std::formatter<test_priority>
{
	constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const test_priority& priority, FormatContext& ctx) const
	{
		return std::format_to(ctx.out(), "{}", test_priority_utils::to_string(priority));
	}
};
#else
/**
 * @brief Specialization of fmt::formatter for test_priority.
 *
 * This formatter allows test_priority to be used with fmt::format.
 * It converts the test_priority enum values to their string representations.
 */
template <> struct fmt::formatter<test_priority>
{
	constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const test_priority& priority, FormatContext& ctx) const
	{
		return format_to(ctx.out(), "{}", test_priority_utils::to_string(priority));
	}
};
#endif