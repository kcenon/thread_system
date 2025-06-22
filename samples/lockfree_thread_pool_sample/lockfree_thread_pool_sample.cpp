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

#include <chrono>
#include <atomic>
#include <vector>
#include <random>
#include <iomanip>
#include <sstream>
#include <thread>

#include "logger/core/logger.h"
#include "utilities/core/formatter.h"
#include "thread_pool/core/lockfree_thread_pool.h"
#include "thread_pool/core/thread_pool.h"
#include "thread_base/jobs/callback_job.h"

using namespace utility_module;
using namespace thread_pool_module;
using namespace thread_module;

// Helper function to format duration
std::string format_duration(std::chrono::nanoseconds ns) {
    if (ns.count() < 1000) {
        return formatter::format("{} ns", ns.count());
    } else if (ns.count() < 1000000) {
        return formatter::format("{:.2f} μs", ns.count() / 1000.0);
    } else if (ns.count() < 1000000000) {
        return formatter::format("{:.2f} ms", ns.count() / 1000000.0);
    } else {
        return formatter::format("{:.2f} s", ns.count() / 1000000000.0);
    }
}

// Benchmark function for comparing thread pools
template<typename PoolType>
void benchmark_pool(const std::string& pool_name, int worker_count, int job_count, int job_duration_us) {
    log_module::write_information("\n=== {} Benchmark ===", pool_name);
    log_module::write_information("Workers: {}, Jobs: {}, Job Duration: {} μs", 
                                worker_count, job_count, job_duration_us);
    
    auto pool = std::make_shared<PoolType>(pool_name);
    
    // Add workers
    if constexpr (std::is_same_v<PoolType, lockfree_thread_pool>) {
        std::vector<std::unique_ptr<lockfree_thread_worker>> workers;
        for (int i = 0; i < worker_count; ++i) {
            auto worker = std::make_unique<lockfree_thread_worker>();
            // Enable batch processing for better performance
            worker->set_batch_processing(true, 16);
            workers.push_back(std::move(worker));
        }
        pool->enqueue_batch(std::move(workers));
    } else {
        std::vector<std::unique_ptr<thread_worker>> workers;
        for (int i = 0; i < worker_count; ++i) {
            workers.push_back(std::make_unique<thread_worker>());
        }
        pool->enqueue_batch(std::move(workers));
    }
    
    // Start the pool
    auto start_result = pool->start();
    if (start_result.has_value()) {
        log_module::write_error("Failed to start pool: {}", start_result.value());
        return;
    }
    
    std::atomic<int> completed_jobs{0};
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Submit jobs
    for (int i = 0; i < job_count; ++i) {
        auto job = std::make_unique<callback_job>([&completed_jobs, job_duration_us]() -> result_void {
            // Simulate work
            if (job_duration_us > 0) {
                auto start = std::chrono::high_resolution_clock::now();
                while (std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::high_resolution_clock::now() - start).count() < job_duration_us) {
                    // Busy wait for accurate timing
                }
            }
            completed_jobs.fetch_add(1);
            return {};
        });
        
        pool->enqueue(std::move(job));
    }
    
    // Wait for completion
    while (completed_jobs.load() < job_count) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    
    // Get statistics if available
    if constexpr (std::is_same_v<PoolType, lockfree_thread_pool>) {
        auto stats = pool->get_queue_statistics();
        log_module::write_information("Results:");
        log_module::write_information("  Total time: {}", format_duration(duration));
        log_module::write_information("  Throughput: {} jobs/sec", 
                                    static_cast<double>(job_count) * 1000000000 / duration.count());
        log_module::write_information("  Queue Statistics:");
        log_module::write_information("    Enqueues: {}", stats.enqueue_count);
        log_module::write_information("    Dequeues: {}", stats.dequeue_count);
        log_module::write_information("    Avg enqueue latency: {} ns", stats.get_average_enqueue_latency_ns());
        log_module::write_information("    Avg dequeue latency: {} ns", stats.get_average_dequeue_latency_ns());
        log_module::write_information("    Retries: {}", stats.retry_count);
    } else {
        log_module::write_information("Results:");
        log_module::write_information("  Total time: {}", format_duration(duration));
        log_module::write_information("  Throughput: {} jobs/sec", 
                                    static_cast<double>(job_count) * 1000000000 / duration.count());
    }
    
    // Stop the pool
    pool->stop();
}

int main() {
    // Initialize logger
    log_module::set_title("LockfreeThreadPoolDemo");
    log_module::console_target(log_module::log_types::Information);
    log_module::start();
    
    log_module::write_information("Lock-Free Thread Pool Demonstration");
    log_module::write_information("===================================");
    
    try {
        // Test 1: Basic functionality test
        log_module::write_information("\n=== Basic Functionality Test ===");
        {
            auto pool = std::make_shared<lockfree_thread_pool>("BasicTest");
            
            // Add workers
            std::vector<std::unique_ptr<lockfree_thread_worker>> workers;
            for (int i = 0; i < 4; ++i) {
                workers.push_back(std::make_unique<lockfree_thread_worker>());
            }
            pool->enqueue_batch(std::move(workers));
            
            // Start pool
            pool->start();
            log_module::write_information("Pool started with {} workers", pool->worker_count());
            
            // Submit some jobs
            std::atomic<int> counter{0};
            const int job_count = 20;
            
            for (int i = 0; i < job_count; ++i) {
                auto job = std::make_unique<callback_job>([&counter, i]() -> result_void {
                    counter.fetch_add(1);
                    std::ostringstream oss;
                    oss << std::this_thread::get_id();
                    log_module::write_information("Job {} completed by thread {}", 
                                                i, oss.str());
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    return {};
                });
                
                pool->enqueue(std::move(job));
            }
            
            // Wait for completion
            while (counter.load() < job_count) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            
            log_module::write_information("All {} jobs completed successfully", job_count);
            log_module::write_information("Pool state: {}", pool->to_string());
            
            pool->stop();
        }
        
        // Test 2: Performance comparison
        log_module::write_information("\n=== Performance Comparison ===");
        
        // Test with different configurations
        std::vector<std::tuple<int, int, int>> test_configs = {
            {4, 1000, 0},      // Light load, no work
            {4, 10000, 10},    // Medium load, light work
            {8, 100000, 0},    // Heavy load, no work
            {8, 10000, 100},   // Medium load, heavy work
        };
        
        for (const auto& [workers, jobs, duration] : test_configs) {
            // Benchmark standard thread pool
            benchmark_pool<thread_pool>("Standard Thread Pool", workers, jobs, duration);
            
            // Benchmark lock-free thread pool
            benchmark_pool<lockfree_thread_pool>("Lock-Free Thread Pool", workers, jobs, duration);
            
            log_module::write_information("");
        }
        
        // Test 3: Batch processing test
        log_module::write_information("\n=== Batch Processing Test ===");
        {
            auto pool = std::make_shared<lockfree_thread_pool>("BatchTest");
            
            // Add workers with batch processing enabled
            std::vector<std::unique_ptr<lockfree_thread_worker>> workers;
            for (int i = 0; i < 4; ++i) {
                auto worker = std::make_unique<lockfree_thread_worker>();
                worker->set_batch_processing(true, 32);  // Process up to 32 jobs at once
                workers.push_back(std::move(worker));
            }
            pool->enqueue_batch(std::move(workers));
            pool->start();
            
            // Submit many small jobs
            const int batch_jobs = 100000;
            std::atomic<int> batch_counter{0};
            
            auto batch_start = std::chrono::high_resolution_clock::now();
            
            // Use batch enqueue for better performance
            std::vector<std::unique_ptr<job>> job_batch;
            for (int i = 0; i < batch_jobs; ++i) {
                job_batch.push_back(std::make_unique<callback_job>([&batch_counter]() -> result_void {
                    batch_counter.fetch_add(1);
                    return {};
                }));
                
                // Submit in batches of 1000
                if (job_batch.size() == 1000) {
                    pool->enqueue_batch(std::move(job_batch));
                    job_batch.clear();
                }
            }
            
            // Submit remaining jobs
            if (!job_batch.empty()) {
                pool->enqueue_batch(std::move(job_batch));
            }
            
            // Wait for completion
            while (batch_counter.load() < batch_jobs) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
            
            auto batch_end = std::chrono::high_resolution_clock::now();
            auto batch_duration = std::chrono::duration_cast<std::chrono::milliseconds>(batch_end - batch_start);
            
            log_module::write_information("Batch processing results:");
            log_module::write_information("  Jobs: {}", batch_jobs);
            log_module::write_information("  Time: {} ms", batch_duration.count());
            log_module::write_information("  Throughput: {} jobs/sec", 
                                        batch_jobs * 1000 / batch_duration.count());
            
            auto stats = pool->get_queue_statistics();
            log_module::write_information("  Batch enqueues: {}", stats.enqueue_batch_count);
            
            pool->stop();
        }
        
        log_module::write_information("\n=== All tests completed successfully! ===");
        
    } catch (const std::exception& e) {
        log_module::write_error("Error: {}", e.what());
        log_module::stop();
        return 1;
    }
    
    log_module::stop();
    return 0;
}