/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.
*****************************************************************************/

/**
 * @file work_stealing_benchmark.cpp
 * @brief Benchmarks for work-stealing scheduler performance
 *
 * Tests various workload patterns to measure work-stealing effectiveness:
 * - Uniform load distribution
 * - Uneven load (90/10 split)
 * - Producer-consumer patterns
 * - Queue contention comparison
 */

#include <benchmark/benchmark.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/thread_worker.h>
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/core/worker_policy.h>
#include <kcenon/thread/utils/formatter.h>

#include <atomic>
#include <random>
#include <vector>
#include <chrono>

using namespace kcenon::thread;
using namespace std::chrono_literals;

// Simulate varying amounts of work
static void do_work(int iterations) {
    volatile int64_t sum = 0;
    for (int i = 0; i < iterations; ++i) {
        sum += i * i;
    }
}

/**
 * @brief Benchmark uniform workload with work-stealing disabled (baseline)
 */
static void BM_UniformLoad_NoStealing(benchmark::State& state) {
    const size_t num_workers = state.range(0);
    const size_t num_jobs = state.range(1);

    for (auto _ : state) {
        auto pool = std::make_shared<thread_pool>("uniform_no_steal");

        // Disable work-stealing
        worker_policy policy;
        policy.enable_work_stealing = false;
        pool->set_worker_policy(policy);

        for (size_t i = 0; i < num_workers; ++i) {
            pool->enqueue(std::make_unique<thread_worker>());
        }
        pool->start();

        std::atomic<size_t> completed{0};

        for (size_t i = 0; i < num_jobs; ++i) {
            pool->enqueue(std::make_unique<callback_job>([&completed]() -> common::VoidResult {
                do_work(1000);
                completed++;
                return common::ok();
            }));
        }

        // Wait for completion
        while (completed < num_jobs) {
            std::this_thread::yield();
        }

        pool->stop();
    }

    state.SetItemsProcessed(state.iterations() * num_jobs);
}

/**
 * @brief Benchmark uniform workload with work-stealing enabled
 */
static void BM_UniformLoad_WithStealing(benchmark::State& state) {
    const size_t num_workers = state.range(0);
    const size_t num_jobs = state.range(1);

    for (auto _ : state) {
        auto pool = std::make_shared<thread_pool>("uniform_steal");

        // Enable work-stealing
        worker_policy policy;
        policy.enable_work_stealing = true;
        policy.victim_selection = steal_policy::random;
        pool->set_worker_policy(policy);

        for (size_t i = 0; i < num_workers; ++i) {
            pool->enqueue(std::make_unique<thread_worker>());
        }
        pool->start();

        std::atomic<size_t> completed{0};

        for (size_t i = 0; i < num_jobs; ++i) {
            pool->enqueue(std::make_unique<callback_job>([&completed]() -> common::VoidResult {
                do_work(1000);
                completed++;
                return common::ok();
            }));
        }

        // Wait for completion
        while (completed < num_jobs) {
            std::this_thread::yield();
        }

        pool->stop();
    }

    state.SetItemsProcessed(state.iterations() * num_jobs);
}

/**
 * @brief Benchmark uneven workload (90% small, 10% large jobs) without stealing
 */
static void BM_UnevenLoad_NoStealing(benchmark::State& state) {
    const size_t num_workers = state.range(0);
    const size_t num_jobs = state.range(1);

    for (auto _ : state) {
        auto pool = std::make_shared<thread_pool>("uneven_no_steal");

        worker_policy policy;
        policy.enable_work_stealing = false;
        pool->set_worker_policy(policy);

        for (size_t i = 0; i < num_workers; ++i) {
            pool->enqueue(std::make_unique<thread_worker>());
        }
        pool->start();

        std::atomic<size_t> completed{0};
        std::mt19937 rng(42);

        for (size_t i = 0; i < num_jobs; ++i) {
            // 90% small jobs, 10% large jobs
            int work_size = (rng() % 10 < 9) ? 100 : 10000;

            pool->enqueue(std::make_unique<callback_job>([&completed, work_size]() -> common::VoidResult {
                do_work(work_size);
                completed++;
                return common::ok();
            }));
        }

        while (completed < num_jobs) {
            std::this_thread::yield();
        }

        pool->stop();
    }

    state.SetItemsProcessed(state.iterations() * num_jobs);
}

/**
 * @brief Benchmark uneven workload with work-stealing enabled
 */
static void BM_UnevenLoad_WithStealing(benchmark::State& state) {
    const size_t num_workers = state.range(0);
    const size_t num_jobs = state.range(1);

    for (auto _ : state) {
        auto pool = std::make_shared<thread_pool>("uneven_steal");

        worker_policy policy;
        policy.enable_work_stealing = true;
        policy.victim_selection = steal_policy::adaptive;  // Best for uneven loads
        pool->set_worker_policy(policy);

        for (size_t i = 0; i < num_workers; ++i) {
            pool->enqueue(std::make_unique<thread_worker>());
        }
        pool->start();

        std::atomic<size_t> completed{0};
        std::mt19937 rng(42);

        for (size_t i = 0; i < num_jobs; ++i) {
            // 90% small jobs, 10% large jobs
            int work_size = (rng() % 10 < 9) ? 100 : 10000;

            pool->enqueue(std::make_unique<callback_job>([&completed, work_size]() -> common::VoidResult {
                do_work(work_size);
                completed++;
                return common::ok();
            }));
        }

        while (completed < num_jobs) {
            std::this_thread::yield();
        }

        pool->stop();
    }

    state.SetItemsProcessed(state.iterations() * num_jobs);
}

/**
 * @brief Compare different steal policies
 */
static void BM_StealPolicy_Random(benchmark::State& state) {
    const size_t num_workers = 4;
    const size_t num_jobs = 1000;

    for (auto _ : state) {
        auto pool = std::make_shared<thread_pool>("steal_random");

        worker_policy policy;
        policy.enable_work_stealing = true;
        policy.victim_selection = steal_policy::random;
        pool->set_worker_policy(policy);

        for (size_t i = 0; i < num_workers; ++i) {
            pool->enqueue(std::make_unique<thread_worker>());
        }
        pool->start();

        std::atomic<size_t> completed{0};
        for (size_t i = 0; i < num_jobs; ++i) {
            pool->enqueue(std::make_unique<callback_job>([&completed]() -> common::VoidResult {
                do_work(500);
                completed++;
                return common::ok();
            }));
        }

        while (completed < num_jobs) {
            std::this_thread::yield();
        }

        pool->stop();
    }

    state.SetItemsProcessed(state.iterations() * num_jobs);
}

static void BM_StealPolicy_RoundRobin(benchmark::State& state) {
    const size_t num_workers = 4;
    const size_t num_jobs = 1000;

    for (auto _ : state) {
        auto pool = std::make_shared<thread_pool>("steal_rr");

        worker_policy policy;
        policy.enable_work_stealing = true;
        policy.victim_selection = steal_policy::round_robin;
        pool->set_worker_policy(policy);

        for (size_t i = 0; i < num_workers; ++i) {
            pool->enqueue(std::make_unique<thread_worker>());
        }
        pool->start();

        std::atomic<size_t> completed{0};
        for (size_t i = 0; i < num_jobs; ++i) {
            pool->enqueue(std::make_unique<callback_job>([&completed]() -> common::VoidResult {
                do_work(500);
                completed++;
                return common::ok();
            }));
        }

        while (completed < num_jobs) {
            std::this_thread::yield();
        }

        pool->stop();
    }

    state.SetItemsProcessed(state.iterations() * num_jobs);
}

static void BM_StealPolicy_Adaptive(benchmark::State& state) {
    const size_t num_workers = 4;
    const size_t num_jobs = 1000;

    for (auto _ : state) {
        auto pool = std::make_shared<thread_pool>("steal_adaptive");

        worker_policy policy;
        policy.enable_work_stealing = true;
        policy.victim_selection = steal_policy::adaptive;
        pool->set_worker_policy(policy);

        for (size_t i = 0; i < num_workers; ++i) {
            pool->enqueue(std::make_unique<thread_worker>());
        }
        pool->start();

        std::atomic<size_t> completed{0};
        for (size_t i = 0; i < num_jobs; ++i) {
            pool->enqueue(std::make_unique<callback_job>([&completed]() -> common::VoidResult {
                do_work(500);
                completed++;
                return common::ok();
            }));
        }

        while (completed < num_jobs) {
            std::this_thread::yield();
        }

        pool->stop();
    }

    state.SetItemsProcessed(state.iterations() * num_jobs);
}

// Register benchmarks
BENCHMARK(BM_UniformLoad_NoStealing)
    ->Args({4, 1000})
    ->Args({8, 1000})
    ->Args({4, 10000})
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_UniformLoad_WithStealing)
    ->Args({4, 1000})
    ->Args({8, 1000})
    ->Args({4, 10000})
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_UnevenLoad_NoStealing)
    ->Args({4, 1000})
    ->Args({8, 1000})
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_UnevenLoad_WithStealing)
    ->Args({4, 1000})
    ->Args({8, 1000})
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_StealPolicy_Random)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_StealPolicy_RoundRobin)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_StealPolicy_Adaptive)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
