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

#include <kcenon/thread/queue/queue_factory.h>
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/core/job.h>
#include <kcenon/thread/interfaces/queue_capabilities_interface.h>

#include <atomic>
#include <chrono>
#include <latch>
#include <thread>
#include <vector>

using namespace kcenon::thread;
using namespace integration_tests;

/**
 * @brief Integration tests for queue_factory
 *
 * Goal: Verify requirements-based queue selection works correctly under real conditions
 * Expected time: < 60 seconds (optimized for CI Debug/Coverage builds)
 * Test scenarios:
 *   1. Requirements satisfaction under load
 *   2. Requirement conflicts (exact_size vs lock_free)
 *   3. Optimal selection based on environment
 *   4. Functional verification of factory-created queues
 */
class QueueFactoryIntegrationTest : public ::testing::Test {
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
            std::this_thread::yield();
        }
        return true;
    }

    std::atomic<size_t> completed_jobs_{0};
    std::atomic<size_t> failed_jobs_{0};
};

// ============================================
// Scenario 1: Requirements satisfaction under load
// ============================================

TEST_F(QueueFactoryIntegrationTest, RequirementsSatisfaction_ExactSizeUnderLoad) {
    queue_factory::requirements reqs;
    reqs.need_exact_size = true;

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    auto* caps_interface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_interface, nullptr);

    auto caps = caps_interface->get_capabilities();
    EXPECT_TRUE(caps.exact_size) << "Queue must have exact_size capability when requested";

    auto* job_q = dynamic_cast<job_queue*>(queue.get());
    ASSERT_NE(job_q, nullptr);

    constexpr size_t producer_count = 4;
    constexpr size_t jobs_per_producer = 50;
    constexpr size_t total_jobs = producer_count * jobs_per_producer;

    std::atomic<size_t> enqueued{0};
    std::atomic<size_t> dequeued{0};
    std::atomic<bool> stop_consumers{false};

    std::vector<std::thread> producers;
    for (size_t p = 0; p < producer_count; ++p) {
        producers.emplace_back([&]() {
            for (size_t i = 0; i < jobs_per_producer; ++i) {
                auto new_job = std::make_unique<callback_job>([&]() -> result_void {
                    completed_jobs_.fetch_add(1, std::memory_order_relaxed);
                    return {};
                });
                if (!job_q->enqueue(std::move(new_job)).has_error()) {
                    enqueued.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    std::vector<std::thread> consumers;
    for (size_t c = 0; c < 2; ++c) {
        consumers.emplace_back([&]() {
            while (!stop_consumers.load(std::memory_order_acquire) ||
                   dequeued.load() < enqueued.load()) {
                if (auto result = job_q->try_dequeue(); result.has_value()) {
                    (void)result.value()->do_work();
                    dequeued.fetch_add(1, std::memory_order_relaxed);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }

    stop_consumers.store(true, std::memory_order_release);

    EXPECT_TRUE(WaitForCondition([&]() {
        return dequeued.load() >= total_jobs;
    }, std::chrono::seconds(10)));

    for (auto& t : consumers) {
        t.join();
    }

    EXPECT_EQ(enqueued.load(), total_jobs);
    EXPECT_EQ(dequeued.load(), total_jobs);
    EXPECT_EQ(completed_jobs_.load(), total_jobs);
}

TEST_F(QueueFactoryIntegrationTest, RequirementsSatisfaction_LockFreeUnderLoad) {
    queue_factory::requirements reqs;
    reqs.prefer_lock_free = true;

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    auto* caps_interface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_interface, nullptr);

    auto caps = caps_interface->get_capabilities();
    EXPECT_TRUE(caps.lock_free) << "Queue must be lock-free when requested";

    auto* lockfree_q = dynamic_cast<lockfree_job_queue*>(queue.get());
    ASSERT_NE(lockfree_q, nullptr);

    constexpr size_t producer_count = 4;
    constexpr size_t jobs_per_producer = 100;
    constexpr size_t total_jobs = producer_count * jobs_per_producer;

    std::atomic<size_t> enqueued{0};
    std::atomic<size_t> dequeued{0};
    std::atomic<bool> stop_consumers{false};

    std::vector<std::thread> producers;
    for (size_t p = 0; p < producer_count; ++p) {
        producers.emplace_back([&]() {
            for (size_t i = 0; i < jobs_per_producer; ++i) {
                auto new_job = std::make_unique<callback_job>([&]() -> result_void {
                    completed_jobs_.fetch_add(1, std::memory_order_relaxed);
                    return {};
                });
                if (!lockfree_q->enqueue(std::move(new_job)).has_error()) {
                    enqueued.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    std::vector<std::thread> consumers;
    for (size_t c = 0; c < 2; ++c) {
        consumers.emplace_back([&]() {
            while (!stop_consumers.load(std::memory_order_acquire) ||
                   dequeued.load() < enqueued.load()) {
                if (auto result = lockfree_q->try_dequeue(); result.has_value()) {
                    (void)result.value()->do_work();
                    dequeued.fetch_add(1, std::memory_order_relaxed);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }

    stop_consumers.store(true, std::memory_order_release);

    EXPECT_TRUE(WaitForCondition([&]() {
        return dequeued.load() >= total_jobs;
    }, std::chrono::seconds(10)));

    for (auto& t : consumers) {
        t.join();
    }

    EXPECT_EQ(enqueued.load(), total_jobs);
    EXPECT_EQ(dequeued.load(), total_jobs);
}

TEST_F(QueueFactoryIntegrationTest, RequirementsSatisfaction_AtomicEmptyVerification) {
    queue_factory::requirements reqs;
    reqs.need_atomic_empty = true;

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    auto* caps_interface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_interface, nullptr);

    auto caps = caps_interface->get_capabilities();
    EXPECT_TRUE(caps.atomic_empty_check) << "Queue must have atomic_empty capability";

    auto* job_q = dynamic_cast<job_queue*>(queue.get());
    ASSERT_NE(job_q, nullptr);

    // Verify empty state consistency (sequential - no race conditions)
    EXPECT_TRUE(job_q->empty());
    EXPECT_EQ(job_q->size(), 0);

    constexpr size_t job_count = 100;
    for (size_t i = 0; i < job_count; ++i) {
        auto new_job = std::make_unique<callback_job>([]() -> result_void { return {}; });
        EXPECT_FALSE(job_q->enqueue(std::move(new_job)).has_error());
    }

    // Verify non-empty state consistency
    EXPECT_FALSE(job_q->empty());
    EXPECT_EQ(job_q->size(), job_count);

    // Dequeue all and verify state transitions
    for (size_t i = 0; i < job_count; ++i) {
        EXPECT_FALSE(job_q->empty()) << "Queue should not be empty at iteration " << i;
        auto result = job_q->try_dequeue();
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(job_q->size(), job_count - i - 1);
    }

    // Verify final empty state
    EXPECT_TRUE(job_q->empty());
    EXPECT_EQ(job_q->size(), 0);
}

TEST_F(QueueFactoryIntegrationTest, RequirementsSatisfaction_BatchOperations) {
    queue_factory::requirements reqs;
    reqs.need_batch_operations = true;

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    auto* caps_interface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_interface, nullptr);

    auto caps = caps_interface->get_capabilities();
    EXPECT_TRUE(caps.supports_batch) << "Queue must support batch operations";

    auto* job_q = dynamic_cast<job_queue*>(queue.get());
    ASSERT_NE(job_q, nullptr) << "Batch operations require job_queue";

    constexpr size_t batch_size = 50;
    std::vector<std::unique_ptr<job>> jobs;
    for (size_t i = 0; i < batch_size; ++i) {
        jobs.push_back(std::make_unique<callback_job>([&]() -> result_void {
            completed_jobs_.fetch_add(1, std::memory_order_relaxed);
            return {};
        }));
    }

    auto batch_result = job_q->enqueue_batch(std::move(jobs));
    EXPECT_FALSE(batch_result.has_error()) << "Batch enqueue should succeed";

    EXPECT_EQ(job_q->size(), batch_size);

    auto dequeued_jobs = job_q->dequeue_batch();
    EXPECT_EQ(dequeued_jobs.size(), batch_size);

    for (auto& dequeued_job : dequeued_jobs) {
        (void)dequeued_job->do_work();
    }

    EXPECT_EQ(completed_jobs_.load(), batch_size);
}

TEST_F(QueueFactoryIntegrationTest, RequirementsSatisfaction_BlockingWait) {
    queue_factory::requirements reqs;
    reqs.need_blocking_wait = true;

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    auto* caps_interface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_interface, nullptr);

    auto caps = caps_interface->get_capabilities();
    EXPECT_TRUE(caps.supports_blocking_wait) << "Queue must support blocking wait";

    auto* job_q = dynamic_cast<job_queue*>(queue.get());
    ASSERT_NE(job_q, nullptr);

    std::atomic<bool> job_received{false};
    std::atomic<bool> consumer_started{false};

    std::thread consumer([&]() {
        consumer_started.store(true, std::memory_order_release);
        auto result = job_q->dequeue();
        if (result.has_value()) {
            (void)result.value()->do_work();
            job_received.store(true, std::memory_order_release);
        }
    });

    EXPECT_TRUE(WaitForCondition([&]() {
        return consumer_started.load(std::memory_order_acquire);
    }));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(job_received.load()) << "Consumer should be blocked waiting";

    auto new_job = std::make_unique<callback_job>([&]() -> result_void {
        completed_jobs_.fetch_add(1, std::memory_order_relaxed);
        return {};
    });
    EXPECT_FALSE(job_q->enqueue(std::move(new_job)).has_error());

    EXPECT_TRUE(WaitForCondition([&]() {
        return job_received.load(std::memory_order_acquire);
    }, std::chrono::seconds(2)));

    consumer.join();

    EXPECT_TRUE(job_received.load());
    EXPECT_EQ(completed_jobs_.load(), 1u);
}

// ============================================
// Scenario 2: Requirement conflicts
// ============================================

TEST_F(QueueFactoryIntegrationTest, RequirementConflicts_ExactSizePrioritizedOverLockFree) {
    queue_factory::requirements reqs;
    reqs.need_exact_size = true;
    reqs.prefer_lock_free = true;

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    auto* caps_interface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_interface, nullptr);

    auto caps = caps_interface->get_capabilities();

    EXPECT_TRUE(caps.exact_size) << "exact_size must be satisfied";
    EXPECT_FALSE(caps.lock_free) << "lock_free should be sacrificed for exact_size";

    auto* job_q = dynamic_cast<job_queue*>(queue.get());
    ASSERT_NE(job_q, nullptr);

    constexpr size_t job_count = 100;
    for (size_t i = 0; i < job_count; ++i) {
        auto new_job = std::make_unique<callback_job>([]() -> result_void { return {}; });
        EXPECT_FALSE(job_q->enqueue(std::move(new_job)).has_error());
    }

    EXPECT_EQ(job_q->size(), job_count) << "Exact size should be accurate";

    for (size_t i = 0; i < job_count; ++i) {
        auto result = job_q->try_dequeue();
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(job_q->size(), job_count - i - 1) << "Size should decrement accurately";
    }
}

TEST_F(QueueFactoryIntegrationTest, RequirementConflicts_AtomicEmptyPrioritizedOverLockFree) {
    queue_factory::requirements reqs;
    reqs.need_atomic_empty = true;
    reqs.prefer_lock_free = true;

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    auto* caps_interface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_interface, nullptr);

    auto caps = caps_interface->get_capabilities();

    EXPECT_TRUE(caps.atomic_empty_check) << "atomic_empty must be satisfied";
    EXPECT_FALSE(caps.lock_free) << "lock_free should be sacrificed";
}

TEST_F(QueueFactoryIntegrationTest, RequirementConflicts_BatchPrioritizedOverLockFree) {
    queue_factory::requirements reqs;
    reqs.need_batch_operations = true;
    reqs.prefer_lock_free = true;

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    auto* caps_interface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_interface, nullptr);

    auto caps = caps_interface->get_capabilities();

    EXPECT_TRUE(caps.supports_batch) << "batch operations must be satisfied";
    EXPECT_FALSE(caps.lock_free) << "lock_free should be sacrificed";
}

TEST_F(QueueFactoryIntegrationTest, RequirementConflicts_BlockingWaitPrioritizedOverLockFree) {
    queue_factory::requirements reqs;
    reqs.need_blocking_wait = true;
    reqs.prefer_lock_free = true;

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    auto* caps_interface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_interface, nullptr);

    auto caps = caps_interface->get_capabilities();

    EXPECT_TRUE(caps.supports_blocking_wait) << "blocking_wait must be satisfied";
    EXPECT_FALSE(caps.lock_free) << "lock_free should be sacrificed";
}

TEST_F(QueueFactoryIntegrationTest, RequirementConflicts_MultipleAccuracyRequirements) {
    queue_factory::requirements reqs;
    reqs.need_exact_size = true;
    reqs.need_atomic_empty = true;
    reqs.need_batch_operations = true;
    reqs.need_blocking_wait = true;
    reqs.prefer_lock_free = true;

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    auto* caps_interface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_interface, nullptr);

    auto caps = caps_interface->get_capabilities();

    EXPECT_TRUE(caps.exact_size);
    EXPECT_TRUE(caps.atomic_empty_check);
    EXPECT_TRUE(caps.supports_batch);
    EXPECT_TRUE(caps.supports_blocking_wait);
    EXPECT_FALSE(caps.lock_free) << "lock_free cannot coexist with accuracy requirements";
}

// ============================================
// Scenario 3: Optimal selection
// ============================================

TEST_F(QueueFactoryIntegrationTest, OptimalSelection_ReturnsValidQueue) {
    auto queue = queue_factory::create_optimal();
    ASSERT_NE(queue, nullptr);

    auto* caps_interface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_interface, nullptr);
}

TEST_F(QueueFactoryIntegrationTest, OptimalSelection_FunctionalUnderLoad) {
    auto queue = queue_factory::create_optimal();
    ASSERT_NE(queue, nullptr);

    constexpr size_t producer_count = 4;
    constexpr size_t jobs_per_producer = 100;
    constexpr size_t total_jobs = producer_count * jobs_per_producer;

    std::atomic<size_t> enqueued{0};
    std::atomic<size_t> dequeued{0};
    std::atomic<bool> stop_consumers{false};

    auto* job_q = dynamic_cast<job_queue*>(queue.get());
    auto* lockfree_q = dynamic_cast<lockfree_job_queue*>(queue.get());
    auto* adaptive_q = dynamic_cast<adaptive_job_queue*>(queue.get());

    std::vector<std::thread> producers;
    for (size_t p = 0; p < producer_count; ++p) {
        producers.emplace_back([&]() {
            for (size_t i = 0; i < jobs_per_producer; ++i) {
                auto new_job = std::make_unique<callback_job>([&]() -> result_void {
                    completed_jobs_.fetch_add(1, std::memory_order_relaxed);
                    return {};
                });
                if (!queue->schedule(std::move(new_job)).has_error()) {
                    enqueued.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    std::vector<std::thread> consumers;
    for (size_t c = 0; c < 2; ++c) {
        consumers.emplace_back([&, job_q, lockfree_q, adaptive_q]() {
            while (!stop_consumers.load(std::memory_order_acquire) ||
                   dequeued.load() < enqueued.load()) {
                bool got_job = false;
                if (job_q != nullptr) {
                    if (auto result = job_q->try_dequeue(); result.has_value()) {
                        (void)result.value()->do_work();
                        got_job = true;
                    }
                } else if (lockfree_q != nullptr) {
                    if (auto result = lockfree_q->try_dequeue(); result.has_value()) {
                        (void)result.value()->do_work();
                        got_job = true;
                    }
                } else if (adaptive_q != nullptr) {
                    if (auto result = adaptive_q->try_dequeue(); result.has_value()) {
                        (void)result.value()->do_work();
                        got_job = true;
                    }
                }
                if (got_job) {
                    dequeued.fetch_add(1, std::memory_order_relaxed);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }

    stop_consumers.store(true, std::memory_order_release);

    EXPECT_TRUE(WaitForCondition([&]() {
        return dequeued.load() >= total_jobs;
    }, std::chrono::seconds(10)));

    for (auto& t : consumers) {
        t.join();
    }

    EXPECT_EQ(enqueued.load(), total_jobs);
    EXPECT_EQ(dequeued.load(), total_jobs);
    EXPECT_EQ(completed_jobs_.load(), total_jobs);
}

TEST_F(QueueFactoryIntegrationTest, OptimalSelection_MatchesDocumentedCriteria) {
    auto queue = queue_factory::create_optimal();
    ASSERT_NE(queue, nullptr);

    const auto core_count = std::thread::hardware_concurrency();

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    constexpr bool strong_memory_model = true;
#else
    constexpr bool strong_memory_model = false;
#endif

    if (!strong_memory_model) {
        auto* job_q = dynamic_cast<job_queue*>(queue.get());
        EXPECT_NE(job_q, nullptr) << "Weak memory model should use job_queue";
    } else if (core_count <= 2) {
        auto* job_q = dynamic_cast<job_queue*>(queue.get());
        EXPECT_NE(job_q, nullptr) << "Low core count should use job_queue";
    } else {
        auto* adaptive_q = dynamic_cast<adaptive_job_queue*>(queue.get());
        EXPECT_NE(adaptive_q, nullptr) << "High core count with strong memory model should use adaptive_job_queue";
    }
}

// ============================================
// Scenario 4: Functional verification
// ============================================

TEST_F(QueueFactoryIntegrationTest, FunctionalVerification_StandardQueue) {
    auto queue = queue_factory::create_standard_queue();
    ASSERT_NE(queue, nullptr);

    scheduler_interface* scheduler = queue.get();
    ASSERT_NE(scheduler, nullptr);

    queue_capabilities_interface* caps_interface = queue.get();
    ASSERT_NE(caps_interface, nullptr);

    constexpr size_t job_count = 200;
    std::atomic<size_t> dequeued{0};

    for (size_t i = 0; i < job_count; ++i) {
        auto new_job = std::make_unique<callback_job>([&]() -> result_void {
            completed_jobs_.fetch_add(1, std::memory_order_relaxed);
            return {};
        });
        EXPECT_FALSE(scheduler->schedule(std::move(new_job)).has_error());
    }

    while (auto result = queue->try_dequeue()) {
        (void)result.value()->do_work();
        dequeued.fetch_add(1, std::memory_order_relaxed);
    }

    EXPECT_EQ(dequeued.load(), job_count);
    EXPECT_EQ(completed_jobs_.load(), job_count);
}

TEST_F(QueueFactoryIntegrationTest, FunctionalVerification_LockfreeQueue) {
    auto queue = queue_factory::create_lockfree_queue();
    ASSERT_NE(queue, nullptr);

    scheduler_interface* scheduler = queue.get();
    ASSERT_NE(scheduler, nullptr);

    queue_capabilities_interface* caps_interface = queue.get();
    ASSERT_NE(caps_interface, nullptr);

    constexpr size_t job_count = 200;
    std::atomic<size_t> dequeued{0};

    for (size_t i = 0; i < job_count; ++i) {
        auto new_job = std::make_unique<callback_job>([&]() -> result_void {
            completed_jobs_.fetch_add(1, std::memory_order_relaxed);
            return {};
        });
        EXPECT_FALSE(scheduler->schedule(std::move(new_job)).has_error());
    }

    while (auto result = queue->try_dequeue()) {
        (void)result.value()->do_work();
        dequeued.fetch_add(1, std::memory_order_relaxed);
    }

    EXPECT_EQ(dequeued.load(), job_count);
    EXPECT_EQ(completed_jobs_.load(), job_count);
}

TEST_F(QueueFactoryIntegrationTest, FunctionalVerification_AdaptiveQueue) {
    auto queue = queue_factory::create_adaptive_queue();
    ASSERT_NE(queue, nullptr);

    scheduler_interface* scheduler = queue.get();
    ASSERT_NE(scheduler, nullptr);

    queue_capabilities_interface* caps_interface = queue.get();
    ASSERT_NE(caps_interface, nullptr);

    constexpr size_t job_count = 200;
    std::atomic<size_t> dequeued{0};

    for (size_t i = 0; i < job_count; ++i) {
        auto new_job = std::make_unique<callback_job>([&]() -> result_void {
            completed_jobs_.fetch_add(1, std::memory_order_relaxed);
            return {};
        });
        EXPECT_FALSE(scheduler->schedule(std::move(new_job)).has_error());
    }

    while (auto result = queue->try_dequeue()) {
        (void)result.value()->do_work();
        dequeued.fetch_add(1, std::memory_order_relaxed);
    }

    EXPECT_EQ(dequeued.load(), job_count);
    EXPECT_EQ(completed_jobs_.load(), job_count);
}

TEST_F(QueueFactoryIntegrationTest, FunctionalVerification_RequirementsBasedQueue) {
    queue_factory::requirements reqs;
    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    auto* caps_interface = dynamic_cast<queue_capabilities_interface*>(queue.get());
    ASSERT_NE(caps_interface, nullptr);

    auto* adaptive_q = dynamic_cast<adaptive_job_queue*>(queue.get());
    ASSERT_NE(adaptive_q, nullptr) << "Default requirements should return adaptive_job_queue";

    constexpr size_t job_count = 200;
    std::atomic<size_t> dequeued{0};

    for (size_t i = 0; i < job_count; ++i) {
        auto new_job = std::make_unique<callback_job>([&]() -> result_void {
            completed_jobs_.fetch_add(1, std::memory_order_relaxed);
            return {};
        });
        EXPECT_FALSE(queue->schedule(std::move(new_job)).has_error());
    }

    while (auto result = adaptive_q->try_dequeue()) {
        (void)result.value()->do_work();
        dequeued.fetch_add(1, std::memory_order_relaxed);
    }

    EXPECT_EQ(dequeued.load(), job_count);
    EXPECT_EQ(completed_jobs_.load(), job_count);
}

TEST_F(QueueFactoryIntegrationTest, FunctionalVerification_ConcurrentLoad) {
    queue_factory::requirements reqs;
    reqs.need_exact_size = true;

    auto queue = queue_factory::create_for_requirements(reqs);
    ASSERT_NE(queue, nullptr);

    auto* job_q = dynamic_cast<job_queue*>(queue.get());
    ASSERT_NE(job_q, nullptr);

    constexpr size_t thread_count = 8;
    constexpr size_t ops_per_thread = 100;

    std::atomic<size_t> successful_enqueues{0};
    std::atomic<size_t> successful_dequeues{0};
    std::latch start_latch(thread_count);

    std::vector<std::thread> threads;
    for (size_t t = 0; t < thread_count; ++t) {
        threads.emplace_back([&, t]() {
            start_latch.arrive_and_wait();

            for (size_t i = 0; i < ops_per_thread; ++i) {
                if (t % 2 == 0) {
                    auto new_job = std::make_unique<callback_job>([&]() -> result_void {
                        completed_jobs_.fetch_add(1, std::memory_order_relaxed);
                        return {};
                    });
                    if (!job_q->enqueue(std::move(new_job)).has_error()) {
                        successful_enqueues.fetch_add(1, std::memory_order_relaxed);
                    }
                } else {
                    if (auto result = job_q->try_dequeue(); result.has_value()) {
                        (void)result.value()->do_work();
                        successful_dequeues.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    while (!job_q->empty()) {
        auto result = job_q->try_dequeue();
        if (result.has_value()) {
            (void)result.value()->do_work();
            successful_dequeues.fetch_add(1, std::memory_order_relaxed);
        }
    }

    EXPECT_EQ(successful_dequeues.load(), successful_enqueues.load());
    EXPECT_EQ(completed_jobs_.load(), successful_dequeues.load());
}
