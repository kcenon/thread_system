#pragma once

#ifdef USE_STD_FORMAT
#include <format>
#else
#include "fmt/format.h"
#endif

#include <string>
#include <array>
#include <cstdint>

namespace priority_thread_pool_module
{
	/**
	 * @enum job_priorities
	 * @brief Enumeration of job priority levels.
	 *
	 * This enum class defines various priority levels for jobs.
	 * It is based on uint8_t for efficient storage.
	 */
	enum class job_priorities : uint8_t
	{
		High = 0, ///< High priority job
		Normal,	  ///< Normal priority job
		Low,	  ///< Low priority job
		COUNT	  ///< Used to get the number of enum values
	};

	namespace job_priority_utils
	{
		constexpr std::array<const char*, static_cast<size_t>(job_priorities::COUNT)>
			job_priority_strings = { "High", "Normal", "Low" };

		/**
		 * @brief Converts a job_priorities value to its string representation.
		 * @param job_priority The job_priorities value to convert.
		 * @return std::string A string representation of the job priority.
		 */
		inline std::string to_string(job_priorities job_priority)
		{
			if (static_cast<size_t>(job_priority) < job_priority_strings.size())
			{
				return std::string(job_priority_strings[static_cast<size_t>(job_priority)]);
			}
			return "Unknown";
		}
	} // namespace job_priority_utils

#ifdef USE_STD_FORMAT
	/**
	 * @brief Specialization of std::formatter for job_priorities.
	 *
	 * This formatter allows job_priorities to be used with std::format.
	 * It converts the job_priorities enum values to their string representations.
	 */
	template <> struct std::formatter<job_priorities>
	{
		constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

		template <typename FormatContext>
		auto format(const job_priorities& job_priority, FormatContext& ctx) const
		{
			return std::format_to(ctx.out(), "{}", job_priority_utils::to_string(job_priority));
		}
	};
#else
	/**
	 * @brief Specialization of fmt::formatter for job_priorities.
	 *
	 * This formatter allows job_priorities to be used with fmt::format.
	 * It converts the job_priorities enum values to their string representations.
	 */
	template <> struct fmt::formatter<job_priorities>
	{
		constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

		template <typename FormatContext>
		auto format(const job_priorities& job_priority, FormatContext& ctx) const
		{
			return fmt::format_to(ctx.out(), "{}", job_priority_utils::to_string(job_priority));
		}
	};
#endif
} // namespace priority_thread_pool_module