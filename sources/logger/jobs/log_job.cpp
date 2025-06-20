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

using namespace utility_module;
using namespace thread_module;

namespace log_module
{
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

	auto log_job::do_work() -> result_void
	{
		try
		{
			datetime_ = formatter::format(
				"{} {}.{}{}", datetime_tool::date(timestamp_), datetime_tool::time(timestamp_),
				datetime_tool::milliseconds(timestamp_), datetime_tool::microseconds(timestamp_));
			std::string converted_message = convert_message();

			if (!start_time_.has_value())
			{
				log_message_ = formatter::format("[{}]", converted_message);
				return result_void{};
			}

			auto time_gap = datetime_tool::time_difference<std::chrono::milliseconds,
														   std::chrono::high_resolution_clock>(
				start_time_.value());

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