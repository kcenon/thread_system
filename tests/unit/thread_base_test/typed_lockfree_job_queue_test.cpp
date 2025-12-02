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

/**
 * @file typed_lockfree_job_queue_test.cpp
 * @brief Tests for typed_lockfree_job_queue_t
 *
 * This test suite validates that the TLS bug (TICKET-001) has been resolved
 * and the typed lock-free job queue works correctly with Hazard Pointers.
 */

#include <gtest/gtest.h>

#include "src/impl/typed_pool/typed_lockfree_job_queue.h"
#include "src/impl/typed_pool/callback_typed_job.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace kcenon::thread;

class TypedLockFreeJobQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
        // Allow hazard pointer cleanup
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
};

// Test basic enqueue/dequeue
TEST_F(TypedLockFreeJobQueueTest, BasicEnqueueDequeue) {
    typed_lockfree_job_queue_t<job_types> queue;

    std::atomic<int> counter{0};
    std::unique_ptr<typed_job_t<job_types>> typed_job =
        std::make_unique<callback_typed_job_t<job_types>>(
            [&counter]() -> result_void {
                counter.fetch_add(1, std::memory_order_relaxed);
                return result_void();
            },
            job_types::Batch
        );

    // Enqueue
    auto enqueue_result = queue.enqueue(std::move(typed_job));
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
TEST_F(TypedLockFreeJobQueueTest, DequeueEmpty) {
    typed_lockfree_job_queue_t<job_types> queue;

    EXPECT_TRUE(queue.empty());

    auto result = queue.dequeue();
    EXPECT_FALSE(result.has_value());
}

// Test multiple types
TEST_F(TypedLockFreeJobQueueTest, MultipleTypes) {
    typed_lockfree_job_queue_t<job_types> queue;

    // Enqueue jobs of different types
    std::unique_ptr<typed_job_t<job_types>> realtime_job =
        std::make_unique<callback_typed_job_t<job_types>>(
            []() -> result_void { return result_void(); },
            job_types::RealTime
        );

    std::unique_ptr<typed_job_t<job_types>> batch_job =
        std::make_unique<callback_typed_job_t<job_types>>(
            []() -> result_void { return result_void(); },
            job_types::Batch
        );

    std::unique_ptr<typed_job_t<job_types>> background_job =
        std::make_unique<callback_typed_job_t<job_types>>(
            []() -> result_void { return result_void(); },
            job_types::Background
        );

    (void)queue.enqueue(std::move(background_job));
    (void)queue.enqueue(std::move(batch_job));
    (void)queue.enqueue(std::move(realtime_job));

    EXPECT_EQ(queue.size(), 3);

    // Dequeue all
    int count = 0;
    while (auto result = queue.dequeue()) {
        (void)result.value()->do_work();
        ++count;
    }

    EXPECT_EQ(count, 3);
    EXPECT_TRUE(queue.empty());
}

// Test concurrent enqueue
TEST_F(TypedLockFreeJobQueueTest, ConcurrentEnqueue) {
    typed_lockfree_job_queue_t<job_types> queue;

    constexpr int NUM_THREADS = 4;
    constexpr int JOBS_PER_THREAD = 100;
    std::atomic<int> counter{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&queue, &counter]() {
            for (int i = 0; i < JOBS_PER_THREAD; ++i) {
                std::unique_ptr<typed_job_t<job_types>> job =
                    std::make_unique<callback_typed_job_t<job_types>>(
                        [&counter]() -> result_void {
                            counter.fetch_add(1, std::memory_order_relaxed);
                            return result_void();
                        },
                        job_types::Batch
                    );

                auto result = queue.enqueue(std::move(job));
                EXPECT_FALSE(result.has_error());
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Dequeue and execute all jobs
    int dequeued = 0;
    while (auto result = queue.dequeue()) {
        auto& job_ptr = result.value();
        (void)job_ptr->do_work();
        ++dequeued;
    }

    EXPECT_EQ(dequeued, NUM_THREADS * JOBS_PER_THREAD);
    EXPECT_EQ(counter.load(), NUM_THREADS * JOBS_PER_THREAD);
}

// =============================================================================
// TICKET-001 Verification: Thread Churn Test for Typed Queue
// =============================================================================
TEST_F(TypedLockFreeJobQueueTest, ThreadChurnTest) {
    typed_lockfree_job_queue_t<job_types> queue;

    constexpr int TOTAL_ITEMS = 500;
    std::atomic<int> consumed{0};
    std::atomic<bool> producers_done{false};

    // Consumer thread (long-running)
    std::thread consumer([&]() {
        while (consumed.load(std::memory_order_relaxed) < TOTAL_ITEMS) {
            auto result = queue.dequeue();
            if (result.has_value()) {
                auto& job_ptr = result.value();
                (void)job_ptr->do_work();
                consumed.fetch_add(1, std::memory_order_relaxed);
            } else if (producers_done.load(std::memory_order_acquire)) {
                break;
            } else {
                std::this_thread::yield();
            }
        }
    });

    // Short-lived producer threads
    for (int i = 0; i < TOTAL_ITEMS; ++i) {
        std::thread producer([&queue, i]() {
            std::unique_ptr<typed_job_t<job_types>> job =
                std::make_unique<callback_typed_job_t<job_types>>(
                    []() -> result_void {
                        return result_void();
                    },
                    static_cast<job_types>(i % 3)  // Rotate through types
                );

            auto result = queue.enqueue(std::move(job));
            EXPECT_FALSE(result.has_error());
        });
        producer.join();
    }

    producers_done.store(true, std::memory_order_release);
    consumer.join();

    EXPECT_EQ(consumed.load(), TOTAL_ITEMS);
    EXPECT_TRUE(queue.empty());
}

// Test concurrent MPMC
TEST_F(TypedLockFreeJobQueueTest, ConcurrentMPMC) {
    typed_lockfree_job_queue_t<job_types> queue;

    constexpr int NUM_PRODUCERS = 2;
    constexpr int NUM_CONSUMERS = 2;
    constexpr int JOBS_PER_PRODUCER = 200;

    std::atomic<int> enqueued{0};
    std::atomic<int> dequeued{0};
    std::atomic<bool> producers_done{false};

    std::vector<std::thread> threads;

    // Producer threads
    for (int t = 0; t < NUM_PRODUCERS; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < JOBS_PER_PRODUCER; ++i) {
                std::unique_ptr<typed_job_t<job_types>> job =
                    std::make_unique<callback_typed_job_t<job_types>>(
                        []() -> result_void { return result_void(); },
                        job_types::Batch
                    );

                auto result = queue.enqueue(std::move(job));
                if (!result.has_error()) {
                    enqueued.fetch_add(1, std::memory_order_relaxed);
                }

                std::this_thread::yield();
            }
        });
    }

    // Consumer threads
    for (int t = 0; t < NUM_CONSUMERS; ++t) {
        threads.emplace_back([&]() {
            while (true) {
                auto result = queue.dequeue();
                if (result.has_value()) {
                    (void)result.value()->do_work();
                    dequeued.fetch_add(1, std::memory_order_relaxed);
                } else if (producers_done.load(std::memory_order_acquire)) {
                    break;
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    // Wait for producers
    for (int t = 0; t < NUM_PRODUCERS; ++t) {
        threads[t].join();
    }

    producers_done.store(true, std::memory_order_release);

    // Wait for consumers
    for (int t = NUM_PRODUCERS; t < NUM_PRODUCERS + NUM_CONSUMERS; ++t) {
        threads[t].join();
    }

    EXPECT_EQ(enqueued.load(), NUM_PRODUCERS * JOBS_PER_PRODUCER);
    EXPECT_EQ(dequeued.load(), NUM_PRODUCERS * JOBS_PER_PRODUCER);
    EXPECT_TRUE(queue.empty());
}

// Test statistics
TEST_F(TypedLockFreeJobQueueTest, Statistics) {
    typed_lockfree_job_queue_t<job_types> queue;

    for (int i = 0; i < 10; ++i) {
        std::unique_ptr<typed_job_t<job_types>> job =
            std::make_unique<callback_typed_job_t<job_types>>(
                []() -> result_void { return result_void(); },
                job_types::RealTime
            );
        (void)queue.enqueue(std::move(job));
    }

    for (int i = 0; i < 5; ++i) {
        std::unique_ptr<typed_job_t<job_types>> job =
            std::make_unique<callback_typed_job_t<job_types>>(
                []() -> result_void { return result_void(); },
                job_types::Batch
            );
        (void)queue.enqueue(std::move(job));
    }

    EXPECT_EQ(queue.size(), 15);
    EXPECT_EQ(queue.size(job_types::RealTime), 10);
    EXPECT_EQ(queue.size(job_types::Batch), 5);

    auto sizes = queue.get_sizes();
    EXPECT_EQ(sizes.at(job_types::RealTime), 10);
    EXPECT_EQ(sizes.at(job_types::Batch), 5);
}
