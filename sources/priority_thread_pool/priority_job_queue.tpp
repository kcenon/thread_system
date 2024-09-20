#include "priority_job_queue.h"

#include <format>

template <typename priority_type>
priority_job_queue<priority_type>::priority_job_queue(void) : job_queue(), queues_()
{
}

template <typename priority_type> priority_job_queue<priority_type>::~priority_job_queue(void) {}

template <typename priority_type>
auto priority_job_queue<priority_type>::enqueue(std::unique_ptr<job>&& value)
	-> std::tuple<bool, std::optional<std::string>>
{
	if (stop_.load())
	{
		return { false, "Job queue is stopped" };
	}

	auto priority_job_ptr = dynamic_cast<priority_job<priority_type>*>(value.get());

	if (!priority_job_ptr)
	{
		return { false, "Enqueued job is not a priority_job" };
	}

	auto job_priority = priority_job_ptr->priority();

	std::unique_lock<std::mutex> lock(mutex_);

	auto iter = queues_.find(job_priority);
	if (iter != queues_.end())
	{
		iter->second.push(std::unique_ptr<priority_job<priority_type>>(
			static_cast<priority_job<priority_type>*>(value.release())));

		if (notify_)
		{
			condition_.notify_one();
		}

		return { true, std::nullopt };
	}

	iter = queues_.emplace(job_priority, std::queue<std::unique_ptr<priority_job<priority_type>>>())
			   .first;
	iter->second.push(std::unique_ptr<priority_job<priority_type>>(
		static_cast<priority_job<priority_type>*>(value.release())));

	if (notify_)
	{
		condition_.notify_one();
	}

	return { true, std::nullopt };
}

template <typename priority_type>
auto priority_job_queue<priority_type>::enqueue(
	std::unique_ptr<priority_job<priority_type>>&& value)
	-> std::tuple<bool, std::optional<std::string>>
{
	if (stop_.load())
	{
		return { false, "Job queue is stopped" };
	}

	if (value == nullptr)
	{
		return { false, "cannot enqueue null job" };
	}

	std::scoped_lock<std::mutex> lock(mutex_);

	auto iter = queues_.find(value->priority());
	if (iter != queues_.end())
	{
		iter->second.push(std::move(value));

		condition_.notify_one();

		return { true, std::nullopt };
	}

	iter = queues_
			   .emplace(value->priority(),
						std::queue<std::unique_ptr<priority_job<priority_type>>>())
			   .first;
	iter->second.push(std::move(value));

	condition_.notify_one();

	return { true, std::nullopt };
}

template <typename priority_type>
auto priority_job_queue<priority_type>::dequeue()
	-> std::tuple<std::optional<std::unique_ptr<job>>, std::optional<std::string>>
{
	return {
		std::nullopt,
		"Dequeue operation without specified priorities is not supported in priority_job_queue"
	};
}

template <typename priority_type>
auto priority_job_queue<priority_type>::dequeue(const std::vector<priority_type>& priorities)
	-> std::tuple<std::optional<std::unique_ptr<priority_job<priority_type>>>,
				  std::optional<std::string>>
{
	std::unique_lock<std::mutex> lock(mutex_);

	auto dequeue_job
		= [this, &priorities]() -> std::optional<std::unique_ptr<priority_job<priority_type>>>
	{
		for (const auto& priority : priorities)
		{
			if (auto value = std::move(try_dequeue_from_priority(priority)))
			{
				return value;
			}
		}
		return std::nullopt;
	};

	std::optional<std::unique_ptr<priority_job<priority_type>>> result;
	condition_.wait(lock,
					[&]()
					{
						result = dequeue_job();
						return result.has_value() || stop_.load();
					});

	if (result.has_value())
	{
		auto job = std::move(result.value());
		lock.unlock();
		return { std::move(job), std::nullopt };
	}

	lock.unlock();

	if (stop_.load())
	{
		return { std::nullopt, "Job queue is stopped" };
	}

	return { std::nullopt, "Unexpected error: No job found after waiting" };
}

template <typename priority_type> auto priority_job_queue<priority_type>::clear() -> void
{
	std::scoped_lock<std::mutex> lock(mutex_);

	for (auto& pair : queues_)
	{
		std::queue<std::unique_ptr<priority_job<priority_type>>> empty;
		std::swap(pair.second, empty);
	}

	queues_.clear();

	condition_.notify_all();
}

template <typename priority_type>
auto priority_job_queue<priority_type>::empty(const std::vector<priority_type>& priorities) const
	-> bool
{
	std::scoped_lock<std::mutex> lock(mutex_);

	return empty_check_without_lock(priorities);
}

template <typename priority_type>
auto priority_job_queue<priority_type>::empty_check_without_lock(
	const std::vector<priority_type>& priorities) const -> bool
{
	for (const auto& priority : priorities)
	{
		auto it = queues_.find(priority);
		if (it != queues_.end() && !it->second.empty())
		{
			return false;
		}
	}

	return true;
}

template <typename priority_type>
auto priority_job_queue<priority_type>::try_dequeue_from_priority(const priority_type& priority)
	-> std::optional<std::unique_ptr<priority_job<priority_type>>>
{
	auto it = queues_.find(priority);
	if (it == queues_.end() || it->second.empty())
	{
		return std::nullopt;
	}

	auto value = std::move(it->second.front());
	it->second.pop();
	return value;
}
