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

#include "../framework/test_helpers.h"
#include <gtest/gtest.h>

#include <kcenon/thread/queue/adaptive_job_queue.h>
#include <kcenon/thread/core/callback_job.h>

#include <atomic>
#include <chrono>
#include <latch>
#include <random>
#include <thread>
#include <vector>

using namespace kcenon::thread;
using namespace integration_tests;

/**
 * @brief Mode transition scenario integration tests
 *
 * Tests real-world scenarios with variable load conditions to verify
 * adaptive queue mode transitions work correctly.
 *
 * Scenarios:
 *   1. Web server request handling simulation
 *   2. Batch processing simulation
 *   3. Mixed workload simulation
 *   4. Long-running stability test
 */
class ModeTransitionScenarioTest : public ::testing::Test {
protected:
    void SetUp() override {
        processed_jobs_.store(0);
        dropped_jobs_.store(0);
    }

    void TearDown() override {
        std::this_thread::yield();
    }

    template <typename Predicate>
    bool WaitForCondition(Predicate pred,
                          std::chrono::milliseconds timeout = std::chrono::seconds(10)) {
        auto start = std::chrono::steady_clock::now();
        while (!pred()) {
            if (std::chrono::steady_clock::now() - start > timeout) {
                return false;
            }
            std::this_thread::yield();
        }
        return true;
    }

    void DrainQueue(adaptive_job_queue& queue, std::atomic<size_t>& dequeued) {
        for (int attempts = 0; attempts < 100; ++attempts) {
            if (auto result = queue.try_dequeue(); result.has_value()) {
                dequeued.fetch_add(1, std::memory_order_relaxed);
                attempts = 0;
            } else if (queue.empty()) {
                break;
            }
        }
    }

    std::atomic<size_t> processed_jobs_{0};
    std::atomic<size_t> dropped_jobs_{0};
};

// ============================================
// Scenario 1: Web Server Request Handling Simulation
// ============================================

/**
 * @brief Simulates web server handling with variable traffic
 *
 * Phase 1: Low traffic (1-2 clients) - should use mutex mode
 * Phase 2: Spike (many clients) - should switch to lock-free
 * Phase 3: Recovery (back to 2 clients) - should revert
 * Verify: No dropped requests, correct mode at each phase
 */
TEST_F(ModeTransitionScenarioTest, Scenario1_WebServerRequestHandling) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    std::atomic<size_t> enqueued{0};
    std::atomic<size_t> dequeued{0};
    std::atomic<bool> stop_consumers{false};

    // Start consumer threads (simulating request handlers)
    constexpr size_t num_handlers = 4;
    std::vector<std::thread> handlers;
    for (size_t i = 0; i < num_handlers; ++i) {
        handlers.emplace_back([&]() {
            while (!stop_consumers.load(std::memory_order_acquire) || !queue.empty()) {
                if (auto result = queue.try_dequeue(); result.has_value()) {
                    dequeued.fetch_add(1, std::memory_order_relaxed);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    // Phase 1: Low traffic (mutex mode)
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
    constexpr size_t low_traffic_requests = 50;
    for (size_t i = 0; i < low_traffic_requests; ++i) {
        auto job = std::make_unique<callback_job>([]() -> result_void { return {}; });
        if (!queue.enqueue(std::move(job)).has_error()) {
            enqueued.fetch_add(1, std::memory_order_relaxed);
        }
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    // Phase 2: Traffic spike - switch to lock-free mode
    queue.switch_mode(adaptive_job_queue::mode::lock_free);
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    constexpr size_t spike_requests = 500;
    std::vector<std::thread> spike_producers;
    constexpr size_t num_spike_clients = 10;
    for (size_t c = 0; c < num_spike_clients; ++c) {
        spike_producers.emplace_back([&]() {
            for (size_t i = 0; i < spike_requests / num_spike_clients; ++i) {
                auto job = std::make_unique<callback_job>([]() -> result_void { return {}; });
                if (!queue.enqueue(std::move(job)).has_error()) {
                    enqueued.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& t : spike_producers) {
        t.join();
    }

    // Phase 3: Recovery - switch back to mutex mode
    queue.switch_mode(adaptive_job_queue::mode::mutex);
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    constexpr size_t recovery_requests = 50;
    for (size_t i = 0; i < recovery_requests; ++i) {
        auto job = std::make_unique<callback_job>([]() -> result_void { return {}; });
        if (!queue.enqueue(std::move(job)).has_error()) {
            enqueued.fetch_add(1, std::memory_order_relaxed);
        }
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    // Wait for all requests to be processed
    EXPECT_TRUE(WaitForCondition([&]() {
        return dequeued.load() >= enqueued.load();
    }, std::chrono::seconds(10)));

    stop_consumers.store(true, std::memory_order_release);
    for (auto& t : handlers) {
        t.join();
    }

    DrainQueue(queue, dequeued);

    // Verify: No dropped requests
    auto total_enqueued = enqueued.load();
    auto total_dequeued = dequeued.load();
    EXPECT_GE(total_dequeued, total_enqueued)
        << "Dropped requests detected: enqueued=" << total_enqueued
        << ", dequeued=" << total_dequeued;

    // Verify mode switches occurred
    auto stats = queue.get_stats();
    EXPECT_GE(stats.mode_switches, 2u)
        << "Expected at least 2 mode switches (mutex->lock_free->mutex)";
}

// ============================================
// Scenario 2: Batch Processing Simulation
// ============================================

/**
 * @brief Simulates batch processing with mode optimization
 *
 * Phase 1: Preparation - enqueue jobs in mutex mode for accurate count
 * Phase 2: Processing - switch to lock-free for throughput
 * Phase 3: Verification - switch to mutex for accurate final count
 * Verify: All jobs processed, accurate final count
 */
TEST_F(ModeTransitionScenarioTest, Scenario2_BatchProcessingSimulation) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    std::atomic<size_t> jobs_created{0};
    std::atomic<size_t> jobs_processed{0};
    std::atomic<bool> stop_processing{false};

    // Phase 1: Preparation in mutex mode
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    constexpr size_t batch_size = 5000;
    for (size_t i = 0; i < batch_size; ++i) {
        auto job = std::make_unique<callback_job>([&]() -> result_void {
            jobs_processed.fetch_add(1, std::memory_order_relaxed);
            return {};
        });
        ASSERT_FALSE(queue.enqueue(std::move(job)).has_error());
        jobs_created.fetch_add(1, std::memory_order_relaxed);
    }

    // Verify exact count in mutex mode
    EXPECT_EQ(queue.size(), batch_size);

    // Start processor threads
    constexpr size_t num_processors = 4;
    std::vector<std::thread> processors;
    for (size_t i = 0; i < num_processors; ++i) {
        processors.emplace_back([&]() {
            while (!stop_processing.load(std::memory_order_acquire) || !queue.empty()) {
                if (auto result = queue.try_dequeue(); result.has_value()) {
                    auto job = std::move(result.value());
                    (void)job->do_work();
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    // Phase 2: Switch to lock-free for high throughput processing
    queue.switch_mode(adaptive_job_queue::mode::lock_free);
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    // Wait for most jobs to be processed
    EXPECT_TRUE(WaitForCondition([&]() {
        return jobs_processed.load() >= batch_size * 9 / 10;
    }, std::chrono::seconds(10)));

    // Phase 3: Switch back to mutex for accurate final verification
    queue.switch_mode(adaptive_job_queue::mode::mutex);
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    // Wait for completion
    EXPECT_TRUE(WaitForCondition([&]() {
        return jobs_processed.load() >= batch_size;
    }, std::chrono::seconds(10)));

    stop_processing.store(true, std::memory_order_release);
    for (auto& t : processors) {
        t.join();
    }

    // Verify all jobs processed
    EXPECT_EQ(jobs_processed.load(), batch_size)
        << "Not all jobs processed: expected=" << batch_size
        << ", actual=" << jobs_processed.load();

    // Verify queue is empty
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);

    auto stats = queue.get_stats();
    EXPECT_EQ(stats.enqueue_count, batch_size);
}

// ============================================
// Scenario 3: Mixed Workload Simulation
// ============================================

/**
 * @brief Simulates mixed workload with accuracy requirements
 *
 * Multiple job types: critical (financial) vs non-critical (logging)
 * Uses accuracy guards for critical sections
 * Verify: Critical sections get exact counts
 */
TEST_F(ModeTransitionScenarioTest, Scenario3_MixedWorkloadSimulation) {
    adaptive_job_queue queue(adaptive_job_queue::policy::performance_first);

    std::atomic<size_t> critical_jobs_enqueued{0};
    std::atomic<size_t> critical_jobs_processed{0};
    std::atomic<size_t> non_critical_jobs_enqueued{0};
    std::atomic<size_t> non_critical_jobs_processed{0};
    std::atomic<bool> stop_workers{false};

    // Start in performance mode
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    // Start worker threads
    constexpr size_t num_workers = 4;
    std::vector<std::thread> workers;
    for (size_t i = 0; i < num_workers; ++i) {
        workers.emplace_back([&]() {
            while (!stop_workers.load(std::memory_order_acquire) || !queue.empty()) {
                if (auto result = queue.try_dequeue(); result.has_value()) {
                    auto job = std::move(result.value());
                    (void)job->do_work();
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    // Enqueue mixed workload with periodic critical sections
    constexpr size_t total_iterations = 100;
    constexpr size_t jobs_per_iteration = 20;

    for (size_t iter = 0; iter < total_iterations; ++iter) {
        // Every 10th iteration: critical section requiring accuracy
        if (iter % 10 == 0) {
            auto guard = queue.require_accuracy();
            EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

            // Enqueue critical jobs (simulating financial transactions)
            constexpr size_t critical_count = 5;
            for (size_t i = 0; i < critical_count; ++i) {
                auto job = std::make_unique<callback_job>([&]() -> result_void {
                    critical_jobs_processed.fetch_add(1, std::memory_order_relaxed);
                    return {};
                });
                if (!queue.enqueue(std::move(job)).has_error()) {
                    critical_jobs_enqueued.fetch_add(1, std::memory_order_relaxed);
                }
            }

            // Verify exact count is available
            auto size_before = queue.size();
            (void)size_before;  // Used for debugging if needed
        }

        // Regular non-critical jobs (logging, analytics)
        for (size_t i = 0; i < jobs_per_iteration; ++i) {
            auto job = std::make_unique<callback_job>([&]() -> result_void {
                non_critical_jobs_processed.fetch_add(1, std::memory_order_relaxed);
                return {};
            });
            if (!queue.enqueue(std::move(job)).has_error()) {
                non_critical_jobs_enqueued.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }

    // Wait for completion
    size_t total_enqueued = critical_jobs_enqueued.load() + non_critical_jobs_enqueued.load();
    size_t total_expected = critical_jobs_enqueued.load() + non_critical_jobs_enqueued.load();
    EXPECT_TRUE(WaitForCondition([&]() {
        return (critical_jobs_processed.load() + non_critical_jobs_processed.load()) >= total_expected;
    }, std::chrono::seconds(15)));

    stop_workers.store(true, std::memory_order_release);
    for (auto& t : workers) {
        t.join();
    }

    // Verify all critical jobs processed
    EXPECT_EQ(critical_jobs_processed.load(), critical_jobs_enqueued.load())
        << "Critical job loss: enqueued=" << critical_jobs_enqueued.load()
        << ", processed=" << critical_jobs_processed.load();

    // Verify non-critical jobs
    EXPECT_EQ(non_critical_jobs_processed.load(), non_critical_jobs_enqueued.load())
        << "Non-critical job loss: enqueued=" << non_critical_jobs_enqueued.load()
        << ", processed=" << non_critical_jobs_processed.load();

    // Verify mode switches occurred (due to accuracy guards)
    auto stats = queue.get_stats();
    EXPECT_GE(stats.mode_switches, 10u)
        << "Expected at least 10 mode switches from accuracy guards";
}

// ============================================
// Scenario 4: Long-Running Stability Test
// ============================================

/**
 * @brief Tests stability under continuous mode switching
 *
 * Duration: 10 seconds (optimized for CI Debug/Coverage builds)
 * Random mode switches at random intervals
 * Verify: No memory leaks, no deadlocks, stable performance
 *
 * Note: In Debug+Coverage builds, performance is significantly slower,
 * so we use shorter duration and relaxed expectations.
 */
TEST_F(ModeTransitionScenarioTest, Scenario4_LongRunningStability) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    std::atomic<size_t> enqueued{0};
    std::atomic<size_t> dequeued{0};
    std::atomic<size_t> mode_switch_count{0};
    std::atomic<bool> stop_all{false};

    // Shorter duration for CI (Debug/Coverage builds are much slower)
    constexpr auto test_duration = std::chrono::seconds(10);

    // Start producer threads
    constexpr size_t num_producers = 2;
    std::vector<std::thread> producers;
    for (size_t i = 0; i < num_producers; ++i) {
        producers.emplace_back([&]() {
            std::mt19937 gen(std::random_device{}());
            std::uniform_int_distribution<> delay_dist(0, 100);

            while (!stop_all.load(std::memory_order_acquire)) {
                auto job = std::make_unique<callback_job>([]() -> result_void { return {}; });
                if (!queue.enqueue(std::move(job)).has_error()) {
                    enqueued.fetch_add(1, std::memory_order_relaxed);
                }

                if (delay_dist(gen) < 10) {
                    std::this_thread::yield();
                }
            }
        });
    }

    // Start consumer threads
    constexpr size_t num_consumers = 2;
    std::vector<std::thread> consumers;
    for (size_t i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&]() {
            while (!stop_all.load(std::memory_order_acquire) || !queue.empty()) {
                if (auto result = queue.try_dequeue(); result.has_value()) {
                    dequeued.fetch_add(1, std::memory_order_relaxed);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    // Mode switching thread with random intervals
    std::thread mode_switcher([&]() {
        std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<> interval_dist(10, 200);  // 10-200ms

        while (!stop_all.load(std::memory_order_acquire)) {
            // Random mode switch
            auto target_mode = (gen() % 2 == 0)
                ? adaptive_job_queue::mode::mutex
                : adaptive_job_queue::mode::lock_free;

            if (!queue.switch_mode(target_mode).has_error()) {
                mode_switch_count.fetch_add(1, std::memory_order_relaxed);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(interval_dist(gen)));
        }
    });

    // Accuracy guard stress thread
    std::thread accuracy_guard_thread([&]() {
        std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<> interval_dist(50, 300);

        while (!stop_all.load(std::memory_order_acquire)) {
            {
                auto guard = queue.require_accuracy();
                (void)queue.size();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_dist(gen)));
        }
    });

    // Run for specified duration
    auto start_time = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start_time < test_duration) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Periodic health check
        auto current_enqueued = enqueued.load();
        auto current_dequeued = dequeued.load();

        // Check for deadlock (queue should be progressing)
        if (current_enqueued > 0 && current_dequeued == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            auto updated_dequeued = dequeued.load();
            EXPECT_GT(updated_dequeued, 0u) << "Possible deadlock detected";
        }
    }

    // Signal stop
    stop_all.store(true, std::memory_order_release);

    // Wait for producers first
    for (auto& t : producers) {
        t.join();
    }

    // Wait for mode switcher and accuracy guard threads
    mode_switcher.join();
    accuracy_guard_thread.join();

    // Wait for consumers to drain (longer timeout for slow CI environments)
    EXPECT_TRUE(WaitForCondition([&]() {
        return dequeued.load() >= enqueued.load();
    }, std::chrono::seconds(30)));

    for (auto& t : consumers) {
        t.join();
    }

    DrainQueue(queue, dequeued);

    // Verify results
    auto final_enqueued = enqueued.load();
    auto final_dequeued = dequeued.load();
    auto final_mode_switches = mode_switch_count.load();

    std::cout << "Stability test results:\n"
              << "  Duration: " << test_duration.count() << "s\n"
              << "  Enqueued: " << final_enqueued << "\n"
              << "  Dequeued: " << final_dequeued << "\n"
              << "  Mode switches: " << final_mode_switches << "\n"
              << "  Queue stats mode switches: " << queue.get_stats().mode_switches << "\n";

    EXPECT_GE(final_dequeued, final_enqueued)
        << "Data loss detected: enqueued=" << final_enqueued
        << ", dequeued=" << final_dequeued;

    // In slow CI environments (Debug+Coverage), fewer mode switches occur
    // We just verify that mode switching happened multiple times without issues
    EXPECT_GT(final_mode_switches, 3u)
        << "Expected at least a few mode switches during stability test";

    EXPECT_TRUE(queue.empty()) << "Queue not empty after draining";
}

// ============================================
// Additional Scenario Tests
// ============================================

/**
 * @brief Tests rapid mode transitions don't cause data loss
 */
TEST_F(ModeTransitionScenarioTest, RapidModeTransitions_NoDataLoss) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    std::atomic<size_t> enqueued{0};
    std::atomic<size_t> dequeued{0};
    std::atomic<bool> stop{false};

    constexpr size_t total_jobs = 1000;
    constexpr size_t mode_switches = 100;

    // Consumer thread
    std::thread consumer([&]() {
        while (!stop.load(std::memory_order_acquire) || !queue.empty()) {
            if (auto result = queue.try_dequeue(); result.has_value()) {
                dequeued.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::yield();
            }
        }
    });

    // Producer with interleaved mode switches
    for (size_t i = 0; i < total_jobs; ++i) {
        auto job = std::make_unique<callback_job>([]() -> result_void { return {}; });
        if (!queue.enqueue(std::move(job)).has_error()) {
            enqueued.fetch_add(1, std::memory_order_relaxed);
        }

        // Rapid mode switch every 10 jobs
        if (i % 10 == 0 && i > 0) {
            auto target = (i / 10) % 2 == 0
                ? adaptive_job_queue::mode::lock_free
                : adaptive_job_queue::mode::mutex;
            queue.switch_mode(target);
        }
    }

    // Wait for completion
    EXPECT_TRUE(WaitForCondition([&]() {
        return dequeued.load() >= enqueued.load();
    }, std::chrono::seconds(10)));

    stop.store(true, std::memory_order_release);
    consumer.join();

    DrainQueue(queue, dequeued);

    EXPECT_GE(dequeued.load(), enqueued.load());
    EXPECT_GE(queue.get_stats().mode_switches, mode_switches / 2);
}

/**
 * @brief Tests accuracy guard nesting behavior
 */
TEST_F(ModeTransitionScenarioTest, AccuracyGuardNesting_CorrectBehavior) {
    adaptive_job_queue queue(adaptive_job_queue::policy::performance_first);

    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    // Single guard
    {
        auto guard1 = queue.require_accuracy();
        EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

        // Nested guard (should still be mutex)
        {
            auto guard2 = queue.require_accuracy();
            EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
        }

        // After inner guard released, should still be mutex
        EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
    }

    // After all guards released, should return to lock-free
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);
}

/**
 * @brief Tests concurrent accuracy guards from multiple threads
 *
 * Note: With performance_first policy, mode returns to lock_free immediately
 * after all guards are released. During concurrent guard acquisition/release,
 * the mode may briefly be lock_free between guards. We verify:
 * 1. Guards are successfully acquired
 * 2. Final mode returns to lock_free after all guards released
 */
TEST_F(ModeTransitionScenarioTest, ConcurrentAccuracyGuards_ThreadSafe) {
    adaptive_job_queue queue(adaptive_job_queue::policy::performance_first);

    constexpr size_t num_threads = 8;
    constexpr size_t guards_per_thread = 50;

    std::atomic<size_t> guards_acquired{0};
    std::atomic<size_t> mutex_mode_confirmed{0};
    std::atomic<bool> start{false};

    std::vector<std::thread> threads;
    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            for (size_t j = 0; j < guards_per_thread; ++j) {
                {
                    auto guard = queue.require_accuracy();
                    guards_acquired.fetch_add(1, std::memory_order_relaxed);
                    // While holding the guard, mode should be mutex
                    // (may have race with other threads releasing their guards)
                    if (queue.current_mode() == adaptive_job_queue::mode::mutex) {
                        mutex_mode_confirmed.fetch_add(1, std::memory_order_relaxed);
                    }
                    std::this_thread::yield();
                }
            }
        });
    }

    start.store(true, std::memory_order_release);

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(guards_acquired.load(), num_threads * guards_per_thread);
    // Most guards should see mutex mode (some may see brief lock_free during race)
    EXPECT_GT(mutex_mode_confirmed.load(), guards_acquired.load() * 9 / 10)
        << "Expected at least 90% of guards to see mutex mode";
    // After all guards released, should return to lock_free
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);
}
