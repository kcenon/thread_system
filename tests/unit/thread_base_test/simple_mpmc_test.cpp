// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#include "gtest/gtest.h"
#include <kcenon/thread/core/job_queue.h>
#include <kcenon/thread/core/callback_job.h>
#include <thread>

using namespace kcenon::thread;

// Test 1: Just create and destroy queue
TEST(SimpleMPMCTest, CreateDestroy)
{
	job_queue queue;
}

// Test 2: Create, enqueue one item, destroy
TEST(SimpleMPMCTest, SingleEnqueue)
{
	job_queue queue;
	
	auto job = std::make_unique<callback_job>([]() -> kcenon::common::VoidResult {
		return kcenon::common::ok();
	});
	
	auto result = queue.enqueue(std::move(job));
	EXPECT_TRUE(result.is_ok());
}

// Test 3: Create, enqueue and dequeue one item, destroy
TEST(SimpleMPMCTest, SingleEnqueueDequeue)
{
	job_queue queue;
	
	auto job = std::make_unique<callback_job>([]() -> kcenon::common::VoidResult {
		return kcenon::common::ok();
	});
	
	auto enqueue_result = queue.enqueue(std::move(job));
	EXPECT_TRUE(enqueue_result.is_ok());

	auto dequeue_result = queue.dequeue();
	EXPECT_TRUE(dequeue_result.is_ok());
}

// Test 4: Multiple queues in sequence
TEST(SimpleMPMCTest, MultipleQueues)
{
	for (int i = 0; i < 3; ++i) {
		job_queue queue;

		auto job = std::make_unique<callback_job>([]() -> kcenon::common::VoidResult {
			return kcenon::common::ok();
		});

		auto enqueue_result = queue.enqueue(std::move(job));
		EXPECT_TRUE(enqueue_result.is_ok());
		
		auto dequeue_result = queue.dequeue();
		EXPECT_TRUE(dequeue_result.is_ok());
	}
}

// Test 5: Thread with queue access
TEST(SimpleMPMCTest, ThreadAccess)
{
	job_queue queue;
	
	std::thread t([&queue]() {
		auto job = std::make_unique<callback_job>([]() -> kcenon::common::VoidResult {
			return kcenon::common::ok();
		});
		
		auto result = queue.enqueue(std::move(job));
		(void)result;
	});
	
	t.join();
}
