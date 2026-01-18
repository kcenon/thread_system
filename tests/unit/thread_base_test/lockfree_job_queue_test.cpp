/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
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
#include <kcenon/thread/lockfree/lockfree_job_queue.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace kcenon::thread;
using namespace kcenon::thread::detail;

class LockFreeJobQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset any global state if needed
    }

    void TearDown() override {
        // Hazard pointer cleanup happens deterministically when pointers go out of scope
    }
};

// Test basic enqueue/dequeue
TEST_F(LockFreeJobQueueTest, BasicEnqueueDequeue) {
    lockfree_job_queue queue;

    std::atomic<int> counter{0};
    auto job = std::make_unique<callback_job>([&counter]() -> kcenon::common::VoidResult {
        counter.fetch_add(1, std::memory_order_relaxed);
        return kcenon::common::ok();  // Success
    });

    // Enqueue
    auto enqueue_result = queue.enqueue(std::move(job));
    EXPECT_FALSE(enqueue_result.is_err());
    EXPECT_FALSE(queue.empty());

    // Dequeue
    auto dequeue_result = queue.dequeue();
    EXPECT_TRUE(dequeue_result.is_ok());
    EXPECT_TRUE(queue.empty());

    // Execute job
    auto& job_ptr = dequeue_result.value();
    auto exec_result = job_ptr->do_work();
    EXPECT_FALSE(exec_result.is_err());
    EXPECT_EQ(counter.load(), 1);
}

// Test dequeue from empty queue
TEST_F(LockFreeJobQueueTest, DequeueEmpty) {
    lockfree_job_queue queue;

    EXPECT_TRUE(queue.empty());

    auto result = queue.dequeue();
    EXPECT_FALSE(result.is_ok());
}

// Test null job rejection
TEST_F(LockFreeJobQueueTest, NullJobRejection) {
    lockfree_job_queue queue;

    auto result = queue.enqueue(nullptr);
    EXPECT_TRUE(result.is_err());
}

// Test multiple enqueue/dequeue
TEST_F(LockFreeJobQueueTest, MultipleOperations) {
    lockfree_job_queue queue;

    constexpr int COUNT = 100;
    std::atomic<int> counter{0};

    // Enqueue multiple jobs
    for (int i = 0; i < COUNT; ++i) {
        auto job = std::make_unique<callback_job>([&counter]() -> kcenon::common::VoidResult {
            counter.fetch_add(1, std::memory_order_relaxed);
            return kcenon::common::ok();
        });

        auto result = queue.enqueue(std::move(job));
        EXPECT_FALSE(result.is_err());
    }

    EXPECT_FALSE(queue.empty());

    // Dequeue and execute all jobs
    for (int i = 0; i < COUNT; ++i) {
        auto result = queue.dequeue();
        EXPECT_TRUE(result.is_ok());

        auto& job_ptr = result.value();
        auto exec_result = job_ptr->do_work();
        EXPECT_FALSE(exec_result.is_err());
    }

    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(counter.load(), COUNT);

    // Queue should be empty now
    auto result = queue.dequeue();
    EXPECT_FALSE(result.is_ok());
}

// Test concurrent enqueue (multiple producers)
TEST_F(LockFreeJobQueueTest, ConcurrentEnqueue) {
    lockfree_job_queue queue;

    constexpr int NUM_THREADS = 4;
    constexpr int JOBS_PER_THREAD = 250;
    std::atomic<int> counter{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&queue, &counter]() {
            for (int i = 0; i < JOBS_PER_THREAD; ++i) {
                auto job = std::make_unique<callback_job>([&counter]() -> kcenon::common::VoidResult {
                    counter.fetch_add(1, std::memory_order_relaxed);
                    return kcenon::common::ok();
                });

                auto result = queue.enqueue(std::move(job));
                EXPECT_FALSE(result.is_err());
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_FALSE(queue.empty());

    // Dequeue all jobs
    int dequeued = 0;
    while (true) {
        auto result = queue.dequeue();
        if (result.is_err()) {
            break;
        }
        auto& job_ptr = result.value();
        auto exec_result = job_ptr->do_work();
        EXPECT_FALSE(exec_result.is_err());
        ++dequeued;
    }

    EXPECT_EQ(dequeued, NUM_THREADS * JOBS_PER_THREAD);
    EXPECT_EQ(counter.load(), NUM_THREADS * JOBS_PER_THREAD);
}

// Test concurrent dequeue (multiple consumers)
TEST_F(LockFreeJobQueueTest, ConcurrentDequeue) {
    lockfree_job_queue queue;

    constexpr int TOTAL_JOBS = 1000;
    std::atomic<int> counter{0};

    // Enqueue jobs
    for (int i = 0; i < TOTAL_JOBS; ++i) {
        auto job = std::make_unique<callback_job>([&counter]() -> kcenon::common::VoidResult {
            counter.fetch_add(1, std::memory_order_relaxed);
            return kcenon::common::ok();
        });

        auto result = queue.enqueue(std::move(job));
        EXPECT_FALSE(result.is_err());
    }

    // Multiple consumer threads
    constexpr int NUM_CONSUMERS = 4;
    std::atomic<int> jobs_processed{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < NUM_CONSUMERS; ++t) {
        threads.emplace_back([&queue, &jobs_processed]() {
            while (true) {
                auto result = queue.dequeue();
                if (!result.is_ok()) {
                    break;  // Queue empty
                }

                auto& job_ptr = result.value();
                auto exec_result = job_ptr->do_work();
                EXPECT_FALSE(exec_result.is_err());

                jobs_processed.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(jobs_processed.load(), TOTAL_JOBS);
    EXPECT_EQ(counter.load(), TOTAL_JOBS);
    EXPECT_TRUE(queue.empty());
}

// Test concurrent enqueue and dequeue (MPMC)
TEST_F(LockFreeJobQueueTest, ConcurrentMPMC) {
    lockfree_job_queue queue;

    constexpr int NUM_PRODUCERS = 2;
    constexpr int NUM_CONSUMERS = 2;
    constexpr int JOBS_PER_PRODUCER = 500;

    std::atomic<int> enqueued{0};
    std::atomic<int> dequeued{0};
    std::atomic<bool> producers_done{false};

    std::vector<std::thread> threads;

    // Producer threads
    for (int t = 0; t < NUM_PRODUCERS; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < JOBS_PER_PRODUCER; ++i) {
                auto job = std::make_unique<callback_job>([&enqueued]() -> kcenon::common::VoidResult {
                    enqueued.fetch_add(1, std::memory_order_relaxed);
                    return kcenon::common::ok();
                });

                auto result = queue.enqueue(std::move(job));
                EXPECT_FALSE(result.is_err());

                // Simulate work
                std::this_thread::yield();
            }
        });
    }

    // Consumer threads
    for (int t = 0; t < NUM_CONSUMERS; ++t) {
        threads.emplace_back([&]() {
            int local_dequeued = 0;
            while (true) {
                auto result = queue.dequeue();
                if (result.is_ok()) {
                    auto& job_ptr = result.value();
                    auto exec_result = job_ptr->do_work();
                    EXPECT_FALSE(exec_result.is_err());

                    ++local_dequeued;
                    dequeued.fetch_add(1, std::memory_order_relaxed);
                } else {
                    // Check if producers are done
                    if (producers_done.load(std::memory_order_acquire)) {
                        break;
                    }

                    // Yield to avoid busy waiting
                    std::this_thread::yield();
                }
            }
        });
    }

    // Wait for producers to finish
    for (int t = 0; t < NUM_PRODUCERS; ++t) {
        threads[t].join();
    }

    producers_done.store(true, std::memory_order_release);

    // Wait for consumers to finish
    for (int t = NUM_PRODUCERS; t < NUM_PRODUCERS + NUM_CONSUMERS; ++t) {
        threads[t].join();
    }

    EXPECT_EQ(enqueued.load(), NUM_PRODUCERS * JOBS_PER_PRODUCER);
    EXPECT_EQ(dequeued.load(), NUM_PRODUCERS * JOBS_PER_PRODUCER);
    EXPECT_TRUE(queue.empty());
}

// Test hazard pointer memory reclamation (no leaks)
TEST_F(LockFreeJobQueueTest, HazardPointerReclamation) {
    lockfree_job_queue queue;

    constexpr int NUM_ITERATIONS = 10;
    constexpr int JOBS_PER_ITERATION = 100;

    for (int iter = 0; iter < NUM_ITERATIONS; ++iter) {
        // Enqueue jobs
        for (int i = 0; i < JOBS_PER_ITERATION; ++i) {
            auto job =
                std::make_unique<callback_job>([]() -> kcenon::common::VoidResult { return kcenon::common::ok(); });

            auto result = queue.enqueue(std::move(job));
            EXPECT_FALSE(result.is_err());
        }

        // Dequeue all
        for (int i = 0; i < JOBS_PER_ITERATION; ++i) {
            auto result = queue.dequeue();
            EXPECT_TRUE(result.is_ok());
        }

        EXPECT_TRUE(queue.empty());

        // Hazard pointer reclamation happens deterministically
        // No sleep needed - the queue is empty and all hazard pointers are released
    }

    // If we reach here without segfault, hazard pointers are working
    SUCCEED();
}

// Test queue destruction with pending jobs (memory safety)
TEST_F(LockFreeJobQueueTest, DestructionWithPendingJobs) {
    {
        lockfree_job_queue queue;

        for (int i = 0; i < 100; ++i) {
            auto job =
                std::make_unique<callback_job>([]() -> kcenon::common::VoidResult { return kcenon::common::ok(); });

            auto result = queue.enqueue(std::move(job));
            EXPECT_FALSE(result.is_err());
        }

        // Queue goes out of scope with pending jobs
        // Destructor should clean up safely
    }

    // If we reach here without crash, destructor is safe
    SUCCEED();
}

// Stress test: High contention MPMC
TEST_F(LockFreeJobQueueTest, StressTest) {
    lockfree_job_queue queue;

    constexpr int NUM_THREADS = 8;
    constexpr int OPERATIONS_PER_THREAD = 1000;

    std::atomic<int> total_enqueued{0};
    std::atomic<int> total_dequeued{0};

    std::vector<std::thread> threads;

    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&, t]() {
            // Mix of enqueue and dequeue
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                if ((t + i) % 2 == 0) {
                    // Enqueue
                    auto job = std::make_unique<callback_job>(
                        []() -> kcenon::common::VoidResult { return kcenon::common::ok(); });

                    auto result = queue.enqueue(std::move(job));
                    if (!result.is_err()) {
                        total_enqueued.fetch_add(1, std::memory_order_relaxed);
                    }
                } else {
                    // Dequeue
                    auto result = queue.dequeue();
                    if (result.is_ok()) {
                        total_dequeued.fetch_add(1, std::memory_order_relaxed);
                    }
                }

                std::this_thread::yield();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Drain remaining jobs
    while (true) {
        auto result = queue.dequeue();
        if (result.is_err()) {
            break;
        }
        total_dequeued.fetch_add(1, std::memory_order_relaxed);
    }

    EXPECT_EQ(total_enqueued.load(), total_dequeued.load());
    EXPECT_TRUE(queue.empty());
}

// =============================================================================
// TICKET-001 Verification: Thread Churn Test
// =============================================================================
// This test validates that the TLS bug has been fixed by reproducing the
// original failure scenario: short-lived producer threads pushing items
// while a long-running consumer thread pops them.
//
// Previous failure mode:
// 1. Thread A pushes a node and exits (TLS destroyed)
// 2. Thread B (still running) tries to pop the node
// 3. CRASH: Use-After-Free because node memory was reclaimed
//
// With Hazard Pointers:
// - Nodes are protected during access
// - GlobalReclamationManager handles orphaned nodes safely
// - No crash, no data loss

TEST_F(LockFreeJobQueueTest, ThreadChurnTest) {
    lockfree_job_queue queue;

    constexpr int TOTAL_ITEMS = 1000;  // Reduced for faster test execution
    std::atomic<int> consumed{0};
    std::atomic<bool> producers_done{false};

    // Consumer thread (long-running)
    std::thread consumer([&]() {
        while (consumed.load(std::memory_order_relaxed) < TOTAL_ITEMS) {
            auto result = queue.dequeue();
            if (result.is_ok()) {
                auto& job_ptr = result.value();
                auto exec_result = job_ptr->do_work();
                EXPECT_FALSE(exec_result.is_err());
                consumed.fetch_add(1, std::memory_order_relaxed);
            } else if (producers_done.load(std::memory_order_acquire)) {
                // If producers are done and queue is empty, we're done
                break;
            } else {
                // Yield to avoid busy waiting
                std::this_thread::yield();
            }
        }
    });

    // Short-lived producer threads
    for (int i = 0; i < TOTAL_ITEMS; ++i) {
        std::thread producer([&queue, i]() {
            auto job = std::make_unique<callback_job>([i]() -> kcenon::common::VoidResult {
                // Simple work
                (void)i;
                return kcenon::common::ok();
            });

            auto result = queue.enqueue(std::move(job));
            EXPECT_FALSE(result.is_err());
            // Thread exits immediately after push, triggering TLS destruction
        });
        producer.join();  // Wait for producer to finish (and TLS to be destroyed)
    }

    producers_done.store(true, std::memory_order_release);
    consumer.join();

    // All items must be consumed without crash
    EXPECT_EQ(consumed.load(), TOTAL_ITEMS);
    EXPECT_TRUE(queue.empty());
}

// Thread Churn Test with High Contention
// Tests multiple short-lived threads under high contention
TEST_F(LockFreeJobQueueTest, ThreadChurnHighContention) {
    lockfree_job_queue queue;

    constexpr int NUM_BATCHES = 10;
    constexpr int THREADS_PER_BATCH = 50;
    std::atomic<int> total_enqueued{0};
    std::atomic<int> total_dequeued{0};

    for (int batch = 0; batch < NUM_BATCHES; ++batch) {
        std::vector<std::thread> producers;

        // Launch batch of short-lived producers
        for (int t = 0; t < THREADS_PER_BATCH; ++t) {
            producers.emplace_back([&queue, &total_enqueued]() {
                auto job =
                    std::make_unique<callback_job>([]() -> kcenon::common::VoidResult { return kcenon::common::ok(); });

                auto result = queue.enqueue(std::move(job));
                if (!result.is_err()) {
                    total_enqueued.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }

        // Wait for all producers to finish
        for (auto& p : producers) {
            p.join();
        }

        // Drain some items between batches
        for (int i = 0; i < THREADS_PER_BATCH / 2; ++i) {
            auto result = queue.dequeue();
            if (result.is_ok()) {
                auto& job_ptr = result.value();
                (void)job_ptr->do_work();
                total_dequeued.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }

    // Drain remaining items
    while (true) {
        auto result = queue.dequeue();
        if (result.is_err()) {
            break;
        }
        auto& job_ptr = result.value();
        (void)job_ptr->do_work();
        total_dequeued.fetch_add(1, std::memory_order_relaxed);
    }

    EXPECT_EQ(total_enqueued.load(), total_dequeued.load());
    EXPECT_TRUE(queue.empty());
}

// =============================================================================
// Phase 3: scheduler_interface and queue_capabilities_interface tests
// =============================================================================

// Test that lockfree_job_queue implements scheduler_interface
TEST_F(LockFreeJobQueueTest, ImplementsSchedulerInterface) {
    lockfree_job_queue queue;

    // Cast to scheduler_interface
    scheduler_interface* scheduler = &queue;
    ASSERT_NE(scheduler, nullptr);

    std::atomic<int> counter{0};
    auto job = std::make_unique<callback_job>([&counter]() -> kcenon::common::VoidResult {
        counter.fetch_add(1, std::memory_order_relaxed);
        return kcenon::common::ok();
    });

    // Use scheduler_interface methods
    auto schedule_result = scheduler->schedule(std::move(job));
    EXPECT_FALSE(schedule_result.is_err());

    auto get_job_result = scheduler->get_next_job();
    EXPECT_TRUE(get_job_result.is_ok());

    // Execute the job
    auto& job_ptr = get_job_result.value();
    auto exec_result = job_ptr->do_work();
    EXPECT_FALSE(exec_result.is_err());
    EXPECT_EQ(counter.load(), 1);
}

// Test schedule() delegates to enqueue()
TEST_F(LockFreeJobQueueTest, ScheduleDelegatesToEnqueue) {
    lockfree_job_queue queue;

    std::atomic<int> counter{0};
    auto job = std::make_unique<callback_job>([&counter]() -> kcenon::common::VoidResult {
        counter.fetch_add(1, std::memory_order_relaxed);
        return kcenon::common::ok();
    });

    // Use schedule() method
    auto result = queue.schedule(std::move(job));
    EXPECT_FALSE(result.is_err());
    EXPECT_FALSE(queue.empty());

    // Verify job was enqueued by dequeuing it
    auto dequeue_result = queue.dequeue();
    EXPECT_TRUE(dequeue_result.is_ok());
    EXPECT_TRUE(queue.empty());
}

// Test get_next_job() delegates to dequeue()
TEST_F(LockFreeJobQueueTest, GetNextJobDelegatesToDequeue) {
    lockfree_job_queue queue;

    // First enqueue a job
    auto job = std::make_unique<callback_job>([]() -> kcenon::common::VoidResult { return kcenon::common::ok(); });
    auto enqueue_result = queue.enqueue(std::move(job));
    EXPECT_FALSE(enqueue_result.is_err());

    // Use get_next_job() method
    auto result = queue.get_next_job();
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(queue.empty());
}

// Test get_next_job() returns error when queue is empty
TEST_F(LockFreeJobQueueTest, GetNextJobReturnsErrorWhenEmpty) {
    lockfree_job_queue queue;

    EXPECT_TRUE(queue.empty());

    auto result = queue.get_next_job();
    EXPECT_FALSE(result.is_ok());
}

// Test schedule() rejects null job
TEST_F(LockFreeJobQueueTest, ScheduleRejectsNullJob) {
    lockfree_job_queue queue;

    auto result = queue.schedule(nullptr);
    EXPECT_TRUE(result.is_err());
}

// Test queue_capabilities_interface implementation
TEST_F(LockFreeJobQueueTest, ImplementsQueueCapabilitiesInterface) {
    lockfree_job_queue queue;

    // Cast to queue_capabilities_interface
    queue_capabilities_interface* cap = &queue;
    ASSERT_NE(cap, nullptr);

    // Verify capabilities
    auto caps = cap->get_capabilities();
    EXPECT_FALSE(caps.exact_size);
    EXPECT_FALSE(caps.atomic_empty_check);
    EXPECT_TRUE(caps.lock_free);
    EXPECT_FALSE(caps.wait_free);
    EXPECT_FALSE(caps.supports_batch);
    EXPECT_FALSE(caps.supports_blocking_wait);
    EXPECT_FALSE(caps.supports_stop);
}

// Test get_capabilities() returns correct values
TEST_F(LockFreeJobQueueTest, GetCapabilitiesReturnsCorrectValues) {
    lockfree_job_queue queue;
    auto caps = queue.get_capabilities();

    // Verify lock-free queue characteristics
    EXPECT_FALSE(caps.exact_size);              // Approximate only
    EXPECT_FALSE(caps.atomic_empty_check);      // Non-atomic
    EXPECT_TRUE(caps.lock_free);                // Lock-free implementation
    EXPECT_FALSE(caps.wait_free);               // Not wait-free
    EXPECT_FALSE(caps.supports_batch);          // No batch operations
    EXPECT_FALSE(caps.supports_blocking_wait);  // Spin-wait only
    EXPECT_FALSE(caps.supports_stop);           // No stop() method
}

// Test convenience methods from queue_capabilities_interface
TEST_F(LockFreeJobQueueTest, ConvenienceMethodsWork) {
    lockfree_job_queue queue;

    EXPECT_FALSE(queue.has_exact_size());
    EXPECT_FALSE(queue.has_atomic_empty());
    EXPECT_TRUE(queue.is_lock_free());
    EXPECT_FALSE(queue.is_wait_free());
    EXPECT_FALSE(queue.supports_batch());
    EXPECT_FALSE(queue.supports_blocking_wait());
    EXPECT_FALSE(queue.supports_stop());
}

// Test capabilities are consistent across multiple calls
TEST_F(LockFreeJobQueueTest, CapabilitiesAreConsistent) {
    lockfree_job_queue queue;

    auto caps1 = queue.get_capabilities();
    auto caps2 = queue.get_capabilities();

    EXPECT_EQ(caps1, caps2);
}

// Test polymorphic use through scheduler_interface
TEST_F(LockFreeJobQueueTest, PolymorphicUse) {
    auto queue = std::make_unique<lockfree_job_queue>();

    // Use through scheduler_interface pointer
    scheduler_interface* scheduler = queue.get();

    constexpr int JOB_COUNT = 10;
    std::atomic<int> counter{0};

    // Schedule multiple jobs through interface
    for (int i = 0; i < JOB_COUNT; ++i) {
        auto job = std::make_unique<callback_job>([&counter]() -> kcenon::common::VoidResult {
            counter.fetch_add(1, std::memory_order_relaxed);
            return kcenon::common::ok();
        });

        auto result = scheduler->schedule(std::move(job));
        EXPECT_FALSE(result.is_err());
    }

    // Get and execute all jobs through interface
    for (int i = 0; i < JOB_COUNT; ++i) {
        auto result = scheduler->get_next_job();
        EXPECT_TRUE(result.is_ok());

        auto& job_ptr = result.value();
        auto exec_result = job_ptr->do_work();
        EXPECT_FALSE(exec_result.is_err());
    }

    EXPECT_EQ(counter.load(), JOB_COUNT);
}

// Test dynamic_cast to queue_capabilities_interface
TEST_F(LockFreeJobQueueTest, DynamicCastToCapabilitiesInterface) {
    lockfree_job_queue queue;

    // Cast from scheduler_interface to queue_capabilities_interface
    scheduler_interface* scheduler = &queue;
    auto* cap = dynamic_cast<queue_capabilities_interface*>(scheduler);

    ASSERT_NE(cap, nullptr);
    EXPECT_TRUE(cap->is_lock_free());
    EXPECT_FALSE(cap->has_exact_size());
}

// Test destructor safety with concurrent operations (stress test)
TEST_F(LockFreeJobQueueTest, DestructorSafetyStressTest) {
    constexpr int ITERATIONS = 100;

    for (int iter = 0; iter < ITERATIONS; ++iter) {
        auto queue = std::make_unique<lockfree_job_queue>();

        std::atomic<bool> stop{false};

        // Producer thread
        std::thread producer([&queue, &stop]() {
            while (!stop.load(std::memory_order_relaxed)) {
                auto job =
                    std::make_unique<callback_job>([]() -> kcenon::common::VoidResult { return kcenon::common::ok(); });
                (void)queue->enqueue(std::move(job));
            }
        });

        // Consumer thread
        std::thread consumer([&queue, &stop]() {
            while (!stop.load(std::memory_order_relaxed)) {
                (void)queue->dequeue();
            }
        });

        // Run for a short time
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // Signal stop
        stop.store(true, std::memory_order_relaxed);

        // Wait for threads
        producer.join();
        consumer.join();

        // Queue is destroyed here - must be safe
    }

    // If we reach here without crash, destructor is safe
    SUCCEED();
}

// =============================================================================
// TICKET-002 Verification: Weak Memory Model (ARM64) Tests
// =============================================================================
// These tests specifically validate memory ordering correctness on
// weak memory model architectures (ARM, etc.) using safe_hazard_pointer.
//
// The original hazard_pointer implementation had memory ordering issues
// (CVSS 8.5) that could cause:
// - Data races under high concurrency
// - Memory leaks (non-reclaimable pointers)
// - ABA problems leading to undefined behavior
//
// The safe_hazard_pointer implementation uses explicit memory_order
// semantics to ensure correctness on all architectures.

// Test rapid enqueue/dequeue cycles with memory barriers
// This pattern exposes weak memory model issues where stores may not
// be visible immediately to other threads
TEST_F(LockFreeJobQueueTest, WeakMemoryModelRapidCycles) {
    lockfree_job_queue queue;

    constexpr int NUM_CYCLES = 1000;
    std::atomic<int> success_count{0};
    std::atomic<int> empty_count{0};

    // Producer thread: rapidly enqueues single items
    std::thread producer([&]() {
        for (int i = 0; i < NUM_CYCLES; ++i) {
            auto job = std::make_unique<callback_job>([]() -> kcenon::common::VoidResult {
                return kcenon::common::ok();
            });
            auto result = queue.enqueue(std::move(job));
            EXPECT_FALSE(result.is_err());
        }
    });

    // Consumer thread: rapidly dequeues, counts successes and empties
    std::thread consumer([&]() {
        for (int i = 0; i < NUM_CYCLES * 2; ++i) {
            auto result = queue.dequeue();
            if (result.is_ok()) {
                success_count.fetch_add(1, std::memory_order_relaxed);
            } else {
                empty_count.fetch_add(1, std::memory_order_relaxed);
            }
            std::this_thread::yield();
        }
    });

    producer.join();
    consumer.join();

    // Drain any remaining items
    while (true) {
        auto result = queue.dequeue();
        if (result.is_err()) break;
        success_count.fetch_add(1, std::memory_order_relaxed);
    }

    // All enqueued items must be dequeued exactly once
    EXPECT_EQ(success_count.load(), NUM_CYCLES);
    EXPECT_TRUE(queue.empty());
}

// Test concurrent read-modify-write operations
// This specifically tests hazard pointer protection during pointer swaps
TEST_F(LockFreeJobQueueTest, WeakMemoryModelConcurrentRMW) {
    lockfree_job_queue queue;

    constexpr int NUM_THREADS = 8;
    constexpr int OPS_PER_THREAD = 500;

    std::atomic<int> total_enqueued{0};
    std::atomic<int> total_dequeued{0};
    std::atomic<bool> stop_flag{false};

    std::vector<std::thread> threads;

    // Half threads are producers, half are consumers
    for (int t = 0; t < NUM_THREADS; ++t) {
        if (t % 2 == 0) {
            // Producer
            threads.emplace_back([&]() {
                for (int i = 0; i < OPS_PER_THREAD; ++i) {
                    auto job = std::make_unique<callback_job>([]() -> kcenon::common::VoidResult {
                        return kcenon::common::ok();
                    });
                    auto result = queue.enqueue(std::move(job));
                    if (!result.is_err()) {
                        total_enqueued.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            });
        } else {
            // Consumer
            threads.emplace_back([&]() {
                while (!stop_flag.load(std::memory_order_acquire) ||
                       total_dequeued.load(std::memory_order_relaxed) <
                       total_enqueued.load(std::memory_order_relaxed)) {
                    auto result = queue.dequeue();
                    if (result.is_ok()) {
                        total_dequeued.fetch_add(1, std::memory_order_relaxed);
                    } else {
                        std::this_thread::yield();
                    }
                }
            });
        }
    }

    // Wait for producers
    for (int t = 0; t < NUM_THREADS; t += 2) {
        threads[t].join();
    }
    stop_flag.store(true, std::memory_order_release);

    // Wait for consumers
    for (int t = 1; t < NUM_THREADS; t += 2) {
        threads[t].join();
    }

    // Drain remaining
    while (true) {
        auto result = queue.dequeue();
        if (result.is_err()) break;
        total_dequeued.fetch_add(1, std::memory_order_relaxed);
    }

    EXPECT_EQ(total_enqueued.load(), total_dequeued.load());
    EXPECT_TRUE(queue.empty());
}

// Test memory reclamation ordering on weak memory models
// This tests that retired nodes are properly synchronized before deletion
TEST_F(LockFreeJobQueueTest, WeakMemoryModelReclamationOrdering) {
    lockfree_job_queue queue;

    constexpr int NUM_ITERATIONS = 50;
    constexpr int BATCH_SIZE = 100;

    for (int iter = 0; iter < NUM_ITERATIONS; ++iter) {
        // Enqueue batch
        for (int i = 0; i < BATCH_SIZE; ++i) {
            auto job = std::make_unique<callback_job>([]() -> kcenon::common::VoidResult {
                return kcenon::common::ok();
            });
            auto result = queue.enqueue(std::move(job));
            EXPECT_FALSE(result.is_err());
        }

        // Dequeue batch from multiple threads
        std::atomic<int> batch_dequeued{0};
        std::vector<std::thread> workers;

        for (int t = 0; t < 4; ++t) {
            workers.emplace_back([&]() {
                while (batch_dequeued.load(std::memory_order_relaxed) < BATCH_SIZE) {
                    auto result = queue.dequeue();
                    if (result.is_ok()) {
                        batch_dequeued.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            });
        }

        for (auto& w : workers) {
            w.join();
        }

        EXPECT_EQ(batch_dequeued.load(), BATCH_SIZE);
        EXPECT_TRUE(queue.empty());
    }

    // If we reach here without crash or hang, memory reclamation is correct
    SUCCEED();
}
