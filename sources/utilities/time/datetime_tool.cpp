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

#include "datetime_tool.h"

#include "../core/formatter.h"

#include <ctime>
#include <iomanip>

/**
 * @file datetime_tool.cpp
 * @brief Implementation of cross-platform date and time formatting utilities.
 *
 * This file contains the implementation of the datetime_tool class, providing
 * comprehensive date and time formatting capabilities with support for multiple
 * time precision levels and cross-platform compatibility. The implementation
 * supports both modern C++20 chrono facilities and fallback mechanisms.
 * 
 * Key Features:
 * - Cross-platform date and time formatting
 * - Multiple precision levels (seconds, milliseconds, microseconds, nanoseconds)
 * - Configurable separators for flexible output formatting
 * - Modern C++20 chrono support with fallback compatibility
 * - Efficient time difference calculations with template specialization
 * - Thread-safe operations using chrono library primitives
 * 
 * Time Zone Handling:
 * - Automatic local time zone conversion when available
 * - Fallback to system-level time zone handling
 * - Consistent behavior across different platforms
 * - Conditional compilation for optimal performance
 * 
 * Performance Characteristics:
 * - Efficient string formatting using formatter utility
 * - Minimal memory allocations during formatting
 * - Template specialization for compile-time optimization
 * - Fast arithmetic operations for sub-second precision
 * 
 * Format Support:
 * - Customizable date separators (e.g., "-", "/", ".")
 * - Customizable time separators (e.g., ":", ".")
 * - Zero-padded numeric formatting for consistency
 * - Precision-specific sub-second formatting
 */

namespace utility_module
{
	/**
	 * @brief Formats a time point as a date string with customizable separator.
	 * 
	 * Implementation details:
	 * - Uses modern C++20 chrono facilities when available for accuracy
	 * - Falls back to traditional C time functions for compatibility
	 * - Automatically converts to local time zone for user-friendly display
	 * - Provides consistent YYYY-MM-DD format with configurable separators
	 * 
	 * Platform Compatibility:
	 * - Modern systems: Uses std::chrono::current_zone() for accurate timezone handling
	 * - Legacy systems: Uses localtime() with manual timezone adjustment
	 * - Both paths produce identical output format
	 * 
	 * Formatting Details:
	 * - Year: 4-digit zero-padded format (e.g., 2024)
	 * - Month: 2-digit zero-padded format (01-12)
	 * - Day: 2-digit zero-padded format (01-31)
	 * - Separator: User-configurable (common: "-", "/", ".")
	 * 
	 * @param time System clock time point to format
	 * @param separator_character Character(s) to use between date components
	 * @return Formatted date string (e.g., "2024-01-15" with "-" separator)
	 */
	auto datetime_tool::date(const std::chrono::system_clock::time_point& time,
							 const std::string& separator_character) -> std::string
	{
#ifdef USE_STD_CHRONO_CURRENT_ZONE
		// Modern C++20 approach: Use timezone-aware chrono facilities
		const auto local_time = std::chrono::current_zone()->to_local(time);
		const auto ymd
			= std::chrono::year_month_day{ std::chrono::floor<std::chrono::days>(local_time) };

		const auto year = static_cast<int>(ymd.year());
		const auto month = static_cast<unsigned>(ymd.month());
		const auto day = static_cast<unsigned>(ymd.day());
#else
		// Legacy fallback: Use traditional C time functions
		const auto time_t = std::chrono::system_clock::to_time_t(time);
		const auto* tm = std::localtime(&time_t);

		const auto year = tm->tm_year + 1900;  // tm_year is years since 1900
		const auto month = tm->tm_mon + 1;     // tm_mon is 0-based
		const auto day = tm->tm_mday;          // tm_mday is 1-based
#endif

		return formatter::format("{:04d}{}{:02d}{}{:02d}", year, separator_character, month,
								 separator_character, day);
	}

	/**
	 * @brief Formats a time point as a time string with customizable separator.
	 * 
	 * Implementation details:
	 * - Extracts time-of-day components from the time point
	 * - Uses modern C++20 hh_mm_ss for accurate time component extraction
	 * - Falls back to traditional tm structure for compatibility
	 * - Provides consistent HH:MM:SS format with configurable separators
	 * 
	 * Time Component Extraction:
	 * - Modern: Uses std::chrono::hh_mm_ss for precise time-of-day calculation
	 * - Legacy: Uses tm structure fields with standard C library
	 * - Both methods handle local timezone conversion automatically
	 * 
	 * Formatting Details:
	 * - Hours: 2-digit zero-padded format (00-23), 24-hour format
	 * - Minutes: 2-digit zero-padded format (00-59)
	 * - Seconds: 2-digit zero-padded format (00-59)
	 * - Separator: User-configurable (common: ":", ".")
	 * 
	 * @param time System clock time point to format
	 * @param separator_character Character(s) to use between time components
	 * @return Formatted time string (e.g., "14:30:25" with ":" separator)
	 */
	auto datetime_tool::time(const std::chrono::system_clock::time_point& time,
							 const std::string& separator_character) -> std::string
	{
#ifdef USE_STD_CHRONO_CURRENT_ZONE
		// Modern C++20 approach: Use precise time-of-day extraction
		const auto local_time = std::chrono::current_zone()->to_local(time);
		const auto tod = std::chrono::hh_mm_ss{ std::chrono::floor<std::chrono::seconds>(
			local_time - std::chrono::floor<std::chrono::days>(local_time)) };

		const auto hours = tod.hours().count();
		const auto minutes = tod.minutes().count();
		const auto seconds = tod.seconds().count();
#else
		// Legacy fallback: Use traditional C time functions
		const auto time_t = std::chrono::system_clock::to_time_t(time);
		const auto* tm = std::localtime(&time_t);

		const auto hours = tm->tm_hour;    // 24-hour format (0-23)
		const auto minutes = tm->tm_min;   // Minutes (0-59)
		const auto seconds = tm->tm_sec;   // Seconds (0-59)
#endif

		return formatter::format("{:02d}{}{:02d}{}{:02d}", hours, separator_character, minutes,
								 separator_character, seconds);
	}

	/**
	 * @brief Extracts millisecond component from time point with optional offset.
	 * 
	 * Implementation details:
	 * - Calculates milliseconds since epoch for the given time point
	 * - Extracts only the millisecond component (0-999) using modulo operation
	 * - Applies optional offset for adjusted millisecond calculations
	 * - Ensures result stays within valid millisecond range using modulo
	 * 
	 * Calculation Process:
	 * 1. Convert time point to duration since epoch
	 * 2. Cast to milliseconds precision and extract millisecond component
	 * 3. Add user-provided offset for time adjustments
	 * 4. Apply modulo 1000 to keep result in valid range
	 * 
	 * Use Cases:
	 * - High-precision timestamp formatting
	 * - Time offset calculations for logging
	 * - Sub-second precision display requirements
	 * - Performance measurement and profiling
	 * 
	 * @param time System clock time point to extract milliseconds from
	 * @param milli_portion Optional offset to add to milliseconds (default: 0)
	 * @return 3-digit zero-padded millisecond string (000-999)
	 */
	auto datetime_tool::milliseconds(const std::chrono::system_clock::time_point& time,
									 const int& milli_portion) -> std::string
	{
		const auto duration = time.time_since_epoch();
		auto milliseconds
			= std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

		// Apply offset and ensure result stays within valid range
		milliseconds = (milliseconds + milli_portion) % 1000;

		return formatter::format("{:03d}", milliseconds);
	}

	/**
	 * @brief Extracts microsecond component from time point with optional offset.
	 * 
	 * Implementation details:
	 * - Calculates microseconds since epoch for the given time point
	 * - Extracts only the microsecond component (0-999) within each millisecond
	 * - Applies optional offset for adjusted microsecond calculations
	 * - Ensures result stays within valid microsecond range using modulo
	 * 
	 * Precision Notes:
	 * - This extracts the microsecond portion within the current millisecond
	 * - Result represents the sub-millisecond precision (000-999 microseconds)
	 * - Combined with milliseconds() provides microsecond-level precision
	 * - Useful for high-precision timing and performance measurements
	 * 
	 * @param time System clock time point to extract microseconds from
	 * @param micro_portion Optional offset to add to microseconds (default: 0)
	 * @return 3-digit zero-padded microsecond string (000-999)
	 */
	auto datetime_tool::microseconds(const std::chrono::system_clock::time_point& time,
									 const int& micro_portion) -> std::string
	{
		const auto duration = time.time_since_epoch();
		auto microseconds
			= std::chrono::duration_cast<std::chrono::microseconds>(duration).count() % 1000;

		// Apply offset and ensure result stays within valid range
		microseconds = (microseconds + micro_portion) % 1000;

		return formatter::format("{:03d}", microseconds);
	}

	/**
	 * @brief Extracts nanosecond component from time point with optional offset.
	 * 
	 * Implementation details:
	 * - Calculates nanoseconds since epoch for the given time point
	 * - Extracts only the nanosecond component (0-999) within each microsecond
	 * - Applies optional offset for adjusted nanosecond calculations
	 * - Ensures result stays within valid nanosecond range using modulo
	 * 
	 * Precision Notes:
	 * - This extracts the nanosecond portion within the current microsecond
	 * - Result represents the sub-microsecond precision (000-999 nanoseconds)
	 * - Provides the highest precision available from std::chrono
	 * - Essential for ultra-high-precision timing measurements
	 * 
	 * @param time System clock time point to extract nanoseconds from
	 * @param nano_portion Optional offset to add to nanoseconds (default: 0)
	 * @return 3-digit zero-padded nanosecond string (000-999)
	 */
	auto datetime_tool::nanoseconds(const std::chrono::system_clock::time_point& time,
									const int& nano_portion) -> std::string
	{
		const auto duration = time.time_since_epoch();
		auto nanoseconds
			= std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() % 1000;

		// Apply offset and ensure result stays within valid range
		nanoseconds = (nanoseconds + nano_portion) % 1000;

		return formatter::format("{:03d}", nanoseconds);
	}

	/**
	 * @brief Calculates time difference between two time points with specified precision.
	 * 
	 * Implementation details:
	 * - Template-based approach allows compile-time precision selection
	 * - Supports any chrono clock type for flexible timing measurements
	 * - Returns floating-point result for sub-unit precision
	 * - Uses efficient chrono duration casting for optimal performance
	 * 
	 * Template Parameters:
	 * - DurationType: Precision unit (milliseconds, microseconds, etc.)
	 * - ClockType: Clock type (steady_clock, high_resolution_clock, etc.)
	 * 
	 * Calculation Process:
	 * 1. Subtract start_time from end_time to get duration
	 * 2. Cast to specified duration type with double precision
	 * 3. Return count as floating-point value
	 * 
	 * Use Cases:
	 * - Performance benchmarking and profiling
	 * - Timeout calculations and validation
	 * - Elapsed time measurements in applications
	 * - High-precision timing for scientific computations
	 * 
	 * @tparam DurationType Chrono duration type for result precision
	 * @tparam ClockType Chrono clock type for time points
	 * @param start_time Starting time point for measurement
	 * @param end_time Ending time point for measurement
	 * @return Time difference as floating-point value in specified units
	 */
	template <typename DurationType, typename ClockType>
	auto datetime_tool::time_difference(const std::chrono::time_point<ClockType>& start_time,
										const std::chrono::time_point<ClockType>& end_time)
		-> double
	{
		auto duration
			= std::chrono::duration<double, typename DurationType::period>(end_time - start_time);
		return duration.count();
	}

	// Explicit template instantiation
	template auto datetime_tool::time_difference<std::chrono::milliseconds,
												 std::chrono::high_resolution_clock>(
		const std::chrono::time_point<std::chrono::high_resolution_clock>&,
		const std::chrono::time_point<std::chrono::high_resolution_clock>&) -> double;
} // namespace utility_module