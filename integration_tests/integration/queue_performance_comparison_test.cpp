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

#include <kcenon/thread/core/job_queue.h>
#include <kcenon/thread/policies/policy_queue.h>
#include <kcenon/thread/core/callback_job.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using namespace kcenon::thread;
using namespace kcenon::common;
using namespace integration_tests;

/**
 * @brief Performance comparison tests between legacy job_queue and policy_queue
 *
 * Goal: Compare performance of different queue implementations
 * Expected time: < 60 seconds
 * Test scenarios:
 *   1. Single-threaded enqueue throughput
 *   2. Concurrent enqueue throughput
 *   3. Mixed enqueue/dequeue throughput
 */
class QueuePerformanceComparisonTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {
        std::this_thread::yield();
    }

    /**
     * @brief Measure enqueue throughput for any queue type
     */
    template<typename QueueType, typename EnqueueFunc>
    double MeasureEnqueueThroughput(QueueType& queue, EnqueueFunc enqueue_fn,
                                    size_t job_count) {
        auto start = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < job_count; ++i) {
            auto job = std::make_unique<callback_job>(
                []() -> VoidResult { return ok(); });
            enqueue_fn(queue, std::move(job));
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

        return CalculateThroughput(job_count, duration);
    }

    /**
     * @brief Measure concurrent enqueue/dequeue throughput
     */
    template<typename QueueType, typename EnqueueFunc, typename TryDequeueFunc>
    double MeasureConcurrentThroughput(QueueType& queue,
                                        EnqueueFunc enqueue_fn,
                                        TryDequeueFunc try_dequeue_fn,
                                        size_t job_count,
                                        size_t num_producers,
                                        size_t num_consumers) {
        std::atomic<size_t> enqueued{0};
        std::atomic<size_t> dequeued{0};
        std::atomic<bool> producers_done{false};

        auto start = std::chrono::high_resolution_clock::now();

        // Start producers
        std::vector<std::thread> producers;
        size_t jobs_per_producer = job_count / num_producers;
        for (size_t t = 0; t < num_producers; ++t) {
            producers.emplace_back([&queue, &enqueue_fn, &enqueued, jobs_per_producer]() {
                for (size_t i = 0; i < jobs_per_producer; ++i) {
                    auto job = std::make_unique<callback_job>(
                        []() -> VoidResult { return ok(); });
                    enqueue_fn(queue, std::move(job));
                    enqueued.fetch_add(1);
                }
            });
        }

        // Start consumers
        std::vector<std::thread> consumers;
        for (size_t t = 0; t < num_consumers; ++t) {
            consumers.emplace_back([&queue, &try_dequeue_fn, &dequeued, &producers_done]() {
                while (!producers_done.load() || dequeued.load() < 1) {
                    if (try_dequeue_fn(queue)) {
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

        // Give consumers time to finish
        auto wait_start = std::chrono::steady_clock::now();
        while (dequeued.load() < enqueued.load()) {
            if (std::chrono::steady_clock::now() - wait_start > std::chrono::seconds(5)) {
                break;
            }
            std::this_thread::yield();
        }

        // Stop consumers
        for (auto& t : consumers) {
            if (t.joinable()) {
                t.join();
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

        return CalculateThroughput(enqueued.load() + dequeued.load(), duration);
    }
};

// ============================================================================
// Single-threaded Throughput Comparison
// ============================================================================

TEST_F(QueuePerformanceComparisonTest, SingleThreadedEnqueueThroughput) {
    const size_t job_count = 10000;

    // Legacy job_queue
    auto legacy_queue = std::make_shared<job_queue>();
    legacy_queue->set_notify(true);

    double legacy_throughput = MeasureEnqueueThroughput(
        *legacy_queue,
        [](job_queue& q, std::unique_ptr<job>&& j) {
            q.enqueue(std::move(j));
        },
        job_count);

    // Standard policy_queue (mutex-based)
    standard_queue policy_std_queue;
    double policy_std_throughput = MeasureEnqueueThroughput(
        policy_std_queue,
        [](standard_queue& q, std::unique_ptr<job>&& j) {
            q.enqueue(std::move(j));
        },
        job_count);

    // Lock-free policy_queue
    policy_lockfree_queue policy_lf_queue;
    double policy_lf_throughput = MeasureEnqueueThroughput(
        policy_lf_queue,
        [](policy_lockfree_queue& q, std::unique_ptr<job>&& j) {
            q.enqueue(std::move(j));
        },
        job_count);

    std::cout << "\n=== Single-threaded Enqueue Throughput ===\n"
              << "  Legacy job_queue:      " << legacy_throughput << " ops/sec\n"
              << "  Standard policy_queue: " << policy_std_throughput << " ops/sec\n"
              << "  Lock-free policy_queue: " << policy_lf_throughput << " ops/sec\n"
              << std::endl;

    // No regression check - all should have reasonable throughput
    EXPECT_GT(legacy_throughput, 50000);
    EXPECT_GT(policy_std_throughput, 50000);
    EXPECT_GT(policy_lf_throughput, 50000);
}

TEST_F(QueuePerformanceComparisonTest, SingleThreadedDequeueLatency) {
    const size_t job_count = 1000;

    // Prepare legacy queue
    auto legacy_queue = std::make_shared<job_queue>();
    legacy_queue->set_notify(true);
    for (size_t i = 0; i < job_count; ++i) {
        auto job = std::make_unique<callback_job>(
            []() -> VoidResult { return ok(); });
        legacy_queue->enqueue(std::move(job));
    }

    // Prepare policy queue
    standard_queue policy_queue;
    for (size_t i = 0; i < job_count; ++i) {
        auto job = std::make_unique<callback_job>(
            []() -> VoidResult { return ok(); });
        policy_queue.enqueue(std::move(job));
    }

    // Measure legacy dequeue latency
    PerformanceMetrics legacy_metrics;
    for (size_t i = 0; i < job_count; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = legacy_queue->try_dequeue();
        auto end = std::chrono::high_resolution_clock::now();
        legacy_metrics.add_sample(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start));
        (void)result;
    }

    // Measure policy queue dequeue latency
    PerformanceMetrics policy_metrics;
    for (size_t i = 0; i < job_count; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = policy_queue.try_dequeue();
        auto end = std::chrono::high_resolution_clock::now();
        policy_metrics.add_sample(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start));
        (void)result;
    }

    std::cout << "\n=== Single-threaded Dequeue Latency ===\n"
              << "  Legacy job_queue:\n"
              << "    Mean: " << legacy_metrics.mean() << " ns\n"
              << "    P50:  " << legacy_metrics.p50() << " ns\n"
              << "    P95:  " << legacy_metrics.p95() << " ns\n"
              << "    P99:  " << legacy_metrics.p99() << " ns\n"
              << "  Standard policy_queue:\n"
              << "    Mean: " << policy_metrics.mean() << " ns\n"
              << "    P50:  " << policy_metrics.p50() << " ns\n"
              << "    P95:  " << policy_metrics.p95() << " ns\n"
              << "    P99:  " << policy_metrics.p99() << " ns\n"
              << std::endl;

    // Both should have reasonable latency
    EXPECT_LT(legacy_metrics.mean(), 10000000);  // < 10ms mean
    EXPECT_LT(policy_metrics.mean(), 10000000);
}

// ============================================================================
// Multi-threaded Throughput Comparison
// ============================================================================

TEST_F(QueuePerformanceComparisonTest, ConcurrentEnqueueThroughput) {
    const size_t job_count = 4000;  // Divisible by 4
    const size_t num_threads = 4;

    // Legacy job_queue
    auto legacy_queue = std::make_shared<job_queue>();
    legacy_queue->set_notify(true);

    std::atomic<size_t> legacy_enqueued{0};
    auto legacy_start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> legacy_producers;
    size_t jobs_per_thread = job_count / num_threads;
    for (size_t t = 0; t < num_threads; ++t) {
        legacy_producers.emplace_back([&legacy_queue, &legacy_enqueued, jobs_per_thread]() {
            for (size_t i = 0; i < jobs_per_thread; ++i) {
                auto job = std::make_unique<callback_job>(
                    []() -> VoidResult { return ok(); });
                if (legacy_queue->enqueue(std::move(job)).is_ok()) {
                    legacy_enqueued.fetch_add(1);
                }
            }
        });
    }
    for (auto& t : legacy_producers) {
        t.join();
    }
    auto legacy_end = std::chrono::high_resolution_clock::now();
    auto legacy_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        legacy_end - legacy_start);
    double legacy_throughput = CalculateThroughput(legacy_enqueued.load(), legacy_duration);

    // Standard policy_queue
    standard_queue policy_std_queue;

    std::atomic<size_t> policy_enqueued{0};
    auto policy_start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> policy_producers;
    for (size_t t = 0; t < num_threads; ++t) {
        policy_producers.emplace_back([&policy_std_queue, &policy_enqueued, jobs_per_thread]() {
            for (size_t i = 0; i < jobs_per_thread; ++i) {
                auto job = std::make_unique<callback_job>(
                    []() -> VoidResult { return ok(); });
                if (policy_std_queue.enqueue(std::move(job)).is_ok()) {
                    policy_enqueued.fetch_add(1);
                }
            }
        });
    }
    for (auto& t : policy_producers) {
        t.join();
    }
    auto policy_end = std::chrono::high_resolution_clock::now();
    auto policy_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        policy_end - policy_start);
    double policy_throughput = CalculateThroughput(policy_enqueued.load(), policy_duration);

    std::cout << "\n=== Concurrent Enqueue Throughput (4 threads) ===\n"
              << "  Legacy job_queue:      " << legacy_throughput << " ops/sec\n"
              << "  Standard policy_queue: " << policy_throughput << " ops/sec\n"
              << std::endl;

    EXPECT_EQ(legacy_enqueued.load(), job_count);
    EXPECT_EQ(policy_enqueued.load(), job_count);
}

TEST_F(QueuePerformanceComparisonTest, MixedOperationsThroughput) {
    const size_t job_count = 2000;
    const size_t num_producers = 2;
    const size_t num_consumers = 2;

    // Legacy job_queue
    auto legacy_queue = std::make_shared<job_queue>();
    legacy_queue->set_notify(true);

    std::atomic<size_t> legacy_enqueued{0};
    std::atomic<size_t> legacy_dequeued{0};
    std::atomic<bool> legacy_producers_done{false};

    auto legacy_start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> legacy_producers;
    size_t jobs_per_producer = job_count / num_producers;
    for (size_t t = 0; t < num_producers; ++t) {
        legacy_producers.emplace_back([&legacy_queue, &legacy_enqueued, jobs_per_producer]() {
            for (size_t i = 0; i < jobs_per_producer; ++i) {
                auto job = std::make_unique<callback_job>(
                    []() -> VoidResult { return ok(); });
                if (legacy_queue->enqueue(std::move(job)).is_ok()) {
                    legacy_enqueued.fetch_add(1);
                }
            }
        });
    }

    std::vector<std::thread> legacy_consumers;
    for (size_t t = 0; t < num_consumers; ++t) {
        legacy_consumers.emplace_back([&legacy_queue, &legacy_dequeued, &legacy_producers_done,
                                       &legacy_enqueued]() {
            while (!legacy_producers_done.load() || legacy_dequeued.load() < legacy_enqueued.load()) {
                if (legacy_queue->try_dequeue().is_ok()) {
                    legacy_dequeued.fetch_add(1);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    for (auto& t : legacy_producers) {
        t.join();
    }
    legacy_producers_done.store(true);

    for (auto& t : legacy_consumers) {
        t.join();
    }

    auto legacy_end = std::chrono::high_resolution_clock::now();
    auto legacy_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        legacy_end - legacy_start);
    double legacy_throughput = CalculateThroughput(
        legacy_enqueued.load() + legacy_dequeued.load(), legacy_duration);

    // Standard policy_queue
    standard_queue policy_queue;

    std::atomic<size_t> policy_enqueued{0};
    std::atomic<size_t> policy_dequeued{0};
    std::atomic<bool> policy_producers_done{false};

    auto policy_start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> policy_producers;
    for (size_t t = 0; t < num_producers; ++t) {
        policy_producers.emplace_back([&policy_queue, &policy_enqueued, jobs_per_producer]() {
            for (size_t i = 0; i < jobs_per_producer; ++i) {
                auto job = std::make_unique<callback_job>(
                    []() -> VoidResult { return ok(); });
                if (policy_queue.enqueue(std::move(job)).is_ok()) {
                    policy_enqueued.fetch_add(1);
                }
            }
        });
    }

    std::vector<std::thread> policy_consumers;
    for (size_t t = 0; t < num_consumers; ++t) {
        policy_consumers.emplace_back([&policy_queue, &policy_dequeued, &policy_producers_done,
                                       &policy_enqueued]() {
            while (!policy_producers_done.load() || policy_dequeued.load() < policy_enqueued.load()) {
                if (policy_queue.try_dequeue().is_ok()) {
                    policy_dequeued.fetch_add(1);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    for (auto& t : policy_producers) {
        t.join();
    }
    policy_producers_done.store(true);

    for (auto& t : policy_consumers) {
        t.join();
    }

    auto policy_end = std::chrono::high_resolution_clock::now();
    auto policy_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        policy_end - policy_start);
    double policy_throughput = CalculateThroughput(
        policy_enqueued.load() + policy_dequeued.load(), policy_duration);

    std::cout << "\n=== Mixed Operations Throughput (2 producers, 2 consumers) ===\n"
              << "  Legacy job_queue:      " << legacy_throughput << " ops/sec\n"
              << "  Standard policy_queue: " << policy_throughput << " ops/sec\n"
              << std::endl;

    EXPECT_EQ(legacy_enqueued.load(), job_count);
    EXPECT_EQ(legacy_dequeued.load(), job_count);
    EXPECT_EQ(policy_enqueued.load(), job_count);
    EXPECT_EQ(policy_dequeued.load(), job_count);
}

// ============================================================================
// Lock-free vs Mutex Comparison
// ============================================================================

TEST_F(QueuePerformanceComparisonTest, LockfreeVsMutexComparison) {
    const size_t job_count = 4000;
    const size_t num_producers = 4;

    // Mutex-based queue
    standard_queue mutex_queue;

    std::atomic<size_t> mutex_enqueued{0};
    auto mutex_start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> mutex_producers;
    size_t jobs_per_thread = job_count / num_producers;
    for (size_t t = 0; t < num_producers; ++t) {
        mutex_producers.emplace_back([&mutex_queue, &mutex_enqueued, jobs_per_thread]() {
            for (size_t i = 0; i < jobs_per_thread; ++i) {
                auto job = std::make_unique<callback_job>(
                    []() -> VoidResult { return ok(); });
                if (mutex_queue.enqueue(std::move(job)).is_ok()) {
                    mutex_enqueued.fetch_add(1);
                }
            }
        });
    }
    for (auto& t : mutex_producers) {
        t.join();
    }
    auto mutex_end = std::chrono::high_resolution_clock::now();
    auto mutex_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        mutex_end - mutex_start);
    double mutex_throughput = CalculateThroughput(mutex_enqueued.load(), mutex_duration);

    // Lock-free queue
    policy_lockfree_queue lf_queue;

    std::atomic<size_t> lf_enqueued{0};
    auto lf_start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> lf_producers;
    for (size_t t = 0; t < num_producers; ++t) {
        lf_producers.emplace_back([&lf_queue, &lf_enqueued, jobs_per_thread]() {
            for (size_t i = 0; i < jobs_per_thread; ++i) {
                auto job = std::make_unique<callback_job>(
                    []() -> VoidResult { return ok(); });
                if (lf_queue.enqueue(std::move(job)).is_ok()) {
                    lf_enqueued.fetch_add(1);
                }
            }
        });
    }
    for (auto& t : lf_producers) {
        t.join();
    }
    auto lf_end = std::chrono::high_resolution_clock::now();
    auto lf_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        lf_end - lf_start);
    double lf_throughput = CalculateThroughput(lf_enqueued.load(), lf_duration);

    std::cout << "\n=== Lock-free vs Mutex Queue (4 threads) ===\n"
              << "  Mutex-based (standard_queue): " << mutex_throughput << " ops/sec\n"
              << "  Lock-free (policy_lockfree_queue): " << lf_throughput << " ops/sec\n"
              << std::endl;

    EXPECT_EQ(mutex_enqueued.load(), job_count);
    EXPECT_EQ(lf_enqueued.load(), job_count);
}
