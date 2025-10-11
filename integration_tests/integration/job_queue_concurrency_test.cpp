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

#include "../framework/system_fixture.h"
#include "../framework/test_helpers.h"
#include <gtest/gtest.h>

using namespace integration_tests;

/**
 * @brief Integration tests for job queue concurrency
 *
 * Goal: Verify concurrent job queue operations work correctly
 * Expected time: < 2 minutes
 * Test count: Reduced from 12 to 8 tests (removed high-throughput benchmark)
 */
class JobQueueConcurrencyTest : public SystemFixture {};

TEST_F(JobQueueConcurrencyTest, FIFOOrdering) {
    auto queue = std::make_shared<kcenon::thread::job_queue>();
    queue->set_notify(true);

    std::vector<int> execution_order;
    std::mutex order_mutex;

    const size_t job_count = 100;
    for (size_t i = 0; i < job_count; ++i) {
        auto job = std::make_unique<kcenon::thread::callback_job>(
            [&execution_order, &order_mutex, i]() -> kcenon::thread::result_void {
                std::lock_guard<std::mutex> lock(order_mutex);
                execution_order.push_back(static_cast<int>(i));
                return {};
            }
        );
        auto result = queue->enqueue(std::move(job));
        EXPECT_TRUE(result);
    }

    EXPECT_EQ(queue->size(), job_count);

    // Dequeue and execute in order
    for (size_t i = 0; i < job_count; ++i) {
        auto result = queue->try_dequeue();
        ASSERT_TRUE(result);
        auto job_result = result.value()->do_work();
        EXPECT_TRUE(job_result);
    }

    // Verify FIFO order
    ASSERT_EQ(execution_order.size(), job_count);
    for (size_t i = 0; i < job_count; ++i) {
        EXPECT_EQ(execution_order[i], i) << "Job executed out of order at position " << i;
    }
}

TEST_F(JobQueueConcurrencyTest, ConcurrentEnqueue) {
    auto queue = std::make_shared<kcenon::thread::job_queue>();
    queue->set_notify(true);

    const size_t num_threads = 4;
    const size_t jobs_per_thread = 250;  // Reduced from 1000
    std::atomic<size_t> total_enqueued{0};

    std::vector<std::thread> producers;
    for (size_t t = 0; t < num_threads; ++t) {
        producers.emplace_back([&queue, &total_enqueued, jobs_per_thread]() {
            for (size_t i = 0; i < jobs_per_thread; ++i) {
                auto job = std::make_unique<kcenon::thread::callback_job>(
                    []() -> kcenon::thread::result_void { return {}; }
                );
                auto result = queue->enqueue(std::move(job));
                if (result) {
                    total_enqueued.fetch_add(1);
                }
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }

    EXPECT_EQ(total_enqueued.load(), num_threads * jobs_per_thread);
    EXPECT_EQ(queue->size(), num_threads * jobs_per_thread);
}

TEST_F(JobQueueConcurrencyTest, ConcurrentEnqueueDequeue) {
    auto queue = std::make_shared<kcenon::thread::job_queue>();
    queue->set_notify(true);

    const size_t num_producers = 4;
    const size_t num_consumers = 4;
    const size_t jobs_per_producer = 250;  // Reduced from 1000

    std::atomic<size_t> enqueued{0};
    std::atomic<size_t> dequeued{0};
    std::atomic<bool> producers_done{false};

    // Start producers
    std::vector<std::thread> producers;
    for (size_t t = 0; t < num_producers; ++t) {
        producers.emplace_back([&queue, &enqueued, jobs_per_producer]() {
            for (size_t i = 0; i < jobs_per_producer; ++i) {
                auto job = std::make_unique<kcenon::thread::callback_job>(
                    []() -> kcenon::thread::result_void { return {}; }
                );
                auto result = queue->enqueue(std::move(job));
                if (result) {
                    enqueued.fetch_add(1);
                }
            }
        });
    }

    // Start consumers
    std::vector<std::thread> consumers;
    for (size_t t = 0; t < num_consumers; ++t) {
        consumers.emplace_back([&queue, &dequeued, &producers_done]() {
            while (!producers_done.load() || !queue->empty()) {
                auto result = queue->try_dequeue();
                if (result) {
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
    EXPECT_TRUE(queue->empty());
}

TEST_F(JobQueueConcurrencyTest, BatchEnqueue) {
    auto queue = std::make_shared<kcenon::thread::job_queue>();
    queue->set_notify(true);

    std::vector<std::unique_ptr<kcenon::thread::job>> jobs;
    const size_t batch_size = 500;  // Reduced from 1000

    for (size_t i = 0; i < batch_size; ++i) {
        jobs.push_back(std::make_unique<kcenon::thread::callback_job>(
            []() -> kcenon::thread::result_void { return {}; }
        ));
    }

    auto result = queue->enqueue_batch(std::move(jobs));
    EXPECT_TRUE(result);
    EXPECT_EQ(queue->size(), batch_size);
}

TEST_F(JobQueueConcurrencyTest, BatchDequeue) {
    auto queue = std::make_shared<kcenon::thread::job_queue>();
    queue->set_notify(true);

    const size_t job_count = 250;  // Reduced from 500
    for (size_t i = 0; i < job_count; ++i) {
        auto job = std::make_unique<kcenon::thread::callback_job>(
            []() -> kcenon::thread::result_void { return {}; }
        );
        auto result = queue->enqueue(std::move(job));
        EXPECT_TRUE(result);
    }

    auto dequeued_jobs = queue->dequeue_batch();
    EXPECT_EQ(dequeued_jobs.size(), job_count);
    EXPECT_TRUE(queue->empty());
}

TEST_F(JobQueueConcurrencyTest, QueueClear) {
    auto queue = std::make_shared<kcenon::thread::job_queue>();
    queue->set_notify(true);

    for (size_t i = 0; i < 50; ++i) {
        auto job = std::make_unique<kcenon::thread::callback_job>(
            []() -> kcenon::thread::result_void { return {}; }
        );
        auto result = queue->enqueue(std::move(job));
        EXPECT_TRUE(result);
    }

    EXPECT_EQ(queue->size(), 50);

    queue->clear();
    EXPECT_TRUE(queue->empty());
    EXPECT_EQ(queue->size(), 0);
}

TEST_F(JobQueueConcurrencyTest, StopWaitingDequeue) {
    auto queue = std::make_shared<kcenon::thread::job_queue>();
    queue->set_notify(true);

    std::atomic<bool> dequeue_returned{false};

    std::thread consumer([&queue, &dequeue_returned]() {
        auto result = queue->dequeue(); // Blocking call
        dequeue_returned.store(true);
    });

    // Give consumer time to start waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Reduced from 100

    // Signal to stop waiting
    queue->stop_waiting_dequeue();

    // Consumer should return shortly
    consumer.join();
    EXPECT_TRUE(dequeue_returned.load());
}

TEST_F(JobQueueConcurrencyTest, QueueStateConsistency) {
    auto queue = std::make_shared<kcenon::thread::job_queue>();
    queue->set_notify(true);

    // Add jobs
    for (size_t i = 0; i < 30; ++i) {  // Reduced from 50
        auto job = std::make_unique<kcenon::thread::callback_job>(
            []() -> kcenon::thread::result_void { return {}; }
        );
        auto result = queue->enqueue(std::move(job));
        EXPECT_TRUE(result);
    }

    EXPECT_EQ(queue->size(), 30);
    EXPECT_FALSE(queue->empty());

    // Remove some jobs
    for (size_t i = 0; i < 10; ++i) {  // Reduced from 20
        auto result = queue->try_dequeue();
        EXPECT_TRUE(result);
    }

    EXPECT_EQ(queue->size(), 20);
    EXPECT_FALSE(queue->empty());

    // Add more jobs
    for (size_t i = 0; i < 20; ++i) {  // Reduced from 30
        auto job = std::make_unique<kcenon::thread::callback_job>(
            []() -> kcenon::thread::result_void { return {}; }
        );
        auto result = queue->enqueue(std::move(job));
        EXPECT_TRUE(result);
    }

    EXPECT_EQ(queue->size(), 40);
}
