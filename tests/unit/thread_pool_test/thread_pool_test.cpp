/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
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

#include "gtest/gtest.h"

#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/job_queue.h>

using namespace kcenon::thread;

// Static assertions for ARM64 compatibility (Issue #223)
// These compile-time checks verify memory alignment requirements
// that are critical for ARM64 architectures which have stricter
// alignment requirements than x86/x64.

// Verify thread_worker alignment for ARM64 compatibility
static_assert(alignof(thread_worker) >= alignof(void*),
	"thread_worker must be at least pointer-aligned for ARM64");

// Verify job_queue alignment for ARM64 compatibility
static_assert(alignof(job_queue) >= alignof(void*),
	"job_queue must be at least pointer-aligned for ARM64");

// Verify thread_pool alignment for ARM64 compatibility
static_assert(alignof(thread_pool) >= alignof(void*),
	"thread_pool must be at least pointer-aligned for ARM64");

// Verify atomic types meet alignment requirements
static_assert(alignof(std::atomic<bool>) >= 1,
	"std::atomic<bool> alignment must be at least 1");
static_assert(alignof(std::atomic<size_t>) >= alignof(size_t),
	"std::atomic<size_t> alignment must be at least size_t alignment");

TEST(thread_pool_test, enqueue)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.is_err());
}

TEST(thread_pool_test, stop)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.is_err());

	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.is_err());
}

TEST(thread_pool_test, stop_immediately)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.is_err());

	auto stop_result = pool->stop(true);
	EXPECT_FALSE(stop_result.is_err());
}

TEST(thread_pool_test, stop_no_workers)
{
	auto pool = std::make_shared<thread_pool>();

	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.is_err());
}

TEST(thread_pool_test, start_and_stop)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.is_err());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.is_err());

	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.is_err());
}

TEST(thread_pool_test, start_and_stop_no_worker)
{
	auto pool = std::make_shared<thread_pool>();

	auto start_result = pool->start();
	EXPECT_TRUE(start_result.is_err());
	EXPECT_EQ(start_result.error().message, "no workers to start");

	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.is_err());
}

TEST(thread_pool_test, start_and_stop_immediately)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.is_err());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.is_err());

	auto stop_result = pool->stop(true);
	EXPECT_FALSE(stop_result.is_err());
}

TEST(thread_pool_test, start_and_stop_immediately_no_worker)
{
	auto pool = std::make_shared<thread_pool>();

	auto start_result = pool->start();
	EXPECT_TRUE(start_result.is_err());
	EXPECT_EQ(start_result.error().message, "no workers to start");

	auto stop_result = pool->stop(true);
	EXPECT_FALSE(stop_result.is_err());
}

TEST(thread_pool_test, start_and_one_sec_job_and_stop)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.is_err());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.is_err());

	result = pool->enqueue(std::make_unique<callback_job>(
		[](void) -> kcenon::common::VoidResult
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));

			return kcenon::common::ok();
		},
		"10sec job"));
	EXPECT_FALSE(result.is_err());

	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.is_err());
}

// Test for Problem 1: enqueue after stop should fail
TEST(thread_pool_test, enqueue_after_stop_should_fail)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.is_err());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.is_err());

	// Stop the pool
	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.is_err());

	// Try to enqueue a job after stop - should fail
	auto job = std::make_unique<callback_job>(
		[](void) -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test job");
	result = pool->enqueue(std::move(job));
	EXPECT_TRUE(result.is_err());
	EXPECT_EQ(result.error().code, static_cast<int>(error_code::queue_stopped));
}

// Test for Problem 1: enqueue_batch after stop should fail
TEST(thread_pool_test, enqueue_batch_after_stop_should_fail)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.is_err());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.is_err());

	// Stop the pool
	auto stop_result = pool->stop(true);
	EXPECT_FALSE(stop_result.is_err());

	// Try to enqueue batch of jobs after stop - should fail
	std::vector<std::unique_ptr<job>> jobs;
	jobs.push_back(std::make_unique<callback_job>(
		[](void) -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test job 1"));
	jobs.push_back(std::make_unique<callback_job>(
		[](void) -> kcenon::common::VoidResult { return kcenon::common::ok(); },
		"test job 2"));

	result = pool->enqueue_batch(std::move(jobs));
	EXPECT_TRUE(result.is_err());
	EXPECT_EQ(result.error().code, static_cast<int>(error_code::queue_stopped));
}

// Test for Problem 2: concurrent stop calls should be safe
TEST(thread_pool_test, concurrent_stop_calls_should_be_safe)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
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
			// Only one thread should successfully execute stop logic
			// Others should return immediately without error
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

// Test for Problem 2: stop can be called multiple times safely
TEST(thread_pool_test, multiple_stop_calls_are_idempotent)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.is_err());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.is_err());

	// First stop should succeed
	auto stop_result1 = pool->stop(false);
	EXPECT_FALSE(stop_result1.is_err());

	// Second stop should also succeed (idempotent)
	auto stop_result2 = pool->stop(false);
	EXPECT_FALSE(stop_result2.is_err());

	// Third stop with immediate flag should also succeed
	auto stop_result3 = pool->stop(true);
	EXPECT_FALSE(stop_result3.is_err());
}

// Test for Issue #223: Manual worker creation and batch enqueue on ARM64
// This pattern was reported to crash with SIGILL/SIGSEGV on macOS ARM64
TEST(thread_pool_test, manual_worker_batch_enqueue_arm64)
{
	// Create thread pool
	auto pool = std::make_shared<thread_pool>("test_pool_arm64");

	// Manual worker creation (this pattern was crashing on ARM64)
	thread_context context;
	std::vector<std::unique_ptr<thread_worker>> workers;

	for (size_t i = 0; i < 4; ++i)
	{
		workers.push_back(std::make_unique<thread_worker>(false, context));
	}

	// Enqueue workers to pool - should not crash
	auto result = pool->enqueue_batch(std::move(workers));
	EXPECT_FALSE(result.is_err());

	// Start pool - should not crash
	auto start_result = pool->start();
	EXPECT_FALSE(start_result.is_err());

	// Submit job - should not crash
	std::atomic<int> counter{0};
	bool submit_result = pool->submit_task([&counter]()
	{
		counter++;
	});
	EXPECT_TRUE(submit_result);

	// Allow job to complete
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	// Stop pool
	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.is_err());

	// Verify job was executed
	EXPECT_EQ(counter.load(), 1);
}

// Test for Issue #223: Multiple manual workers with concurrent job submission
TEST(thread_pool_test, manual_workers_concurrent_job_submission_arm64)
{
	auto pool = std::make_shared<thread_pool>("test_pool_concurrent");

	// Create multiple workers manually
	thread_context context;
	std::vector<std::unique_ptr<thread_worker>> workers;
	const size_t worker_count = 8;

	for (size_t i = 0; i < worker_count; ++i)
	{
		workers.push_back(std::make_unique<thread_worker>(true, context));
	}

	auto result = pool->enqueue_batch(std::move(workers));
	EXPECT_FALSE(result.is_err());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.is_err());

	// Submit multiple jobs concurrently
	std::atomic<int> counter{0};
	const int job_count = 100;

	for (int i = 0; i < job_count; ++i)
	{
		bool submit_result = pool->submit_task([&counter]()
		{
			counter++;
		});
		EXPECT_TRUE(submit_result);
	}

	// Wait for all jobs to complete
	int timeout_ms = 5000;
	while (counter.load() < job_count && timeout_ms > 0)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		timeout_ms -= 10;
	}

	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.is_err());

	EXPECT_EQ(counter.load(), job_count);
}

// Test for Issue #223: Worker enqueue one by one vs batch
TEST(thread_pool_test, manual_workers_individual_vs_batch_arm64)
{
	// Test individual enqueue
	{
		auto pool = std::make_shared<thread_pool>("test_individual");
		thread_context context;

		for (size_t i = 0; i < 4; ++i)
		{
			auto worker = std::make_unique<thread_worker>(false, context);
			auto result = pool->enqueue(std::move(worker));
			EXPECT_FALSE(result.is_err());
		}

		auto start_result = pool->start();
		EXPECT_FALSE(start_result.is_err());

		std::atomic<int> counter{0};
		pool->submit_task([&counter]() { counter++; });

		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		pool->stop(false);
		EXPECT_GE(counter.load(), 1);
	}

	// Test batch enqueue
	{
		auto pool = std::make_shared<thread_pool>("test_batch");
		thread_context context;
		std::vector<std::unique_ptr<thread_worker>> workers;

		for (size_t i = 0; i < 4; ++i)
		{
			workers.push_back(std::make_unique<thread_worker>(false, context));
		}

		auto result = pool->enqueue_batch(std::move(workers));
		EXPECT_FALSE(result.is_err());

		auto start_result = pool->start();
		EXPECT_FALSE(start_result.is_err());

		std::atomic<int> counter{0};
		pool->submit_task([&counter]() { counter++; });

		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		pool->stop(false);
		EXPECT_GE(counter.load(), 1);
	}
}
