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
	 * @brief A utility class for date and time operations.
	 *
	 * This class provides static methods for formatting dates and times,
	 * as well as calculating time differences.
	 */
	class datetime_tool
	{
	public:
		/**
		 * @brief Formats the date part of a time point.
		 * @param time The time point to format.
		 * @param separator_character The character to use as a separator in the date string
		 * (default is "-").
		 * @return A string representing the formatted date.
		 */
		static auto date(const std::chrono::system_clock::time_point& time,
						 const std::string& separator_character = "-") -> std::string;

		/**
		 * @brief Formats the time part of a time point.
		 * @param time The time point to format.
		 * @param separator_character The character to use as a separator in the time string
		 * (default is ":").
		 * @return A string representing the formatted time.
		 */
		static auto time(const std::chrono::system_clock::time_point& time,
						 const std::string& separator_character = ":") -> std::string;

		/**
		 * @brief Extracts the milliseconds part of a time point.
		 * @param time The time point to process.
		 * @return A string representing the milliseconds.
		 */
		static auto milliseconds(const std::chrono::system_clock::time_point& time) -> std::string;

		/**
		 * @brief Extracts the microseconds part of a time point.
		 * @param time The time point to process.
		 * @return A string representing the microseconds.
		 */
		static auto microseconds(const std::chrono::system_clock::time_point& time) -> std::string;

		/**
		 * @brief Extracts the nanoseconds part of a time point.
		 * @param time The time point to process.
		 * @return A string representing the nanoseconds.
		 */
		static auto nanoseconds(const std::chrono::system_clock::time_point& time) -> std::string;

		/**
		 * @brief Calculates the time difference between two time points.
		 * @tparam DurationType The duration type to use for the result (default is
		 * std::chrono::milliseconds).
		 * @tparam ClockType The clock type of the time points (default is
		 * std::chrono::high_resolution_clock).
		 * @param start_time The starting time point.
		 * @param end_time The ending time point (default is the current time).
		 * @return The time difference as a double value in the specified duration type.
		 */
		template <typename DurationType = std::chrono::milliseconds,
				  typename ClockType = std::chrono::high_resolution_clock>
		static auto time_difference(const std::chrono::time_point<ClockType>& start_time,
									const std::chrono::time_point<ClockType>& end_time
									= ClockType::now()) -> double;
	};
} // namespace datetime_module