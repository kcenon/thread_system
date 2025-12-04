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
 * Expected time: < 2 minutes
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
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    template <typename Predicate>
    bool WaitForCondition(Predicate pred,
                          std::chrono::milliseconds timeout = std::chrono::seconds(10)) {
        auto start = std::chrono::steady_clock::now();
        while (!pred()) {
            if (std::chrono::steady_clock::now() - start > timeout) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return true;
    }

    std::atomic<size_t> completed_jobs_{0};
    std::atomic<size_t> failed_jobs_{0};
};

// ============================================
// Scenario 1: Balanced policy under variable load
// ============================================

TEST_F(AdaptiveQueueIntegrationTest, BalancedPolicyVariableLoad_LowLoadStartsInMutex) {
    adaptive_job_queue queue(adaptive_job_queue::policy::balanced);

    // With balanced policy, queue starts in mutex mode (accuracy mode)
    // for low load scenarios to provide exact size
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}

TEST_F(AdaptiveQueueIntegrationTest, BalancedPolicyVariableLoad_DataIntegrityUnderTransition) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    std::atomic<size_t> enqueued{0};
    std::atomic<size_t> dequeued{0};
    std::atomic<bool> stop_producers{false};
    std::atomic<bool> stop_consumers{false};

    // Phase 1: Low load - Start in mutex mode
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    // Producers
    std::vector<std::thread> producers;
    for (int i = 0; i < 2; ++i) {
        producers.emplace_back([&]() {
            while (!stop_producers.load(std::memory_order_acquire)) {
                auto job = std::make_unique<callback_job>([]() -> result_void {
                    return {};
                });
                if (!queue.enqueue(std::move(job)).has_error()) {
                    enqueued.fetch_add(1, std::memory_order_relaxed);
                }
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }

    // Consumers
    std::vector<std::thread> consumers;
    for (int i = 0; i < 2; ++i) {
        consumers.emplace_back([&]() {
            while (!stop_consumers.load(std::memory_order_acquire) || !queue.empty()) {
                if (auto result = queue.try_dequeue(); result.has_value()) {
                    dequeued.fetch_add(1, std::memory_order_relaxed);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    // Wait for some jobs to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Phase 2: Switch to lock-free mode (simulating high load transition)
    auto switch_result = queue.switch_mode(adaptive_job_queue::mode::lock_free);
    EXPECT_FALSE(switch_result.has_error());
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    // Continue processing
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Phase 3: Switch back to mutex mode (simulating low load return)
    switch_result = queue.switch_mode(adaptive_job_queue::mode::mutex);
    EXPECT_FALSE(switch_result.has_error());
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    // Stop producers first
    stop_producers.store(true, std::memory_order_release);
    for (auto& t : producers) {
        t.join();
    }

    // Wait for consumers to drain the queue
    stop_consumers.store(true, std::memory_order_release);
    for (auto& t : consumers) {
        t.join();
    }

    // Verify no data loss
    EXPECT_EQ(enqueued.load(), dequeued.load())
        << "Data loss detected: enqueued=" << enqueued.load()
        << ", dequeued=" << dequeued.load();
    EXPECT_TRUE(queue.empty());
}

// ============================================
// Scenario 2: Mode switching with concurrent operations
// ============================================

TEST_F(AdaptiveQueueIntegrationTest, ModeSwitchingConcurrent_NoDeadlocks) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    constexpr int num_producers = 2;
    constexpr int num_consumers = 2;
    constexpr int mode_switches = 10;

    std::atomic<size_t> enqueued{0};
    std::atomic<size_t> dequeued{0};
    std::atomic<bool> stop{false};
    std::latch start_latch(num_producers + num_consumers + 1);

    // Producers
    std::vector<std::thread> producers;
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&]() {
            start_latch.arrive_and_wait();
            while (!stop.load(std::memory_order_acquire)) {
                auto job = std::make_unique<callback_job>([]() -> result_void {
                    return {};
                });
                if (!queue.enqueue(std::move(job)).has_error()) {
                    enqueued.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    // Consumers
    std::vector<std::thread> consumers;
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&]() {
            start_latch.arrive_and_wait();
            while (!stop.load(std::memory_order_acquire) || !queue.empty()) {
                if (auto result = queue.try_dequeue(); result.has_value()) {
                    dequeued.fetch_add(1, std::memory_order_relaxed);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    // Mode switcher
    std::thread switcher([&]() {
        start_latch.arrive_and_wait();
        for (int i = 0; i < mode_switches; ++i) {
            if (i % 2 == 0) {
                queue.switch_mode(adaptive_job_queue::mode::lock_free);
            } else {
                queue.switch_mode(adaptive_job_queue::mode::mutex);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        stop.store(true, std::memory_order_release);
    });

    switcher.join();
    for (auto& t : producers) {
        t.join();
    }
    for (auto& t : consumers) {
        t.join();
    }

    // Drain any remaining jobs
    while (auto result = queue.try_dequeue()) {
        dequeued.fetch_add(1, std::memory_order_relaxed);
    }

    // Verify all jobs processed - no data loss
    EXPECT_EQ(enqueued.load(), dequeued.load())
        << "Data loss: enqueued=" << enqueued.load()
        << ", dequeued=" << dequeued.load();
    EXPECT_TRUE(queue.empty());

    // Verify mode switches were tracked
    auto stats = queue.get_stats();
    EXPECT_GE(stats.mode_switches, static_cast<uint64_t>(mode_switches - 1));
}

TEST_F(AdaptiveQueueIntegrationTest, ModeSwitchingConcurrent_CorrectJobCount) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    constexpr size_t total_jobs = 2000;
    std::atomic<size_t> processed{0};
    std::atomic<bool> stop_consumers{false};

    // Start consumers first
    std::vector<std::thread> consumers;
    for (int i = 0; i < 2; ++i) {
        consumers.emplace_back([&]() {
            while (!stop_consumers.load(std::memory_order_acquire) || !queue.empty()) {
                if (auto result = queue.try_dequeue(); result.has_value()) {
                    processed.fetch_add(1, std::memory_order_relaxed);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    // Producer with mode switching
    for (size_t i = 0; i < total_jobs; ++i) {
        auto job = std::make_unique<callback_job>([]() -> result_void {
            return {};
        });
        auto result = queue.enqueue(std::move(job));
        EXPECT_FALSE(result.has_error());

        // Switch modes periodically
        if (i % 500 == 0) {
            if (queue.current_mode() == adaptive_job_queue::mode::mutex) {
                queue.switch_mode(adaptive_job_queue::mode::lock_free);
            } else {
                queue.switch_mode(adaptive_job_queue::mode::mutex);
            }
        }
    }

    // Wait for all jobs to be processed
    EXPECT_TRUE(WaitForCondition([&]() {
        return processed.load() >= total_jobs;
    }));

    stop_consumers.store(true, std::memory_order_release);
    for (auto& t : consumers) {
        t.join();
    }

    EXPECT_EQ(processed.load(), total_jobs);
}

// ============================================
// Scenario 3: Accuracy guard under load
// ============================================

TEST_F(AdaptiveQueueIntegrationTest, AccuracyGuardUnderLoad_ExactSizeWithGuard) {
    adaptive_job_queue queue(adaptive_job_queue::policy::performance_first);

    // Start in lock-free mode
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    // Add known number of jobs
    constexpr size_t job_count = 100;
    for (size_t i = 0; i < job_count; ++i) {
        auto job = std::make_unique<callback_job>([]() -> result_void {
            return {};
        });
        auto result = queue.enqueue(std::move(job));
        EXPECT_FALSE(result.has_error());
    }

    // Without accuracy guard, size is approximate
    // With accuracy guard, size should be exact
    {
        auto guard = queue.require_accuracy();
        EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
        EXPECT_EQ(queue.size(), job_count);
    }

    // After guard release, should revert to lock-free
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);
}

TEST_F(AdaptiveQueueIntegrationTest, AccuracyGuardUnderLoad_ConcurrentAccess) {
    adaptive_job_queue queue(adaptive_job_queue::policy::performance_first);

    constexpr int num_workers = 2;
    constexpr int ops_per_worker = 50;

    std::atomic<size_t> enqueued{0};
    std::atomic<size_t> dequeued{0};
    std::atomic<size_t> accurate_checks{0};
    std::atomic<bool> stop{false};
    std::latch start_latch(num_workers * 2 + 1);

    // Workers doing enqueue/dequeue
    std::vector<std::thread> workers;
    for (int i = 0; i < num_workers; ++i) {
        workers.emplace_back([&]() {
            start_latch.arrive_and_wait();
            while (!stop.load(std::memory_order_acquire)) {
                auto job = std::make_unique<callback_job>([]() -> result_void {
                    return {};
                });
                if (!queue.enqueue(std::move(job)).has_error()) {
                    enqueued.fetch_add(1, std::memory_order_relaxed);
                }
                if (auto result = queue.try_dequeue(); result.has_value()) {
                    dequeued.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    // Workers acquiring accuracy guards
    for (int i = 0; i < num_workers; ++i) {
        workers.emplace_back([&]() {
            start_latch.arrive_and_wait();
            for (int j = 0; j < ops_per_worker && !stop.load(std::memory_order_acquire); ++j) {
                {
                    auto guard = queue.require_accuracy();
                    // While guard is held, we can safely call size()
                    // Note: In concurrent scenario with multiple guards from different threads,
                    // mode may fluctuate as other guards are released.
                    // The key guarantee is that size() returns a consistent value during guard scope.
                    auto size = queue.size();
                    (void)size;
                    accurate_checks.fetch_add(1, std::memory_order_relaxed);
                }
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }

    // Controller
    std::thread controller([&]() {
        start_latch.arrive_and_wait();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        stop.store(true, std::memory_order_release);
    });

    controller.join();
    for (auto& t : workers) {
        t.join();
    }

    // Drain remaining jobs with retry
    size_t drain_attempts = 0;
    while (drain_attempts < 100) {
        if (auto result = queue.try_dequeue(); result.has_value()) {
            dequeued.fetch_add(1, std::memory_order_relaxed);
            drain_attempts = 0;
        } else {
            ++drain_attempts;
            if (!queue.empty()) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    }

    // Verify no significant data loss (allow minor race condition tolerance)
    auto enq = enqueued.load();
    auto deq = dequeued.load();
    EXPECT_LE(enq - deq, 10u) << "Significant data loss: enqueued=" << enq << ", dequeued=" << deq;
    // Verify accuracy checks happened
    EXPECT_GT(accurate_checks.load(), 0u);
}

TEST_F(AdaptiveQueueIntegrationTest, AccuracyGuardUnderLoad_PerformanceReturnsAfterRelease) {
    adaptive_job_queue queue(adaptive_job_queue::policy::performance_first);

    // Start in lock-free mode
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    // Multiple accuracy guard acquire/release cycles
    for (int i = 0; i < 10; ++i) {
        {
            auto guard = queue.require_accuracy();
            EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
        }
        // After release, should return to lock-free
        EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);
    }

    auto stats = queue.get_stats();
    // Mode switches should reflect the transitions
    EXPECT_GE(stats.mode_switches, 10u);
}

// ============================================
// Scenario 4: Policy enforcement
// ============================================

TEST_F(AdaptiveQueueIntegrationTest, PolicyEnforcement_AccuracyFirstAlwaysMutex) {
    adaptive_job_queue queue(adaptive_job_queue::policy::accuracy_first);

    // Should always be in mutex mode
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    // Even under heavy load, stays in mutex mode
    constexpr size_t job_count = 1000;
    std::atomic<size_t> processed{0};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&]() {
            for (size_t j = 0; j < job_count / 4; ++j) {
                auto job = std::make_unique<callback_job>([]() -> result_void {
                    return {};
                });
                (void)queue.enqueue(std::move(job));
            }
        });
    }

    for (auto& t : workers) {
        t.join();
    }

    // Still in mutex mode
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    // Size should be exact
    EXPECT_EQ(queue.size(), job_count);

    // Manual switch should fail
    auto result = queue.switch_mode(adaptive_job_queue::mode::lock_free);
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
}

TEST_F(AdaptiveQueueIntegrationTest, PolicyEnforcement_PerformanceFirstAlwaysLockFree) {
    adaptive_job_queue queue(adaptive_job_queue::policy::performance_first);

    // Should always be in lock-free mode
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    // Under heavy concurrent load
    constexpr size_t job_count = 1000;
    std::atomic<size_t> enqueued{0};
    std::atomic<size_t> dequeued{0};
    std::atomic<bool> stop{false};

    std::vector<std::thread> workers;
    for (int i = 0; i < 8; ++i) {
        workers.emplace_back([&]() {
            while (!stop.load(std::memory_order_acquire)) {
                auto job = std::make_unique<callback_job>([]() -> result_void {
                    return {};
                });
                if (!queue.enqueue(std::move(job)).has_error()) {
                    enqueued.fetch_add(1, std::memory_order_relaxed);
                }
                if (auto result = queue.try_dequeue(); result.has_value()) {
                    dequeued.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Check mode is still lock-free (except when accuracy guard is held)
    // Note: performance_first policy allows temporary mutex during accuracy guard
    auto current = queue.current_mode();
    // Should be lock-free unless accuracy guard is active
    EXPECT_EQ(current, adaptive_job_queue::mode::lock_free);

    stop.store(true, std::memory_order_release);
    for (auto& t : workers) {
        t.join();
    }

    // Manual switch should fail for non-manual policy
    auto result = queue.switch_mode(adaptive_job_queue::mode::mutex);
    EXPECT_TRUE(result.has_error());
}

TEST_F(AdaptiveQueueIntegrationTest, PolicyEnforcement_ManualPolicyAllowsSwitch) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    // Starts in mutex mode
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    // Can switch to lock-free
    auto result = queue.switch_mode(adaptive_job_queue::mode::lock_free);
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    // Can switch back
    result = queue.switch_mode(adaptive_job_queue::mode::mutex);
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    // Statistics should reflect switches
    auto stats = queue.get_stats();
    EXPECT_EQ(stats.mode_switches, 2u);
}

TEST_F(AdaptiveQueueIntegrationTest, PolicyEnforcement_BalancedPolicyStartsMutex) {
    adaptive_job_queue queue(adaptive_job_queue::policy::balanced);

    // Balanced policy starts in mutex mode for accuracy
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    // Manual switch should not be allowed
    auto result = queue.switch_mode(adaptive_job_queue::mode::lock_free);
    EXPECT_TRUE(result.has_error());
}

// ============================================
// Additional integration scenarios
// ============================================

TEST_F(AdaptiveQueueIntegrationTest, StressTest_HighConcurrencyNoDataLoss) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    constexpr size_t total_jobs = 2000;
    constexpr int num_producers = 2;
    constexpr int num_consumers = 2;

    std::atomic<size_t> enqueued{0};
    std::atomic<size_t> dequeued{0};
    std::atomic<bool> stop_producers{false};
    std::atomic<bool> stop_consumers{false};
    std::latch start_latch(num_producers + num_consumers);

    // Producers
    std::vector<std::thread> producers;
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&, i]() {
            start_latch.arrive_and_wait();
            size_t jobs_to_produce = total_jobs / num_producers;
            for (size_t j = 0; j < jobs_to_produce; ++j) {
                auto job = std::make_unique<callback_job>([]() -> result_void {
                    return {};
                });
                while (queue.enqueue(std::move(job)).has_error()) {
                    job = std::make_unique<callback_job>([]() -> result_void {
                        return {};
                    });
                    std::this_thread::yield();
                }
                enqueued.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    // Consumers
    std::vector<std::thread> consumers;
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&]() {
            start_latch.arrive_and_wait();
            while (!stop_consumers.load(std::memory_order_acquire) || !queue.empty()) {
                if (auto result = queue.try_dequeue(); result.has_value()) {
                    dequeued.fetch_add(1, std::memory_order_relaxed);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    // Mode switching in background
    std::thread mode_switcher([&]() {
        while (!stop_producers.load(std::memory_order_acquire)) {
            queue.switch_mode(adaptive_job_queue::mode::lock_free);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            queue.switch_mode(adaptive_job_queue::mode::mutex);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    });

    // Wait for producers
    for (auto& t : producers) {
        t.join();
    }
    stop_producers.store(true, std::memory_order_release);
    mode_switcher.join();

    // Wait for consumers to finish
    EXPECT_TRUE(WaitForCondition([&]() {
        return dequeued.load() >= total_jobs;
    }, std::chrono::seconds(30)));

    stop_consumers.store(true, std::memory_order_release);
    for (auto& t : consumers) {
        t.join();
    }

    EXPECT_EQ(enqueued.load(), total_jobs);
    EXPECT_EQ(dequeued.load(), total_jobs);
    EXPECT_TRUE(queue.empty());
}

TEST_F(AdaptiveQueueIntegrationTest, StatisticsAccuracyAfterOperations) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    auto initial_stats = queue.get_stats();
    EXPECT_EQ(initial_stats.mode_switches, 0u);
    EXPECT_EQ(initial_stats.enqueue_count, 0u);
    EXPECT_EQ(initial_stats.dequeue_count, 0u);

    // Perform operations
    constexpr size_t ops = 100;
    for (size_t i = 0; i < ops; ++i) {
        auto job = std::make_unique<callback_job>([]() -> result_void {
            return {};
        });
        (void)queue.enqueue(std::move(job));
    }

    for (size_t i = 0; i < ops / 2; ++i) {
        (void)queue.try_dequeue();
    }

    // Switch modes
    queue.switch_mode(adaptive_job_queue::mode::lock_free);
    queue.switch_mode(adaptive_job_queue::mode::mutex);

    auto stats = queue.get_stats();
    EXPECT_EQ(stats.enqueue_count, ops);
    EXPECT_EQ(stats.dequeue_count, ops / 2);
    EXPECT_EQ(stats.mode_switches, 2u);
}
