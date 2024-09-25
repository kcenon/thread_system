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
	Bottom
};

namespace detail
{
	constexpr std::array test_priority_strings = { "Top", "Middle", "Bottom" };

	constexpr size_t test_priority_count = test_priority_strings.size();

	// Compile-time check to ensure test_priority_strings and test_priority are in sync
	static_assert(test_priority_count == static_cast<size_t>(test_priority::Bottom) + 1,
				  "test_priority_strings and test_priority enum are out of sync");
}

/**
 * @brief Converts a test_priority value to its string representation.
 * @param priority The test_priority value to convert.
 * @return std::string_view A string representation of the test priority.
 */
constexpr std::string_view to_string(test_priority priority)
{
	auto index = static_cast<size_t>(priority);
	return (index < detail::test_priority_count) ? detail::test_priority_strings[index] : "Unknown";
}

#ifdef USE_STD_FORMAT
namespace std
{
	/**
	 * @brief Specialization of std::formatter for test_priority.
	 *
	 * This formatter allows test_priority to be used with std::format.
	 * It converts the test_priority enum values to their string representations.
	 */
	template <> struct formatter<test_priority>
	{
		constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

		template <typename FormatContext>
		auto format(const test_priority& priority, FormatContext& ctx) const
		{
			return format_to(ctx.out(), "{}", to_string(priority));
		}
	};
}
#else
namespace fmt
{
	/**
	 * @brief Specialization of fmt::formatter for test_priority.
	 *
	 * This formatter allows test_priority to be used with fmt::format.
	 * It converts the test_priority enum values to their string representations.
	 */
	template <> struct formatter<test_priority>
	{
		constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

		template <typename FormatContext>
		auto format(const test_priority& priority, FormatContext& ctx) const
		{
			return format_to(ctx.out(), "{}", to_string(priority));
		}
	};
}
#endif