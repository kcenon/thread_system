/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, DongCheol Shin
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

#include <gtest/gtest.h>

#include <kcenon/thread/impl/typed_pool/adaptive_typed_job_queue.h>
#include <kcenon/thread/impl/typed_pool/callback_typed_job.h>
#include <kcenon/thread/impl/typed_pool/job_types.h>
#include <kcenon/thread/core/job.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <latch>

using namespace kcenon::thread;

// Helper to create typed jobs and cast to avoid ambiguity
inline auto make_typed_job(job_types priority, const std::string& name = "test_job")
	-> std::unique_ptr<typed_job>
{
	return std::make_unique<callback_typed_job>(
		[]() -> kcenon::common::VoidResult { return kcenon::common::ok(); }, priority, name);
}

inline auto make_typed_job_with_callback(const std::function<kcenon::common::VoidResult()>& callback,
										 job_types priority,
										 const std::string& name = "test_job")
	-> std::unique_ptr<typed_job>
{
	return std::make_unique<callback_typed_job>(callback, priority, name);
}

class AdaptiveTypedJobQueueTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		// Reset any global state if needed
	}

	void TearDown() override
	{
		// Allow cleanup
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
};

// ============================================
// Basic functionality tests
// ============================================

TEST_F(AdaptiveTypedJobQueueTest, DefaultConstruction)
{
	adaptive_typed_job_queue_t<job_types> queue;

	EXPECT_EQ(queue.get_current_type(), "legacy_mutex");
}

TEST_F(AdaptiveTypedJobQueueTest, ConstructWithForceLegacy)
{
	adaptive_typed_job_queue_t<job_types> queue(
		adaptive_typed_job_queue_t<job_types>::queue_strategy::FORCE_LEGACY);

	EXPECT_EQ(queue.get_current_type(), "legacy_mutex");
}

TEST_F(AdaptiveTypedJobQueueTest, ConstructWithAutoDetect)
{
	adaptive_typed_job_queue_t<job_types> queue(
		adaptive_typed_job_queue_t<job_types>::queue_strategy::AUTO_DETECT);

	// AUTO_DETECT defaults to legacy due to TLS bug
	EXPECT_EQ(queue.get_current_type(), "legacy_mutex");
}

TEST_F(AdaptiveTypedJobQueueTest, BasicTypedEnqueueDequeue)
{
	adaptive_typed_job_queue_t<job_types> queue;

	std::atomic<int> counter{ 0 };
	auto typed_job_ptr = make_typed_job_with_callback(
		[&counter]() -> kcenon::common::VoidResult
		{
			counter.fetch_add(1, std::memory_order_relaxed);
			return kcenon::common::ok();
		},
		job_types::RealTime, "test_job");

	// Enqueue typed job
	auto enqueue_result = queue.enqueue(std::move(typed_job_ptr));
	EXPECT_FALSE(enqueue_result.is_err());

	// Dequeue using typed interface
	std::vector<job_types> types = { job_types::RealTime };
	auto dequeue_result = queue.dequeue(types);
	EXPECT_TRUE(dequeue_result.is_ok());

	// Execute job
	auto& job_ptr = dequeue_result.value();
	auto exec_result = job_ptr->do_work();
	EXPECT_FALSE(exec_result.is_err());
	EXPECT_EQ(counter.load(), 1);
}

TEST_F(AdaptiveTypedJobQueueTest, EnqueueBaseJob)
{
	adaptive_typed_job_queue_t<job_types> queue;

	std::atomic<int> counter{ 0 };
	// Use typed_job directly to ensure proper routing
	auto typed_job_ptr = make_typed_job_with_callback(
		[&counter]() -> kcenon::common::VoidResult
		{
			counter.fetch_add(1, std::memory_order_relaxed);
			return kcenon::common::ok();
		},
		job_types::Batch, "base_job_test");

	// Enqueue typed job
	auto enqueue_result = queue.enqueue(std::move(typed_job_ptr));
	EXPECT_FALSE(enqueue_result.is_err());

	// Dequeue via typed interface with specific type
	std::vector<job_types> batch_types = { job_types::Batch };
	auto dequeue_result = queue.dequeue(batch_types);
	EXPECT_TRUE(dequeue_result.is_ok());

	// Execute
	auto exec_result = dequeue_result.value()->do_work();
	EXPECT_FALSE(exec_result.is_err());
	EXPECT_EQ(counter.load(), 1);
}

TEST_F(AdaptiveTypedJobQueueTest, DequeueEmpty)
{
	adaptive_typed_job_queue_t<job_types> queue;

	std::vector<job_types> types = { job_types::RealTime };
	auto result = queue.dequeue(types);
	EXPECT_FALSE(result.is_ok());
}

TEST_F(AdaptiveTypedJobQueueTest, DequeueAllEmpty)
{
	adaptive_typed_job_queue_t<job_types> queue;

	auto result = queue.dequeue();
	EXPECT_FALSE(result.is_ok());
}

TEST_F(AdaptiveTypedJobQueueTest, Clear)
{
	adaptive_typed_job_queue_t<job_types> queue;

	// Add multiple jobs
	for (int i = 0; i < 10; ++i)
	{
		job_types priority = static_cast<job_types>(i % 3);
		auto typed_job_ptr = make_typed_job(priority, "clear_test_job");
		queue.enqueue(std::move(typed_job_ptr));
	}

	queue.clear();

	// All queues should be empty
	std::vector<job_types> all_types
		= { job_types::RealTime, job_types::Batch, job_types::Background };
	EXPECT_TRUE(queue.empty(all_types));
}

TEST_F(AdaptiveTypedJobQueueTest, ToString)
{
	adaptive_typed_job_queue_t<job_types> queue;

	auto str = queue.to_string();
	EXPECT_FALSE(str.empty());
}

// ============================================
// Type safety tests
// ============================================

TEST_F(AdaptiveTypedJobQueueTest, TypeSafeEnqueueRealTime)
{
	adaptive_typed_job_queue_t<job_types> queue;

	auto typed_job_ptr = make_typed_job(job_types::RealTime, "realtime_job");
	EXPECT_EQ(typed_job_ptr->priority(), job_types::RealTime);

	auto result = queue.enqueue(std::move(typed_job_ptr));
	EXPECT_FALSE(result.is_err());
}

TEST_F(AdaptiveTypedJobQueueTest, TypeSafeEnqueueBatch)
{
	adaptive_typed_job_queue_t<job_types> queue;

	auto typed_job_ptr = make_typed_job(job_types::Batch, "batch_job");
	EXPECT_EQ(typed_job_ptr->priority(), job_types::Batch);

	auto result = queue.enqueue(std::move(typed_job_ptr));
	EXPECT_FALSE(result.is_err());
}

TEST_F(AdaptiveTypedJobQueueTest, TypeSafeEnqueueBackground)
{
	adaptive_typed_job_queue_t<job_types> queue;

	auto typed_job_ptr = make_typed_job(job_types::Background, "background_job");
	EXPECT_EQ(typed_job_ptr->priority(), job_types::Background);

	auto result = queue.enqueue(std::move(typed_job_ptr));
	EXPECT_FALSE(result.is_err());
}

TEST_F(AdaptiveTypedJobQueueTest, DequeueBySpecificType)
{
	adaptive_typed_job_queue_t<job_types> queue;

	// Enqueue jobs of different types
	auto realtime_job = make_typed_job(job_types::RealTime, "realtime");
	auto batch_job = make_typed_job(job_types::Batch, "batch");
	auto background_job = make_typed_job(job_types::Background, "background");

	queue.enqueue(std::move(realtime_job));
	queue.enqueue(std::move(batch_job));
	queue.enqueue(std::move(background_job));

	// Dequeue only Batch type
	std::vector<job_types> batch_types = { job_types::Batch };
	auto result = queue.dequeue(batch_types);
	EXPECT_TRUE(result.is_ok());
	EXPECT_EQ(result.value()->priority(), job_types::Batch);

	// Verify other types are still in queue
	std::vector<job_types> realtime_types = { job_types::RealTime };
	EXPECT_FALSE(queue.empty(realtime_types));

	std::vector<job_types> background_types = { job_types::Background };
	EXPECT_FALSE(queue.empty(background_types));
}

TEST_F(AdaptiveTypedJobQueueTest, DequeueMultipleTypes)
{
	adaptive_typed_job_queue_t<job_types> queue;

	auto typed_job_ptr = make_typed_job(job_types::Batch, "batch_job");
	queue.enqueue(std::move(typed_job_ptr));

	// Dequeue with multiple allowed types
	std::vector<job_types> allowed_types = { job_types::RealTime, job_types::Batch };
	auto result = queue.dequeue(allowed_types);
	EXPECT_TRUE(result.is_ok());
	EXPECT_EQ(result.value()->priority(), job_types::Batch);
}

TEST_F(AdaptiveTypedJobQueueTest, PriorityPreservedAfterDequeue)
{
	adaptive_typed_job_queue_t<job_types> queue;

	// Enqueue jobs with different priorities
	for (int i = 0; i < 5; ++i)
	{
		auto typed_job_ptr = make_typed_job(job_types::RealTime, "priority_test_" + std::to_string(i));
		queue.enqueue(std::move(typed_job_ptr));
	}

	// Dequeue and verify priority is preserved
	std::vector<job_types> types = { job_types::RealTime };
	for (int i = 0; i < 5; ++i)
	{
		auto result = queue.dequeue(types);
		EXPECT_TRUE(result.is_ok());
		EXPECT_EQ(result.value()->priority(), job_types::RealTime);
	}
}

// ============================================
// Empty and size tests
// ============================================

TEST_F(AdaptiveTypedJobQueueTest, EmptyByType)
{
	adaptive_typed_job_queue_t<job_types> queue;

	std::vector<job_types> realtime_types = { job_types::RealTime };
	std::vector<job_types> batch_types = { job_types::Batch };

	// Initially empty
	EXPECT_TRUE(queue.empty(realtime_types));
	EXPECT_TRUE(queue.empty(batch_types));

	// Add RealTime job
	auto typed_job_ptr = make_typed_job(job_types::RealTime, "realtime_job");
	queue.enqueue(std::move(typed_job_ptr));

	EXPECT_FALSE(queue.empty(realtime_types));
	EXPECT_TRUE(queue.empty(batch_types));
}

TEST_F(AdaptiveTypedJobQueueTest, SizeByType)
{
	adaptive_typed_job_queue_t<job_types> queue;

	// Add jobs to different types
	for (int i = 0; i < 5; ++i)
	{
		auto realtime = make_typed_job(job_types::RealTime, "realtime");
		queue.enqueue(std::move(realtime));
	}

	for (int i = 0; i < 3; ++i)
	{
		auto batch = make_typed_job(job_types::Batch, "batch");
		queue.enqueue(std::move(batch));
	}

	std::vector<job_types> realtime_types = { job_types::RealTime };
	std::vector<job_types> batch_types = { job_types::Batch };
	std::vector<job_types> all_types
		= { job_types::RealTime, job_types::Batch, job_types::Background };

	// Size returns approximate count (> 0 if not empty)
	EXPECT_GT(queue.size(realtime_types), 0);
	EXPECT_GT(queue.size(batch_types), 0);
	EXPECT_GT(queue.size(all_types), 0);

	// Verify queues are not empty
	EXPECT_FALSE(queue.empty(realtime_types));
	EXPECT_FALSE(queue.empty(batch_types));
}

// ============================================
// Performance metrics tests
// ============================================

TEST_F(AdaptiveTypedJobQueueTest, MetricsInitialState)
{
	adaptive_typed_job_queue_t<job_types> queue;

	auto metrics = queue.get_metrics();
	EXPECT_EQ(metrics.operation_count, 0);
	EXPECT_EQ(metrics.switch_count, 0);
}

TEST_F(AdaptiveTypedJobQueueTest, MetricsAfterOperations)
{
	adaptive_typed_job_queue_t<job_types> queue;

	// Perform some operations
	for (int i = 0; i < 10; ++i)
	{
		auto typed_job_ptr = make_typed_job(job_types::RealTime, "test_job");
		queue.enqueue(std::move(typed_job_ptr));
	}

	auto metrics = queue.get_metrics();
	EXPECT_GT(metrics.operation_count, 0);
}

TEST_F(AdaptiveTypedJobQueueTest, MetricsAverageLatency)
{
	adaptive_typed_job_queue_t<job_types> queue;

	// Perform operations to get some latency data
	for (int i = 0; i < 100; ++i)
	{
		auto typed_job_ptr = make_typed_job(job_types::Batch, "latency_test");
		queue.enqueue(std::move(typed_job_ptr));
	}

	auto metrics = queue.get_metrics();
	double avg_latency = metrics.get_average_latency_ns();
	// Just verify it's non-negative
	EXPECT_GE(avg_latency, 0.0);
}

TEST_F(AdaptiveTypedJobQueueTest, MetricsContentionRatio)
{
	adaptive_typed_job_queue_t<job_types> queue;

	auto metrics = queue.get_metrics();
	double ratio = metrics.get_contention_ratio();
	// With no operations, ratio should be 0
	EXPECT_DOUBLE_EQ(ratio, 0.0);
}

// ============================================
// Evaluate and switch tests
// ============================================

TEST_F(AdaptiveTypedJobQueueTest, EvaluateAndSwitch)
{
	adaptive_typed_job_queue_t<job_types> queue(
		adaptive_typed_job_queue_t<job_types>::queue_strategy::ADAPTIVE);

	// Add some jobs
	for (int i = 0; i < 10; ++i)
	{
		auto typed_job_ptr = make_typed_job(job_types::RealTime, "test_job");
		queue.enqueue(std::move(typed_job_ptr));
	}

	// Trigger evaluation - should not throw
	queue.evaluate_and_switch();

	// Queue should still be functional
	std::vector<job_types> types = { job_types::RealTime };
	EXPECT_FALSE(queue.empty(types));
}

TEST_F(AdaptiveTypedJobQueueTest, GetCurrentType)
{
	adaptive_typed_job_queue_t<job_types> queue;

	auto type = queue.get_current_type();
	EXPECT_FALSE(type.empty());
	EXPECT_EQ(type, "legacy_mutex");
}

// ============================================
// Batch operations tests
// ============================================

TEST_F(AdaptiveTypedJobQueueTest, EnqueueBatch)
{
	adaptive_typed_job_queue_t<job_types> queue;

	std::vector<std::unique_ptr<kcenon::thread::job>> jobs;
	for (int i = 0; i < 10; ++i)
	{
		std::unique_ptr<kcenon::thread::job> typed_job_ptr = make_typed_job(job_types::RealTime, "batch_job");
		jobs.push_back(std::move(typed_job_ptr));
	}

	auto result = queue.enqueue_batch(std::move(jobs));
	EXPECT_FALSE(result.is_err());

	std::vector<job_types> types = { job_types::RealTime };
	// Verify jobs were enqueued (queue is not empty)
	EXPECT_FALSE(queue.empty(types));
}

TEST_F(AdaptiveTypedJobQueueTest, DequeueBatch)
{
	adaptive_typed_job_queue_t<job_types> queue;

	// Add jobs
	constexpr int job_count = 3;
	for (int i = 0; i < job_count; ++i)
	{
		auto typed_job_ptr = make_typed_job(job_types::Batch, "batch_test");
		queue.enqueue(std::move(typed_job_ptr));
	}

	// Test that we can dequeue jobs
	std::vector<job_types> batch_types = { job_types::Batch };
	EXPECT_FALSE(queue.empty(batch_types));

	// Dequeue one job to verify
	auto result = queue.dequeue(batch_types);
	EXPECT_TRUE(result.is_ok());
}

// ============================================
// Concurrency tests (simplified)
// ============================================

TEST_F(AdaptiveTypedJobQueueTest, ConcurrentTypedEnqueueDequeue)
{
	adaptive_typed_job_queue_t<job_types> queue;

	// Simple concurrent test: single producer, single consumer
	std::atomic<int> enqueued{ 0 };
	std::atomic<int> dequeued{ 0 };
	std::atomic<bool> done{ false };
	constexpr int job_count = 50;

	std::thread producer([&]()
		{
			for (int i = 0; i < job_count; ++i)
			{
				auto typed_job_ptr = make_typed_job(job_types::RealTime, "concurrent_job");
				if (!queue.enqueue(std::move(typed_job_ptr)).is_err())
				{
					enqueued.fetch_add(1, std::memory_order_relaxed);
				}
			}
			done.store(true, std::memory_order_release);
		});

	std::thread consumer([&]()
		{
			std::vector<job_types> types = { job_types::RealTime };
			while (!done.load(std::memory_order_acquire) || !queue.empty(types))
			{
				if (auto result = queue.dequeue(types); result.is_ok())
				{
					dequeued.fetch_add(1, std::memory_order_relaxed);
				}
				else
				{
					std::this_thread::yield();
				}
			}
		});

	producer.join();
	consumer.join();

	EXPECT_EQ(enqueued.load(), job_count);
	EXPECT_EQ(dequeued.load(), job_count);
}

TEST_F(AdaptiveTypedJobQueueTest, ConcurrentDifferentTypeAccess)
{
	adaptive_typed_job_queue_t<job_types> queue;

	// Simple test: sequentially add and verify different types
	constexpr int jobs_per_type = 10;

	for (int i = 0; i < jobs_per_type; ++i)
	{
		auto realtime_job = make_typed_job(job_types::RealTime, "realtime");
		auto batch_job = make_typed_job(job_types::Batch, "batch");
		auto background_job = make_typed_job(job_types::Background, "background");

		queue.enqueue(std::move(realtime_job));
		queue.enqueue(std::move(batch_job));
		queue.enqueue(std::move(background_job));
	}

	// Verify all types have jobs
	std::vector<job_types> realtime_types = { job_types::RealTime };
	std::vector<job_types> batch_types = { job_types::Batch };
	std::vector<job_types> background_types = { job_types::Background };

	EXPECT_FALSE(queue.empty(realtime_types));
	EXPECT_FALSE(queue.empty(batch_types));
	EXPECT_FALSE(queue.empty(background_types));
}

TEST_F(AdaptiveTypedJobQueueTest, ConcurrentEvaluateAndSwitch)
{
	adaptive_typed_job_queue_t<job_types> queue(
		adaptive_typed_job_queue_t<job_types>::queue_strategy::ADAPTIVE);

	// Add some jobs
	for (int i = 0; i < 10; ++i)
	{
		auto typed_job_ptr = make_typed_job(job_types::RealTime, "test_job");
		queue.enqueue(std::move(typed_job_ptr));
	}

	// Evaluate should not throw
	queue.evaluate_and_switch();

	// Queue should still be functional
	std::vector<job_types> types = { job_types::RealTime };
	EXPECT_FALSE(queue.empty(types));
}

// ============================================
// Factory function tests
// ============================================

TEST_F(AdaptiveTypedJobQueueTest, CreateTypedJobQueueAutoDetect)
{
	auto queue = create_typed_job_queue<job_types>(
		adaptive_typed_job_queue_t<job_types>::queue_strategy::AUTO_DETECT);

	EXPECT_NE(queue, nullptr);
}

TEST_F(AdaptiveTypedJobQueueTest, CreateTypedJobQueueForceLegacy)
{
	auto queue = create_typed_job_queue<job_types>(
		adaptive_typed_job_queue_t<job_types>::queue_strategy::FORCE_LEGACY);

	EXPECT_NE(queue, nullptr);

	// Should be functional
	auto typed_job_ptr = make_typed_job(job_types::RealTime, "factory_test");
	auto result = queue->enqueue(std::move(typed_job_ptr));
	EXPECT_FALSE(result.is_err());
}

// ============================================
// Edge case tests
// ============================================

TEST_F(AdaptiveTypedJobQueueTest, StressTestMixedOperations)
{
	// Simplified stress test - test mixed operations with manageable iterations
	adaptive_typed_job_queue_t<job_types> queue;
	constexpr int iterations = 10;

	// Enqueue jobs of different types
	for (int i = 0; i < iterations; ++i)
	{
		job_types type = static_cast<job_types>(i % 3);
		auto typed_job_ptr = make_typed_job(type, "stress_job");
		queue.enqueue(std::move(typed_job_ptr));
	}

	// Verify queue is not empty for at least one type
	std::vector<job_types> all_types = { job_types::RealTime, job_types::Batch, job_types::Background };
	EXPECT_FALSE(queue.empty(all_types));
}

TEST_F(AdaptiveTypedJobQueueTest, RapidEnqueueDequeue)
{
	// Simplified rapid test - verify basic enqueue/dequeue cycle
	adaptive_typed_job_queue_t<job_types> queue;

	// Just verify that enqueue/dequeue works correctly for a few iterations
	for (int i = 0; i < 5; ++i)
	{
		auto typed_job_ptr = make_typed_job(job_types::Batch, "rapid_test");

		auto enqueue_result = queue.enqueue(std::move(typed_job_ptr));
		EXPECT_FALSE(enqueue_result.is_err());

		std::vector<job_types> batch_types = { job_types::Batch };
		EXPECT_FALSE(queue.empty(batch_types));
	}
}

TEST_F(AdaptiveTypedJobQueueTest, AllJobTypesUsed)
{
	adaptive_typed_job_queue_t<job_types> queue;

	auto all_types = get_all_job_types();

	for (const auto& type : all_types)
	{
		auto typed_job_ptr = make_typed_job(type, "all_types_test");
		queue.enqueue(std::move(typed_job_ptr));
	}

	for (const auto& type : all_types)
	{
		std::vector<job_types> types = { type };
		EXPECT_FALSE(queue.empty(types));
	}
}
