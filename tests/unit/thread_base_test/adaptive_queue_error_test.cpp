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

#include <kcenon/thread/queue/adaptive_job_queue.h>
#include <kcenon/thread/core/callback_job.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <latch>

using namespace kcenon::thread;

class AdaptiveQueueErrorTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
};

// ============================================
// 1. Mode Switch Error Handling Tests
// ============================================

TEST_F(AdaptiveQueueErrorTest, ModeSwitchWithAccuracyFirstPolicy) {
    adaptive_job_queue queue(adaptive_job_queue::policy::accuracy_first);

    auto result = queue.switch_mode(adaptive_job_queue::mode::lock_free);
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
}

TEST_F(AdaptiveQueueErrorTest, ModeSwitchWithPerformanceFirstPolicy) {
    adaptive_job_queue queue(adaptive_job_queue::policy::performance_first);

    auto result = queue.switch_mode(adaptive_job_queue::mode::mutex);
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);
}

TEST_F(AdaptiveQueueErrorTest, ModeSwitchWithBalancedPolicy) {
    adaptive_job_queue queue(adaptive_job_queue::policy::balanced);

    auto result = queue.switch_mode(adaptive_job_queue::mode::lock_free);
    EXPECT_TRUE(result.has_error());
}

TEST_F(AdaptiveQueueErrorTest, ModeSwitchDuringEnqueue) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);
    std::atomic<bool> enqueue_done{false};
    std::atomic<int> enqueue_count{0};
    constexpr int total_jobs = 10000;

    std::thread enqueuer([&]() {
        for (int i = 0; i < total_jobs; ++i) {
            auto job = std::make_unique<callback_job>([]() -> result_void {
                return result_void();
            });
            if (!queue.enqueue(std::move(job)).has_error()) {
                enqueue_count.fetch_add(1, std::memory_order_relaxed);
            }
        }
        enqueue_done.store(true, std::memory_order_release);
    });

    // Switch mode while enqueue in flight
    while (!enqueue_done.load(std::memory_order_acquire)) {
        queue.switch_mode(adaptive_job_queue::mode::lock_free);
        std::this_thread::yield();
        queue.switch_mode(adaptive_job_queue::mode::mutex);
    }

    enqueuer.join();

    // Verify no jobs lost - dequeue all and count
    int dequeue_count = 0;
    while (queue.try_dequeue().has_value()) {
        ++dequeue_count;
    }

    EXPECT_EQ(enqueue_count.load(), dequeue_count);
    EXPECT_TRUE(queue.empty());
}

TEST_F(AdaptiveQueueErrorTest, ModeSwitchToSameMode) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    // Switch to same mode should succeed without side effects
    auto result = queue.switch_mode(adaptive_job_queue::mode::mutex);
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    auto stats = queue.get_stats();
    EXPECT_EQ(stats.mode_switches, 0);
}

// ============================================
// 2. Accuracy Guard Edge Cases
// ============================================

TEST_F(AdaptiveQueueErrorTest, AccuracyGuardNestingLimit) {
    adaptive_job_queue queue(adaptive_job_queue::policy::performance_first);

    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    // Deep nesting - should handle gracefully
    std::vector<adaptive_job_queue::accuracy_guard> guards;
    for (int i = 0; i < 100; ++i) {
        guards.push_back(queue.require_accuracy());
    }

    // All guards active - should be in mutex mode
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    // Queue should be functional
    auto job = std::make_unique<callback_job>([]() -> result_void {
        return result_void();
    });
    auto result = queue.enqueue(std::move(job));
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(queue.size(), 1);

    // Release all guards
    guards.clear();

    // Should revert to lock-free for performance_first policy
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    // Queue should still be functional
    auto dequeue_result = queue.try_dequeue();
    EXPECT_TRUE(dequeue_result.has_value());
}

TEST_F(AdaptiveQueueErrorTest, AccuracyGuardWithConcurrentRelease) {
    adaptive_job_queue queue(adaptive_job_queue::policy::performance_first);
    std::atomic<int> guard_operations{0};

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 100; ++j) {
                auto guard = queue.require_accuracy();
                guard_operations.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Queue should be stable after all guards released
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);
    EXPECT_EQ(guard_operations.load(), 1000);

    // Verify queue is still functional
    auto job = std::make_unique<callback_job>([]() -> result_void {
        return result_void();
    });
    EXPECT_FALSE(queue.enqueue(std::move(job)).has_error());
}

TEST_F(AdaptiveQueueErrorTest, AccuracyGuardWithManualPolicy) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    // Switch to lock-free mode
    queue.switch_mode(adaptive_job_queue::mode::lock_free);
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    {
        auto guard = queue.require_accuracy();
        EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
    }

    // Should restore previous mode (lock-free)
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);
}

// ============================================
// 3. Data Integrity Under Stress
// ============================================

TEST_F(AdaptiveQueueErrorTest, DataIntegrityDuringModeSwitch) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);
    std::atomic<int> enqueued{0};
    std::atomic<int> dequeued{0};
    std::atomic<bool> stop{false};

    // Producer
    std::thread producer([&]() {
        while (!stop.load(std::memory_order_acquire)) {
            auto job = std::make_unique<callback_job>([]() -> result_void {
                return result_void();
            });
            if (!queue.enqueue(std::move(job)).has_error()) {
                enqueued.fetch_add(1, std::memory_order_relaxed);
            }
            std::this_thread::yield();
        }
    });

    // Consumer
    std::thread consumer([&]() {
        while (!stop.load(std::memory_order_acquire) || !queue.empty()) {
            if (queue.try_dequeue().has_value()) {
                dequeued.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::yield();
            }
        }
    });

    // Mode switcher
    std::thread switcher([&]() {
        for (int i = 0; i < 100; ++i) {
            queue.switch_mode(adaptive_job_queue::mode::lock_free);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            queue.switch_mode(adaptive_job_queue::mode::mutex);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    switcher.join();
    stop.store(true, std::memory_order_release);
    producer.join();
    consumer.join();

    // Drain remaining jobs
    while (queue.try_dequeue().has_value()) {
        dequeued.fetch_add(1, std::memory_order_relaxed);
    }

    // Verify no data loss
    EXPECT_EQ(enqueued.load(), dequeued.load());
    EXPECT_TRUE(queue.empty());
}

TEST_F(AdaptiveQueueErrorTest, DataIntegrityWithMultipleProducersConsumers) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);
    constexpr int num_producers = 4;
    constexpr int num_consumers = 4;
    constexpr int jobs_per_producer = 500;

    std::atomic<int> enqueued{0};
    std::atomic<int> dequeued{0};
    std::atomic<bool> producers_done{false};
    std::latch start_latch(num_producers + num_consumers + 1); // +1 for switcher

    // Producers
    std::vector<std::thread> producers;
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&]() {
            start_latch.arrive_and_wait();
            for (int j = 0; j < jobs_per_producer; ++j) {
                auto job = std::make_unique<callback_job>([]() -> result_void {
                    return result_void();
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
            while (!producers_done.load(std::memory_order_acquire) || !queue.empty()) {
                if (queue.try_dequeue().has_value()) {
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
        while (!producers_done.load(std::memory_order_acquire)) {
            queue.switch_mode(adaptive_job_queue::mode::lock_free);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            queue.switch_mode(adaptive_job_queue::mode::mutex);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    // Wait for producers
    for (auto& t : producers) {
        t.join();
    }
    producers_done.store(true, std::memory_order_release);

    // Wait for switcher and consumers
    switcher.join();
    for (auto& t : consumers) {
        t.join();
    }

    // Drain any remaining
    while (queue.try_dequeue().has_value()) {
        dequeued.fetch_add(1, std::memory_order_relaxed);
    }

    EXPECT_EQ(enqueued.load(), dequeued.load());
    EXPECT_EQ(enqueued.load(), num_producers * jobs_per_producer);
}

// ============================================
// 4. Empty Queue Operations
// ============================================

TEST_F(AdaptiveQueueErrorTest, TryDequeueFromEmptyQueue) {
    adaptive_job_queue queue;

    auto result = queue.try_dequeue();
    // Should return no value (error), not crash
    EXPECT_FALSE(result.has_value());
}

TEST_F(AdaptiveQueueErrorTest, ModeSwitchOnEmptyQueue) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    EXPECT_TRUE(queue.empty());

    // Should work on empty queue
    EXPECT_FALSE(queue.switch_mode(adaptive_job_queue::mode::lock_free).has_error());
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);
    EXPECT_TRUE(queue.empty());

    EXPECT_FALSE(queue.switch_mode(adaptive_job_queue::mode::mutex).has_error());
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
    EXPECT_TRUE(queue.empty());
}

TEST_F(AdaptiveQueueErrorTest, SizeAndEmptyOnEmptyQueue) {
    adaptive_job_queue mutex_queue(adaptive_job_queue::policy::accuracy_first);
    adaptive_job_queue lockfree_queue(adaptive_job_queue::policy::performance_first);

    // Mutex mode
    EXPECT_EQ(mutex_queue.size(), 0);
    EXPECT_TRUE(mutex_queue.empty());

    // Lock-free mode
    EXPECT_EQ(lockfree_queue.size(), 0);
    EXPECT_TRUE(lockfree_queue.empty());
}

TEST_F(AdaptiveQueueErrorTest, ClearEmptyQueue) {
    adaptive_job_queue queue;

    EXPECT_TRUE(queue.empty());

    // Clear on empty queue should not crash
    queue.clear();

    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}

// ============================================
// 5. Null/Invalid Job Handling
// ============================================

TEST_F(AdaptiveQueueErrorTest, NullJobRejectedInMutexMode) {
    adaptive_job_queue queue(adaptive_job_queue::policy::accuracy_first);

    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    auto result = queue.enqueue(nullptr);
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(queue.size(), 0);
}

TEST_F(AdaptiveQueueErrorTest, NullJobRejectedInLockFreeMode) {
    adaptive_job_queue queue(adaptive_job_queue::policy::performance_first);

    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    auto result = queue.enqueue(nullptr);
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(queue.size(), 0);
}

// ============================================
// 6. Statistics Accuracy
// ============================================

TEST_F(AdaptiveQueueErrorTest, StatisticsAccuracyAfterModeSwitch) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    // Initial stats
    auto stats1 = queue.get_stats();
    EXPECT_EQ(stats1.mode_switches, 0);
    EXPECT_EQ(stats1.enqueue_count, 0);
    EXPECT_EQ(stats1.dequeue_count, 0);

    // Switch modes multiple times
    queue.switch_mode(adaptive_job_queue::mode::lock_free);
    queue.switch_mode(adaptive_job_queue::mode::mutex);
    queue.switch_mode(adaptive_job_queue::mode::lock_free);

    auto stats2 = queue.get_stats();
    EXPECT_EQ(stats2.mode_switches, 3);

    // Enqueue and dequeue
    for (int i = 0; i < 10; ++i) {
        auto job = std::make_unique<callback_job>([]() -> result_void {
            return result_void();
        });
        queue.enqueue(std::move(job));
    }

    for (int i = 0; i < 5; ++i) {
        queue.try_dequeue();
    }

    auto stats3 = queue.get_stats();
    EXPECT_EQ(stats3.enqueue_count, 10);
    EXPECT_EQ(stats3.dequeue_count, 5);
    EXPECT_EQ(stats3.mode_switches, 3);
}

TEST_F(AdaptiveQueueErrorTest, StatisticsTimeTracking) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    // Start in mutex mode
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto stats1 = queue.get_stats();
    EXPECT_GE(stats1.time_in_mutex_ms, 40); // Allow some tolerance

    // Switch to lock-free
    queue.switch_mode(adaptive_job_queue::mode::lock_free);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto stats2 = queue.get_stats();
    EXPECT_GE(stats2.time_in_lockfree_ms, 40);
}

// ============================================
// 7. Stop/Shutdown Edge Cases
// ============================================

TEST_F(AdaptiveQueueErrorTest, EnqueueAfterStop) {
    adaptive_job_queue queue;

    queue.stop();
    EXPECT_TRUE(queue.is_stopped());

    auto job = std::make_unique<callback_job>([]() -> result_void {
        return result_void();
    });
    auto result = queue.enqueue(std::move(job));
    EXPECT_TRUE(result.has_error());
}

TEST_F(AdaptiveQueueErrorTest, DequeueAfterStop) {
    adaptive_job_queue queue;

    // Add a job before stopping
    auto job = std::make_unique<callback_job>([]() -> result_void {
        return result_void();
    });
    queue.enqueue(std::move(job));

    queue.stop();

    auto result = queue.dequeue();
    EXPECT_FALSE(result.has_value());
}

TEST_F(AdaptiveQueueErrorTest, TryDequeueAfterStop) {
    adaptive_job_queue queue;

    auto job = std::make_unique<callback_job>([]() -> result_void {
        return result_void();
    });
    queue.enqueue(std::move(job));

    queue.stop();

    auto result = queue.try_dequeue();
    EXPECT_FALSE(result.has_value());
}

TEST_F(AdaptiveQueueErrorTest, ModeSwitchAfterStop) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    queue.stop();

    // Mode switch should still work (no restriction in implementation)
    // But operations on the queue should fail
    queue.switch_mode(adaptive_job_queue::mode::lock_free);

    auto job = std::make_unique<callback_job>([]() -> result_void {
        return result_void();
    });
    auto result = queue.enqueue(std::move(job));
    EXPECT_TRUE(result.has_error());
}
