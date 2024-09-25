#include "priority_thread_pool.h"

#include "logger.h"

#ifdef USE_STD_FORMAT
#include <format>
#else
#include <fmt/format.h>
#endif

using namespace log_module;

namespace priority_thread_pool_module
{
	template <typename priority_type>
	priority_thread_pool<priority_type>::priority_thread_pool(void)
		: job_queue_(std::make_shared<priority_job_queue<priority_type>>()), start_pool_(false)
	{
	}

	template <typename priority_type> priority_thread_pool<priority_type>::~priority_thread_pool()
	{
		stop();
	}

	template <typename priority_type>
	auto priority_thread_pool<priority_type>::get_ptr(void)
		-> std::shared_ptr<priority_thread_pool<priority_type>>
	{
		return this->shared_from_this();
	}

	template <typename priority_type>
	auto priority_thread_pool<priority_type>::start(void)
		-> std::tuple<bool, std::optional<std::string>>
	{
		if (workers_.empty())
		{
			return { false, "no workers to start" };
		}

		for (auto& worker : workers_)
		{
			auto [started, start_error] = worker->start();
			if (!started)
			{
				stop();
				return { false, start_error };
			}
		}

		start_pool_.store(true);

		return { true, std::nullopt };
	}

	template <typename priority_type>
	auto priority_thread_pool<priority_type>::get_job_queue(void)
		-> std::shared_ptr<priority_job_queue<priority_type>>
	{
		return job_queue_;
	}

	template <typename priority_type>
	auto priority_thread_pool<priority_type>::enqueue(
		std::unique_ptr<priority_job<priority_type>>&& job)
		-> std::tuple<bool, std::optional<std::string>>
	{
		if (job == nullptr)
		{
			return { false, "cannot enqueue null job" };
		}

		if (job_queue_ == nullptr)
		{
			return { false, "cannot enqueue job to null job queue" };
		}

		auto [enqueued, enqueue_error] = job_queue_->enqueue(std::move(job));
		if (!enqueued)
		{
			return { false, enqueue_error };
		}

		return { true, std::nullopt };
	}

	template <typename priority_type>
	auto priority_thread_pool<priority_type>::enqueue(
		std::unique_ptr<priority_thread_worker<priority_type>>&& worker)
		-> std::tuple<bool, std::optional<std::string>>
	{
		if (worker == nullptr)
		{
			return { false, "cannot enqueue null worker" };
		}

		if (job_queue_ == nullptr)
		{
			return { false, "cannot enqueue worker due to null job queue" };
		}

		worker->set_job_queue(job_queue_);

		if (start_pool_.load())
		{
			auto [started, start_error] = worker->start();
			if (!started)
			{
				stop();
				return { false, start_error };
			}
		}

		workers_.emplace_back(std::move(worker));

		return { true, std::nullopt };
	}

	template <typename priority_type>
	auto priority_thread_pool<priority_type>::stop(const bool& immediately_stop) -> void
	{
		if (!start_pool_.load())
		{
			return;
		}

		if (job_queue_ != nullptr)
		{
			job_queue_->stop_waiting_dequeue();

			if (immediately_stop)
			{
				job_queue_->clear();
			}
		}

		for (auto& worker : workers_)
		{
			auto [stopped, stop_error] = worker->stop();
			if (!stopped)
			{
				if (logger::handle().get_file_target() >= log_types::Error
					|| logger::handle().get_console_target() >= log_types::Error)
				{
					logger::handle().write(
						log_types::Error,
#ifdef USE_STD_FORMAT
						std::format
#else
						fmt::format
#endif
						("error stopping worker: {}", stop_error.value_or("unknown error")));
				}
			}
		}

		start_pool_.store(false);
	}
} // namespace priority_thread_pool_module