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
 * @file monitoring_overhead_benchmark.cpp
 * @brief Google Benchmark-based performance tests for monitoring system overhead
 * 
 * This file measures the performance impact of the monitoring system on
 * thread pool operations, including:
 * - Metric collection overhead
 * - Memory overhead
 * - CPU overhead with various collection intervals
 */

#include <benchmark/benchmark.h>
#include "../../sources/monitoring/core/metrics_collector.h"
#include "../../sources/thread_pool/core/thread_pool.h"
#include "../../sources/thread_pool/workers/thread_worker.h"
#include "../../sources/thread_base/jobs/callback_job.h"
#include "../../sources/utilities/core/formatter.h"

#include <vector>
#include <thread>
#include <atomic>
#include <memory>

using namespace monitoring_module;
using namespace thread_pool_module;
using namespace thread_module;

/**
 * @brief Benchmark thread pool without monitoring (baseline)
 * 
 * Establishes baseline performance without any monitoring overhead.
 */
static void BM_ThreadPoolWithoutMonitoring(benchmark::State& state) {
    const size_t num_workers = state.range(0);
    const size_t job_duration_us = state.range(1);
    
    // Create thread pool
    auto pool = std::make_shared<thread_pool>("unmonitored_pool");
    for (size_t i = 0; i < num_workers; ++i) {
        auto worker = std::make_unique<thread_worker>();
        pool->enqueue(std::move(worker));
    }
    pool->start();
    
    // Benchmark job execution
    for (auto _ : state) {
        std::atomic<size_t> completed{0};
        const size_t num_jobs = 1000;
        
        // Submit jobs
        for (size_t i = 0; i < num_jobs; ++i) {
            auto job = std::make_unique<callback_job>([&completed, job_duration_us]() -> result_void {
                if (job_duration_us > 0) {
                    auto end = std::chrono::high_resolution_clock::now() + 
                              std::chrono::microseconds(job_duration_us);
                    while (std::chrono::high_resolution_clock::now() < end) {
                        // Busy wait
                    }
                }
                completed.fetch_add(1);
                return result_void{};
            });
            pool->enqueue(std::move(job));
        }
        
        // Wait for completion
        while (completed.load() < num_jobs) {
            std::this_thread::yield();
        }
    }
    
    pool->stop();
    
    state.SetItemsProcessed(state.iterations() * 1000);
    state.counters["workers"] = num_workers;
    state.counters["job_duration_us"] = job_duration_us;
}

// Test matrix: Workers x Job duration
BENCHMARK(BM_ThreadPoolWithoutMonitoring)
    ->Args({4, 0})
    ->Args({4, 10})
    ->Args({4, 100})
    ->Args({8, 0})
    ->Args({8, 10})
    ->Args({8, 100});

/**
 * @brief Benchmark thread pool with monitoring enabled
 * 
 * Measures the overhead of monitoring on thread pool performance.
 */
static void BM_ThreadPoolWithMonitoring(benchmark::State& state) {
    const size_t num_workers = state.range(0);
    const size_t job_duration_us = state.range(1);
    const size_t collection_interval_ms = state.range(2);
    
    // Initialize monitoring
    monitoring_config config;
    config.collection_interval = std::chrono::milliseconds(collection_interval_ms);
    config.buffer_size = 10000;
    
    auto& global_collector = global_metrics_collector::instance();
    if (auto result = global_collector.initialize(config); !result) {
        state.SkipWithError("Failed to initialize global metrics collector");
        return;
    }
    
    // Global collector starts automatically when initialized
    
    // Create monitored thread pool
    auto pool = std::make_shared<thread_pool>("monitored_pool");
    for (size_t i = 0; i < num_workers; ++i) {
        auto worker = std::make_unique<thread_worker>();
        pool->enqueue(std::move(worker));
    }
    pool->start();
    
    // Create and register pool metrics manually
    auto pool_metrics = std::make_shared<thread_pool_metrics>();
    if (auto collector = global_collector.get_collector()) {
        collector->register_thread_pool_metrics(pool_metrics);
    }
    
    // Benchmark job execution with monitoring
    for (auto _ : state) {
        std::atomic<size_t> completed{0};
        const size_t num_jobs = 1000;
        
        // Submit jobs
        for (size_t i = 0; i < num_jobs; ++i) {
            auto job = std::make_unique<callback_job>([&completed, job_duration_us]() -> result_void {
                if (job_duration_us > 0) {
                    auto end = std::chrono::high_resolution_clock::now() + 
                              std::chrono::microseconds(job_duration_us);
                    while (std::chrono::high_resolution_clock::now() < end) {
                        // Busy wait
                    }
                }
                
                completed.fetch_add(1);
                return result_void{};
            });
            
            pool->enqueue(std::move(job));
        }
        
        // Wait for completion
        while (completed.load() < num_jobs) {
            std::this_thread::yield();
        }
    }
    
    pool->stop();
    global_collector.shutdown();
    
    state.SetItemsProcessed(state.iterations() * 1000);
    state.counters["workers"] = num_workers;
    state.counters["job_duration_us"] = job_duration_us;
    state.counters["collection_interval_ms"] = collection_interval_ms;
}

// Test matrix: Workers x Job duration x Collection interval
BENCHMARK(BM_ThreadPoolWithMonitoring)
    ->Args({4, 0, 10})
    ->Args({4, 0, 100})
    ->Args({4, 10, 10})
    ->Args({4, 10, 100})
    ->Args({4, 100, 10})
    ->Args({4, 100, 100})
    ->Args({8, 0, 10})
    ->Args({8, 0, 100})
    ->Args({8, 10, 10})
    ->Args({8, 10, 100})
    ->Args({8, 100, 10})
    ->Args({8, 100, 100});

/**
 * @brief Benchmark metric collection overhead
 * 
 * Measures the raw overhead of the metrics collection process.
 */
static void BM_MetricCollectionOverhead(benchmark::State& state) {
    const size_t collection_interval_ms = state.range(0);
    
    // Initialize monitoring
    monitoring_config config;
    config.collection_interval = std::chrono::milliseconds(collection_interval_ms);
    config.buffer_size = 10000;
    
    auto collector = std::make_unique<metrics_collector>(config);
    
    // Create dummy metrics
    auto sys_metrics = std::make_shared<system_metrics>();
    auto pool_metrics = std::make_shared<thread_pool_metrics>();
    
    collector->register_system_metrics(sys_metrics);
    collector->register_thread_pool_metrics(pool_metrics);
    
    // Start collection
    if (auto result = collector->start(); !result) {
        state.SkipWithError("Failed to start metrics collector");
        return;
    }
    
    // Benchmark: simulate work while metrics are being collected
    for (auto _ : state) {
        // Update metrics values to simulate real usage
        sys_metrics->cpu_usage_percent.store(50);
        sys_metrics->memory_usage_bytes.store(1024 * 1024 * 100);
        sys_metrics->active_threads.store(8);
        
        pool_metrics->jobs_completed.fetch_add(95);
        pool_metrics->jobs_pending.store(5);
        pool_metrics->worker_threads.store(8);
        pool_metrics->idle_threads.store(4);
        
        // Simulate some work
        auto end = std::chrono::high_resolution_clock::now() + 
                  std::chrono::microseconds(100);
        while (std::chrono::high_resolution_clock::now() < end) {
            // Busy wait
        }
    }
    
    collector->stop();
    
    // Get collection statistics
    auto stats = collector->get_collection_stats();
    
    state.counters["collection_interval_ms"] = collection_interval_ms;
    state.counters["total_collections"] = static_cast<double>(stats.total_collections.load());
    state.counters["collection_errors"] = static_cast<double>(stats.collection_errors.load());
}

BENCHMARK(BM_MetricCollectionOverhead)
    ->Arg(1)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Arg(500)
    ->Arg(1000);

/**
 * @brief Memory overhead benchmark
 * 
 * Measures memory consumption of the monitoring system.
 */
static void BM_MonitoringMemoryOverhead(benchmark::State& state) {
    const size_t buffer_size = state.range(0);
    
    for (auto _ : state) {
        // Create monitoring with specified buffer size
        monitoring_config config;
        config.buffer_size = buffer_size;
        config.collection_interval = std::chrono::milliseconds(100);
        
        auto collector = std::make_unique<metrics_collector>(config);
        
        // Register some metrics
        auto sys_metrics = std::make_shared<system_metrics>();
        auto pool_metrics = std::make_shared<thread_pool_metrics>();
        
        collector->register_system_metrics(sys_metrics);
        collector->register_thread_pool_metrics(pool_metrics);
        
        // Start and run briefly
        if (auto result = collector->start(); result) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            collector->stop();
        }
        
        // Force destruction to measure allocation/deallocation
        collector.reset();
    }
    
    state.counters["buffer_size"] = buffer_size;
    
    // Estimate memory usage (this is approximate)
    size_t estimated_memory = buffer_size * sizeof(metrics_snapshot) + 
                             sizeof(metrics_collector) + 
                             sizeof(system_metrics) + 
                             sizeof(thread_pool_metrics);
    state.counters["estimated_memory_bytes"] = estimated_memory;
}

BENCHMARK(BM_MonitoringMemoryOverhead)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000);

// Run the benchmarks
BENCHMARK_MAIN();