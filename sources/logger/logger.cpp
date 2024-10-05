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

#include "logger.h"

#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace log_module
{
#pragma region singleton
	std::unique_ptr<logger> logger::handle_;
	std::once_flag logger::once_;

	auto logger::handle(void) -> logger&
	{
		call_once(once_, []() { handle_.reset(new logger); });

		return *handle_.get();
	}

	auto logger::destroy() -> void { handle_.reset(); }

#pragma endregion

	logger::logger()
		: collector_(std::make_shared<log_collector>())
		, console_writer_(std::make_shared<console_writer>())
		, file_writer_(std::make_shared<file_writer>())
	{
	}

	auto logger::set_title(const std::string& title) -> void
	{
		if (file_writer_ == nullptr)
		{
			return;
		}

		file_writer_->set_title(title);
	}

	auto logger::set_file_target(const log_types& type) -> void
	{
		if (collector_ == nullptr)
		{
			return;
		}

		collector_->set_file_target(type);
	}

	auto logger::get_file_target() const -> log_types
	{
		if (collector_ == nullptr)
		{
			return log_types::None;
		}

		return collector_->get_file_target();
	}

	auto logger::set_console_target(const log_types& type) -> void
	{
		if (collector_ == nullptr)
		{
			return;
		}

		collector_->set_console_target(type);
	}

	auto logger::get_console_target() const -> log_types
	{
		if (collector_ == nullptr)
		{
			return log_types::None;
		}

		return collector_->get_console_target();
	}

	auto logger::set_max_lines(uint32_t max_lines) -> void
	{
		if (file_writer_ == nullptr)
		{
			return;
		}

		file_writer_->set_max_lines(max_lines);
	}

	auto logger::get_max_lines() const -> uint32_t
	{
		if (file_writer_ == nullptr)
		{
			return 0;
		}

		return file_writer_->get_max_lines();
	}

	auto logger::set_use_backup(bool use_backup) -> void
	{
		if (file_writer_ == nullptr)
		{
			return;
		}

		file_writer_->set_use_backup(use_backup);
	}

	auto logger::get_use_backup() const -> bool
	{
		if (file_writer_ == nullptr)
		{
			return false;
		}

		return file_writer_->get_use_backup();
	}

	auto logger::set_wake_interval(std::chrono::milliseconds interval) -> void
	{
		if (console_writer_ != nullptr)
		{
			console_writer_->set_wake_interval(interval);
		}

		if (file_writer_ != nullptr)
		{
			file_writer_->set_wake_interval(interval);
		}
	}

	auto logger::start() -> std::tuple<bool, std::optional<std::string>>
	{
		if (collector_ == nullptr)
		{
			return { false, "there is no collector" };
		}

		collector_->set_console_queue(console_writer_->get_job_queue());
		collector_->set_file_queue(file_writer_->get_job_queue());

		auto [started, start_error] = collector_->start();
		if (!started)
		{
			return { false, start_error };
		}

		auto [console_started, console_start_error] = console_writer_->start();
		if (!console_started)
		{
			return { false, console_start_error };
		}

		auto [file_started, file_start_error] = file_writer_->start();
		if (!file_started)
		{
			return { false, file_start_error };
		}

		return { true, std::nullopt };
	}

	auto logger::stop() -> void
	{
		if (collector_ != nullptr)
		{
			collector_->stop();
		}

		if (console_writer_ != nullptr)
		{
			console_writer_->stop();
		}

		if (file_writer_ != nullptr)
		{
			file_writer_->stop();
		}
	}

	auto logger::time_point() -> std::chrono::time_point<std::chrono::high_resolution_clock>
	{
		return std::chrono::high_resolution_clock::now();
	}

	auto logger::write(log_types type,
					   const std::string& message,
					   std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>>
						   start_time) -> void
	{
		if (collector_ == nullptr)
		{
			return;
		}

		collector_->write(type, message, start_time);
	}

#ifdef _WIN32_BUT_NOT_TESTED
	auto logger::write(log_types type,
					   const std::wstring& message,
					   std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>>
						   start_time) -> void
	{
		if (collector_ == nullptr)
		{
			return;
		}

		collector_->write(type, message, start_time);
	}
#endif

	auto logger::write(log_types type,
					   const std::u16string& message,
					   std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>>
						   start_time) -> void
	{
		if (collector_ == nullptr)
		{
			return;
		}

		collector_->write(type, message, start_time);
	}

	auto logger::write(log_types type,
					   const std::u32string& message,
					   std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>>
						   start_time) -> void
	{
		if (collector_ == nullptr)
		{
			return;
		}

		collector_->write(type, message, start_time);
	}
} // namespace log_module