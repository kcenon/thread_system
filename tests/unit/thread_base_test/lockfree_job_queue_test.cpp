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

#include <kcenon/thread/lockfree/lockfree_job_queue.h>
#include <kcenon/thread/core/callback_job.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace kcenon::thread;

class LockFreeJobQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset any global state if needed
    }

    void TearDown() override {
        // Allow hazard pointer cleanup
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
};

// Test basic enqueue/dequeue
TEST_F(LockFreeJobQueueTest, BasicEnqueueDequeue) {
    lockfree_job_queue queue;

    std::atomic<int> counter{0};
    auto job = std::make_unique<callback_job>([&counter]() -> result_void {
        counter.fetch_add(1, std::memory_order_relaxed);
        return result_void();  // Success
    });

    // Enqueue
    auto enqueue_result = queue.enqueue(std::move(job));
    EXPECT_FALSE(enqueue_result.has_error());
    EXPECT_FALSE(queue.empty());

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

// Test dequeue from empty queue
TEST_F(LockFreeJobQueueTest, DequeueEmpty) {
    lockfree_job_queue queue;

    EXPECT_TRUE(queue.empty());

    auto result = queue.dequeue();
    EXPECT_FALSE(result.has_value());
}

// Test null job rejection
TEST_F(LockFreeJobQueueTest, NullJobRejection) {
    lockfree_job_queue queue;

    auto result = queue.enqueue(nullptr);
    EXPECT_TRUE(result.has_error());
}

// Test multiple enqueue/dequeue
TEST_F(LockFreeJobQueueTest, MultipleOperations) {
    lockfree_job_queue queue;

    constexpr int COUNT = 100;
    std::atomic<int> counter{0};

    // Enqueue multiple jobs
    for (int i = 0; i < COUNT; ++i) {
        auto job = std::make_unique<callback_job>([&counter]() -> result_void {
            counter.fetch_add(1, std::memory_order_relaxed);
            return result_void();
        });

        auto result = queue.enqueue(std::move(job));
        EXPECT_FALSE(result.has_error());
    }

    EXPECT_FALSE(queue.empty());

    // Dequeue and execute all jobs
    for (int i = 0; i < COUNT; ++i) {
        auto result = queue.dequeue();
        EXPECT_TRUE(result.has_value());

        auto& job_ptr = result.value();
        auto exec_result = job_ptr->do_work();
        EXPECT_FALSE(exec_result.has_error());
    }

    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(counter.load(), COUNT);

    // Queue should be empty now
    auto result = queue.dequeue();
    EXPECT_FALSE(result.has_value());
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
                auto job = std::make_unique<callback_job>([&counter]() -> result_void {
                    counter.fetch_add(1, std::memory_order_relaxed);
                    return result_void();
                });

                auto result = queue.enqueue(std::move(job));
                EXPECT_FALSE(result.has_error());
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_FALSE(queue.empty());

    // Dequeue all jobs
    int dequeued = 0;
    while (auto result = queue.dequeue()) {
        EXPECT_TRUE(result.has_value());
        auto& job_ptr = result.value();
        auto exec_result = job_ptr->do_work();
        EXPECT_FALSE(exec_result.has_error());
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
        auto job = std::make_unique<callback_job>([&counter]() -> result_void {
            counter.fetch_add(1, std::memory_order_relaxed);
            return result_void();
        });

        auto result = queue.enqueue(std::move(job));
        EXPECT_FALSE(result.has_error());
    }

    // Multiple consumer threads
    constexpr int NUM_CONSUMERS = 4;
    std::atomic<int> jobs_processed{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < NUM_CONSUMERS; ++t) {
        threads.emplace_back([&queue, &jobs_processed]() {
            while (true) {
                auto result = queue.dequeue();
                if (!result.has_value()) {
                    break;  // Queue empty
                }

                auto& job_ptr = result.value();
                auto exec_result = job_ptr->do_work();
                EXPECT_FALSE(exec_result.has_error());

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
                auto job = std::make_unique<callback_job>([&enqueued]() -> result_void {
                    enqueued.fetch_add(1, std::memory_order_relaxed);
                    return result_void();
                });

                auto result = queue.enqueue(std::move(job));
                EXPECT_FALSE(result.has_error());

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
                if (result.has_value()) {
                    auto& job_ptr = result.value();
                    auto exec_result = job_ptr->do_work();
                    EXPECT_FALSE(exec_result.has_error());

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
            auto job = std::make_unique<callback_job>([]() -> result_void {
                return result_void();
            });

            auto result = queue.enqueue(std::move(job));
            EXPECT_FALSE(result.has_error());
        }

        // Dequeue all
        for (int i = 0; i < JOBS_PER_ITERATION; ++i) {
            auto result = queue.dequeue();
            EXPECT_TRUE(result.has_value());
        }

        EXPECT_TRUE(queue.empty());

        // Allow hazard pointer reclamation
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // If we reach here without segfault, hazard pointers are working
    SUCCEED();
}

// Test queue destruction with pending jobs (memory safety)
TEST_F(LockFreeJobQueueTest, DestructionWithPendingJobs) {
    {
        lockfree_job_queue queue;

        for (int i = 0; i < 100; ++i) {
            auto job = std::make_unique<callback_job>([]() -> result_void {
                return result_void();
            });

            auto result = queue.enqueue(std::move(job));
            EXPECT_FALSE(result.has_error());
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
                    auto job = std::make_unique<callback_job>([]() -> result_void {
                        return result_void();
                    });

                    auto result = queue.enqueue(std::move(job));
                    if (!result.has_error()) {
                        total_enqueued.fetch_add(1, std::memory_order_relaxed);
                    }
                } else {
                    // Dequeue
                    auto result = queue.dequeue();
                    if (result.has_value()) {
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
    while (auto result = queue.dequeue()) {
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
            if (result.has_value()) {
                auto& job_ptr = result.value();
                auto exec_result = job_ptr->do_work();
                EXPECT_FALSE(exec_result.has_error());
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
            auto job = std::make_unique<callback_job>([i]() -> result_void {
                // Simple work
                return result_void();
            });

            auto result = queue.enqueue(std::move(job));
            EXPECT_FALSE(result.has_error());
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
                auto job = std::make_unique<callback_job>([]() -> result_void {
                    return result_void();
                });

                auto result = queue.enqueue(std::move(job));
                if (!result.has_error()) {
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
            if (result.has_value()) {
                auto& job_ptr = result.value();
                job_ptr->do_work();
                total_dequeued.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }

    // Drain remaining items
    while (auto result = queue.dequeue()) {
        auto& job_ptr = result.value();
        job_ptr->do_work();
        total_dequeued.fetch_add(1, std::memory_order_relaxed);
    }

    EXPECT_EQ(total_enqueued.load(), total_dequeued.load());
    EXPECT_TRUE(queue.empty());
}
