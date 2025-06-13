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

#include <iostream>
#include <chrono>
#include <atomic>
#include <vector>
#include <random>
#include <iomanip>

#include "logger.h"
#include "formatter.h"
#include "typed_thread_pool.h"
#include "callback_typed_job.h"

using namespace utility_module;
using namespace typed_thread_pool_module;
using namespace thread_module;

int main() {
    std::cout << "Lock-Free Typed Thread Pool Sample\n";
    std::cout << "==================================\n\n";

    try {
        // Test 1: Basic Type-based Job Processing
        std::cout << "=== Basic Type-based Job Processing ===\n";
        
        // Create typed thread pool with lock-free queues
        auto pool = std::make_unique<typed_thread_pool_t<job_types>>(4, true); // true for lock-free
        pool->start();
        std::cout << "Created lock-free typed thread pool with 4 workers\n";

        std::atomic<int> realtime_completed{0};
        std::atomic<int> batch_completed{0};
        std::atomic<int> background_completed{0};

        const int jobs_per_type = 10;

        // Submit RealTime jobs
        for (int i = 0; i < jobs_per_type; ++i) {
            auto job = std::make_unique<callback_typed_job<job_types>>(
                job_types::RealTime,
                [&realtime_completed, i]() -> std::optional<std::string> {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    realtime_completed.fetch_add(1);
                    std::cout << "RealTime job " << i << " completed on thread " 
                              << std::this_thread::get_id() << "\n";
                    return std::nullopt;
                }
            );
            
            pool->enqueue(std::move(job));
        }

        // Submit Batch jobs
        for (int i = 0; i < jobs_per_type; ++i) {
            auto job = std::make_unique<callback_typed_job<job_types>>(
                job_types::Batch,
                [&batch_completed, i]() -> std::optional<std::string> {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    batch_completed.fetch_add(1);
                    std::cout << "Batch job " << i << " completed on thread " 
                              << std::this_thread::get_id() << "\n";
                    return std::nullopt;
                }
            );
            
            pool->enqueue(std::move(job));
        }

        // Submit Background jobs
        for (int i = 0; i < jobs_per_type; ++i) {
            auto job = std::make_unique<callback_typed_job<job_types>>(
                job_types::Background,
                [&background_completed, i]() -> std::optional<std::string> {
                    std::this_thread::sleep_for(std::chrono::milliseconds(15));
                    background_completed.fetch_add(1);
                    std::cout << "Background job " << i << " completed on thread " 
                              << std::this_thread::get_id() << "\n";
                    return std::nullopt;
                }
            );
            
            pool->enqueue(std::move(job));
        }

        // Wait for completion
        while (realtime_completed.load() + batch_completed.load() + background_completed.load() < jobs_per_type * 3) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        std::cout << "Job completion summary:\n";
        std::cout << "  RealTime jobs: " << realtime_completed.load() << "/" << jobs_per_type << "\n";
        std::cout << "  Batch jobs: " << batch_completed.load() << "/" << jobs_per_type << "\n";
        std::cout << "  Background jobs: " << background_completed.load() << "/" << jobs_per_type << "\n\n";

        pool->stop();

        // Test 2: Performance Test with Different Types
        std::cout << "=== Performance Test with Mixed Types ===\n";
        
        auto perf_pool = std::make_unique<typed_thread_pool_t<job_types>>(4, true);
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

            auto job = std::make_unique<callback_typed_job<job_types>>(
                type,
                [&perf_completed, counter]() -> std::optional<std::string> {
                    perf_completed.fetch_add(1);
                    counter->fetch_add(1);
                    return std::nullopt;
                }
            );

            perf_pool->add_job(std::move(job));
        }

        // Wait for completion
        while (perf_completed.load() < perf_jobs) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        auto perf_end = std::chrono::high_resolution_clock::now();
        auto perf_duration = std::chrono::duration_cast<std::chrono::milliseconds>(perf_end - perf_start);

        std::cout << "Typed Pool Performance Results:\n";
        std::cout << "  Total jobs: " << perf_jobs << "\n";
        std::cout << "  RealTime: " << realtime_perf.load() << "\n";
        std::cout << "  Batch: " << batch_perf.load() << "\n";
        std::cout << "  Background: " << background_perf.load() << "\n";
        std::cout << "  Time: " << perf_duration.count() << " ms\n";
        std::cout << "  Throughput: " << (perf_jobs * 1000 / perf_duration.count()) << " jobs/sec\n\n";

        perf_pool->stop();

        // Test 3: Load Distribution Test
        std::cout << "=== Load Distribution Test ===\n";
        
        auto load_pool = std::make_unique<typed_thread_pool_t<job_types>>(4, true);
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

            auto job = std::make_unique<callback_typed_job<job_types>>(
                type,
                [counter]() -> std::optional<std::string> {
                    // Simulate variable work
                    std::this_thread::sleep_for(std::chrono::microseconds(50));
                    counter->fetch_add(1);
                    return std::nullopt;
                }
            );

            load_pool->add_job(std::move(job));
        }

        // Wait for completion
        while (load_realtime.load() + load_batch.load() + load_background.load() < load_jobs) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        auto load_end = std::chrono::high_resolution_clock::now();
        auto load_duration = std::chrono::duration_cast<std::chrono::milliseconds>(load_end - load_start);

        std::cout << "Load balancing results:\n";
        std::cout << "  RealTime jobs processed: " << load_realtime.load() << "\n";
        std::cout << "  Batch jobs processed: " << load_batch.load() << "\n";
        std::cout << "  Background jobs processed: " << load_background.load() << "\n";
        std::cout << "  Total jobs: " << (load_realtime.load() + load_batch.load() + load_background.load()) << "\n";
        std::cout << "  Processing time: " << load_duration.count() << " ms\n";
        std::cout << "  Throughput: " << (load_jobs * 1000 / load_duration.count()) << " jobs/sec\n";

        load_pool->stop();

        std::cout << "\n=== All Lock-Free Typed Thread Pool demos completed successfully! ===\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}