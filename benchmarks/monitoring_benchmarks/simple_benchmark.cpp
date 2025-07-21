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
 * @file simple_benchmark.cpp
 * @brief Google Benchmark-based simple monitoring system performance tests
 * 
 * Basic benchmarks for monitoring system overhead and performance characteristics.
 */

#include <benchmark/benchmark.h>
#include "../../sources/monitoring/core/metrics_collector.h"
#include "../../sources/utilities/core/formatter.h"

#include <thread>
#include <vector>
#include <atomic>
#include <memory>

using namespace monitoring_module;
using namespace utility_module;

/**
 * @brief Benchmark baseline atomic counter increment without monitoring
 * 
 * Establishes a performance baseline for comparison.
 */
static void BM_BaselineAtomicCounter(benchmark::State& state) {
    std::atomic<uint64_t> counter{0};
    
    for (auto _ : state) {
        counter.fetch_add(1);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_BaselineAtomicCounter);

/**
 * @brief Benchmark metric counter increment with monitoring enabled
 * 
 * Measures the overhead of updating metrics while monitoring is active.
 */
static void BM_MonitoringCounterIncrement(benchmark::State& state) {
    const size_t collection_interval_ms = state.range(0);
    
    // Initialize monitoring
    monitoring_config config;
    config.collection_interval = std::chrono::milliseconds(collection_interval_ms);
    config.buffer_size = 60;
    
    if (auto result = metrics::start_global_monitoring(config); result.has_error()) {
        state.SkipWithError("Failed to start monitoring");
        return;
    }
    
    auto collector = global_metrics_collector::instance().get_collector();
    auto thread_pool_metrics = std::make_shared<monitoring_module::thread_pool_metrics>();
    collector->register_thread_pool_metrics(thread_pool_metrics);
    
    // Benchmark
    for (auto _ : state) {
        thread_pool_metrics->jobs_completed.fetch_add(1);
    }
    
    // Get collection stats
    auto stats = collector->get_collection_stats();
    
    metrics::stop_global_monitoring();
    
    state.SetItemsProcessed(state.iterations());
    state.counters["collection_interval_ms"] = collection_interval_ms;
    state.counters["collections"] = stats.total_collections.load();
    state.counters["collection_errors"] = stats.collection_errors.load();
}
// Test with different collection intervals
BENCHMARK(BM_MonitoringCounterIncrement)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Arg(500)
    ->Arg(1000);

/**
 * @brief Benchmark memory usage with large buffer and high frequency collection
 * 
 * Tests the monitoring system under memory pressure.
 */
static void BM_MonitoringMemoryUsage(benchmark::State& state) {
    const size_t buffer_size = state.range(0);
    const size_t collection_interval_ms = 10;  // High frequency
    
    // Initialize monitoring
    monitoring_config config;
    config.collection_interval = std::chrono::milliseconds(collection_interval_ms);
    config.buffer_size = buffer_size;
    
    if (auto result = metrics::start_global_monitoring(config); result.has_error()) {
        state.SkipWithError("Failed to start monitoring");
        return;
    }
    
    auto collector = global_metrics_collector::instance().get_collector();
    auto system_metrics = std::make_shared<monitoring_module::system_metrics>();
    auto thread_pool_metrics = std::make_shared<monitoring_module::thread_pool_metrics>();
    auto worker_metrics = std::make_shared<monitoring_module::worker_metrics>();
    
    collector->register_system_metrics(system_metrics);
    collector->register_thread_pool_metrics(thread_pool_metrics);
    collector->register_worker_metrics(worker_metrics);
    
    // Benchmark
    for (auto _ : state) {
        // Generate metrics at high rate
        thread_pool_metrics->jobs_completed.fetch_add(1);
        thread_pool_metrics->jobs_pending.store(state.iterations() % 100);
        worker_metrics->jobs_processed.fetch_add(1);
        worker_metrics->total_processing_time_ns.fetch_add(50000);  // 50us
        
        // Small delay to allow collections
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    
    auto final_stats = collector->get_collection_stats();
    
    metrics::stop_global_monitoring();
    
    state.SetItemsProcessed(state.iterations());
    state.counters["buffer_size"] = buffer_size;
    state.counters["total_collections"] = final_stats.total_collections.load();
    state.counters["buffer_overflows"] = final_stats.buffer_overflows.load();
}
// Test with different buffer sizes
BENCHMARK(BM_MonitoringMemoryUsage)
    ->Arg(60)
    ->Arg(600)
    ->Arg(6000)
    ->Arg(60000)
    ->UseRealTime();

/**
 * @brief Benchmark concurrent metric updates from multiple threads
 * 
 * Measures contention and performance with concurrent metric updates.
 */
static void BM_ConcurrentMetricUpdates(benchmark::State& state) {
    const size_t num_threads = state.range(0);
    const size_t updates_per_thread = 10000;
    
    // Initialize monitoring
    monitoring_config config;
    config.collection_interval = std::chrono::milliseconds(100);
    config.buffer_size = 600;
    
    if (auto result = metrics::start_global_monitoring(config); result.has_error()) {
        state.SkipWithError("Failed to start monitoring");
        return;
    }
    
    auto collector = global_metrics_collector::instance().get_collector();
    auto thread_pool_metrics = std::make_shared<monitoring_module::thread_pool_metrics>();
    collector->register_thread_pool_metrics(thread_pool_metrics);
    
    // Benchmark
    for (auto _ : state) {
        std::vector<std::thread> threads;
        std::atomic<size_t> total_updates{0};
        
        state.PauseTiming();
        // Create threads
        for (size_t i = 0; i < num_threads; ++i) {
            threads.emplace_back([&thread_pool_metrics, &total_updates, updates_per_thread]() {
                for (size_t j = 0; j < updates_per_thread; ++j) {
                    thread_pool_metrics->jobs_completed.fetch_add(1);
                    thread_pool_metrics->jobs_pending.store(j % 100);
                    total_updates.fetch_add(1);
                }
            });
        }
        state.ResumeTiming();
        
        // Wait for all threads
        for (auto& t : threads) {
            t.join();
        }
    }
    
    metrics::stop_global_monitoring();
    
    state.SetItemsProcessed(state.iterations() * num_threads * updates_per_thread);
    state.counters["threads"] = num_threads;
}
// Test with different thread counts
BENCHMARK(BM_ConcurrentMetricUpdates)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->Arg(16);

/**
 * @brief Benchmark collection cycle overhead
 * 
 * Measures the overhead of the metric collection process itself.
 */
static void BM_CollectionCycleOverhead(benchmark::State& state) {
    const size_t num_metrics = state.range(0);
    
    // Initialize monitoring
    monitoring_config config;
    config.collection_interval = std::chrono::seconds(1);  // Long interval to control manually
    config.buffer_size = num_metrics * 10;
    
    if (auto result = metrics::start_global_monitoring(config); result.has_error()) {
        state.SkipWithError("Failed to start monitoring");
        return;
    }
    
    auto collector = global_metrics_collector::instance().get_collector();
    
    // Register many metrics
    std::vector<std::shared_ptr<monitoring_module::thread_pool_metrics>> metrics_list;
    for (size_t i = 0; i < num_metrics / 3; ++i) {
        auto metrics = std::make_shared<monitoring_module::thread_pool_metrics>();
        collector->register_thread_pool_metrics(metrics);
        metrics_list.push_back(metrics);
    }
    
    // Populate metrics with some data
    for (auto& metrics : metrics_list) {
        metrics->jobs_completed.store(1000);
        metrics->jobs_pending.store(50);
        metrics->jobs_failed.store(10);
    }
    
    // Benchmark collection time
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Force a collection cycle
        collector->collect_metrics();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e9);
    }
    
    metrics::stop_global_monitoring();
    
    state.SetItemsProcessed(state.iterations() * num_metrics);
    state.counters["num_metrics"] = num_metrics;
}
// Test with different numbers of metrics
BENCHMARK(BM_CollectionCycleOverhead)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->UseManualTime();

// Main function to run benchmarks
BENCHMARK_MAIN();