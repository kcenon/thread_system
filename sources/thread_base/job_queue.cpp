#include "job_queue.h"

job_queue::job_queue() : notify_(true), stop_(false), mutex_(), condition_(), queue_() {}

job_queue::~job_queue(void) {}

auto job_queue::is_stopped() const -> bool { return stop_.load(); }

auto job_queue::set_notify(bool notify) -> void { notify_.store(notify); }

auto job_queue::enqueue(std::unique_ptr<job>&& value)
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

	queue_.push(std::move(value));

	if (notify_)
	{
		condition_.notify_one();
	}

	return { true, std::nullopt };
}

std::shared_ptr<job_queue> job_queue::get_ptr(void) { return shared_from_this(); }

auto job_queue::dequeue()
	-> std::tuple<std::optional<std::unique_ptr<job>>, std::optional<std::string>>
{
	std::unique_lock<std::mutex> lock(mutex_);
	condition_.wait(lock, [this]() { return !queue_.empty() || stop_.load(); });

	if (queue_.empty())
	{
		return { std::nullopt, "there are no jobs to dequeue" };
	}

	auto value = std::move(queue_.front());
	queue_.pop();

	return { std::move(value), std::nullopt };
}

auto job_queue::clear(void) -> void
{
	std::scoped_lock<std::mutex> lock(mutex_);

	std::queue<std::unique_ptr<job>> empty;
	std::swap(queue_, empty);

	condition_.notify_all();
}

auto job_queue::empty(void) const -> bool
{
	std::scoped_lock<std::mutex> lock(mutex_);

	return queue_.empty();
}

auto job_queue::stop_waiting_dequeue(void) -> void
{
	std::scoped_lock<std::mutex> lock(mutex_);

	stop_.store(true);

	condition_.notify_all();
}

auto job_queue::dequeue_all(void) -> std::queue<std::unique_ptr<job>>
{
	std::queue<std::unique_ptr<job>> all_items;
	{
		std::scoped_lock<std::mutex> lock(mutex_);

		std::swap(queue_, all_items);

		condition_.notify_all();
	}

	return all_items;
}
