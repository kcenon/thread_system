#include "queue_base.h"

queue_base::queue_base() : mutex_(), condition_() {}

queue_base::~queue_base(void) {}

auto queue_base::enqueue(std::unique_ptr<job>&& value)
	-> std::tuple<bool, std::optional<std::string>>
{
	if (value == nullptr)
	{
		return { false, "cannot enqueue null job" };
	}

	std::scoped_lock<std::mutex> lock(mutex_);

	auto [enqueued, enqueue_error] = do_enqueue(std::move(value));
	if (!enqueued)
	{
		return { false, enqueue_error };
	}

	condition_.notify_one();

	return { true, std::nullopt };
}

std::shared_ptr<queue_base> queue_base::get_ptr(void) { return shared_from_this(); }

auto queue_base::dequeue()
	-> std::tuple<std::optional<std::unique_ptr<job>>, std::optional<std::string>>
{
	std::unique_lock<std::mutex> lock(mutex_);
	condition_.wait(lock, [this]() { return !do_empty(); });

	if (do_empty())
	{
		return { std::nullopt, "there are no jobs to dequeue" };
	}

	auto [value, pop_error] = do_dequeue();
	if (!value.has_value())
	{
		return { std::nullopt, pop_error };
	}

	return { std::move(value.value()), std::nullopt };
}

auto queue_base::clear(void) -> void
{
	std::scoped_lock<std::mutex> lock(mutex_);

	do_clear();

	condition_.notify_all();
}

auto queue_base::empty(void) const -> bool
{
	std::scoped_lock<std::mutex> lock(mutex_);

	return do_empty();
}

auto queue_base::dequeue_all(void) -> std::queue<std::unique_ptr<job>>
{
	std::queue<std::unique_ptr<job>> all_items;
	{
		std::scoped_lock<std::mutex> lock(mutex_);

		all_items = do_dequeue_all();

		condition_.notify_all();
	}

	return all_items;
}
