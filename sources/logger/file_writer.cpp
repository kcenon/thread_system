#include "file_writer.h"

#include "message_job.h"

#ifdef USE_STD_FORMAT
#include <format>
#else
#include <fmt/format.h>
#endif

#include <filesystem>

namespace log_module
{
	file_writer::file_writer(void)
		: job_queue_(std::make_shared<job_queue>())
		, title_("log")
		, use_backup_(false)
		, max_lines_(0)
		, log_file_(nullptr)
		, backup_file_(nullptr)
	{
	}

	auto file_writer::set_title(const std::string& title) -> void { title_ = title; }

	auto file_writer::get_title() const -> std::string { return title_; }

	auto file_writer::set_use_backup(const bool& use_backup) -> void { use_backup_ = use_backup; }

	auto file_writer::get_use_backup() const -> bool { return use_backup_; }

	auto file_writer::set_max_lines(const uint32_t& max_lines) -> void { max_lines_ = max_lines; }

	auto file_writer::get_max_lines() const -> uint32_t { return max_lines_; }

	auto file_writer::has_work() const -> bool { return !job_queue_->empty(); }

	auto file_writer::before_start() -> std::tuple<bool, std::optional<std::string>>
	{
		if (job_queue_ == nullptr)
		{
			return { false, "error creating job_queue" };
		}

		job_queue_->set_notify(!wake_interval_.has_value());

		check_file_handle();

		return { true, std::nullopt };
	}

	auto file_writer::do_work() -> std::tuple<bool, std::optional<std::string>>
	{
		if (job_queue_ == nullptr)
		{
			return { false, "there is no job_queue" };
		}

		check_file_handle();

		auto remaining_logs = job_queue_->dequeue_all();
		while (!remaining_logs.empty())
		{
			auto current_job = std::move(remaining_logs.front());
			remaining_logs.pop();

			auto current_log
				= std::unique_ptr<message_job>(static_cast<message_job*>(current_job.release()));

			auto [worked, work_error] = current_log->do_work();
			if (!worked)
			{
				continue;
			}

			log_lines_.push_back(current_log->message(true));
		}

		if (max_lines_ == 0)
		{
			log_file_ = write_lines(std::move(log_file_), log_lines_);
			log_lines_.clear();

			return { true, std::nullopt };
		}

		if (log_lines_.size() > max_lines_)
		{
			size_t index = log_lines_.size() - max_lines_ + 1;

			if (use_backup_)
			{
				if (backup_file_ == nullptr)
				{
					backup_file_ = std::make_unique<std::fstream>(
						backup_name_, std::ios_base::out | std::ios_base::app);
				}

				std::deque<std::string> backup_lines(log_lines_.begin(),
													 log_lines_.begin() + index);

				if (backup_file_ && backup_file_->is_open())
				{
					backup_file_->seekp(0, std::ios::end);
					backup_file_ = write_lines(std::move(backup_file_), backup_lines);
				}
			}

			log_lines_.erase(log_lines_.begin(), log_lines_.begin() + index);
		}

		log_file_ = write_lines(std::move(log_file_), log_lines_);
		log_file_->close();
		log_file_.reset();

		return { true, std::nullopt };
	}

	auto file_writer::after_stop() -> std::tuple<bool, std::optional<std::string>>
	{
		if (job_queue_ == nullptr)
		{
			return { false, "there is no job_queue" };
		}

		close_file_handle();

		return { true, std::nullopt };
	}

	auto file_writer::generate_file_name() -> std::tuple<std::string, std::string>
	{
		const auto now = std::chrono::system_clock::now();
#ifdef USE_STD_CHRONO_CURRENT_ZONE
		const auto local_time = std::chrono::current_zone()->to_local(now);
		const auto today = std::chrono::floor<std::chrono::days>(local_time);
		const auto year_month_day = std::chrono::year_month_day{ today };

		const auto year = static_cast<int>(year_month_day.year());
		const auto month = static_cast<unsigned>(year_month_day.month());
		const auto day = static_cast<unsigned>(year_month_day.day());

		const auto formatted_date =
#ifdef USE_STD_FORMAT
			std::format
#else
			fmt::format
#endif
			("{:04d}-{:02d}-{:02d}", year, month, day);
#else
		auto in_time_t = std::chrono::system_clock::to_time_t(now);
		auto timeinfo = std::localtime(&in_time_t);

		const auto formatted_date =
#ifdef USE_STD_FORMAT
			std::format
#else
			fmt::format
#endif
			("{:04d}-{:02d}-{:02d}", static_cast<int>(timeinfo->tm_year + 1900),
			 static_cast<unsigned>(timeinfo->tm_mon + 1), static_cast<unsigned>(timeinfo->tm_mday));
#endif

		const auto file_name =
#ifdef USE_STD_FORMAT
			std::format
#else
			fmt::format
#endif
			("{}_{}.log", title_, formatted_date);

		const auto backup_name =
#ifdef USE_STD_FORMAT
			std::format
#else
			fmt::format
#endif
			("{}_{}.backup", title_, formatted_date);

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
			return std::move(file_handle);
		}

		for (const auto& message : messages)
		{
			file_handle->write(message.c_str(), message.size());
		}

		file_handle->flush();

		return std::move(file_handle);
	}
}