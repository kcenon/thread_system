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
	future.wait();
	pool->stop();
	ASSERT_EQ(counter, 1);
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
	future.wait();
	pool->stop();
	ASSERT_EQ(counter, 1);
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
	future.wait();
	pool->stop();
	ASSERT_EQ(counter, 1);
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
	future.wait();
	pool->stop();
	ASSERT_TRUE(error_occurred);
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
	future1.wait();
	pool->stop();
	ASSERT_EQ(counter, 1);

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
	future2.wait();
	pool->stop();
	ASSERT_EQ(counter, 2);
}

TEST_F(PriorityThreadPoolTest, StopBehaviorTest)
{
	auto pool = createPool(1, 1, 1);
	std::atomic<int> counter{ 0 };

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

	ASSERT_EQ(counter, 0);
	pool->stop();
	ASSERT_EQ(counter, 0);

	pool = createPool(1, 1, 1);
	counter.store(0);

	pool->start();
	std::vector<std::future<void>> futures;

	for (int i = 0; i < 15; i++)
	{
		auto promise = std::make_shared<std::promise<void>>();
		futures.push_back(promise->get_future());

		auto priority = static_cast<job_priorities>(i % 3);
		pool->enqueue(std::make_unique<priority_job>(
			[&counter, promise](void) -> std::optional<std::string>
			{
				counter++;
				promise->set_value();
				return std::nullopt;
			},
			priority));
	}

	for (auto& future : futures)
	{
		future.wait();
	}

	pool->stop();
	ASSERT_EQ(counter, 15);
}