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

#include "logger/core/logger.h"
#include "utilities/core/formatter.h"
#include "typed_thread_pool/pool/typed_thread_pool.h"
#include "typed_thread_pool/jobs/callback_typed_job.h"

using namespace utility_module;
using namespace typed_thread_pool_module;
using namespace thread_module;

int main() {
    // Initialize logger
    log_module::start();
    log_module::console_target(log_module::log_types::Information);
    
    log_module::write_information("Lock-Free Typed Thread Pool Sample");
    log_module::write_information("==================================");

    try {
        // Test 1: Basic Type-based Job Processing
        log_module::write_information("\n=== Basic Type-based Job Processing ===");
        
        // Create typed thread pool
        auto pool = std::make_unique<typed_thread_pool>("lockfree_typed_pool");
        pool->start();
        log_module::write_information("Created lock-free typed thread pool with 4 workers");

        std::atomic<int> realtime_completed{0};
        std::atomic<int> batch_completed{0};
        std::atomic<int> background_completed{0};

        const int jobs_per_type = 10;

        // Submit RealTime jobs
        for (int i = 0; i < jobs_per_type; ++i) {
            auto job = std::make_unique<callback_typed_job>(
                [&realtime_completed, i]() -> result_void {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    realtime_completed.fetch_add(1);
                    std::ostringstream oss;
                    oss << std::this_thread::get_id();
                    log_module::write_information("RealTime job {} completed on thread {}",
                              i, oss.str());
                    return result_void();
                },
                job_types::RealTime
            );
            
            pool->enqueue(std::move(job));
        }

        // Submit Batch jobs
        for (int i = 0; i < jobs_per_type; ++i) {
            auto job = std::make_unique<callback_typed_job>(
                [&batch_completed, i]() -> result_void {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    batch_completed.fetch_add(1);
                    std::ostringstream oss;
                    oss << std::this_thread::get_id();
                    log_module::write_information("Batch job {} completed on thread {}",
                              i, oss.str());
                    return result_void();
                },
                job_types::Batch
            );
            
            pool->enqueue(std::move(job));
        }

        // Submit Background jobs
        for (int i = 0; i < jobs_per_type; ++i) {
            auto job = std::make_unique<callback_typed_job>(
                [&background_completed, i]() -> result_void {
                    std::this_thread::sleep_for(std::chrono::milliseconds(15));
                    background_completed.fetch_add(1);
                    std::ostringstream oss;
                    oss << std::this_thread::get_id();
                    log_module::write_information("Background job {} completed on thread {}",
                              i, oss.str());
                    return result_void();
                },
                job_types::Background
            );
            
            pool->enqueue(std::move(job));
        }

        // Wait for completion
        while (realtime_completed.load() + batch_completed.load() + background_completed.load() < jobs_per_type * 3) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        log_module::write_information("Job completion summary:");
        log_module::write_information("  RealTime jobs: {}/{}", realtime_completed.load(), jobs_per_type);
        log_module::write_information("  Batch jobs: {}/{}", batch_completed.load(), jobs_per_type);
        log_module::write_information("  Background jobs: {}/{}", background_completed.load(), jobs_per_type);

        pool->stop();

        // Test 2: Performance Test with Different Types
        log_module::write_information("\n=== Performance Test with Mixed Types ===");
        
        auto perf_pool = std::make_unique<typed_thread_pool>("perf_pool");
        perf_pool->start();

        const int perf_jobs = 30000;
        std::atomic<int> perf_completed{0};
        std::atomic<int> realtime_perf{0};
        std::atomic<int> batch_perf{0};
        std::atomic<int> background_perf{0};

        auto perf_start = std::chrono::high_resolution_clock::now();

        // Distribute jobs across all types
        for (int i = 0; i < perf_jobs; ++i) {
            job_types type;
            std::atomic<int>* counter;
            
            switch (i % 3) {
                case 0: 
                    type = job_types::RealTime; 
                    counter = &realtime_perf;
                    break;
                case 1: 
                    type = job_types::Batch; 
                    counter = &batch_perf;
                    break;
                case 2: 
                    type = job_types::Background; 
                    counter = &background_perf;
                    break;
            }

            auto job = std::make_unique<callback_typed_job>(
                [&perf_completed, counter]() -> result_void {
                    perf_completed.fetch_add(1);
                    counter->fetch_add(1);
                    return result_void();
                },
                type
            );

            perf_pool->enqueue(std::move(job));
        }

        // Wait for completion
        while (perf_completed.load() < perf_jobs) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        auto perf_end = std::chrono::high_resolution_clock::now();
        auto perf_duration = std::chrono::duration_cast<std::chrono::milliseconds>(perf_end - perf_start);

        log_module::write_information("Typed Pool Performance Results:");
        log_module::write_information("  Total jobs: {}", perf_jobs);
        log_module::write_information("  RealTime: {}", realtime_perf.load());
        log_module::write_information("  Batch: {}", batch_perf.load());
        log_module::write_information("  Background: {}", background_perf.load());
        log_module::write_information("  Time: {} ms", perf_duration.count());
        log_module::write_information("  Throughput: {} jobs/sec", perf_jobs * 1000 / perf_duration.count());

        perf_pool->stop();

        // Test 3: Load Distribution Test
        log_module::write_information("\n=== Load Distribution Test ===");
        
        auto load_pool = std::make_unique<typed_thread_pool>("load_pool");
        load_pool->start();

        const int load_jobs = 15000;
        std::atomic<int> load_realtime{0};
        std::atomic<int> load_batch{0};
        std::atomic<int> load_background{0};

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> type_dis(0, 2);

        auto load_start = std::chrono::high_resolution_clock::now();

        // Submit jobs with random types to test load balancing
        for (int i = 0; i < load_jobs; ++i) {
            job_types type;
            std::atomic<int>* counter;
            
            switch (type_dis(gen)) {
                case 0: 
                    type = job_types::RealTime; 
                    counter = &load_realtime; 
                    break;
                case 1: 
                    type = job_types::Batch; 
                    counter = &load_batch; 
                    break;
                case 2: 
                    type = job_types::Background; 
                    counter = &load_background; 
                    break;
            }

            auto job = std::make_unique<callback_typed_job>(
                [counter]() -> result_void {
                    // Simulate variable work
                    std::this_thread::sleep_for(std::chrono::microseconds(50));
                    counter->fetch_add(1);
                    return result_void();
                },
                type
            );

            load_pool->enqueue(std::move(job));
        }

        // Wait for completion
        while (load_realtime.load() + load_batch.load() + load_background.load() < load_jobs) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        auto load_end = std::chrono::high_resolution_clock::now();
        auto load_duration = std::chrono::duration_cast<std::chrono::milliseconds>(load_end - load_start);

        log_module::write_information("Load balancing results:");
        log_module::write_information("  RealTime jobs processed: {}", load_realtime.load());
        log_module::write_information("  Batch jobs processed: {}", load_batch.load());
        log_module::write_information("  Background jobs processed: {}", load_background.load());
        log_module::write_information("  Total jobs: {}", load_realtime.load() + load_batch.load() + load_background.load());
        log_module::write_information("  Processing time: {} ms", load_duration.count());
        log_module::write_information("  Throughput: {} jobs/sec", load_jobs * 1000 / load_duration.count());

        load_pool->stop();

        log_module::write_information("\n=== All Lock-Free Typed Thread Pool demos completed successfully! ===");
        
        log_module::stop();
        return 0;
    } catch (const std::exception& e) {
        log_module::write_error("Error: {}", e.what());
        log_module::stop();
        return 1;
    }
}