#include <gtest/gtest.h>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <queue>
#include <future>

#include "thread_pool.h"

using namespace thread_pool_module;

class ThreadPoolTest : public ::testing::Test
{
protected:
	std::shared_ptr<thread_pool> createPool(uint16_t worker_count)
	{
		auto pool = std::make_shared<thread_pool>();
		for (uint16_t i = 0; i < worker_count; ++i)
		{
			pool->enqueue(std::make_unique<thread_worker>());
		}
		return pool;
	}
};

TEST_F(ThreadPoolTest, CreationTest)
{
	auto pool = std::make_shared<thread_pool>();
	ASSERT_NE(pool, nullptr);
}

TEST_F(ThreadPoolTest, WorkerEnqueueTest)
{
	auto pool = std::make_shared<thread_pool>();

	auto result = pool->enqueue(std::make_unique<thread_worker>());
	ASSERT_FALSE(result.has_value());

	for (int i = 0; i < 5; ++i)
	{
		result = pool->enqueue(std::make_unique<thread_worker>());
		ASSERT_FALSE(result.has_value());
	}
}

TEST_F(ThreadPoolTest, JobExecutionTest)
{
	auto pool = createPool(4);
	std::atomic<int> counter{ 0 };
	std::vector<std::future<void>> futures;

	pool->start();

	for (int i = 0; i < 100; i++)
	{
		auto promise = std::make_shared<std::promise<void>>();
		futures.push_back(promise->get_future());

		auto result = pool->enqueue(std::make_unique<job>(
			[&counter, promise](void) -> std::optional<std::string>
			{
				counter++;
				promise->set_value();
				return std::nullopt;
			}));
		ASSERT_FALSE(result.has_value());
	}

	bool all_completed = true;
	for (auto& future : futures)
	{
		auto status = future.wait_for(std::chrono::seconds(1));
		if (status != std::future_status::ready)
		{
			all_completed = false;
			break;
		}
	}

	ASSERT_TRUE(all_completed) << "Not all tasks completed within timeout";
	pool->stop();
	ASSERT_EQ(counter, 100) << "Not all tasks executed successfully";
}

TEST_F(ThreadPoolTest, JobOrderTest)
{
	auto pool = createPool(1);
	std::vector<int> results;
	std::mutex result_mutex;
	std::vector<std::future<void>> futures;

	pool->start();

	for (int i = 0; i < 10; i++)
	{
		auto promise = std::make_shared<std::promise<void>>();
		futures.push_back(promise->get_future());

		pool->enqueue(std::make_unique<job>(
			[i, &results, &result_mutex, promise](void) -> std::optional<std::string>
			{
				{
					std::lock_guard<std::mutex> lock(result_mutex);
					results.push_back(i);
				}
				promise->set_value();
				return std::nullopt;
			}));
	}

	bool all_completed = true;
	for (auto& future : futures)
	{
		auto status = future.wait_for(std::chrono::seconds(1));
		if (status != std::future_status::ready)
		{
			all_completed = false;
			break;
		}
	}

	ASSERT_TRUE(all_completed) << "Not all tasks completed within timeout";
	pool->stop();

	for (size_t i = 0; i < results.size(); i++)
	{
		ASSERT_EQ(results[i], i) << "Tasks were not executed in the correct order";
	}
}

TEST_F(ThreadPoolTest, ConcurrencyTest)
{
	const int WORKER_COUNT = 4;
	auto pool = createPool(WORKER_COUNT);
	std::atomic<int> concurrent_count{ 0 };
	std::atomic<int> max_concurrent{ 0 };
	std::vector<std::future<void>> futures;

	pool->start();

	for (int i = 0; i < 100; i++)
	{
		auto promise = std::make_shared<std::promise<void>>();
		futures.push_back(promise->get_future());

		pool->enqueue(std::make_unique<job>(
			[&concurrent_count, &max_concurrent, promise](void) -> std::optional<std::string>
			{
				int current = ++concurrent_count;
				max_concurrent = std::max(max_concurrent.load(), current);
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				--concurrent_count;
				promise->set_value();
				return std::nullopt;
			}));
	}

	bool all_completed = true;
	for (auto& future : futures)
	{
		auto status = future.wait_for(std::chrono::seconds(1));
		if (status != std::future_status::ready)
		{
			all_completed = false;
			break;
		}
	}

	ASSERT_TRUE(all_completed) << "Not all concurrent tasks completed within timeout";
	pool->stop();

	ASSERT_GT(max_concurrent, 1) << "Tasks were not executed concurrently";
	ASSERT_LE(max_concurrent, WORKER_COUNT) << "More tasks were concurrent than worker count";
}

TEST_F(ThreadPoolTest, ErrorHandlingTest)
{
	auto pool = createPool(4);
	std::atomic<bool> error_occurred{ false };
	std::promise<void> promise;
	auto future = promise.get_future();

	pool->start();

	auto result = pool->enqueue(std::make_unique<job>(
		[&error_occurred, &promise](void) -> std::optional<std::string>
		{
			error_occurred = true;
			promise.set_value();
			return "Intentional error";
		}));
	ASSERT_FALSE(result.has_value());

	auto status = future.wait_for(std::chrono::seconds(1));
	ASSERT_EQ(status, std::future_status::ready)
		<< "Error handling task did not complete within timeout";
	pool->stop();

	ASSERT_TRUE(error_occurred) << "Error was not properly handled";
}

TEST_F(ThreadPoolTest, StopRestartTest)
{
	auto pool = createPool(4);
	std::atomic<int> counter{ 0 };

	pool->start();
	std::vector<std::future<void>> futures;

	// First batch
	for (int i = 0; i < 25; i++)
	{
		auto promise = std::make_shared<std::promise<void>>();
		futures.push_back(promise->get_future());

		pool->enqueue(std::make_unique<job>(
			[&counter, promise](void) -> std::optional<std::string>
			{
				counter++;
				promise->set_value();
				return std::nullopt;
			}));
	}

	bool first_batch_completed = true;
	for (auto& future : futures)
	{
		auto status = future.wait_for(std::chrono::seconds(1));
		if (status != std::future_status::ready)
		{
			first_batch_completed = false;
			break;
		}
	}

	ASSERT_TRUE(first_batch_completed) << "First batch tasks did not complete within timeout";
	int first_batch = counter.load();
	ASSERT_EQ(first_batch, 25) << "First batch did not execute all tasks";

	pool->stop();
	pool.reset();

	// Second batch
	pool = createPool(4);
	pool->start();
	futures.clear();

	for (int i = 0; i < 25; i++)
	{
		auto promise = std::make_shared<std::promise<void>>();
		futures.push_back(promise->get_future());

		pool->enqueue(std::make_unique<job>(
			[&counter, promise](void) -> std::optional<std::string>
			{
				counter++;
				promise->set_value();
				return std::nullopt;
			}));
	}

	bool second_batch_completed = true;
	for (auto& future : futures)
	{
		auto status = future.wait_for(std::chrono::seconds(1));
		if (status != std::future_status::ready)
		{
			second_batch_completed = false;
			break;
		}
	}

	ASSERT_TRUE(second_batch_completed) << "Second batch tasks did not complete within timeout";
	int second_batch = counter.load() - first_batch;
	ASSERT_EQ(second_batch, 25) << "Second batch did not execute all tasks";

	pool->stop();
	ASSERT_EQ(counter, 50) << "Total number of executed tasks is incorrect";
}

TEST_F(ThreadPoolTest, StopBehaviorTest)
{
	auto pool = createPool(4);
	std::atomic<int> counter{ 0 };
	std::vector<std::future<void>> futures;

	pool->start();

	for (int i = 0; i < 50; i++)
	{
		auto promise = std::make_shared<std::promise<void>>();
		futures.push_back(promise->get_future());

		pool->enqueue(std::make_unique<job>(
			[&counter, promise](void) -> std::optional<std::string>
			{
				counter++;
				promise->set_value();
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				return std::nullopt;
			}));
	}

	bool all_completed = true;
	for (auto& future : futures)
	{
		auto status = future.wait_for(std::chrono::seconds(1));
		if (status != std::future_status::ready)
		{
			all_completed = false;
			break;
		}
	}

	ASSERT_TRUE(all_completed) << "Not all tasks completed within timeout";
	ASSERT_EQ(counter, 50) << "Not all tasks were executed before stop";
	pool->stop();
}