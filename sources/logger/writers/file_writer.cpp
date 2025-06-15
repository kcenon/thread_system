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

#include "file_writer.h"

#include "../../utilities/core/formatter.h"
#include "../jobs/message_job.h"
#include "../../utilities/time/datetime_tool.h"

#include <filesystem>

using namespace utility_module;
using namespace thread_module;

namespace log_module
{
	file_writer::file_writer(void)
		: thread_base("file_writer")
		, title_("log")
		, use_backup_(false)
		, file_target_(log_types::None)
		, max_lines_(0)
		, log_file_(nullptr)
		, backup_file_(nullptr)
		, job_queue_(std::make_shared<job_queue>())
	{
	}

	auto file_writer::set_title(const std::string& title) -> void { title_ = title; }

	auto file_writer::get_title() const -> std::string { return title_; }

	auto file_writer::set_use_backup(const bool& use_backup) -> void { use_backup_ = use_backup; }

	auto file_writer::get_use_backup() const -> bool { return use_backup_; }

	auto file_writer::set_max_lines(const uint32_t& max_lines) -> void { max_lines_ = max_lines; }

	auto file_writer::get_max_lines() const -> uint32_t { return max_lines_; }

	auto file_writer::file_target(const log_types& type) -> void { file_target_ = type; }

	auto file_writer::should_continue_work() const -> bool { return !job_queue_->empty(); }

	auto file_writer::before_start() -> result_void
	{
		if (job_queue_ == nullptr)
		{
			return result_void{error{error_code::resource_allocation_failed, "error creating job_queue"}};
		}

		if (file_target_ == log_types::None)
		{
			return {};
		}

		job_queue_->set_notify(!wake_interval_.has_value());

		check_file_handle();

		return {};
	}

	auto file_writer::do_work() -> result_void
	{
		if (job_queue_ == nullptr)
		{
			return result_void{error{error_code::resource_allocation_failed, "there is no job_queue"}};
		}

		if (file_target_ == log_types::None)
		{
			return {};
		}

		check_file_handle();

		auto remaining_logs = job_queue_->dequeue_batch();
		while (!remaining_logs.empty())
		{
			auto current_job = std::move(remaining_logs.front());
			remaining_logs.pop_front();

			auto current_log
				= std::unique_ptr<message_job>(static_cast<message_job*>(current_job.release()));

			auto work_result = current_log->do_work();
			if (work_result.has_error())
			{
				continue;
			}

			if (current_log->log_type() == log_types::None)
			{
				log_lines_.push_back(formatter::format("[{}]{}", current_log->datetime(),
												   current_log->message(true)));

				continue;
			}

			log_lines_.push_back(formatter::format("[{}][{}] {}", current_log->datetime(),
											   current_log->log_type(),
											   current_log->message(true)));
		}

		if (max_lines_ == 0)
		{
			log_file_ = write_lines(std::move(log_file_), log_lines_);
			log_lines_.clear();

			return {};
		}

		if (log_lines_.size() <= max_lines_)
		{
			log_file_ = write_lines(std::move(log_file_), log_lines_);
			log_file_->close();
			log_file_.reset();

			return {};
		}

		size_t index = log_lines_.size() - max_lines_ + 1;

		if (use_backup_)
		{
			if (backup_file_ == nullptr)
			{
				backup_file_ = std::make_unique<std::fstream>(
					backup_name_, std::ios_base::out | std::ios_base::app);
			}

			std::deque<std::string> backup_lines(log_lines_.begin(), log_lines_.begin() + static_cast<std::deque<std::string>::difference_type>(index));

			if (backup_file_ && backup_file_->is_open())
			{
				backup_file_->seekp(0, std::ios::end);
				backup_file_ = write_lines(std::move(backup_file_), backup_lines);
			}
		}

		log_lines_.erase(log_lines_.begin(), log_lines_.begin() + static_cast<std::deque<std::string>::difference_type>(index));

		log_file_ = write_lines(std::move(log_file_), log_lines_);
		log_file_->close();
		log_file_.reset();

		return {};
	}

	auto file_writer::after_stop() -> result_void
	{
		if (job_queue_ == nullptr)
		{
			return result_void{error{error_code::resource_allocation_failed, "there is no job_queue"}};
		}

		if (file_target_ == log_types::None)
		{
			return {};
		}

		close_file_handle();

		return {};
	}

	auto file_writer::generate_file_name() -> std::tuple<std::string, std::string>
	{
		const auto formatted_date = datetime_tool::date(std::chrono::system_clock::now());

		const auto file_name = formatter::format("{}_{}.log", title_, formatted_date);

		const auto backup_name = formatter::format("{}_{}.backup", title_, formatted_date);

		return { file_name, backup_name };
	}

	auto file_writer::check_file_handle(void) -> void
	{
		auto [file_name, backup_name] = generate_file_name();

		if (file_name_ != file_name)
		{
			close_file_handle();
		}

		if (max_lines_ == 0)
		{
			if (log_file_ == nullptr)
			{
				log_file_ = std::make_unique<std::fstream>(file_name,
												   std::ios_base::out | std::ios_base::app);
			}
		}
		else
		{
			if (log_file_ == nullptr)
			{
				log_file_ = std::make_unique<std::fstream>(file_name, std::ios_base::out
														  | std::ios_base::trunc);
			}

			if (use_backup_)
			{
				if (backup_file_ == nullptr)
				{
					backup_file_ = std::make_unique<std::fstream>(
						backup_name, std::ios_base::out | std::ios_base::app);
				}
			}
		}

		file_name_ = file_name;
		backup_name_ = backup_name;
	}

	auto file_writer::close_file_handle(void) -> void
	{
		if (log_file_ != nullptr)
		{
			log_file_->close();
			log_file_.reset();
			file_name_ = "";
		}

		if (backup_file_ != nullptr)
		{
			backup_file_->close();
			backup_file_.reset();
			backup_name_ = "";
		}
	}

	auto file_writer::write_lines(std::unique_ptr<std::fstream> file_handle,
								  const std::deque<std::string>& messages)
		-> std::unique_ptr<std::fstream>
	{
		if (file_handle == nullptr || !file_handle->is_open())
		{
			return file_handle;
		}

		for (const auto& message : messages)
		{
			file_handle->write(message.c_str(), static_cast<std::streamsize>(message.size()));
		}

		file_handle->flush();

		return file_handle;
	}
}