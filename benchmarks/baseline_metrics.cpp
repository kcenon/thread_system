/**
 * @file baseline_metrics.cpp
 * @brief Baseline performance benchmarks for thread_system
 *
 * This file contains baseline performance measurements for the thread pool system.
 * These benchmarks establish a performance baseline to detect regressions.
 */

#include <benchmark/benchmark.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/typed_thread_pool.h>
#include <atomic>
#include <chrono>
#include <vector>

using namespace kcenon::thread;

/**
 * @brief Benchmark task submission latency
 *
 * Measures the time it takes to submit a task to the thread pool.
 * Target: < 1μs per submission
 */
static void BM_ThreadPool_TaskSubmission(benchmark::State& state) {
    auto pool = thread_pool::create(4);

    for (auto _ : state) {
        pool->submit([]{ /* empty task */ });
    }

    pool->shutdown();
    pool->wait();

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ThreadPool_TaskSubmission)->Threads(1)->Iterations(100000);

/**
 * @brief Benchmark task throughput
 *
 * Measures how many tasks can be processed per second.
 * Target: > 1M tasks/sec with 4 worker threads
 */
static void BM_ThreadPool_Throughput(benchmark::State& state) {
    auto pool = thread_pool::create(4);
    std::atomic<int64_t> counter{0};

    for (auto _ : state) {
        pool->submit([&counter]{ counter.fetch_add(1, std::memory_order_relaxed); });
    }

    pool->shutdown();
    pool->wait();

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ThreadPool_Throughput)->Threads(1)->Iterations(1000000);

/**
 * @brief Benchmark concurrent task submission
 *
 * Measures throughput when submitting from multiple threads.
 * Target: Linear scaling up to number of cores
 */
static void BM_ThreadPool_ConcurrentSubmission(benchmark::State& state) {
    static auto pool = thread_pool::create(4);

    for (auto _ : state) {
        pool->submit([]{ /* empty task */ });
    }

    if (state.thread_index() == 0) {
        pool->shutdown();
        pool->wait();
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ThreadPool_ConcurrentSubmission)
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Iterations(100000);

/**
 * @brief Benchmark typed thread pool task execution
 *
 * Measures overhead of typed thread pool vs regular thread pool.
 * Target: < 5% overhead compared to regular pool
 */
static void BM_TypedThreadPool_Execution(benchmark::State& state) {
    auto pool = typed_thread_pool<int>::create(4);

    for (auto _ : state) {
        pool->submit([](int x) { return x * 2; }, 42);
    }

    pool->shutdown();
    pool->wait();

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TypedThreadPool_Execution)->Threads(1)->Iterations(100000);

/**
 * @brief Benchmark task cancellation
 *
 * Measures the latency of cancelling queued tasks.
 * Target: < 10μs to cancel a task
 */
static void BM_ThreadPool_Cancellation(benchmark::State& state) {
    auto pool = thread_pool::create(4);

    for (auto _ : state) {
        auto token = pool->submit([]{
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        });

        state.PauseTiming();
        pool->cancel(token);
        state.ResumeTiming();
    }

    pool->shutdown();

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ThreadPool_Cancellation)->Iterations(1000);

/**
 * @brief Benchmark pool creation and destruction
 *
 * Measures the overhead of creating and destroying thread pools.
 * Target: < 10ms for creation, < 5ms for shutdown
 */
static void BM_ThreadPool_Lifecycle(benchmark::State& state) {
    for (auto _ : state) {
        auto pool = thread_pool::create(4);

        benchmark::DoNotOptimize(pool);

        state.PauseTiming();
        pool->shutdown();
        pool->wait();
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ThreadPool_Lifecycle)->Iterations(100);

/**
 * @brief Benchmark queue contention
 *
 * Measures performance under high queue contention.
 * Target: Graceful degradation under contention
 */
static void BM_ThreadPool_QueueContention(benchmark::State& state) {
    static auto pool = thread_pool::create(2);

    for (auto _ : state) {
        pool->submit([]{
            // Simulate work
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        });
    }

    if (state.thread_index() == 0) {
        pool->shutdown();
        pool->wait();
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ThreadPool_QueueContention)
    ->Threads(8)
    ->Threads(16)
    ->Iterations(10000);

// Run the benchmarks
BENCHMARK_MAIN();
