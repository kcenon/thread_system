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
 * @file basic_benchmark.cpp
 * @brief Google Benchmark-based tests for monitoring system collection overhead
 * 
 * This file measures the performance characteristics of the monitoring system
 * including:
 * - Basic metric update overhead
 * - Collection interval impact
 * - Memory usage patterns
 * - CPU overhead with different metric counts
 */

#include <benchmark/benchmark.h>
#include "../../sources/monitoring/core/metrics_collector.h"
#include "../../sources/monitoring/core/metrics.h"
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
static void BM_BaselineAtomicIncrement(benchmark::State& state) {
    std::atomic<uint64_t> counter{0};
    
    for (auto _ : state) {
        counter.fetch_add(1);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_BaselineAtomicIncrement);
    
/**
 * @brief Benchmark metric increment with monitoring enabled
 * 
 * Measures the overhead of updating metrics while monitoring is active.
 */
static void BM_MetricIncrementWithMonitoring(benchmark::State& state) {
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
    
    // Benchmark metric updates
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
BENCHMARK(BM_MetricIncrementWithMonitoring)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Arg(500)
    ->Arg(1000);
    
/**
 * @brief Benchmark high frequency monitoring with multiple metrics
 * 
 * Measures system behavior under high frequency metric collection.
 */
static void BM_HighFrequencyMonitoring(benchmark::State& state) {
    const size_t collection_interval_ms = state.range(0);
    const size_t duration_seconds = 3;
    
    // Initialize monitoring
    monitoring_config config;
    config.collection_interval = std::chrono::milliseconds(collection_interval_ms);
    config.buffer_size = 1000;
    
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
    
    // Benchmark metric generation
    for (auto _ : state) {
        std::atomic<bool> running{true};
        std::atomic<int> operations{0};
        
        // Start metrics generator thread
        std::thread metrics_generator([&]() {
            int counter = 0;
            while (running.load()) {
                thread_pool_metrics->jobs_completed.fetch_add(1);
                thread_pool_metrics->jobs_pending.store(counter % 50);
                worker_metrics->jobs_processed.fetch_add(1);
                worker_metrics->total_processing_time_ns.fetch_add(25000);  // 25us
                
                operations.fetch_add(4);  // 4 atomic operations per iteration
                counter++;
                
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        });
        
        // Run for specified duration
        std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));
        running.store(false);
        
        if (metrics_generator.joinable()) {
            metrics_generator.join();
        }
        
        state.SetItemsProcessed(operations.load());
    }
    
    auto final_stats = collector->get_collection_stats();
    auto snapshot = metrics::get_current_metrics();
    
    metrics::stop_global_monitoring();
    
    state.counters["collection_interval_ms"] = collection_interval_ms;
    state.counters["total_collections"] = final_stats.total_collections.load();
    state.counters["collection_errors"] = final_stats.collection_errors.load();
    state.counters["buffer_overflows"] = final_stats.buffer_overflows.load();
    state.counters["final_jobs_completed"] = snapshot.thread_pool.jobs_completed.load();
}
// Test with different collection intervals
BENCHMARK(BM_HighFrequencyMonitoring)
    ->Arg(10)
    ->Arg(25)
    ->Arg(50)
    ->Arg(100)
    ->Unit(benchmark::kSecond);

/**
 * @brief Benchmark multiple metric types update overhead
 * 
 * Measures the overhead of updating different types of metrics.
 */
static void BM_MultipleMetricTypesUpdate(benchmark::State& state) {
    const size_t num_metric_types = state.range(0);
    
    // Initialize monitoring
    metrics_collector::instance().start(std::chrono::milliseconds(100));
    
    // Create different metric types
    auto counter = metrics_collector::instance().register_counter(
        "test_counter", "Test counter metric"
    );
    auto gauge = metrics_collector::instance().register_gauge(
        "test_gauge", "Test gauge metric"
    );
    auto histogram = metrics_collector::instance().register_histogram(
        "test_histogram", "Test histogram metric", 
        {1.0, 5.0, 10.0, 25.0, 50.0, 100.0}
    );
    
    // Benchmark metric updates
    size_t iteration = 0;
    for (auto _ : state) {
        if (num_metric_types >= 1) {
            counter->increment();
        }
        if (num_metric_types >= 2) {
            gauge->set(static_cast<double>(iteration));
        }
        if (num_metric_types >= 3) {
            histogram->observe(static_cast<double>(iteration % 100));
        }
        iteration++;
    }
    
    metrics_collector::instance().stop();
    
    state.SetItemsProcessed(state.iterations() * num_metric_types);
    state.counters["metric_types"] = num_metric_types;
}
// Test with different numbers of metric types
BENCHMARK(BM_MultipleMetricTypesUpdate)
    ->Arg(1)
    ->Arg(2)
    ->Arg(3);

/**
 * @brief Benchmark metric collection cycle overhead
 * 
 * Measures the CPU time consumed by the collection process itself.
 */
static void BM_CollectionCycleOverhead(benchmark::State& state) {
    const size_t num_metrics = state.range(0);
    
    // Initialize monitoring
    metrics_collector::instance().start(std::chrono::milliseconds(10));
    
    // Register many metrics
    std::vector<std::shared_ptr<counter>> counters;
    std::vector<std::shared_ptr<gauge>> gauges;
    
    for (size_t i = 0; i < num_metrics / 2; ++i) {
        counters.push_back(
            metrics_collector::instance().register_counter(
                formatter::format("counter_{}", i),
                "Test counter"
            )
        );
        gauges.push_back(
            metrics_collector::instance().register_gauge(
                formatter::format("gauge_{}", i),
                "Test gauge"
            )
        );
    }
    
    // Update all metrics once
    for (size_t i = 0; i < num_metrics / 2; ++i) {
        counters[i]->increment();
        gauges[i]->set(static_cast<double>(i));
    }
    
    // Measure collection time
    for (auto _ : state) {
        // Force a collection cycle
        auto start = std::chrono::high_resolution_clock::now();
        metrics_collector::instance().collect_metrics();
        auto end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e9);
    }
    
    metrics_collector::instance().stop();
    
    state.SetItemsProcessed(state.iterations() * num_metrics);
    state.counters["num_metrics"] = num_metrics;
}
// Test with different metric counts
BENCHMARK(BM_CollectionCycleOverhead)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->UseManualTime();

// Main function to run benchmarks
BENCHMARK_MAIN();