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

#ifdef USE_STD_FORMAT
#include <format>
#else
#include "fmt/chrono.h"
#include "fmt/format.h"
#endif

#include <codecvt>
#include <iomanip>
#include <sstream>

namespace log_module
{
	log_job::log_job(
		const std::string& message,
		std::optional<log_types> type,
		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time)
		: job(nullptr, "log_job")
		, type_(type)
		, message_type_(message_types::String)
		, message_(message)
		, timestamp_(std::chrono::system_clock::now())
		, start_time_(start_time)
		, log_message_("")
	{
	}
	log_job::log_job(
		const std::wstring& message,
		std::optional<log_types> type,
		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time)
		: job(nullptr, "log_job")
		, type_(type)
		, message_type_(message_types::WString)
		, wmessage_(message)
		, timestamp_(std::chrono::system_clock::now())
		, start_time_(start_time)
		, log_message_("")
	{
	}
	log_job::log_job(
		const std::u16string& message,
		std::optional<log_types> type,
		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time)
		: job(nullptr, "log_job")
		, type_(type)
		, message_type_(message_types::U16String)
		, u16message_(message)
		, timestamp_(std::chrono::system_clock::now())
		, start_time_(start_time)
		, log_message_("")
	{
	}
	log_job::log_job(
		const std::u32string& message,
		std::optional<log_types> type,
		std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time)
		: job(nullptr, "log_job")
		, type_(type)
		, message_type_(message_types::U32String)
		, u32message_(message)
		, timestamp_(std::chrono::system_clock::now())
		, start_time_(start_time)
		, log_message_("")
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

	auto log_job::message() const -> std::string { return log_message_; }

	auto log_job::do_work() -> std::tuple<bool, std::optional<std::string>>
	{
		try
		{
			auto in_time_t = std::chrono::system_clock::to_time_t(timestamp_);
			auto timeinfo = std::localtime(&in_time_t);
			auto duration = timestamp_.time_since_epoch();
			auto milliseconds
				= std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;
			auto microseconds
				= std::chrono::duration_cast<std::chrono::microseconds>(duration).count() % 1000;

			std::ostringstream time_stream;
			time_stream << std::put_time(timeinfo, "%Y-%m-%d %H:%M:%S") << '.' << std::setw(3)
						<< std::setfill('0') << milliseconds << std::setw(3) << std::setfill('0')
						<< microseconds;

			std::string formatted_time = time_stream.str();

			if (start_time_.has_value())
			{
				std::chrono::duration<double, std::milli> elapsed
					= std::chrono::high_resolution_clock::now() - start_time_.value();

				if (!type_.has_value())
				{
					log_message_ =
#ifdef USE_STD_FORMAT
						std::format
#else
						fmt::format
#endif
						("[{}][{}] [{} ms]", formatted_time, convert_message(), elapsed.count());
				}
				else
				{
					log_message_ =
#ifdef USE_STD_FORMAT
						std::format
#else
						fmt::format
#endif
						("[{}][{}]: {} [{} ms]", formatted_time, type_.value(), convert_message(),
						 elapsed.count());
				}
			}
			else
			{
				if (!type_.has_value())
				{
					log_message_ =
#ifdef USE_STD_FORMAT
						std::format
#else
						fmt::format
#endif
						("[{}][{}]", formatted_time, convert_message());
				}
				else
				{
					log_message_ =
#ifdef USE_STD_FORMAT
						std::format
#else
						fmt::format
#endif
						("[{}][{}]: {}", formatted_time, type_.value(), convert_message());
				}
			}

			return { true, std::nullopt };
		}
		catch (const std::exception& e)
		{
			return { false, std::string(e.what()) };
		}
		catch (...)
		{
			return { false, "unknown error" };
		}
	}

	auto log_job::convert_message() const -> std::string
	{
		switch (message_type_)
		{
		case message_types::String:
			return message_;
		case message_types::WString:
			return to_string(wmessage_);
		case message_types::U16String:
			return to_string(u16message_);
		case message_types::U32String:
			return to_string(u32message_);
		default:
			return "";
		}
	}

	auto log_job::to_string(const std::wstring& message) const -> std::string
	{
		if (message.empty())
		{
			return std::string();
		}

		typedef std::codecvt<wchar_t, char, mbstate_t> codecvt_t;
		codecvt_t const& codecvt = std::use_facet<codecvt_t>(std::locale());

		mbstate_t state = mbstate_t();

		std::vector<char> result((message.size() + 1) * codecvt.max_length());
		wchar_t const* in_text = message.data();
		char* out_text = &result[0];

		codecvt_t::result condition
			= codecvt.out(state, message.data(), message.data() + message.size(), in_text,
						  &result[0], &result[0] + result.size(), out_text);

		return std::string(&result[0], out_text);
	}

	auto log_job::to_string(const std::u16string& message) const -> std::string
	{
		if (message.empty())
		{
			return std::string();
		}

		typedef std::codecvt<char16_t, char, mbstate_t> codecvt_t;
		codecvt_t const& codecvt = std::use_facet<codecvt_t>(std::locale());

		mbstate_t state = mbstate_t();

		std::vector<char> result((message.size() + 1) * codecvt.max_length());
		char16_t const* in_text = message.data();
		char* out_text = &result[0];

		codecvt_t::result condition = codecvt.out(state, in_text, in_text + message.size(), in_text,
												  out_text, out_text + result.size(), out_text);

		return std::string(result.data());
	}

	auto log_job::to_string(const std::u32string& message) const -> std::string
	{
		if (message.empty())
		{
			return std::string();
		}

		typedef std::codecvt<char32_t, char, mbstate_t> codecvt_t;
		codecvt_t const& codecvt = std::use_facet<codecvt_t>(std::locale());

		mbstate_t state = mbstate_t();

		std::vector<char> result((message.size() + 1) * codecvt.max_length());
		char32_t const* in_text = message.data();
		char* out_text = &result[0];

		codecvt_t::result condition = codecvt.out(state, in_text, in_text + message.size(), in_text,
												  out_text, out_text + message.size(), out_text);

		return std::string(result.data());
	}
} // namespace log_module