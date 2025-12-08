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
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/queue/adaptive_job_queue.h>

#include <atomic>
#include <chrono>
#include <latch>
#include <thread>
#include <vector>

using namespace kcenon::thread;

class AdaptiveJobQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset any global state if needed
    }

    void TearDown() override {
        // Hazard pointer cleanup happens deterministically when pointers go out of scope
    }
};

// ============================================
// Basic functionality tests
// ============================================

TEST_F(AdaptiveJobQueueTest, DefaultConstruction) {
    adaptive_job_queue queue;

    EXPECT_EQ(queue.current_policy(), adaptive_job_queue::policy::balanced);
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
    EXPECT_FALSE(queue.is_stopped());
}

TEST_F(AdaptiveJobQueueTest, ConstructWithPolicy) {
    adaptive_job_queue accuracy_queue(adaptive_job_queue::policy::accuracy_first);
    EXPECT_EQ(accuracy_queue.current_policy(), adaptive_job_queue::policy::accuracy_first);
    EXPECT_EQ(accuracy_queue.current_mode(), adaptive_job_queue::mode::mutex);

    adaptive_job_queue perf_queue(adaptive_job_queue::policy::performance_first);
    EXPECT_EQ(perf_queue.current_policy(), adaptive_job_queue::policy::performance_first);
    EXPECT_EQ(perf_queue.current_mode(), adaptive_job_queue::mode::lock_free);

    adaptive_job_queue manual_queue(adaptive_job_queue::policy::manual);
    EXPECT_EQ(manual_queue.current_policy(), adaptive_job_queue::policy::manual);
    EXPECT_EQ(manual_queue.current_mode(), adaptive_job_queue::mode::mutex);
}

TEST_F(AdaptiveJobQueueTest, BasicEnqueueDequeue) {
    adaptive_job_queue queue;

    std::atomic<int> counter{0};
    auto job = std::make_unique<callback_job>([&counter]() -> result_void {
        counter.fetch_add(1, std::memory_order_relaxed);
        return result_void();
    });

    // Enqueue
    auto enqueue_result = queue.enqueue(std::move(job));
    EXPECT_FALSE(enqueue_result.has_error());
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 1);

    // Dequeue
    auto dequeue_result = queue.dequeue();
    EXPECT_TRUE(dequeue_result.has_value());
    EXPECT_TRUE(queue.empty());

    // Execute job
    auto& job_ptr = dequeue_result.value();
    auto exec_result = job_ptr->do_work();
    EXPECT_FALSE(exec_result.has_error());
    EXPECT_EQ(counter.load(), 1);
}

TEST_F(AdaptiveJobQueueTest, DequeueEmpty) {
    adaptive_job_queue queue;

    EXPECT_TRUE(queue.empty());

    auto result = queue.dequeue();
    EXPECT_FALSE(result.has_value());
}

TEST_F(AdaptiveJobQueueTest, NullJobRejection) {
    adaptive_job_queue queue;

    auto result = queue.enqueue(nullptr);
    EXPECT_TRUE(result.has_error());
}

TEST_F(AdaptiveJobQueueTest, TryDequeue) {
    adaptive_job_queue queue;

    // Empty queue
    auto empty_result = queue.try_dequeue();
    EXPECT_FALSE(empty_result.has_value());

    // Add job
    auto job = std::make_unique<callback_job>([]() -> result_void { return result_void(); });
    queue.enqueue(std::move(job));

    // Non-empty queue
    auto result = queue.try_dequeue();
    EXPECT_TRUE(result.has_value());
}

TEST_F(AdaptiveJobQueueTest, Clear) {
    adaptive_job_queue queue;

    // Add multiple jobs
    for (int i = 0; i < 10; ++i) {
        auto job = std::make_unique<callback_job>([]() -> result_void { return result_void(); });
        queue.enqueue(std::move(job));
    }

    EXPECT_EQ(queue.size(), 10);

    queue.clear();

    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}

TEST_F(AdaptiveJobQueueTest, StopQueue) {
    adaptive_job_queue queue;

    // Add a job
    auto job = std::make_unique<callback_job>([]() -> result_void { return result_void(); });
    queue.enqueue(std::move(job));

    // Stop
    queue.stop();
    EXPECT_TRUE(queue.is_stopped());

    // Enqueue should fail
    auto another_job =
        std::make_unique<callback_job>([]() -> result_void { return result_void(); });
    auto result = queue.enqueue(std::move(another_job));
    EXPECT_TRUE(result.has_error());

    // Dequeue should also fail
    auto dequeue_result = queue.dequeue();
    EXPECT_FALSE(dequeue_result.has_value());
}

// ============================================
// Mode switching tests
// ============================================

TEST_F(AdaptiveJobQueueTest, ManualModeSwitch) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    // Switch to lock-free
    auto result = queue.switch_mode(adaptive_job_queue::mode::lock_free);
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    // Switch back to mutex
    result = queue.switch_mode(adaptive_job_queue::mode::mutex);
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
}

TEST_F(AdaptiveJobQueueTest, ModeSwitchNotAllowedWithoutManualPolicy) {
    adaptive_job_queue queue(adaptive_job_queue::policy::balanced);

    auto result = queue.switch_mode(adaptive_job_queue::mode::lock_free);
    EXPECT_TRUE(result.has_error());
}

TEST_F(AdaptiveJobQueueTest, ModeSwitchPreservesJobs) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    // Add jobs in mutex mode
    std::atomic<int> counter{0};
    for (int i = 0; i < 5; ++i) {
        auto job = std::make_unique<callback_job>([&counter]() -> result_void {
            counter.fetch_add(1, std::memory_order_relaxed);
            return result_void();
        });
        queue.enqueue(std::move(job));
    }

    EXPECT_EQ(queue.size(), 5);

    // Switch to lock-free mode
    queue.switch_mode(adaptive_job_queue::mode::lock_free);
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    // Jobs should still be available (size is approximate in lock-free)
    EXPECT_FALSE(queue.empty());

    // Dequeue and execute all jobs
    int dequeued = 0;
    while (auto result = queue.dequeue()) {
        result.value()->do_work();
        ++dequeued;
    }

    EXPECT_EQ(dequeued, 5);
    EXPECT_EQ(counter.load(), 5);
}

// ============================================
// Accuracy guard tests
// ============================================

TEST_F(AdaptiveJobQueueTest, AccuracyGuardSwitchesToMutexMode) {
    adaptive_job_queue queue(adaptive_job_queue::policy::performance_first);

    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    {
        auto guard = queue.require_accuracy();
        EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
    }

    // After guard destruction, should revert to lock-free for performance_first policy
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);
}

TEST_F(AdaptiveJobQueueTest, AccuracyGuardStaysInMutexModeForAccuracyFirst) {
    adaptive_job_queue queue(adaptive_job_queue::policy::accuracy_first);

    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    {
        auto guard = queue.require_accuracy();
        EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
    }

    // Should stay in mutex mode
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
}

TEST_F(AdaptiveJobQueueTest, MultipleAccuracyGuards) {
    adaptive_job_queue queue(adaptive_job_queue::policy::performance_first);

    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    {
        auto guard1 = queue.require_accuracy();
        EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

        {
            auto guard2 = queue.require_accuracy();
            EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
        }

        // Still in mutex mode because guard1 is active
        EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
    }

    // Now should revert
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);
}

TEST_F(AdaptiveJobQueueTest, AccuracyGuardMoveSemantics) {
    adaptive_job_queue queue(adaptive_job_queue::policy::performance_first);

    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);

    std::optional<adaptive_job_queue::accuracy_guard> holder;

    {
        auto guard = queue.require_accuracy();
        EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

        holder.emplace(std::move(guard));
        EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);
    }

    // Still in mutex mode because holder is active
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::mutex);

    holder.reset();
    EXPECT_EQ(queue.current_mode(), adaptive_job_queue::mode::lock_free);
}

// ============================================
// Capabilities tests
// ============================================

TEST_F(AdaptiveJobQueueTest, CapabilitiesInMutexMode) {
    adaptive_job_queue queue(adaptive_job_queue::policy::accuracy_first);

    auto caps = queue.get_capabilities();
    EXPECT_TRUE(caps.exact_size);
    EXPECT_TRUE(caps.atomic_empty_check);
    EXPECT_FALSE(caps.lock_free);
    EXPECT_TRUE(caps.supports_batch);
    EXPECT_TRUE(caps.supports_blocking_wait);
    EXPECT_TRUE(caps.supports_stop);
}

TEST_F(AdaptiveJobQueueTest, CapabilitiesInLockFreeMode) {
    adaptive_job_queue queue(adaptive_job_queue::policy::performance_first);

    auto caps = queue.get_capabilities();
    EXPECT_FALSE(caps.exact_size);
    EXPECT_FALSE(caps.atomic_empty_check);
    EXPECT_TRUE(caps.lock_free);
    EXPECT_FALSE(caps.supports_batch);
    EXPECT_FALSE(caps.supports_blocking_wait);
    EXPECT_TRUE(caps.supports_stop);
}

// ============================================
// Statistics tests
// ============================================

TEST_F(AdaptiveJobQueueTest, StatisticsTracking) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);

    auto initial_stats = queue.get_stats();
    EXPECT_EQ(initial_stats.mode_switches, 0);
    EXPECT_EQ(initial_stats.enqueue_count, 0);
    EXPECT_EQ(initial_stats.dequeue_count, 0);

    // Enqueue some jobs
    for (int i = 0; i < 10; ++i) {
        auto job = std::make_unique<callback_job>([]() -> result_void { return result_void(); });
        queue.enqueue(std::move(job));
    }

    auto after_enqueue = queue.get_stats();
    EXPECT_EQ(after_enqueue.enqueue_count, 10);

    // Dequeue some jobs
    for (int i = 0; i < 5; ++i) {
        queue.dequeue();
    }

    auto after_dequeue = queue.get_stats();
    EXPECT_EQ(after_dequeue.dequeue_count, 5);

    // Mode switch
    queue.switch_mode(adaptive_job_queue::mode::lock_free);

    auto after_switch = queue.get_stats();
    EXPECT_EQ(after_switch.mode_switches, 1);
}

// ============================================
// scheduler_interface tests
// ============================================

TEST_F(AdaptiveJobQueueTest, SchedulerInterface) {
    adaptive_job_queue queue;

    std::atomic<int> counter{0};
    auto job = std::make_unique<callback_job>([&counter]() -> result_void {
        counter.fetch_add(1, std::memory_order_relaxed);
        return result_void();
    });

    // Use scheduler_interface methods
    scheduler_interface& scheduler = queue;

    auto schedule_result = scheduler.schedule(std::move(job));
    EXPECT_FALSE(schedule_result.has_error());

    auto get_result = scheduler.get_next_job();
    EXPECT_TRUE(get_result.has_value());

    get_result.value()->do_work();
    EXPECT_EQ(counter.load(), 1);
}

// ============================================
// Concurrent access tests
// ============================================

TEST_F(AdaptiveJobQueueTest, ConcurrentEnqueueDequeue) {
    adaptive_job_queue queue;
    constexpr int num_producers = 4;
    constexpr int num_consumers = 4;
    constexpr int jobs_per_producer = 1000;

    std::atomic<int> enqueued{0};
    std::atomic<int> dequeued{0};
    std::atomic<bool> stop_consumers{false};

    std::latch start_latch(num_producers + num_consumers);

    // Producer threads
    std::vector<std::thread> producers;
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&]() {
            start_latch.arrive_and_wait();
            for (int j = 0; j < jobs_per_producer; ++j) {
                auto job =
                    std::make_unique<callback_job>([]() -> result_void { return result_void(); });
                if (!queue.enqueue(std::move(job)).has_error()) {
                    enqueued.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    // Consumer threads
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

    // Wait for producers
    for (auto& t : producers) {
        t.join();
    }

    // Signal consumers to stop
    stop_consumers.store(true, std::memory_order_release);

    // Wait for consumers
    for (auto& t : consumers) {
        t.join();
    }

    EXPECT_EQ(enqueued.load(), num_producers * jobs_per_producer);
    EXPECT_EQ(dequeued.load(), num_producers * jobs_per_producer);
    EXPECT_TRUE(queue.empty());
}

TEST_F(AdaptiveJobQueueTest, ConcurrentModeSwitchWithOperations) {
    adaptive_job_queue queue(adaptive_job_queue::policy::manual);
    constexpr int num_ops = 1000;

    std::atomic<bool> stop{false};
    std::atomic<int> successful_ops{0};

    // Worker thread doing enqueue/dequeue
    std::thread worker([&]() {
        while (!stop.load(std::memory_order_acquire)) {
            auto job =
                std::make_unique<callback_job>([]() -> result_void { return result_void(); });
            if (!queue.enqueue(std::move(job)).has_error()) {
                successful_ops.fetch_add(1, std::memory_order_relaxed);
            }
            if (auto result = queue.try_dequeue(); result.has_value()) {
                successful_ops.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });

    // Mode switching thread
    std::thread switcher([&]() {
        for (int i = 0; i < num_ops; ++i) {
            if (i % 2 == 0) {
                queue.switch_mode(adaptive_job_queue::mode::lock_free);
            } else {
                queue.switch_mode(adaptive_job_queue::mode::mutex);
            }
        }
        stop.store(true, std::memory_order_release);
    });

    switcher.join();
    worker.join();

    // Verify no data corruption - just check we got some operations done
    EXPECT_GT(successful_ops.load(), 0);

    // Stats should show mode switches
    auto stats = queue.get_stats();
    EXPECT_GT(stats.mode_switches, 0);
}

TEST_F(AdaptiveJobQueueTest, ConcurrentAccuracyGuards) {
    adaptive_job_queue queue(adaptive_job_queue::policy::performance_first);
    constexpr int num_threads = 4;
    constexpr int ops_per_thread = 100;

    std::atomic<int> guard_created{0};
    std::latch start_latch(num_threads);

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            start_latch.arrive_and_wait();
            for (int j = 0; j < ops_per_thread; ++j) {
                {
                    auto guard = queue.require_accuracy();
                    guard_created.fetch_add(1, std::memory_order_relaxed);

                    // Do some operations that require accuracy
                    auto size = queue.size();
                    (void)size;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(guard_created.load(), num_threads * ops_per_thread);
}
