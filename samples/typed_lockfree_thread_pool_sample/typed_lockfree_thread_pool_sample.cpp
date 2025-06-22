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

#include "typed_thread_pool/pool/typed_lockfree_thread_pool.h"
#include "typed_thread_pool/pool/typed_thread_pool.h"
#include "typed_thread_pool/jobs/callback_typed_job.h"
#include "logger/core/logger.h"
#include "utilities/core/formatter.h"

#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include <sstream>

using namespace typed_thread_pool_module;
using namespace thread_module;
using namespace utility_module;

// Performance test configuration
constexpr size_t NUM_THREADS = 4;
constexpr size_t JOBS_PER_BATCH = 10000;
constexpr size_t TOTAL_JOBS = 100000;

// Atomic counters for verification
std::atomic<size_t> realtime_processed{0};
std::atomic<size_t> batch_processed{0};
std::atomic<size_t> background_processed{0};

// Job processing simulation
void process_job(job_types type)
{
    switch (type) {
        case job_types::RealTime:
            realtime_processed.fetch_add(1);
            break;
        case job_types::Batch:
            batch_processed.fetch_add(1);
            break;
        case job_types::Background:
            background_processed.fetch_add(1);
            break;
    }
}

// Create test jobs
std::vector<std::unique_ptr<typed_job_t<job_types>>> create_test_jobs(size_t count)
{
    std::vector<std::unique_ptr<typed_job_t<job_types>>> jobs;
    jobs.reserve(count);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> type_dist(0, 2);
    
    for (size_t i = 0; i < count; ++i) {
        job_types type;
        switch (type_dist(gen)) {
            case 0: type = job_types::RealTime; break;
            case 1: type = job_types::Batch; break;
            default: type = job_types::Background; break;
        }
        
        jobs.push_back(std::make_unique<callback_typed_job>(
            [type]() -> result_void {
                process_job(type);
                return {};
            },
            type
        ));
    }
    
    return jobs;
}

// Performance comparison function
template<typename PoolType>
void run_performance_test(const std::string& pool_name)
{
    log_module::write_information("\n=== Testing {} ===", pool_name);
    
    // Reset counters
    realtime_processed.store(0);
    batch_processed.store(0);
    background_processed.store(0);
    
    // Create pool
    auto pool = std::make_shared<PoolType>(pool_name);
    
    // Create workers
    std::vector<std::unique_ptr<typename std::conditional<
        std::is_same_v<PoolType, typed_lockfree_thread_pool>,
        typed_lockfree_thread_worker,
        typed_thread_worker
    >::type>> workers;
    
    // Create specialized workers for each priority type
    for (size_t i = 0; i < NUM_THREADS; ++i) {
        job_types assigned_type;
        std::string worker_name;
        
        switch (i % 3) {
            case 0:
                assigned_type = job_types::RealTime;
                worker_name = formatter::format("RealTime Worker {}", i);
                break;
            case 1:
                assigned_type = job_types::Batch;
                worker_name = formatter::format("Batch Worker {}", i);
                break;
            default:
                assigned_type = job_types::Background;
                worker_name = formatter::format("Background Worker {}", i);
                break;
        }
        
        if constexpr (std::is_same_v<PoolType, typed_lockfree_thread_pool>) {
            workers.push_back(std::make_unique<typed_lockfree_thread_worker>(
                std::vector<job_types>{assigned_type}, worker_name));
        } else {
            workers.push_back(std::make_unique<typed_thread_worker>(
                std::vector<job_types>{assigned_type}));
        }
    }
    
    // Add one worker that handles all types
    if constexpr (std::is_same_v<PoolType, typed_lockfree_thread_pool>) {
        workers.push_back(std::make_unique<typed_lockfree_thread_worker>(
            typed_thread_pool_module::all_types(), "Universal Worker"));
    } else {
        workers.push_back(std::make_unique<typed_thread_worker>(
            typed_thread_pool_module::all_types()));
    }
    
    auto enqueue_result = pool->enqueue_batch(std::move(workers));
    if (enqueue_result.has_error()) {
        log_module::write_error("Failed to enqueue workers: {}", 
                               enqueue_result.get_error().message());
        return;
    }
    
    // Start pool
    auto start_result = pool->start();
    if (start_result.has_error()) {
        log_module::write_error("Failed to start pool: {}", 
                               start_result.get_error().message());
        return;
    }
    
    // Start timing
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Submit jobs in batches
    size_t jobs_submitted = 0;
    while (jobs_submitted < TOTAL_JOBS) {
        size_t batch_size = std::min(JOBS_PER_BATCH, TOTAL_JOBS - jobs_submitted);
        auto jobs = create_test_jobs(batch_size);
        
        auto batch_result = pool->enqueue_batch(std::move(jobs));
        if (batch_result.has_error()) {
            log_module::write_error("Failed to enqueue batch: {}", 
                                   batch_result.get_error().message());
            break;
        }
        
        jobs_submitted += batch_size;
    }
    
    // Wait for all jobs to complete
    while (realtime_processed.load() + batch_processed.load() + background_processed.load() < jobs_submitted) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // End timing
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Print results
    log_module::write_information("Time taken: {} ms", duration.count());
    log_module::write_information("Jobs submitted: {}", jobs_submitted);
    log_module::write_information("Jobs processed: {}", 
                                 realtime_processed.load() + batch_processed.load() + background_processed.load());
    log_module::write_information("Throughput: {} jobs/sec", 
                                 jobs_submitted * 1000.0 / duration.count());
    log_module::write_information("Priority distribution:");
    log_module::write_information("  RealTime: {}", realtime_processed.load());
    log_module::write_information("  Batch: {}", batch_processed.load());
    log_module::write_information("  Background: {}", background_processed.load());
    
    // Get queue statistics if available
    if constexpr (std::is_same_v<PoolType, typed_lockfree_thread_pool>) {
        auto stats = pool->get_queue_statistics();
        log_module::write_information("Queue statistics:");
        log_module::write_information("  Type switches: {}", stats.type_switch_count);
        log_module::write_information("  Average enqueue latency: {} ns", 
                                     stats.get_average_enqueue_latency_ns());
        log_module::write_information("  Average dequeue latency: {} ns", 
                                     stats.get_average_dequeue_latency_ns());
    }
    
    // Stop pool
    pool->stop();
    
    log_module::write_information("{}", pool->to_string());
}

// Feature demonstration
void demonstrate_features()
{
    log_module::write_information("\n=== Feature Demonstration ===");
    
    // Create a typed lock-free thread pool
    auto pool = std::make_shared<typed_lockfree_thread_pool>("demo_pool");
    
    // Create workers with different priority specializations
    std::vector<std::unique_ptr<typed_lockfree_thread_worker>> workers;
    
    workers.push_back(std::make_unique<typed_lockfree_thread_worker>(
        std::vector<job_types>{job_types::RealTime}, "RealTime Specialist"));
    workers.push_back(std::make_unique<typed_lockfree_thread_worker>(
        std::vector<job_types>{job_types::Batch}, "Batch Specialist"));
    workers.push_back(std::make_unique<typed_lockfree_thread_worker>(
        std::vector<job_types>{job_types::Background}, "Background Specialist"));
    workers.push_back(std::make_unique<typed_lockfree_thread_worker>(
        typed_thread_pool_module::all_types(), "Generalist"));
    
    pool->enqueue_batch(std::move(workers));
    pool->start();
    
    log_module::write_information("Created lock-free pool with specialized workers");
    
    // Test 1: Priority ordering
    log_module::write_information("\n1. Priority ordering test:");
    
    // Submit jobs in reverse priority order
    for (int i = 0; i < 3; ++i) {
        pool->enqueue(std::make_unique<callback_typed_job>(
            [i]() -> result_void {
                log_module::write_information("   Background job {} executed", i);
                return {};
            },
            job_types::Background
        ));
    }
    
    for (int i = 0; i < 3; ++i) {
        pool->enqueue(std::make_unique<callback_typed_job>(
            [i]() -> result_void {
                log_module::write_information("   Batch job {} executed", i);
                return {};
            },
            job_types::Batch
        ));
    }
    
    for (int i = 0; i < 3; ++i) {
        pool->enqueue(std::make_unique<callback_typed_job>(
            [i]() -> result_void {
                log_module::write_information("   RealTime job {} executed", i);
                return {};
            },
            job_types::RealTime
        ));
    }
    
    // Wait for completion
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Test 2: Dynamic worker addition
    log_module::write_information("\n2. Dynamic worker addition:");
    
    auto new_worker = std::make_unique<typed_lockfree_thread_worker>(
        std::vector<job_types>{job_types::RealTime}, "Dynamic RealTime Worker");
    
    pool->enqueue(std::move(new_worker));
    log_module::write_information("   Added new RealTime worker dynamically");
    
    // Test 3: Batch job submission
    log_module::write_information("\n3. Batch job submission:");
    
    std::vector<std::unique_ptr<typed_job_t<job_types>>> batch_jobs;
    for (int i = 0; i < 5; ++i) {
        batch_jobs.push_back(std::make_unique<callback_typed_job>(
            [i]() -> result_void {
                log_module::write_information("   Batch submitted job {} executed", i);
                return {};
            },
            job_types::Batch
        ));
    }
    
    pool->enqueue_batch(std::move(batch_jobs));
    
    // Wait for completion
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    pool->stop();
}

int main()
{
    // Initialize logger
    log_module::start();
    log_module::console_target(log_module::log_types::Information);
    
    log_module::write_information("=== Typed Lock-Free Thread Pool Sample ===");
    log_module::write_information("Comparing performance between typed_thread_pool and typed_lockfree_thread_pool");
    
    // Feature demonstration
    demonstrate_features();
    
    // Performance comparison
    log_module::write_information("\n=== Performance Comparison ===");
    
    // Test traditional typed pool
    run_performance_test<typed_thread_pool>("typed_thread_pool (mutex-based)");
    
    // Test lock-free typed pool
    run_performance_test<typed_lockfree_thread_pool>("typed_lockfree_thread_pool");
    
    log_module::write_information("\n=== Test completed successfully ===");
    
    // Cleanup
    log_module::stop();
    
    return 0;
}