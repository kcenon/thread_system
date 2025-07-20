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

#include "log_job.h"

#include "../../utilities/core/formatter.h"
#include "../../utilities/time/datetime_tool.h"
#include "../../utilities/conversion/convert_string.h"

#include <codecvt>
#include <iomanip>
#include <sstream>

#include <iostream>

/**
 * @file log_job.cpp
 * @brief Implementation of log job for asynchronous logging system.
 *
 * This file contains the implementation of the log_job class, which processes
 * log messages asynchronously within the thread system. Log jobs handle message
 * formatting, timestamp generation, and performance timing calculations.
 * 
 * Key Features:
 * - Asynchronous log message processing
 * - Support for both narrow and wide string messages
 * - Automatic timestamp generation with microsecond precision
 * - Optional performance timing measurement
 * - Cross-platform string encoding conversion
 * - Structured log message formatting
 * 
 * Message Processing:
 * - Captures timestamp at job creation time
 * - Formats timestamp with date, time, and sub-second precision
 * - Converts wide strings to system-appropriate encoding
 * - Calculates execution timing if start time provided
 * - Produces formatted log messages for output writers
 * 
 * Performance Characteristics:
 * - Minimal overhead during log message capture
 * - Deferred formatting reduces caller blocking time
 * - Efficient string conversion and formatting
 * - Thread-safe message processing
 */

using namespace utility_module;
using namespace thread_module;

namespace log_module
{
	/**
	 * @brief Constructs a log job for narrow string messages.
	 * 
	 * Implementation details:
	 * - Captures current system time as message timestamp
	 * - Stores message and optional performance timing start point
	 * - Initializes message type for proper string handling
	 * - Defers actual formatting until job execution
	 * 
	 * Timing Behavior:
	 * - timestamp_ captures when log was initiated
	 * - start_time_ enables performance measurement if provided
	 * - Timing calculation performed during do_work() execution
	 * 
	 * @param message Log message content as narrow string
	 * @param type Optional log level/type classification
	 * @param start_time Optional start time for performance measurement
	 */
	log_job::log_job(
		const std::string& message,
		std::optional<log_types> type,
		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time)
		: job("log_job")
		, message_type_(message_types::String)
		, message_(message)
		, log_message_("")
		, type_(type)
		, timestamp_(std::chrono::system_clock::now())
		, start_time_(start_time)
	{
	}

	/**
	 * @brief Constructs a log job for wide string messages.
	 * 
	 * Implementation details:
	 * - Similar to narrow string constructor but handles wide characters
	 * - Wide string will be converted to system encoding during processing
	 * - Supports Unicode content on all platforms
	 * - Timestamp capture and timing behavior identical to narrow version
	 * 
	 * Encoding Handling:
	 * - Wide string stored as-is until processing
	 * - Conversion to narrow string occurs during do_work()
	 * - Platform-appropriate encoding conversion applied
	 * 
	 * @param message Log message content as wide string
	 * @param type Optional log level/type classification
	 * @param start_time Optional start time for performance measurement
	 */
	log_job::log_job(
		const std::wstring& message,
		std::optional<log_types> type,
		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time)
		: job("log_job")
		, message_type_(message_types::WString)
		, wmessage_(message)
		, log_message_("")
		, type_(type)
		, timestamp_(std::chrono::system_clock::now())
		, start_time_(start_time)
	{
	}

	auto log_job::get_type() const -> log_types
	{
		if (!type_.has_value())
		{
			return log_types::None;
		}

		return type_.value_or(log_types::None);
	}

	auto log_job::datetime() const -> std::string { return datetime_; }

	auto log_job::message() const -> std::string { return log_message_; }

	/**
	 * @brief Processes the log job by formatting the message and timestamp.
	 * 
	 * Implementation details:
	 * - Formats timestamp with full precision (date, time, milliseconds, microseconds)
	 * - Converts message to appropriate string encoding if needed
	 * - Calculates performance timing if start time was provided
	 * - Produces final formatted log message for output writers
	 * - Handles all formatting exceptions gracefully
	 * 
	 * Message Formatting:
	 * 1. Generate detailed timestamp string with sub-second precision
	 * 2. Convert message content to system-appropriate encoding
	 * 3. Calculate timing information if performance measurement enabled
	 * 4. Format final log message with brackets and timing info
	 * 
	 * Timestamp Format:
	 * - Date and time components separated by space
	 * - Milliseconds and microseconds appended for precision
	 * - Example: \"2024-01-15 14:30:45.123456\"
	 * 
	 * Performance Timing:
	 * - Calculates milliseconds elapsed since start_time_
	 * - Appends timing information to log message
	 * - Format: \"[message] [123 ms]\" when timing enabled
	 * - Format: \"[message]\" when timing disabled
	 * 
	 * Error Handling:
	 * - Catches standard exceptions and converts to error result
	 * - Catches all other exceptions with generic error message
	 * - Ensures job never throws exceptions to caller
	 * 
	 * @return result_void indicating success or detailed error information
	 */
	auto log_job::do_work() -> result_void
	{
		try
		{
			// Format timestamp with full precision for accurate logging
			datetime_ = formatter::format(
				"{} {}.{}{}", datetime_tool::date(timestamp_), datetime_tool::time(timestamp_),
				datetime_tool::milliseconds(timestamp_), datetime_tool::microseconds(timestamp_));
			
			// Convert message to appropriate string encoding
			std::string converted_message = convert_message();

			// Format without timing information if no start time provided
			if (!start_time_.has_value())
			{
				log_message_ = formatter::format("[{}]", converted_message);
				return result_void{};
			}

			// Calculate performance timing in milliseconds
			auto time_gap = datetime_tool::time_difference<std::chrono::milliseconds,
														   std::chrono::high_resolution_clock>(
				start_time_.value());

			// Format with timing information included
			log_message_ = formatter::format("[{}] [{} ms]", converted_message, time_gap);

			return result_void{};
		}
		catch (const std::exception& e)
		{
			return error{error_code::job_execution_failed, e.what()};
		}
		catch (...)
		{
			return error{error_code::job_execution_failed, "unknown error"};
		}
	}

	auto log_job::convert_message() const -> std::string
	{
		switch (message_type_)
		{
		case message_types::String:
		{
			return message_;
		}
		break;
		case message_types::WString:
		{
			auto [converted, convert_error] = convert_string::to_string(wmessage_);
			if (!converted.has_value())
			{
				return std::string();
			}
			return converted.value();
		}
		break;
		default:
			return std::string();
		}
	}
} // namespace log_module