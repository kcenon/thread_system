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

#ifdef USE_STD_FORMAT
#include <format>
#else
#include <fmt/format.h>
#endif

namespace utility_module
{
	auto datetime_tool::date(const std::chrono::system_clock::time_point& time,
							 const std::string& separator_character) -> std::string
	{
#ifdef USE_STD_CHRONO_CURRENT_ZONE
		const auto local_time = std::chrono::current_zone()->to_local(time);
		const auto ymd
			= std::chrono::year_month_day{ std::chrono::floor<std::chrono::days>(local_time) };

		const auto year = static_cast<int>(ymd.year());
		const auto month = static_cast<unsigned>(ymd.month());
		const auto day = static_cast<unsigned>(ymd.day());
#else
		const auto time_t = std::chrono::system_clock::to_time_t(time);
		const auto* tm = std::localtime(&time_t);

		const auto year = tm->tm_year + 1900;
		const auto month = tm->tm_mon + 1;
		const auto day = tm->tm_mday;
#endif

		return
#ifdef USE_STD_FORMAT
			std::format
#else
			fmt::format
#endif
			("{:04d}{}{:02d}{}{:02d}", year, separator_character, month, separator_character, day);
	}

	auto datetime_tool::time(const std::chrono::system_clock::time_point& time,
							 const std::string& separator_character) -> std::string
	{
#ifdef USE_STD_CHRONO_CURRENT_ZONE
		const auto local_time = std::chrono::current_zone()->to_local(time);
		const auto tod = std::chrono::hh_mm_ss{ std::chrono::floor<std::chrono::seconds>(
			local_time - std::chrono::floor<std::chrono::days>(local_time)) };

		const auto hours = tod.hours().count();
		const auto minutes = tod.minutes().count();
		const auto seconds = tod.seconds().count();
#else
		const auto time_t = std::chrono::system_clock::to_time_t(time);
		const auto* tm = std::localtime(&time_t);

		const auto hours = tm->tm_hour;
		const auto minutes = tm->tm_min;
		const auto seconds = tm->tm_sec;
#endif

		return
#ifdef USE_STD_FORMAT
			std::format
#else
			fmt::format
#endif
			("{:02d}{}{:02d}{}{:02d}", hours, separator_character, minutes, separator_character,
			 seconds);
	}

	auto datetime_tool::milliseconds(const std::chrono::system_clock::time_point& time)
		-> std::string
	{
		const auto duration = time.time_since_epoch();
		const auto milliseconds
			= std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

		return
#ifdef USE_STD_FORMAT
			std::format
#else
			fmt::format
#endif
			("{:03d}", milliseconds);
	}

	auto datetime_tool::microseconds(const std::chrono::system_clock::time_point& time)
		-> std::string
	{
		const auto duration = time.time_since_epoch();
		const auto microseconds
			= std::chrono::duration_cast<std::chrono::microseconds>(duration).count() % 1000;

		return
#ifdef USE_STD_FORMAT
			std::format
#else
			fmt::format
#endif
			("{:03d}", microseconds);
	}

	auto datetime_tool::nanoseconds(const std::chrono::system_clock::time_point& time)
		-> std::string
	{
		const auto duration = time.time_since_epoch();
		const auto nanoseconds
			= std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() % 1000;

		return
#ifdef USE_STD_FORMAT
			std::format
#else
			fmt::format
#endif
			("{:03d}", nanoseconds);
	}

	template <typename DurationType, typename ClockType>
	auto datetime_tool::time_difference(const std::chrono::time_point<ClockType>& start_time,
										const std::chrono::time_point<ClockType>& end_time)
		-> double
	{
		auto duration
			= std::chrono::duration<double, typename DurationType::period>(end_time - start_time);

		return duration.count();
	}

	template auto datetime_tool::time_difference<std::chrono::milliseconds,
												 std::chrono::high_resolution_clock>(
		const std::chrono::time_point<std::chrono::high_resolution_clock>&,
		const std::chrono::time_point<std::chrono::high_resolution_clock>&) -> double;
} // namespace datetime_module