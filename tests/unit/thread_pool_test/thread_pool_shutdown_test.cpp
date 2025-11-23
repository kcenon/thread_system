/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, kcenon
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

/**
 * @file thread_pool_shutdown_test.cpp
 * @brief Comprehensive shutdown scenario tests for thread_pool
 *
 * Tests cover:
 * - Graceful shutdown with pending tasks
 * - Shutdown under high load pressure
 * - Immediate shutdown behavior
 * - Queue draining during graceful shutdown
 */

#include "gtest/gtest.h"

#include <kcenon/thread/core/thread_pool.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace kcenon::thread;

class ThreadPoolShutdownTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		pool_ = std::make_shared<thread_pool>();
	}

	void TearDown() override
	{
		if (pool_)
		{
			pool_->stop(true);
		}
	}

	void add_workers(int count)
	{
		for (int i = 0; i < count; ++i)
		{
			auto worker = std::make_unique<thread_worker>();
			auto result = pool_->enqueue(std::move(worker));
			ASSERT_FALSE(result.has_error());
		}
	}

	std::shared_ptr<thread_pool> pool_;
};

// Test graceful shutdown with many pending tasks
TEST_F(ThreadPoolShutdownTest, GracefulShutdownWithPendingTasks)
{
	add_workers(4);

	auto start_result = pool_->start();
	ASSERT_FALSE(start_result.has_error());

	std::atomic<int> completed{0};
	constexpr int task_count = 100;

	// Submit many tasks
	for (int i = 0; i < task_count; ++i)
	{
		auto job = std::make_unique<callback_job>(
			[&completed](void) -> result_void
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				completed++;
				return result_void();
			},
			"pending_task_" + std::to_string(i));
		auto result = pool_->enqueue(std::move(job));
		// Some tasks may be rejected if queue is full, but that's ok
	}

	// Graceful shutdown - should wait for pending tasks
	auto stop_result = pool_->stop(false);
	EXPECT_FALSE(stop_result.has_error());

	// With graceful shutdown, some tasks should have completed
	EXPECT_GT(completed.load(), 0);
}

// Test immediate shutdown clears queue
TEST_F(ThreadPoolShutdownTest, ImmediateShutdownClearsQueue)
{
	add_workers(2);

	auto start_result = pool_->start();
	ASSERT_FALSE(start_result.has_error());

	std::atomic<int> started{0};
	std::atomic<int> completed{0};
	constexpr int task_count = 50;

	// Submit tasks that take some time
	for (int i = 0; i < task_count; ++i)
	{
		auto job = std::make_unique<callback_job>(
			[&started, &completed](void) -> result_void
			{
				started++;
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				completed++;
				return result_void();
			},
			"long_task_" + std::to_string(i));
		pool_->enqueue(std::move(job));
	}

	// Give some time for tasks to start
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	// Immediate shutdown - should not wait for pending tasks
	auto stop_result = pool_->stop(true);
	EXPECT_FALSE(stop_result.has_error());

	// Not all tasks should complete with immediate shutdown
	EXPECT_LT(completed.load(), task_count);
}

// Test shutdown under high concurrent load
TEST_F(ThreadPoolShutdownTest, ShutdownUnderHighLoad)
{
	add_workers(4);

	auto start_result = pool_->start();
	ASSERT_FALSE(start_result.has_error());

	std::atomic<bool> stop_requested{false};
	std::atomic<int> submitted{0};
	std::atomic<int> completed{0};

	// Thread continuously submitting tasks
	std::thread producer([this, &stop_requested, &submitted, &completed]()
	{
		while (!stop_requested.load())
		{
			auto job = std::make_unique<callback_job>(
				[&completed](void) -> result_void
				{
					completed++;
					return result_void();
				},
				"high_load_task");
			auto result = pool_->enqueue(std::move(job));
			if (!result.has_error())
			{
				submitted++;
			}
			std::this_thread::yield();
		}
	});

	// Let it run for a bit
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	// Request stop
	stop_requested.store(true);

	// Graceful shutdown while producer is still trying to submit
	auto stop_result = pool_->stop(false);
	EXPECT_FALSE(stop_result.has_error());

	producer.join();

	// Some tasks should have been processed
	EXPECT_GT(submitted.load(), 0);
}

// Test double start should be handled correctly
TEST_F(ThreadPoolShutdownTest, DoubleStartIsHandled)
{
	add_workers(2);

	auto start_result1 = pool_->start();
	EXPECT_FALSE(start_result1.has_error());

	// Second start should not cause issues
	auto start_result2 = pool_->start();
	// May either succeed or return already started error
	// The important thing is no crash or undefined behavior

	auto stop_result = pool_->stop(false);
	EXPECT_FALSE(stop_result.has_error());
}

// Test restart after stop behavior
TEST_F(ThreadPoolShutdownTest, RestartAfterStopBehavior)
{
	add_workers(2);

	auto start_result = pool_->start();
	EXPECT_FALSE(start_result.has_error());

	auto stop_result = pool_->stop(false);
	EXPECT_FALSE(stop_result.has_error());

	// After stop, start behavior depends on implementation
	// The important thing is it doesn't crash or cause undefined behavior
	auto restart_result = pool_->start();
	// Whether restart succeeds or fails, pool should be in consistent state
	// Verify no crash occurs
}

// Test shutdown with long-running task
TEST_F(ThreadPoolShutdownTest, ShutdownWithLongRunningTask)
{
	add_workers(1);

	auto start_result = pool_->start();
	ASSERT_FALSE(start_result.has_error());

	std::atomic<bool> task_started{false};
	std::atomic<bool> task_completed{false};

	auto job = std::make_unique<callback_job>(
		[&task_started, &task_completed](void) -> result_void
		{
			task_started.store(true);
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			task_completed.store(true);
			return result_void();
		},
		"long_running_task");
	pool_->enqueue(std::move(job));

	// Wait for task to start
	while (!task_started.load())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	// Graceful shutdown should wait for running task
	auto stop_result = pool_->stop(false);
	EXPECT_FALSE(stop_result.has_error());
	EXPECT_TRUE(task_completed.load());
}

// Test graceful then immediate shutdown
TEST_F(ThreadPoolShutdownTest, GracefulThenImmediateShutdown)
{
	add_workers(2);

	auto start_result = pool_->start();
	ASSERT_FALSE(start_result.has_error());

	// First graceful stop
	auto stop1 = pool_->stop(false);
	EXPECT_FALSE(stop1.has_error());

	// Second immediate stop should also succeed (idempotent)
	auto stop2 = pool_->stop(true);
	EXPECT_FALSE(stop2.has_error());
}

// Test shutdown with no tasks submitted
TEST_F(ThreadPoolShutdownTest, ShutdownWithNoTasksSubmitted)
{
	add_workers(4);

	auto start_result = pool_->start();
	ASSERT_FALSE(start_result.has_error());

	// Immediately shutdown without submitting any tasks
	auto stop_result = pool_->stop(false);
	EXPECT_FALSE(stop_result.has_error());
}

// Test rapid start-stop cycles
TEST_F(ThreadPoolShutdownTest, RapidStartStopCycles)
{
	for (int cycle = 0; cycle < 5; ++cycle)
	{
		auto pool = std::make_shared<thread_pool>();

		for (int i = 0; i < 2; ++i)
		{
			auto worker = std::make_unique<thread_worker>();
			auto result = pool->enqueue(std::move(worker));
			ASSERT_FALSE(result.has_error());
		}

		auto start_result = pool->start();
		ASSERT_FALSE(start_result.has_error());

		// Submit a quick task
		auto job = std::make_unique<callback_job>(
			[](void) -> result_void { return result_void(); },
			"quick_task");
		pool->enqueue(std::move(job));

		auto stop_result = pool->stop(false);
		EXPECT_FALSE(stop_result.has_error());
	}
}
