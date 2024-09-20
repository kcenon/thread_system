#include "thread_pool.h"

#include "logger.h"

#ifdef USE_STD_FORMAT
#include <format>
#else
#include <fmt/format.h>
#endif

thread_pool::thread_pool(void) : job_queue_(std::make_shared<job_queue>()), start_pool_(false) {}

thread_pool::~thread_pool() { stop(); }

auto thread_pool::get_ptr(void) -> std::shared_ptr<thread_pool> { return this->shared_from_this(); }

auto thread_pool::start(void) -> std::tuple<bool, std::optional<std::string>>
{
	if (workers_.empty())
	{
		return { false, "No workers to start" };
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

auto thread_pool::get_job_queue(void) -> std::shared_ptr<job_queue> { return job_queue_; }

auto thread_pool::enqueue(std::unique_ptr<job>&& job)
	-> std::tuple<bool, std::optional<std::string>>
{
	if (job == nullptr)
	{
		return { false, "Job is null" };
	}

	if (job_queue_ == nullptr)
	{
		return { false, "Job queue is null" };
	}

	auto [enqueued, enqueue_error] = job_queue_->enqueue(std::move(job));
	if (!enqueued)
	{
		return { false, enqueue_error };
	}

	return { true, std::nullopt };
}

auto thread_pool::enqueue(std::unique_ptr<thread_worker>&& worker)
	-> std::tuple<bool, std::optional<std::string>>
{
	if (worker == nullptr)
	{
		return { false, "Worker is null" };
	}

	if (job_queue_ == nullptr)
	{
		return { false, "Job queue is null" };
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

auto thread_pool::stop(const bool& immediately_stop) -> void
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

	start_pool_.store(false);
}