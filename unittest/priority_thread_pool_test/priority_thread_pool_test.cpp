#include <gtest/gtest.h>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <queue>
#include <future>

#include "priority_thread_pool.h"

using namespace priority_thread_pool_module;

class PriorityThreadPoolTest : public ::testing::Test
{
protected:
	void TearDown() override
	{
		if (pool_)
		{
			pool_->stop();
			pool_.reset();
		}
	}

	std::shared_ptr<priority_thread_pool> createPool(uint16_t high_workers,
													 uint16_t normal_workers,
													 uint16_t low_workers)
	{
		pool_ = std::make_shared<priority_thread_pool>();

		for (uint16_t i = 0; i < high_workers; ++i)
		{
			pool_->enqueue(std::make_unique<priority_thread_worker>(
				std::vector<job_priorities>{ job_priorities::High }));
		}

		for (uint16_t i = 0; i < normal_workers; ++i)
		{
			pool_->enqueue(std::make_unique<priority_thread_worker>(
				std::vector<job_priorities>{ job_priorities::Normal }));
		}

		for (uint16_t i = 0; i < low_workers; ++i)
		{
			pool_->enqueue(std::make_unique<priority_thread_worker>(
				std::vector<job_priorities>{ job_priorities::Low }));
		}

		return pool_;
	}

private:
	std::shared_ptr<priority_thread_pool> pool_;
};

TEST_F(PriorityThreadPoolTest, CreationTest)
{
	auto pool = createPool(1, 1, 1);
	ASSERT_NE(pool, nullptr);
}

TEST_F(PriorityThreadPoolTest, HighPriorityJobTest)
{
	auto pool = createPool(1, 0, 0);
	std::atomic<int> counter{ 0 };
	std::promise<void> promise;
	auto future = promise.get_future();

	pool->start();

	auto result = pool->enqueue(std::make_unique<priority_job>(
		[&counter, &promise](void) -> std::optional<std::string>
		{
			counter++;
			promise.set_value();
			return std::nullopt;
		},
		job_priorities::High));

	ASSERT_FALSE(result.has_value());
	auto status = future.wait_for(std::chrono::seconds(10));
	ASSERT_EQ(status, std::future_status::ready)
		<< "High priority task did not complete within timeout";
	pool->stop();
	ASSERT_EQ(counter, 1) << "High priority task did not execute successfully";
}

TEST_F(PriorityThreadPoolTest, NormalPriorityJobTest)
{
	auto pool = createPool(0, 1, 0);
	std::atomic<int> counter{ 0 };
	std::promise<void> promise;
	auto future = promise.get_future();

	pool->start();

	auto result = pool->enqueue(std::make_unique<priority_job>(
		[&counter, &promise](void) -> std::optional<std::string>
		{
			counter++;
			promise.set_value();
			return std::nullopt;
		},
		job_priorities::Normal));

	ASSERT_FALSE(result.has_value());
	auto status = future.wait_for(std::chrono::seconds(10));
	ASSERT_EQ(status, std::future_status::ready)
		<< "Normal priority task did not complete within timeout";
	pool->stop();
	ASSERT_EQ(counter, 1) << "Normal priority task did not execute successfully";
}

TEST_F(PriorityThreadPoolTest, LowPriorityJobTest)
{
	auto pool = createPool(0, 0, 1);
	std::atomic<int> counter{ 0 };
	std::promise<void> promise;
	auto future = promise.get_future();

	pool->start();

	auto result = pool->enqueue(std::make_unique<priority_job>(
		[&counter, &promise](void) -> std::optional<std::string>
		{
			counter++;
			promise.set_value();
			return std::nullopt;
		},
		job_priorities::Low));

	ASSERT_FALSE(result.has_value());
	auto status = future.wait_for(std::chrono::seconds(10));
	ASSERT_EQ(status, std::future_status::ready)
		<< "Low priority task did not complete within timeout";
	pool->stop();
	ASSERT_EQ(counter, 1) << "Low priority task did not execute successfully";
}

TEST_F(PriorityThreadPoolTest, ErrorHandlingTest)
{
	auto pool = createPool(1, 0, 0);
	std::atomic<bool> error_occurred{ false };
	std::promise<void> promise;
	auto future = promise.get_future();

	pool->start();

	auto result = pool->enqueue(std::make_unique<priority_job>(
		[&error_occurred, &promise](void) -> std::optional<std::string>
		{
			error_occurred = true;
			promise.set_value();
			return "Intentional error";
		},
		job_priorities::High));

	ASSERT_FALSE(result.has_value());
	auto status = future.wait_for(std::chrono::seconds(10));
	ASSERT_EQ(status, std::future_status::ready)
		<< "Error handling task did not complete within timeout";
	pool->stop();
	ASSERT_TRUE(error_occurred) << "Error was not properly handled";
}

TEST_F(PriorityThreadPoolTest, StopRestartTest)
{
	auto pool = createPool(1, 0, 0);
	std::atomic<int> counter{ 0 };

	pool->start();
	std::promise<void> promise1;
	auto future1 = promise1.get_future();

	auto result = pool->enqueue(std::make_unique<priority_job>(
		[&counter, &promise1](void) -> std::optional<std::string>
		{
			counter++;
			promise1.set_value();
			return std::nullopt;
		},
		job_priorities::High));

	ASSERT_FALSE(result.has_value());
	auto status1 = future1.wait_for(std::chrono::seconds(10));
	ASSERT_EQ(status1, std::future_status::ready) << "First task did not complete within timeout";
	pool->stop();
	ASSERT_EQ(counter, 1) << "First task did not execute successfully";

	pool = createPool(1, 0, 0);
	pool->start();

	std::promise<void> promise2;
	auto future2 = promise2.get_future();

	result = pool->enqueue(std::make_unique<priority_job>(
		[&counter, &promise2](void) -> std::optional<std::string>
		{
			counter++;
			promise2.set_value();
			return std::nullopt;
		},
		job_priorities::High));

	ASSERT_FALSE(result.has_value());
	auto status2 = future2.wait_for(std::chrono::seconds(10));
	ASSERT_EQ(status2, std::future_status::ready) << "Second task did not complete within timeout";
	pool->stop();
	ASSERT_EQ(counter, 2) << "Second task did not execute successfully";
}

TEST_F(PriorityThreadPoolTest, StopBehaviorTest)
{
	auto pool = createPool(1, 1, 1);
	std::atomic<int> counter{ 0 };
	std::vector<std::future<void>> futures;

	pool->start();

	for (int i = 0; i < 50; i++)
	{
		auto promise = std::make_shared<std::promise<void>>();
		futures.push_back(promise->get_future());

		auto priority = static_cast<job_priorities>(i % 3);
		pool->enqueue(std::make_unique<priority_job>(
			[&counter, promise](void) -> std::optional<std::string>
			{
				counter++;
				promise->set_value();
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				return std::nullopt;
			},
			priority));
	}

	bool all_completed = true;
	for (auto& future : futures)
	{
		auto status = future.wait_for(std::chrono::seconds(10));
		if (status != std::future_status::ready)
		{
			all_completed = false;
			break;
		}
	}

	ASSERT_TRUE(all_completed) << "Not all tasks completed within timeout";
	ASSERT_EQ(counter, 50) << "Not all tasks were executed before stop";
	pool->stop();

	// Test enqueueing jobs while stopped
	counter.store(0);
	futures.clear();

	for (int i = 0; i < 30; i++)
	{
		auto priority = static_cast<job_priorities>(i % 3);
		pool->enqueue(std::make_unique<priority_job>(
			[&counter](void) -> std::optional<std::string>
			{
				counter++;
				return std::nullopt;
			},
			priority));
	}

	ASSERT_EQ(counter, 0) << "Tasks should not execute when pool is stopped";
}