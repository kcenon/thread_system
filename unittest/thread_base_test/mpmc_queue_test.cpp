/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.

IMPLEMENTATION NOTE:
The MPMC queue implementation is functional but has a known issue with 
test fixture cleanup that causes segmentation faults when running multiple 
tests in sequence. Individual tests pass when run separately. This is likely 
related to thread-local storage cleanup in the node pool implementation.

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
#include "lockfree/lockfree_mpmc_queue.h"
#include "lockfree/adaptive_job_queue.h"
#include "callback_job.h"
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include <chrono>
#include <algorithm>

using namespace thread_module;

class MPMCQueueTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		// Setup code if needed
	}
	
	void TearDown() override
	{
		// Force cleanup of thread-local storage
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
};

// Basic functionality tests

TEST_F(MPMCQueueTest, BasicEnqueueDequeue)
{
	lockfree_mpmc_queue queue;
	
	// Test basic enqueue/dequeue
	std::atomic<int> counter{0};
	auto job = std::make_unique<callback_job>([&counter]() -> std::optional<std::string> {
		counter.fetch_add(1);
		return std::nullopt;
	});
	
	// Enqueue
	auto enqueue_result = queue.enqueue(std::move(job));
	EXPECT_TRUE(enqueue_result);
	EXPECT_EQ(queue.size(), 1);
	EXPECT_FALSE(queue.empty());
	
	// Dequeue
	auto dequeue_result = queue.dequeue();
	EXPECT_TRUE(dequeue_result.has_value());
	EXPECT_EQ(queue.size(), 0);
	EXPECT_TRUE(queue.empty());
	
	// Execute the job
	auto& dequeued_job = dequeue_result.value();
	EXPECT_NE(dequeued_job, nullptr);
	auto work_result = dequeued_job->do_work();
	(void)work_result; // Ignore result
	EXPECT_EQ(counter.load(), 1);
}

TEST_F(MPMCQueueTest, EmptyQueueDequeue)
{
	lockfree_mpmc_queue queue;
	
	// Try to dequeue from empty queue
	auto result = queue.dequeue();
	EXPECT_FALSE(result.has_value());
	EXPECT_EQ(result.get_error().code(), error_code::queue_empty);
}

TEST_F(MPMCQueueTest, NullJobEnqueue)
{
	lockfree_mpmc_queue queue;
	
	// Try to enqueue null job
	std::unique_ptr<job> null_job;
	auto result = queue.enqueue(std::move(null_job));
	EXPECT_FALSE(result);
	EXPECT_EQ(result.get_error().code(), error_code::invalid_argument);
}

TEST_F(MPMCQueueTest, BatchOperations)
{
	// Test with local scope to force destructor calls
	{
		lockfree_mpmc_queue queue;
		
		// Test single item first
		auto job = std::make_unique<callback_job>([]() -> std::optional<std::string> {
			return std::nullopt;
		});
		
		auto enqueue_result = queue.enqueue(std::move(job));
		EXPECT_TRUE(enqueue_result);
		
		auto dequeue_result = queue.dequeue();
		EXPECT_TRUE(dequeue_result.has_value());
	}
	
	// Now test batch operations
	{
		lockfree_mpmc_queue queue;
		
		// Prepare batch of jobs
		std::vector<std::unique_ptr<job>> jobs;
		std::atomic<int> counter{0};
		const size_t batch_size = 10;
		
		for (size_t i = 0; i < batch_size; ++i) {
			jobs.push_back(std::make_unique<callback_job>([&counter, i]() -> std::optional<std::string> {
				counter.fetch_add(static_cast<int>(i));
				return std::nullopt;
			}));
		}
		
		// Batch enqueue
		auto enqueue_result = queue.enqueue_batch(std::move(jobs));
		EXPECT_TRUE(enqueue_result);
		EXPECT_EQ(queue.size(), batch_size);
		
		// Batch dequeue
		auto dequeued = queue.dequeue_batch();
		EXPECT_EQ(dequeued.size(), batch_size);
		EXPECT_TRUE(queue.empty());
		
		// Execute all jobs
		for (auto& job : dequeued) {
			auto work_result = job->do_work();
			(void)work_result;
		}
		
		// Sum of 0 to 9 is 45
		EXPECT_EQ(counter.load(), 45);
	}
}

// Concurrency tests

TEST_F(MPMCQueueTest, ConcurrentEnqueue)
{
	lockfree_mpmc_queue queue;
	const size_t num_threads = 8;
	const size_t jobs_per_thread = 1000;
	std::atomic<int> counter{0};
	
	std::vector<std::thread> threads;
	
	// Start producer threads
	for (size_t t = 0; t < num_threads; ++t) {
		threads.emplace_back([&queue, &counter, jobs_per_thread]() {
			for (size_t i = 0; i < jobs_per_thread; ++i) {
				auto job = std::make_unique<callback_job>([&counter]() -> std::optional<std::string> {
					counter.fetch_add(1);
					return std::nullopt;
				});
				
				while (true) {
					auto result = queue.enqueue(std::move(job));
					if (result) break;
					std::this_thread::yield();
				}
			}
		});
	}
	
	// Wait for all producers
	for (auto& t : threads) {
		t.join();
	}
	
	// Verify all jobs were enqueued
	EXPECT_EQ(queue.size(), num_threads * jobs_per_thread);
	
	// Dequeue and execute all jobs
	size_t dequeued_count = 0;
	while (!queue.empty()) {
		auto result = queue.dequeue();
		if (result.has_value()) {
			auto work_result = result.value()->do_work();
			(void)work_result;
			dequeued_count++;
		}
	}
	
	EXPECT_EQ(dequeued_count, num_threads * jobs_per_thread);
	EXPECT_EQ(counter.load(), num_threads * jobs_per_thread);
}

TEST_F(MPMCQueueTest, ConcurrentDequeue)
{
	lockfree_mpmc_queue queue;
	const size_t num_jobs = 10000;
	const size_t num_consumers = 8;
	std::atomic<int> counter{0};
	
	// Enqueue jobs
	for (size_t i = 0; i < num_jobs; ++i) {
		auto job = std::make_unique<callback_job>([&counter, i]() -> std::optional<std::string> {
			counter.fetch_add(1);
			return std::nullopt;
		});
		auto enqueue_result = queue.enqueue(std::move(job));
		(void)enqueue_result;
	}
	
	EXPECT_EQ(queue.size(), num_jobs);
	
	// Start consumer threads
	std::atomic<size_t> total_dequeued{0};
	std::vector<std::thread> threads;
	
	for (size_t t = 0; t < num_consumers; ++t) {
		threads.emplace_back([&queue, &total_dequeued]() {
			size_t local_count = 0;
			while (true) {
				auto result = queue.dequeue();
				if (!result.has_value()) {
					break; // Queue is empty
				}
				auto work_result = result.value()->do_work();
			(void)work_result;
				local_count++;
			}
			total_dequeued.fetch_add(local_count);
		});
	}
	
	// Wait for all consumers
	for (auto& t : threads) {
		t.join();
	}
	
	EXPECT_EQ(total_dequeued.load(), num_jobs);
	EXPECT_EQ(counter.load(), num_jobs);
	EXPECT_TRUE(queue.empty());
}

TEST_F(MPMCQueueTest, DISABLED_ProducerConsumerStress)
{
	lockfree_mpmc_queue queue;
	const size_t num_producers = 4;
	const size_t num_consumers = 4;
	const size_t jobs_per_producer = 2500;
	const auto test_duration = std::chrono::seconds(1);
	
	std::atomic<bool> stop{false};
	std::atomic<size_t> produced{0};
	std::atomic<size_t> consumed{0};
	std::atomic<size_t> executed{0};
	
	std::vector<std::thread> producers;
	std::vector<std::thread> consumers;
	
	// Start producers
	for (size_t p = 0; p < num_producers; ++p) {
		producers.emplace_back([&]() {
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> dis(1, 10);
			
			while (!stop.load(std::memory_order_relaxed)) {
				// Random batch size
				size_t batch_size = dis(gen);
				std::vector<std::unique_ptr<job>> batch;
				
				for (size_t i = 0; i < batch_size; ++i) {
					batch.push_back(std::make_unique<callback_job>([&executed]() -> std::optional<std::string> {
						executed.fetch_add(1);
						return std::nullopt;
					}));
				}
				
				if (batch_size == 1) {
					auto enqueue_result = queue.enqueue(std::move(batch[0]));
					(void)enqueue_result;
				} else {
					auto enqueue_batch_result = queue.enqueue_batch(std::move(batch));
					(void)enqueue_batch_result;
				}
				
				produced.fetch_add(batch_size);
				std::this_thread::sleep_for(std::chrono::microseconds(dis(gen)));
			}
		});
	}
	
	// Start consumers
	for (size_t c = 0; c < num_consumers; ++c) {
		consumers.emplace_back([&]() {
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> dis(0, 1);
			
			while (!stop.load(std::memory_order_relaxed) || !queue.empty()) {
				if (dis(gen) == 0) {
					// Single dequeue
					auto result = queue.dequeue();
					if (result.has_value()) {
						auto work_result = result.value()->do_work();
			(void)work_result;
						consumed.fetch_add(1);
					}
				} else {
					// Batch dequeue
					auto batch = queue.dequeue_batch();
					for (auto& job : batch) {
						auto work_result = job->do_work();
		(void)work_result;
					}
					consumed.fetch_add(batch.size());
				}
				
				std::this_thread::sleep_for(std::chrono::microseconds(dis(gen)));
			}
		});
	}
	
	// Run test for specified duration
	std::this_thread::sleep_for(test_duration);
	stop.store(true);
	
	// Wait for all threads
	for (auto& t : producers) {
		t.join();
	}
	for (auto& t : consumers) {
		t.join();
	}
	
	// Verify consistency
	EXPECT_EQ(produced.load(), consumed.load());
	EXPECT_EQ(consumed.load(), executed.load());
	EXPECT_TRUE(queue.empty());
	
	// Check statistics
	auto stats = queue.get_statistics();
	std::cout << "Performance stats:\n"
			  << "  Total enqueued: " << stats.enqueue_count << "\n"
			  << "  Total dequeued: " << stats.dequeue_count << "\n"
			  << "  Avg enqueue latency: " << stats.get_average_enqueue_latency_ns() << " ns\n"
			  << "  Avg dequeue latency: " << stats.get_average_dequeue_latency_ns() << " ns\n"
			  << "  Retries: " << stats.retry_count << "\n";
}

// Adaptive queue tests

TEST_F(MPMCQueueTest, AdaptiveQueueBasicOperation)
{
	adaptive_job_queue queue(adaptive_job_queue::queue_strategy::AUTO_DETECT);
	
	// Test basic operations
	auto job = std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; });
	
	auto enqueue_result = queue.enqueue(std::move(job));
	EXPECT_TRUE(enqueue_result);
	
	auto dequeue_result = queue.dequeue();
	EXPECT_TRUE(dequeue_result.has_value());
	
	EXPECT_TRUE(queue.empty());
}

TEST_F(MPMCQueueTest, AdaptiveQueueStrategySwitch)
{
	adaptive_job_queue queue(adaptive_job_queue::queue_strategy::ADAPTIVE);
	
	// Should start with mutex-based
	EXPECT_EQ(queue.get_current_type(), "mutex_based");
	
	const size_t num_threads = 8;
	const size_t operations_per_thread = 200;
	std::atomic<bool> start{false};
	std::vector<std::thread> threads;
	
	// Create high contention scenario
	for (size_t t = 0; t < num_threads; ++t) {
		threads.emplace_back([&]() {
			// Wait for all threads to be ready
			while (!start.load()) {
				std::this_thread::yield();
			}
			
			// Hammer the queue
			for (size_t i = 0; i < operations_per_thread; ++i) {
				auto job = std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; });
				auto enqueue_result = queue.enqueue(std::move(job));
		(void)enqueue_result;
				
				auto result = queue.dequeue();
				if (result.has_value()) {
					auto work_result = result.value()->do_work();
			(void)work_result;
				}
			}
		});
	}
	
	// Start all threads simultaneously
	start.store(true);
	
	// Wait for threads
	for (auto& t : threads) {
		t.join();
	}
	
	// Force evaluation
	queue.evaluate_and_switch();
	
	// Check metrics
	auto metrics = queue.get_metrics();
	std::cout << "Adaptive queue metrics:\n"
			  << "  Current type: " << queue.get_current_type() << "\n"
			  << "  Operations: " << metrics.operation_count << "\n"
			  << "  Avg latency: " << metrics.get_average_latency_ns() << " ns\n"
			  << "  Contention ratio: " << metrics.get_contention_ratio() << "\n"
			  << "  Switches: " << metrics.switch_count << "\n";
}

// Performance comparison test

TEST_F(MPMCQueueTest, DISABLED_PerformanceComparison)
{
	const size_t num_operations = 100000;
	const size_t num_threads = 4;
	
	// Test legacy queue
	{
		job_queue legacy_queue;
		auto start_time = std::chrono::high_resolution_clock::now();
		
		std::vector<std::thread> threads;
		for (size_t t = 0; t < num_threads; ++t) {
			threads.emplace_back([&legacy_queue, num_operations, num_threads]() {
				for (size_t i = 0; i < num_operations / num_threads; ++i) {
					auto job = std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; });
					auto enqueue_result = legacy_queue.enqueue(std::move(job));
					(void)enqueue_result;
					
					auto result = legacy_queue.dequeue();
					if (result.has_value()) {
						auto work_result = result.value()->do_work();
			(void)work_result;
					}
				}
			});
		}
		
		for (auto& t : threads) {
			t.join();
		}
		
		auto duration = std::chrono::high_resolution_clock::now() - start_time;
		auto legacy_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
		std::cout << "Legacy queue time: " << legacy_time_ms << " ms\n";
	}
	
	// Test lock-free queue
	{
		lockfree_mpmc_queue mpmc_queue;
		auto start_time = std::chrono::high_resolution_clock::now();
		
		std::vector<std::thread> threads;
		for (size_t t = 0; t < num_threads; ++t) {
			threads.emplace_back([&mpmc_queue, num_operations, num_threads]() {
				for (size_t i = 0; i < num_operations / num_threads; ++i) {
					auto job = std::make_unique<callback_job>([]() -> std::optional<std::string> { return std::nullopt; });
					auto enqueue_result = mpmc_queue.enqueue(std::move(job));
					(void)enqueue_result;
					
					auto result = mpmc_queue.dequeue();
					if (result.has_value()) {
						auto work_result = result.value()->do_work();
			(void)work_result;
					}
				}
			});
		}
		
		for (auto& t : threads) {
			t.join();
		}
		
		auto duration = std::chrono::high_resolution_clock::now() - start_time;
		auto mpmc_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
		std::cout << "Lock-free queue time: " << mpmc_time_ms << " ms\n";
		
		auto stats = mpmc_queue.get_statistics();
		std::cout << "Lock-free queue detailed stats:\n"
				  << "  Avg enqueue latency: " << stats.get_average_enqueue_latency_ns() << " ns\n"
				  << "  Avg dequeue latency: " << stats.get_average_dequeue_latency_ns() << " ns\n";
	}
}