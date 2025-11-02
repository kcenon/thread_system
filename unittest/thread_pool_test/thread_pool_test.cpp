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

using namespace kcenon::thread;

TEST(thread_pool_test, enqueue)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.has_error());
}

TEST(thread_pool_test, stop)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.has_error());

	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.has_error());
}

TEST(thread_pool_test, stop_immediately)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.has_error());

	auto stop_result = pool->stop(true);
	EXPECT_FALSE(stop_result.has_error());
}

TEST(thread_pool_test, stop_no_workers)
{
	auto pool = std::make_shared<thread_pool>();

	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.has_error());
}

TEST(thread_pool_test, start_and_stop)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.has_error());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.has_error());

	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.has_error());
}

TEST(thread_pool_test, start_and_stop_no_worker)
{
	auto pool = std::make_shared<thread_pool>();

	auto start_result = pool->start();
	EXPECT_TRUE(start_result.has_error());
	EXPECT_EQ(start_result.get_error().message(), "no workers to start");

	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.has_error());
}

TEST(thread_pool_test, start_and_stop_immediately)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.has_error());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.has_error());

	auto stop_result = pool->stop(true);
	EXPECT_FALSE(stop_result.has_error());
}

TEST(thread_pool_test, start_and_stop_immediately_no_worker)
{
	auto pool = std::make_shared<thread_pool>();

	auto start_result = pool->start();
	EXPECT_TRUE(start_result.has_error());
	EXPECT_EQ(start_result.get_error().message(), "no workers to start");

	auto stop_result = pool->stop(true);
	EXPECT_FALSE(stop_result.has_error());
}

TEST(thread_pool_test, start_and_one_sec_job_and_stop)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.has_error());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.has_error());

	result = pool->enqueue(std::make_unique<callback_job>(
		[](void) -> result_void
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));

			return result_void();
		},
		"10sec job"));
	EXPECT_FALSE(result.has_error());

	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.has_error());
}

// Test for Problem 1: enqueue after stop should fail
TEST(thread_pool_test, enqueue_after_stop_should_fail)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.has_error());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.has_error());

	// Stop the pool
	auto stop_result = pool->stop(false);
	EXPECT_FALSE(stop_result.has_error());

	// Try to enqueue a job after stop - should fail
	auto job = std::make_unique<callback_job>(
		[](void) -> result_void { return result_void(); },
		"test job");
	result = pool->enqueue(std::move(job));
	EXPECT_TRUE(result.has_error());
	EXPECT_EQ(result.get_error().code(), error_code::queue_stopped);
}

// Test for Problem 1: enqueue_batch after stop should fail
TEST(thread_pool_test, enqueue_batch_after_stop_should_fail)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.has_error());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.has_error());

	// Stop the pool
	auto stop_result = pool->stop(true);
	EXPECT_FALSE(stop_result.has_error());

	// Try to enqueue batch of jobs after stop - should fail
	std::vector<std::unique_ptr<job>> jobs;
	jobs.push_back(std::make_unique<callback_job>(
		[](void) -> result_void { return result_void(); },
		"test job 1"));
	jobs.push_back(std::make_unique<callback_job>(
		[](void) -> result_void { return result_void(); },
		"test job 2"));

	result = pool->enqueue_batch(std::move(jobs));
	EXPECT_TRUE(result.has_error());
	EXPECT_EQ(result.get_error().code(), error_code::queue_stopped);
}

// Test for Problem 2: concurrent stop calls should be safe
TEST(thread_pool_test, concurrent_stop_calls_should_be_safe)
{
	auto pool = std::make_shared<thread_pool>();

	auto worker = std::make_unique<thread_worker>();
	auto result = pool->enqueue(std::move(worker));
	EXPECT_FALSE(result.has_error());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.has_error());

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
			if (!stop_result.has_error())
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
	EXPECT_FALSE(result.has_error());

	auto start_result = pool->start();
	EXPECT_FALSE(start_result.has_error());

	// First stop should succeed
	auto stop_result1 = pool->stop(false);
	EXPECT_FALSE(stop_result1.has_error());

	// Second stop should also succeed (idempotent)
	auto stop_result2 = pool->stop(false);
	EXPECT_FALSE(stop_result2.has_error());

	// Third stop with immediate flag should also succeed
	auto stop_result3 = pool->stop(true);
	EXPECT_FALSE(stop_result3.has_error());
}
