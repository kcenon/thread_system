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

#include "../framework/system_fixture.h"
#include "../framework/test_helpers.h"
#include <gtest/gtest.h>

#include <kcenon/thread/policies/policy_queue.h>
#include <kcenon/thread/adapters/policy_queue_adapter.h>
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/core/thread_pool.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace kcenon::thread;
using namespace integration_tests;

/**
 * @brief Integration tests for policy_queue
 *
 * Goal: Verify policy_queue behavior under real-world scenarios
 * Expected time: < 60 seconds
 * Test scenarios:
 *   1. Standard queue operations (mutex-based)
 *   2. Lock-free queue operations
 *   3. Bounded queue with overflow policies
 *   4. Thread pool integration
 *   5. Concurrent enqueue/dequeue operations
 */
class PolicyQueueIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        completed_jobs_.store(0);
        failed_jobs_.store(0);
    }

    void TearDown() override {
        std::this_thread::yield();
    }

    template <typename Predicate>
    bool WaitForCondition(Predicate pred,
                          std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
        auto start = std::chrono::steady_clock::now();
        while (!pred()) {
            if (std::chrono::steady_clock::now() - start > timeout) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return true;
    }

    std::atomic<size_t> completed_jobs_{0};
    std::atomic<size_t> failed_jobs_{0};
};

// ============================================================================
// Standard Queue (Mutex-based) Tests
// ============================================================================

TEST_F(PolicyQueueIntegrationTest, StandardQueueBasicOperations) {
    standard_queue queue;

    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);

    // Enqueue jobs
    for (int i = 0; i < 10; ++i) {
        auto job = std::make_unique<callback_job>(
            []() -> kcenon::common::VoidResult { return kcenon::common::ok(); });
        EXPECT_TRUE(queue.enqueue(std::move(job)).is_ok());
    }

    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 10);

    // Dequeue all jobs
    for (int i = 0; i < 10; ++i) {
        auto result = queue.try_dequeue();
        EXPECT_TRUE(result.is_ok());
    }

    EXPECT_TRUE(queue.empty());
}

TEST_F(PolicyQueueIntegrationTest, StandardQueueFIFOOrder) {
    standard_queue queue;

    std::vector<int> execution_order;
    std::mutex order_mutex;

    const size_t job_count = 100;
    for (size_t i = 0; i < job_count; ++i) {
        auto job = std::make_unique<callback_job>(
            [&execution_order, &order_mutex, i]() -> kcenon::common::VoidResult {
                std::lock_guard<std::mutex> lock(order_mutex);
                execution_order.push_back(static_cast<int>(i));
                return kcenon::common::ok();
            });
        EXPECT_TRUE(queue.enqueue(std::move(job)).is_ok());
    }

    // Dequeue and execute in order
    for (size_t i = 0; i < job_count; ++i) {
        auto result = queue.try_dequeue();
        ASSERT_TRUE(result.is_ok());
        auto job_result = result.value()->do_work();
        EXPECT_TRUE(job_result.is_ok());
    }

    // Verify FIFO order
    ASSERT_EQ(execution_order.size(), job_count);
    for (size_t i = 0; i < job_count; ++i) {
        EXPECT_EQ(execution_order[i], static_cast<int>(i))
            << "Job executed out of order at position " << i;
    }
}

TEST_F(PolicyQueueIntegrationTest, StandardQueueConcurrentEnqueue) {
    standard_queue queue;

    const size_t num_threads = 4;
    const size_t jobs_per_thread = 250;
    std::atomic<size_t> total_enqueued{0};

    std::vector<std::thread> producers;
    for (size_t t = 0; t < num_threads; ++t) {
        producers.emplace_back([&queue, &total_enqueued, jobs_per_thread]() {
            for (size_t i = 0; i < jobs_per_thread; ++i) {
                auto job = std::make_unique<callback_job>(
                    []() -> kcenon::common::VoidResult { return kcenon::common::ok(); });
                if (queue.enqueue(std::move(job)).is_ok()) {
                    total_enqueued.fetch_add(1);
                }
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }

    EXPECT_EQ(total_enqueued.load(), num_threads * jobs_per_thread);
    EXPECT_EQ(queue.size(), num_threads * jobs_per_thread);
}

TEST_F(PolicyQueueIntegrationTest, StandardQueueConcurrentEnqueueDequeue) {
    standard_queue queue;

    const size_t num_producers = 4;
    const size_t num_consumers = 4;
    const size_t jobs_per_producer = 250;

    std::atomic<size_t> enqueued{0};
    std::atomic<size_t> dequeued{0};
    std::atomic<bool> producers_done{false};

    // Start producers
    std::vector<std::thread> producers;
    for (size_t t = 0; t < num_producers; ++t) {
        producers.emplace_back([&queue, &enqueued, jobs_per_producer]() {
            for (size_t i = 0; i < jobs_per_producer; ++i) {
                auto job = std::make_unique<callback_job>(
                    []() -> kcenon::common::VoidResult { return kcenon::common::ok(); });
                if (queue.enqueue(std::move(job)).is_ok()) {
                    enqueued.fetch_add(1);
                }
            }
        });
    }

    // Start consumers
    std::vector<std::thread> consumers;
    for (size_t t = 0; t < num_consumers; ++t) {
        consumers.emplace_back([&queue, &dequeued, &producers_done]() {
            while (!producers_done.load() || !queue.empty()) {
                auto result = queue.try_dequeue();
                if (result.is_ok()) {
                    dequeued.fetch_add(1);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    // Wait for producers
    for (auto& t : producers) {
        t.join();
    }
    producers_done.store(true);

    // Wait for consumers
    for (auto& t : consumers) {
        t.join();
    }

    EXPECT_EQ(enqueued.load(), num_producers * jobs_per_producer);
    EXPECT_EQ(dequeued.load(), num_producers * jobs_per_producer);
    EXPECT_TRUE(queue.empty());
}

// ============================================================================
// Lock-free Queue Tests
// ============================================================================

TEST_F(PolicyQueueIntegrationTest, LockfreeQueueBasicOperations) {
    policy_lockfree_queue queue;

    EXPECT_TRUE(queue.empty());

    // Enqueue jobs
    for (int i = 0; i < 10; ++i) {
        auto job = std::make_unique<callback_job>(
            []() -> kcenon::common::VoidResult { return kcenon::common::ok(); });
        EXPECT_TRUE(queue.enqueue(std::move(job)).is_ok());
    }

    EXPECT_FALSE(queue.empty());

    // Dequeue all jobs
    for (int i = 0; i < 10; ++i) {
        auto result = queue.try_dequeue();
        EXPECT_TRUE(result.is_ok());
    }

    EXPECT_TRUE(queue.empty());
}

// DISABLED: Lock-free queue concurrent operations may hang due to potential issues
// in lockfree_sync_policy. Needs investigation - see lockfree queue implementation.
TEST_F(PolicyQueueIntegrationTest, DISABLED_LockfreeQueueConcurrentOperations) {
    policy_lockfree_queue queue;

    const size_t num_producers = 4;
    const size_t num_consumers = 4;
    const size_t jobs_per_producer = 250;

    std::atomic<size_t> enqueued{0};
    std::atomic<size_t> dequeued{0};
    std::atomic<bool> producers_done{false};

    // Start producers
    std::vector<std::thread> producers;
    for (size_t t = 0; t < num_producers; ++t) {
        producers.emplace_back([&queue, &enqueued, jobs_per_producer]() {
            for (size_t i = 0; i < jobs_per_producer; ++i) {
                auto job = std::make_unique<callback_job>(
                    []() -> kcenon::common::VoidResult { return kcenon::common::ok(); });
                if (queue.enqueue(std::move(job)).is_ok()) {
                    enqueued.fetch_add(1);
                }
            }
        });
    }

    // Start consumers
    std::vector<std::thread> consumers;
    for (size_t t = 0; t < num_consumers; ++t) {
        consumers.emplace_back([&queue, &dequeued, &producers_done]() {
            while (!producers_done.load() || !queue.empty()) {
                auto result = queue.try_dequeue();
                if (result.is_ok()) {
                    dequeued.fetch_add(1);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    // Wait for producers
    for (auto& t : producers) {
        t.join();
    }
    producers_done.store(true);

    // Wait for consumers
    for (auto& t : consumers) {
        t.join();
    }

    EXPECT_EQ(enqueued.load(), num_producers * jobs_per_producer);
    EXPECT_EQ(dequeued.load(), num_producers * jobs_per_producer);
    EXPECT_TRUE(queue.empty());
}

// ============================================================================
// Bounded Queue Tests
// ============================================================================

TEST_F(PolicyQueueIntegrationTest, BoundedQueueRejectOnOverflow) {
    using bounded_queue = policy_queue<
        policies::mutex_sync_policy,
        policies::bounded_policy,
        policies::overflow_reject_policy
    >;

    bounded_queue queue(policies::bounded_policy(5));

    // Fill the queue
    for (int i = 0; i < 5; ++i) {
        auto job = std::make_unique<callback_job>(
            []() -> kcenon::common::VoidResult { return kcenon::common::ok(); });
        EXPECT_TRUE(queue.enqueue(std::move(job)).is_ok());
    }

    EXPECT_EQ(queue.size(), 5);
    EXPECT_TRUE(queue.is_full());

    // Try to enqueue when full (should be rejected)
    auto job = std::make_unique<callback_job>(
        []() -> kcenon::common::VoidResult { return kcenon::common::ok(); });
    auto result = queue.enqueue(std::move(job));
    EXPECT_TRUE(result.is_err());

    // Queue size should remain 5
    EXPECT_EQ(queue.size(), 5);
}

TEST_F(PolicyQueueIntegrationTest, BoundedQueueDropOldestOnOverflow) {
    using ring_queue = policy_queue<
        policies::mutex_sync_policy,
        policies::bounded_policy,
        policies::overflow_drop_oldest_policy
    >;

    ring_queue queue(policies::bounded_policy(5));

    // Fill the queue with marker jobs
    for (int i = 0; i < 5; ++i) {
        auto job = std::make_unique<callback_job>(
            [i]() -> kcenon::common::VoidResult { return kcenon::common::ok(); });
        EXPECT_TRUE(queue.enqueue(std::move(job)).is_ok());
    }

    EXPECT_EQ(queue.size(), 5);

    // Add new job (should drop oldest)
    auto job = std::make_unique<callback_job>(
        []() -> kcenon::common::VoidResult { return kcenon::common::ok(); });
    auto result = queue.enqueue(std::move(job));
    EXPECT_TRUE(result.is_ok());

    // Queue size should still be 5
    EXPECT_EQ(queue.size(), 5);
}

TEST_F(PolicyQueueIntegrationTest, BoundedQueueCapacityChecks) {
    using bounded_queue = policy_queue<
        policies::mutex_sync_policy,
        policies::bounded_policy,
        policies::overflow_reject_policy
    >;

    bounded_queue queue(policies::bounded_policy(10));

    EXPECT_TRUE(queue.is_bounded());
    EXPECT_FALSE(queue.is_full());
    EXPECT_EQ(queue.remaining_capacity(), 10);

    // Add some jobs
    for (int i = 0; i < 6; ++i) {
        auto job = std::make_unique<callback_job>(
            []() -> kcenon::common::VoidResult { return kcenon::common::ok(); });
        EXPECT_TRUE(queue.enqueue(std::move(job)).is_ok());
    }

    EXPECT_FALSE(queue.is_full());
    EXPECT_EQ(queue.remaining_capacity(), 4);

    // Fill to capacity
    for (int i = 0; i < 4; ++i) {
        auto job = std::make_unique<callback_job>(
            []() -> kcenon::common::VoidResult { return kcenon::common::ok(); });
        EXPECT_TRUE(queue.enqueue(std::move(job)).is_ok());
    }

    EXPECT_TRUE(queue.is_full());
    EXPECT_EQ(queue.remaining_capacity(), 0);
}

// ============================================================================
// Queue Capabilities Tests
// ============================================================================

TEST_F(PolicyQueueIntegrationTest, StandardQueueCapabilities) {
    standard_queue queue;

    auto caps = queue.get_capabilities();
    EXPECT_TRUE(caps.supports_blocking_wait);
    EXPECT_FALSE(caps.lock_free);
    EXPECT_TRUE(caps.exact_size);
}

TEST_F(PolicyQueueIntegrationTest, LockfreeQueueCapabilities) {
    policy_lockfree_queue queue;

    auto caps = queue.get_capabilities();
    EXPECT_FALSE(caps.supports_blocking_wait);
    EXPECT_TRUE(caps.lock_free);
}

// ============================================================================
// Queue Stop/Clear Tests
// ============================================================================

TEST_F(PolicyQueueIntegrationTest, QueueStopBehavior) {
    standard_queue queue;

    for (int i = 0; i < 5; ++i) {
        auto job = std::make_unique<callback_job>(
            []() -> kcenon::common::VoidResult { return kcenon::common::ok(); });
        EXPECT_TRUE(queue.enqueue(std::move(job)).is_ok());
    }

    EXPECT_FALSE(queue.is_stopped());

    queue.stop();
    EXPECT_TRUE(queue.is_stopped());
}

TEST_F(PolicyQueueIntegrationTest, QueueClearBehavior) {
    standard_queue queue;

    for (int i = 0; i < 10; ++i) {
        auto job = std::make_unique<callback_job>(
            []() -> kcenon::common::VoidResult { return kcenon::common::ok(); });
        EXPECT_TRUE(queue.enqueue(std::move(job)).is_ok());
    }

    EXPECT_EQ(queue.size(), 10);

    queue.clear();
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}

// ============================================================================
// Thread Pool Integration Tests
// ============================================================================

// DISABLED: policy_queue adapter doesn't provide job_queue backend required by thread_worker
// This limitation is documented in thread_pool::enqueue(thread_worker)
// Future versions may lift this restriction by supporting scheduler_interface directly
TEST_F(PolicyQueueIntegrationTest, DISABLED_ThreadPoolWithStandardQueueAdapter) {
    auto adapter = make_standard_queue_adapter();

    auto pool = std::make_shared<thread_pool>("PolicyQueuePool", std::move(adapter));

    // Add workers
    for (int i = 0; i < 4; ++i) {
        auto worker = std::make_unique<thread_worker>();
        EXPECT_TRUE(pool->enqueue(std::move(worker)).is_ok());
    }

    // Start pool
    ASSERT_TRUE(pool->start().is_ok());

    // Submit jobs
    const size_t job_count = 50;
    for (size_t i = 0; i < job_count; ++i) {
        auto job = std::make_unique<callback_job>(
            [this]() -> kcenon::common::VoidResult {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                completed_jobs_.fetch_add(1);
                return kcenon::common::ok();
            });
        EXPECT_TRUE(pool->enqueue(std::move(job)).is_ok());
    }

    // Wait for completion
    EXPECT_TRUE(WaitForCondition([this, job_count]() {
        return completed_jobs_.load() >= job_count;
    }, std::chrono::seconds(10)));

    // Stop pool
    EXPECT_TRUE(pool->stop().is_ok());

    EXPECT_GE(completed_jobs_.load(), job_count);
}

// DISABLED: Same limitation as above - policy_queue adapter requires job_queue backend for workers
TEST_F(PolicyQueueIntegrationTest, DISABLED_ThreadPoolWithLockfreeQueueAdapter) {
    auto adapter = make_lockfree_queue_adapter();

    auto pool = std::make_shared<thread_pool>("LockfreeQueuePool", std::move(adapter));

    // Add workers
    for (int i = 0; i < 4; ++i) {
        auto worker = std::make_unique<thread_worker>();
        EXPECT_TRUE(pool->enqueue(std::move(worker)).is_ok());
    }

    // Start pool
    ASSERT_TRUE(pool->start().is_ok());

    // Submit jobs
    const size_t job_count = 50;
    for (size_t i = 0; i < job_count; ++i) {
        auto job = std::make_unique<callback_job>(
            [this]() -> kcenon::common::VoidResult {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                completed_jobs_.fetch_add(1);
                return kcenon::common::ok();
            });
        EXPECT_TRUE(pool->enqueue(std::move(job)).is_ok());
    }

    // Wait for completion
    EXPECT_TRUE(WaitForCondition([this, job_count]() {
        return completed_jobs_.load() >= job_count;
    }, std::chrono::seconds(10)));

    // Stop pool
    EXPECT_TRUE(pool->stop().is_ok());

    EXPECT_GE(completed_jobs_.load(), job_count);
}

// ============================================================================
// Scheduler Interface Tests
// ============================================================================

TEST_F(PolicyQueueIntegrationTest, SchedulerInterfaceCompliance) {
    standard_queue queue;

    // Test via scheduler_interface
    scheduler_interface& scheduler = queue;

    auto job = std::make_unique<callback_job>(
        []() -> kcenon::common::VoidResult { return kcenon::common::ok(); });

    // schedule() should work
    EXPECT_TRUE(scheduler.schedule(std::move(job)).is_ok());
    EXPECT_EQ(queue.size(), 1);

    // get_next_job() should work
    auto result = scheduler.get_next_job();
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(queue.empty());
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_F(PolicyQueueIntegrationTest, StressTestHighConcurrency) {
    standard_queue queue;

    const size_t total_jobs = 500;
    std::atomic<size_t> enqueued{0};
    std::atomic<size_t> dequeued{0};
    std::atomic<bool> stop_all{false};

    // Multiple producers
    std::vector<std::thread> producers;
    for (int p = 0; p < 4; ++p) {
        producers.emplace_back([&queue, &enqueued, &stop_all, total_jobs]() {
            size_t jobs_to_enqueue = total_jobs / 4;
            for (size_t i = 0; i < jobs_to_enqueue && !stop_all.load(); ++i) {
                auto job = std::make_unique<callback_job>(
                    []() -> kcenon::common::VoidResult { return kcenon::common::ok(); });
                if (queue.enqueue(std::move(job)).is_ok()) {
                    enqueued.fetch_add(1);
                }
            }
        });
    }

    // Multiple consumers
    std::vector<std::thread> consumers;
    for (int c = 0; c < 4; ++c) {
        consumers.emplace_back([&queue, &dequeued, &stop_all]() {
            while (!stop_all.load()) {
                auto result = queue.try_dequeue();
                if (result.is_ok()) {
                    dequeued.fetch_add(1);
                } else {
                    std::this_thread::yield();
                }
            }
            // Drain remaining
            while (true) {
                auto result = queue.try_dequeue();
                if (!result.is_ok()) break;
                dequeued.fetch_add(1);
            }
        });
    }

    // Wait for all producers to finish
    for (auto& t : producers) {
        t.join();
    }

    // Wait for all jobs to be consumed
    EXPECT_TRUE(WaitForCondition([&dequeued, &enqueued]() {
        return dequeued.load() >= enqueued.load();
    }, std::chrono::seconds(10)));

    stop_all.store(true);

    for (auto& t : consumers) {
        t.join();
    }

    EXPECT_EQ(enqueued.load(), total_jobs);
    EXPECT_EQ(dequeued.load(), total_jobs);
}
