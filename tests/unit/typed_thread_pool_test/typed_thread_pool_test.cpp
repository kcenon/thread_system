// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#include "gtest/gtest.h"

#include <kcenon/thread/core/job_types.h>
#include <kcenon/thread/core/typed_thread_pool.h>
#include <kcenon/thread/core/typed_thread_worker.h>
#include <kcenon/thread/impl/typed_pool/callback_typed_job.h>

using namespace kcenon::thread;

TEST(typed_thread_pool_test, enqueue)
{
	auto pool = std::make_shared<typed_thread_pool>();

	auto worker = std::make_unique<typed_thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.is_err());
}

TEST(typed_thread_pool_test, stop)
{
	auto pool = std::make_shared<typed_thread_pool>();

	auto worker = std::make_unique<typed_thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.is_err());

	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.is_err());
}

TEST(typed_thread_pool_test, stop_immediately)
{
	auto pool = std::make_shared<typed_thread_pool>();

	auto worker = std::make_unique<typed_thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.is_err());

	auto stop_result = pool->stop(true);
	EXPECT_FALSE(stop_result.is_err());
}

TEST(typed_thread_pool_test, stop_no_workers)
{
	auto pool = std::make_shared<typed_thread_pool>();

	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.is_err());
}

TEST(thread_pool_test, start_and_stop)
{
	auto pool = std::make_shared<typed_thread_pool>();

	auto worker = std::make_unique<typed_thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.is_err());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.is_err());

	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.is_err());
}

TEST(typed_thread_pool_test, start_and_stop_no_worker)
{
	auto pool = std::make_shared<typed_thread_pool>();

	auto start_result = pool->start();
	EXPECT_TRUE(start_result.is_err());
	EXPECT_EQ(start_result.error().message, "no workers to start");

	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.is_err());
}

TEST(typed_thread_pool_test, start_and_stop_immediately)
{
	auto pool = std::make_shared<typed_thread_pool>();

	auto worker = std::make_unique<typed_thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.is_err());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.is_err());

	auto stop_result = pool->stop(true);
	EXPECT_FALSE(stop_result.is_err());
}

TEST(typed_thread_pool_test, start_and_stop_immediately_no_worker)
{
	auto pool = std::make_shared<typed_thread_pool>();

	auto start_result = pool->start();
	EXPECT_TRUE(start_result.is_err());
	EXPECT_EQ(start_result.error().message, "no workers to start");

	auto stop_result = pool->stop(true);
	EXPECT_FALSE(stop_result.is_err());
}

TEST(typed_thread_pool_test, start_and_one_sec_job_and_stop)
{
	auto pool = std::make_shared<typed_thread_pool>();

	auto worker = std::make_unique<typed_thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.is_err());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.is_err());

	result = pool->enqueue(std::make_unique<callback_typed_job>(
		[](void) -> kcenon::common::VoidResult
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));

			return kcenon::common::ok();
		},
		job_types::RealTime, "10sec job"));
	EXPECT_FALSE(result.is_err());

	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.is_err());
}

// Test for Problem 4: graceful stop should always call queue->stop()
TEST(typed_thread_pool_test, graceful_stop_prevents_new_jobs)
{
	auto pool = std::make_shared<typed_thread_pool>();

	auto worker = std::make_unique<typed_thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.is_err());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.is_err());

	// Graceful stop (clear_queue = false) should still prevent new jobs
	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.is_err());

	// Try to enqueue a job after graceful stop - should fail
	auto job = std::make_unique<callback_typed_job>(
		[](void) -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		job_types::Batch, "test job");
	result = pool->enqueue(std::move(job));
	EXPECT_TRUE(result.is_err());
	// Note: Error code may vary between implementations, just verify it fails
}

// Test for Problem 4: immediate stop with clear_queue=true
TEST(typed_thread_pool_test, immediate_stop_clears_queue_and_prevents_new_jobs)
{
	auto pool = std::make_shared<typed_thread_pool>();

	auto worker = std::make_unique<typed_thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.is_err());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.is_err());

	// Immediate stop (clear_queue = true)
	auto stop_result = pool->stop(true);
	EXPECT_FALSE(stop_result.is_err());

	// Try to enqueue a job after immediate stop - should fail
	auto job = std::make_unique<callback_typed_job>(
		[](void) -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		job_types::Batch, "test job");
	result = pool->enqueue(std::move(job));
	EXPECT_TRUE(result.is_err());
	// Note: Error code may vary between implementations, just verify it fails
}

// Test for Problem 2: concurrent stop calls on typed_pool
TEST(typed_thread_pool_test, concurrent_stop_calls_are_safe)
{
	auto pool = std::make_shared<typed_thread_pool>();

	auto worker = std::make_unique<typed_thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.is_err());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.is_err());

	// Call stop from multiple threads simultaneously
	std::atomic<int> stop_success_count{0};
	std::vector<std::thread> threads;

	for (int i = 0; i < 5; ++i)
	{
		threads.emplace_back([&pool, &stop_success_count]()
		{
			auto stop_result = pool->stop(false);
			if (!stop_result.is_err())
			{
				stop_success_count++;
			}
		});
	}

	for (auto& thread : threads)
	{
		thread.join();
	}

	// All stop calls should succeed (idempotent)
	EXPECT_EQ(stop_success_count.load(), 5);
}

// Test for Problem 2: multiple sequential stop calls
TEST(typed_thread_pool_test, multiple_stop_calls_are_idempotent)
{
	auto pool = std::make_shared<typed_thread_pool>();

	auto worker = std::make_unique<typed_thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.is_err());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.is_err());

	// Multiple stop calls should all succeed
	auto stop_result1 = pool->stop(false);
	EXPECT_FALSE(stop_result1.is_err());

	auto stop_result2 = pool->stop(true);
	EXPECT_FALSE(stop_result2.is_err());

	auto stop_result3 = pool->stop(false);
	EXPECT_FALSE(stop_result3.is_err());
}
