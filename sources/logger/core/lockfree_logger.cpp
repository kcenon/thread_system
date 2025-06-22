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

#include "lockfree_logger.h"
#include "lockfree_log_collector.h"
#include "../writers/console_writer.h"
#include "../writers/file_writer.h"
#include "../writers/callback_writer.h"
#include "../../utilities/core/formatter.h"

namespace log_module
{
	namespace implementation
	{
		// Static member definitions
		std::unique_ptr<lockfree_logger> lockfree_logger::handle_ = nullptr;
		std::once_flag lockfree_logger::once_;

		lockfree_logger::lockfree_logger()
		{
			// Create lock-free log collector
			lockfree_collector_ = std::make_shared<lockfree_log_collector>();
			
			// Create writers
			console_writer_ = std::make_shared<console_writer>();
			file_writer_ = std::make_shared<file_writer>();
			callback_writer_ = std::make_shared<callback_writer>();
		}

		auto lockfree_logger::set_title(const std::string& title) -> void
		{
			if (file_writer_)
			{
				file_writer_->set_title(title);
			}
		}

		auto lockfree_logger::callback_target(const log_types& type) -> void
		{
			if (lockfree_collector_)
			{
				lockfree_collector_->callback_target(type);
			}
		}

		auto lockfree_logger::callback_target(void) const -> log_types
		{
			return lockfree_collector_ ? lockfree_collector_->callback_target() : log_types::None;
		}

		auto lockfree_logger::file_target(const log_types& type) -> void
		{
			if (lockfree_collector_)
			{
				lockfree_collector_->file_target(type);
			}
		}

		auto lockfree_logger::file_target(void) const -> log_types
		{
			return lockfree_collector_ ? lockfree_collector_->file_target() : log_types::None;
		}

		auto lockfree_logger::console_target(const log_types& type) -> void
		{
			if (lockfree_collector_)
			{
				lockfree_collector_->console_target(type);
			}
		}

		auto lockfree_logger::console_target(void) const -> log_types
		{
			return lockfree_collector_ ? lockfree_collector_->console_target() : log_types::None;
		}

		auto lockfree_logger::message_callback(
			const std::function<void(const log_types&, const std::string&, const std::string&)>&
				callback) -> void
		{
			if (callback_writer_)
			{
				callback_writer_->message_callback(callback);
			}
		}

		auto lockfree_logger::set_max_lines(uint32_t max_lines) -> void
		{
			if (file_writer_)
			{
				file_writer_->set_max_lines(max_lines);
			}
		}

		auto lockfree_logger::get_max_lines(void) const -> uint32_t
		{
			return file_writer_ ? file_writer_->get_max_lines() : 0;
		}

		auto lockfree_logger::set_use_backup(bool use_backup) -> void
		{
			if (file_writer_)
			{
				file_writer_->set_use_backup(use_backup);
			}
		}

		auto lockfree_logger::get_use_backup(void) const -> bool
		{
			return file_writer_ ? file_writer_->get_use_backup() : false;
		}

		auto lockfree_logger::set_wake_interval(std::chrono::milliseconds /*interval*/) -> void
		{
			// This can be used to configure the collector's processing interval if needed
		}

		auto lockfree_logger::time_point(void) -> std::chrono::time_point<std::chrono::high_resolution_clock>
		{
			return std::chrono::high_resolution_clock::now();
		}

		auto lockfree_logger::stop(void) -> void
		{
			if (callback_writer_)
			{
				callback_writer_->stop();
			}
			if (file_writer_)
			{
				file_writer_->stop();
			}
			if (console_writer_)
			{
				console_writer_->stop();
			}
			if (lockfree_collector_)
			{
				lockfree_collector_->stop();
			}
		}

		auto lockfree_logger::start(void) -> std::optional<std::string>
		{

			// Set writer queues to the lock-free collector
			lockfree_collector_->set_console_queue(console_writer_->get_job_queue());
			lockfree_collector_->set_file_queue(file_writer_->get_job_queue());
			lockfree_collector_->set_callback_queue(callback_writer_->get_job_queue());

			// Start all components
			if (auto error = lockfree_collector_->start(); error.has_error())
			{
				return formatter::format("Failed to start lock-free log collector: {}", 
										error.get_error().to_string());
			}

			if (auto error = console_writer_->start(); error.has_error())
			{
				lockfree_collector_->stop();
				return formatter::format("Failed to start console writer: {}", 
										error.get_error().to_string());
			}

			if (auto error = file_writer_->start(); error.has_error())
			{
				console_writer_->stop();
				lockfree_collector_->stop();
				return formatter::format("Failed to start file writer: {}", 
										error.get_error().to_string());
			}

			if (auto error = callback_writer_->start(); error.has_error())
			{
				file_writer_->stop();
				console_writer_->stop();
				lockfree_collector_->stop();
				return formatter::format("Failed to start callback writer: {}", 
										error.get_error().to_string());
			}

			return std::nullopt;
		}

		auto lockfree_logger::handle() -> lockfree_logger&
		{
			std::call_once(once_, []() {
				handle_ = std::unique_ptr<lockfree_logger>(new lockfree_logger());
			});
			return *handle_;
		}

		auto lockfree_logger::destroy() -> void
		{
			handle_.reset();
		}

	} // namespace implementation
} // namespace log_module