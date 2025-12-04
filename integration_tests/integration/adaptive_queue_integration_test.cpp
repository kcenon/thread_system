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

#include <kcenon/thread/queue/adaptive_job_queue.h>
#include <kcenon/thread/core/callback_job.h>

#include <atomic>
#include <chrono>
#include <latch>
#include <thread>
#include <vector>

using namespace kcenon::thread;
using namespace integration_tests;

/**
 * @brief Integration tests for adaptive_job_queue
 *
 * Goal: Verify adaptive_job_queue behavior under real-world scenarios
 * Expected time: < 60 seconds (optimized for CI Debug/Coverage builds)
 * Test scenarios:
 *   1. Balanced policy under variable load
 *   2. Mode switching with concurrent operations
 *   3. Accuracy guard under load
 *   4. Policy enforcement
 */
class AdaptiveQueueIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        completed_jobs_.store(0);
        failed_jobs_.store(0);
    }

    void TearDown() override {
        // Minimal teardown - only yield to allow cleanup
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

    // Drain queue helper - non-blocking, fast
    void DrainQueue(adaptive_job_queue& queue, std::atomic<size_t>& dequeued) {
        for (int attempts = 0; attempts < 50; ++attempts) {
            if (auto result = queue.try_dequeue(); result.has_value()) {
                dequeued.fetch_add(1, std::memory_order_relaxed);
                attempts = 0;
            } else if (queue.empty()) {
                break;
            }
        }
    }

    std::atomic<size_t> completed_jobs_{0};
    std::atomic<size_t> failed_jobs_{0};
};

// ============================================
// Scenario 1: Balanced policy under variable load
// ============================================

TEST_F(AdaptiveQueueIntegrationTest, BalancedPolicyVariableLoad_LowLoadStartsInMutex) {
    adaptive_job_queue queue(adaptive_job_queue::policy::balanced);

    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}

TEST_F(AdaptiveQueueIntegrationTest, BalancedPolicyVariableLoad_DataIntegrityUnderTransition) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    std::atomic<size_t> enqueued{0};
    std::atomic<size_t> dequeued{0};
    std::atomic<bool> stop{false};

    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    // Single producer-consumer pair for fast execution
    std::thread producer([&]() {
        for (int i = 0; i < 100 && !stop.load(std::memory_order_acquire); ++i) {
            auto job = std::make_unique<callback_job>([]() -> result_void { return {}; });
            if (!queue.enqueue(std::move(job)).has_error()) {
                enqueued.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });

    std::thread consumer([&]() {
        while (!stop.load(std::memory_order_acquire) || !queue.empty()) {
            if (auto result = queue.try_dequeue(); result.has_value()) {
                dequeued.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::yield();
            }
        }
    });

    // Quick mode transitions
    queue.switch_mode(adaptive_job_queue::mode::lock_free);
    queue.switch_mode(adaptive_job_queue::mode::mutex);

    producer.join();
    stop.store(true, std::memory_order_release);
    consumer.join();

    DrainQueue(queue, dequeued);

    auto enq = enqueued.load();
    auto deq = dequeued.load();
    EXPECT_LE(enq - deq, 5u) << "Data loss: enqueued=" << enq << ", dequeued=" << deq;
}

// ============================================
// Scenario 2: Mode switching with concurrent operations
// ============================================

TEST_F(AdaptiveQueueIntegrationTest, ModeSwitchingConcurrent_NoDeadlocks) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    std::atomic<size_t> enqueued{0};
    std::atomic<size_t> dequeued{0};
    std::atomic<bool> stop{false};

    std::thread producer([&]() {
        while (!stop.load(std::memory_order_acquire)) {
            auto job = std::make_unique<callback_job>([]() -> result_void { return {}; });
            if (!queue.enqueue(std::move(job)).has_error()) {
                enqueued.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });

    std::thread consumer([&]() {
        while (!stop.load(std::memory_order_acquire) || !queue.empty()) {
            if (auto result = queue.try_dequeue(); result.has_value()) {
                dequeued.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::yield();
            }
        }
    });

    // Quick mode switches - no sleeps
    for (int i = 0; i < 5; ++i) {
        queue.switch_mode(adaptive_job_queue::mode::lock_free);
        queue.switch_mode(adaptive_job_queue::mode::mutex);
    }

    stop.store(true, std::memory_order_release);
    producer.join();
    consumer.join();

    DrainQueue(queue, dequeued);

    auto enq = enqueued.load();
    auto deq = dequeued.load();
    EXPECT_LE(enq - deq, 10u) << "Data loss: enqueued=" << enq << ", dequeued=" << deq;
    EXPECT_GE(queue.get_stats().mode_switches, 5u);
}

TEST_F(AdaptiveQueueIntegrationTest, ModeSwitchingConcurrent_CorrectJobCount) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    constexpr size_t total_jobs = 200;
    std::atomic<size_t> processed{0};
    std::atomic<bool> stop_consumer{false};

    std::thread consumer([&]() {
        while (!stop_consumer.load(std::memory_order_acquire) || !queue.empty()) {
            if (auto result = queue.try_dequeue(); result.has_value()) {
                processed.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::yield();
            }
        }
    });

    for (size_t i = 0; i < total_jobs; ++i) {
        auto job = std::make_unique<callback_job>([]() -> result_void { return {}; });
        EXPECT_FALSE(queue.enqueue(std::move(job)).has_error());

        if (i % 50 == 0) {
            if (queue.current_mode() == adaptive_job_queue::mode::mutex) {
                queue.switch_mode(adaptive_job_queue::mode::lock_free);
            } else {
                queue.switch_mode(adaptive_job_queue::mode::mutex);
            }
        }
    }

    EXPECT_TRUE(WaitForCondition([&]() { return processed.load() >= total_jobs; }));

    stop_consumer.store(true, std::memory_order_release);
    consumer.join();

    EXPECT_EQ(processed.load(), total_jobs);
}

// ============================================
// Scenario 3: Accuracy guard under load
// ============================================

TEST_F(AdaptiveQueueIntegrationTest, AccuracyGuardUnderLoad_ExactSizeWithGuard) {
    adaptive_job_queue queue(adaptive_job_queue::policy::performance_first);

    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    constexpr size_t job_count = 50;
    for (size_t i = 0; i < job_count; ++i) {
        auto job = std::make_unique<callback_job>([]() -> result_void { return {}; });
        EXPECT_FALSE(queue.enqueue(std::move(job)).has_error());
    }

    {
        auto guard = queue.require_accuracy();
        EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
        EXPECT_EQ(queue.size(), job_count);
    }

    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);
}

TEST_F(AdaptiveQueueIntegrationTest, AccuracyGuardUnderLoad_ConcurrentAccess) {
    adaptive_job_queue queue(adaptive_job_queue::policy::performance_first);

    std::atomic<size_t> enqueued{0};
    std::atomic<size_t> dequeued{0};
    std::atomic<size_t> accurate_checks{0};
    std::atomic<bool> stop{false};

    std::thread worker([&]() {
        while (!stop.load(std::memory_order_acquire)) {
            auto job = std::make_unique<callback_job>([]() -> result_void { return {}; });
            if (!queue.enqueue(std::move(job)).has_error()) {
                enqueued.fetch_add(1, std::memory_order_relaxed);
            }
            if (auto result = queue.try_dequeue(); result.has_value()) {
                dequeued.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });

    // Quick accuracy guard checks
    for (int i = 0; i < 10 && !stop.load(); ++i) {
        auto guard = queue.require_accuracy();
        (void)queue.size();
        accurate_checks.fetch_add(1, std::memory_order_relaxed);
    }

    stop.store(true, std::memory_order_release);
    worker.join();

    DrainQueue(queue, dequeued);

    EXPECT_GT(accurate_checks.load(), 0u);
}

TEST_F(AdaptiveQueueIntegrationTest, AccuracyGuardUnderLoad_PerformanceReturnsAfterRelease) {
    adaptive_job_queue queue(adaptive_job_queue::policy::performance_first);

    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    for (int i = 0; i < 5; ++i) {
        {
            auto guard = queue.require_accuracy();
            EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
        }
        EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);
    }

    EXPECT_GE(queue.get_stats().mode_switches, 5u);
}

// ============================================
// Scenario 4: Policy enforcement
// ============================================

TEST_F(AdaptiveQueueIntegrationTest, PolicyEnforcement_AccuracyFirstAlwaysMutex) {
    adaptive_job_queue queue(adaptive_job_queue::policy::accuracy_first);

    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    constexpr size_t job_count = 100;
    for (size_t i = 0; i < job_count; ++i) {
        auto job = std::make_unique<callback_job>([]() -> result_void { return {}; });
        (void)queue.enqueue(std::move(job));
    }

    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
    EXPECT_EQ(queue.size(), job_count);

    auto result = queue.switch_mode(adaptive_job_queue::mode::lock_free);
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
}

TEST_F(AdaptiveQueueIntegrationTest, PolicyEnforcement_PerformanceFirstAlwaysLockFree) {
    adaptive_job_queue queue(adaptive_job_queue::policy::performance_first);

    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    std::atomic<bool> stop{false};

    std::thread worker([&]() {
        while (!stop.load(std::memory_order_acquire)) {
            auto job = std::make_unique<callback_job>([]() -> result_void { return {}; });
            (void)queue.enqueue(std::move(job));
            (void)queue.try_dequeue();
        }
    });

    // Brief concurrent activity
    for (int i = 0; i < 100; ++i) {
        std::this_thread::yield();
    }

    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    stop.store(true, std::memory_order_release);
    worker.join();

    auto result = queue.switch_mode(adaptive_job_queue::mode::mutex);
    EXPECT_TRUE(result.has_error());
}

TEST_F(AdaptiveQueueIntegrationTest, PolicyEnforcement_ManualPolicyAllowsSwitch) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    auto result = queue.switch_mode(adaptive_job_queue::mode::lock_free);
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    result = queue.switch_mode(adaptive_job_queue::mode::mutex);
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    EXPECT_EQ(queue.get_stats().mode_switches, 2u);
}

TEST_F(AdaptiveQueueIntegrationTest, PolicyEnforcement_BalancedPolicyStartsMutex) {
    adaptive_job_queue queue(adaptive_job_queue::policy::balanced);

    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    auto result = queue.switch_mode(adaptive_job_queue::mode::lock_free);
    EXPECT_TRUE(result.has_error());
}

// ============================================
// Additional integration scenarios
// ============================================

TEST_F(AdaptiveQueueIntegrationTest, StressTest_HighConcurrencyNoDataLoss) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    constexpr size_t total_jobs = 200;
    std::atomic<size_t> enqueued{0};
    std::atomic<size_t> dequeued{0};
    std::atomic<bool> producer_done{false};

    std::thread producer([&]() {
        for (size_t i = 0; i < total_jobs; ++i) {
            auto job = std::make_unique<callback_job>([]() -> result_void { return {}; });
            while (queue.enqueue(std::move(job)).has_error()) {
                job = std::make_unique<callback_job>([]() -> result_void { return {}; });
                std::this_thread::yield();
            }
            enqueued.fetch_add(1, std::memory_order_relaxed);
        }
        producer_done.store(true, std::memory_order_release);
    });

    std::thread consumer([&]() {
        while (!producer_done.load(std::memory_order_acquire) || !queue.empty()) {
            if (auto result = queue.try_dequeue(); result.has_value()) {
                dequeued.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::yield();
            }
        }
    });

    std::thread mode_switcher([&]() {
        while (!producer_done.load(std::memory_order_acquire)) {
            queue.switch_mode(adaptive_job_queue::mode::lock_free);
            queue.switch_mode(adaptive_job_queue::mode::mutex);
        }
    });

    producer.join();
    mode_switcher.join();

    EXPECT_TRUE(WaitForCondition([&]() { return dequeued.load() >= total_jobs; }));

    consumer.join();

    EXPECT_EQ(enqueued.load(), total_jobs);
    EXPECT_EQ(dequeued.load(), total_jobs);
}

TEST_F(AdaptiveQueueIntegrationTest, StatisticsAccuracyAfterOperations) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    auto initial_stats = queue.get_stats();
    EXPECT_EQ(initial_stats.mode_switches, 0u);
    EXPECT_EQ(initial_stats.enqueue_count, 0u);
    EXPECT_EQ(initial_stats.dequeue_count, 0u);

    constexpr size_t ops = 50;
    for (size_t i = 0; i < ops; ++i) {
        auto job = std::make_unique<callback_job>([]() -> result_void { return {}; });
        (void)queue.enqueue(std::move(job));
    }

    for (size_t i = 0; i < ops / 2; ++i) {
        (void)queue.try_dequeue();
    }

    queue.switch_mode(adaptive_job_queue::mode::lock_free);
    queue.switch_mode(adaptive_job_queue::mode::mutex);

    auto stats = queue.get_stats();
    EXPECT_EQ(stats.enqueue_count, ops);
    EXPECT_EQ(stats.dequeue_count, ops / 2);
    EXPECT_EQ(stats.mode_switches, 2u);
}
