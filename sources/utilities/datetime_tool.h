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

#include <chrono>
#include <string>

namespace utility_module
{
	/**
	 * @class datetime_tool
	 * @brief A utility class offering date/time formatting and time difference calculations.
	 *
	 * The @c datetime_tool class provides static methods to:
	 * - Format date and time from @c std::chrono::system_clock::time_point into human-readable
	 * strings.
	 * - Extract sub-second units (milliseconds, microseconds, nanoseconds).
	 * - Compute time differences between two time points in various duration units.
	 *
	 * ### Typical Usage
	 * @code
	 * auto now = std::chrono::system_clock::now();
	 * std::string date_str = datetime_tool::date(now);        // e.g. "2025-02-13"
	 * std::string time_str = datetime_tool::time(now);        // e.g. "12:34:56"
	 * std::string ms_str   = datetime_tool::milliseconds(now);// e.g. "123"
	 * auto start = std::chrono::high_resolution_clock::now();
	 * // ... do some work ...
	 * double duration_ms = datetime_tool::time_difference<>(start); // Default = milliseconds
	 * std::cout << "Elapsed ms: " << duration_ms << std::endl;
	 * @endcode
	 */
	class datetime_tool
	{
	public:
		/**
		 * @brief Formats the date portion (YYYY-MM-DD by default) of a time point.
		 * @param time The @c std::chrono::system_clock::time_point to convert.
		 * @param separator_character The separator character placed between year, month, and day
		 *        (default is "-").
		 * @return A string in the format "YYYY<sep>MM<sep>DD".
		 *
		 * This function uses the local time zone for conversion. To handle time zones explicitly,
		 * additional logic may be required.
		 */
		static auto date(const std::chrono::system_clock::time_point& time,
						 const std::string& separator_character = "-") -> std::string;

		/**
		 * @brief Formats the time portion (HH:MM:SS by default) of a time point.
		 * @param time The @c std::chrono::system_clock::time_point to convert.
		 * @param separator_character The separator character placed between hours, minutes, and
		 *        seconds (default is ":").
		 * @return A string in the format "HH<sep>MM<sep>SS".
		 *
		 * Similar to @c date(), this function converts the given time point to local time
		 * before extracting hours, minutes, and seconds.
		 */
		static auto time(const std::chrono::system_clock::time_point& time,
						 const std::string& separator_character = ":") -> std::string;

		/**
		 * @brief Extracts the milliseconds (0-999) component from a time point.
		 * @param time The @c std::chrono::system_clock::time_point from which to extract.
		 * @param milli_portion An additional offset added to the extracted millisecond value
		 *        (default 0). If the final value exceeds 999, it may roll over into the next second
		 *        depending on usage.
		 * @return A zero-padded three-digit string representing milliseconds (e.g., "007", "150").
		 *
		 * Only the sub-second part of the time point is used; the rest is discarded. The result is
		 * formatted to always be 3 digits, enabling easy concatenation into timestamps.
		 */
		static auto milliseconds(const std::chrono::system_clock::time_point& time,
								 const int& milli_portion = 0) -> std::string;

		/**
		 * @brief Extracts the microseconds (0-999) component from a time point.
		 * @param time The @c std::chrono::system_clock::time_point from which to extract.
		 * @param micro_portion An additional offset added to the extracted microsecond value
		 *        (default 0). If the final value exceeds 999, it may roll over into the next
		 *        millisecond or second depending on usage.
		 * @return A zero-padded three-digit string representing microseconds (e.g., "000", "256").
		 *
		 * Typically used when higher precision than milliseconds is desired (e.g., logging).
		 * Formatted to 3 digits, so it represents only the thousandths part of a millisecond.
		 */
		static auto microseconds(const std::chrono::system_clock::time_point& time,
								 const int& micro_portion = 0) -> std::string;

		/**
		 * @brief Extracts the nanoseconds (0-999) component from a time point.
		 * @param time The @c std::chrono::system_clock::time_point from which to extract.
		 * @param nano_portion An additional offset added to the extracted nanosecond value
		 *        (default 0). If the final value exceeds 999, it may roll over into the next
		 *        microsecond or second depending on usage.
		 * @return A zero-padded three-digit string representing nanoseconds (e.g., "000", "999").
		 *
		 * If full nanosecond resolution is required, consider a different approach, as
		 * this method only shows the first 3 digits in the nanosecond field.
		 */
		static auto nanoseconds(const std::chrono::system_clock::time_point& time,
								const int& nano_portion = 0) -> std::string;

		/**
		 * @brief Calculates the time difference between two time points in a specified duration
		 * type.
		 * @tparam DurationType The duration type to compute (e.g. @c std::chrono::milliseconds,
		 *         @c std::chrono::seconds). Defaults to @c std::chrono::milliseconds.
		 * @tparam ClockType The clock type of the time points (e.g. @c
		 * std::chrono::high_resolution_clock). Defaults to @c std::chrono::high_resolution_clock.
		 * @param start_time The initial (start) time point.
		 * @param end_time The final (end) time point. Defaults to @c ClockType::now().
		 * @return A @c double representing the difference in @p DurationType units.
		 *
		 * If you wish to measure performance or elapsed time between two operations, store a
		 * time point before the operation, then call this function afterwards (or at any later
		 * time). The result can be cast to floating-point for fractional values (e.g. 12.34 ms).
		 */
		template <typename DurationType = std::chrono::milliseconds,
				  typename ClockType = std::chrono::high_resolution_clock>
		static auto time_difference(const std::chrono::time_point<ClockType>& start_time,
									const std::chrono::time_point<ClockType>& end_time
									= ClockType::now()) -> double;
	};
} // namespace utility_module