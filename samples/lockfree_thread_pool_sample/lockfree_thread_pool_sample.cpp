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
#include "thread_pool.h"
#include "../../sources/thread_base/lockfree/lockfree_mpmc_queue.h"

using namespace utility_module;
using namespace thread_pool_module;
using namespace thread_module;

int main() {
    std::cout << "Lock-Free Thread Pool Sample\n";
    std::cout << "===========================\n\n";

    try {
        // Test 1: Basic Thread Pool Usage
        std::cout << "=== Basic Thread Pool Usage ===\n";
        
        auto pool = std::make_unique<thread_pool>("worker");
        pool->start();
        std::cout << "Created thread pool\n";

        std::atomic<int> completed_jobs{0};
        const int total_jobs = 20;

        auto start_time = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < total_jobs; ++i) {
            auto job = std::make_unique<callback_job>([&completed_jobs, i]() -> std::optional<std::string> {
                // Simulate some work
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                completed_jobs.fetch_add(1);
                std::cout << "Completed job " << i << " on thread " 
                          << std::this_thread::get_id() << "\n";
                return std::nullopt;
            });

            pool->enqueue(std::move(job));
        }

        // Wait for completion
        while (completed_jobs.load() < total_jobs) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        std::cout << "All " << total_jobs << " jobs completed in " << duration.count() << " ms\n";
        pool->stop();
        std::cout << "Thread pool stopped gracefully\n\n";

        // Test 2: Lock-Free MPMC Queue Direct Usage
        std::cout << "=== Lock-Free MPMC Queue Direct Usage ===\n";
        
        auto lockfree_queue = std::make_unique<lockfree_mpmc_queue>();
        const int test_jobs = 1000;

        auto queue_start = std::chrono::high_resolution_clock::now();

        // Enqueue jobs
        for (int i = 0; i < test_jobs; ++i) {
            auto test_job = std::make_unique<callback_job>([i]() -> std::optional<std::string> {
                return std::nullopt;
            });
            
            lockfree_queue->enqueue(std::move(test_job));
        }

        // Dequeue all jobs
        int dequeued_count = 0;
        while (dequeued_count < test_jobs) {
            auto result = lockfree_queue->dequeue();
            if (result.has_value() && result.value()) {
                dequeued_count++;
            }
        }

        auto queue_end = std::chrono::high_resolution_clock::now();
        auto queue_duration = std::chrono::duration_cast<std::chrono::microseconds>(queue_end - queue_start);

        std::cout << "Lock-Free Queue Performance:\n";
        std::cout << "  Jobs processed: " << dequeued_count << "/" << test_jobs << "\n";
        std::cout << "  Total time: " << queue_duration.count() << " μs\n";
        std::cout << "  Throughput: " << (test_jobs * 1000000 / queue_duration.count()) << " ops/sec\n";

        // Display queue statistics
        auto stats = lockfree_queue->get_statistics();
        std::cout << "  Enqueue count: " << stats.enqueue_count << "\n";
        std::cout << "  Dequeue count: " << stats.dequeue_count << "\n";
        std::cout << "  Average enqueue latency: " << std::fixed << std::setprecision(2) 
                  << stats.get_average_enqueue_latency_ns() << " ns\n";
        std::cout << "  Average dequeue latency: " << std::fixed << std::setprecision(2)
                  << stats.get_average_dequeue_latency_ns() << " ns\n\n";

        // Test 3: Performance Test
        std::cout << "=== Performance Test ===\n";
        
        auto perf_pool = std::make_unique<thread_pool>("perf_worker");
        perf_pool->start();

        const int perf_jobs = 50000;
        std::atomic<int> perf_completed{0};

        auto perf_start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < perf_jobs; ++i) {
            auto job = std::make_unique<callback_job>([&perf_completed]() -> std::optional<std::string> {
                perf_completed.fetch_add(1);
                return std::nullopt;
            });

            perf_pool->enqueue(std::move(job));
        }

        // Wait for completion
        while (perf_completed.load() < perf_jobs) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        auto perf_end = std::chrono::high_resolution_clock::now();
        auto perf_duration = std::chrono::duration_cast<std::chrono::milliseconds>(perf_end - perf_start);

        std::cout << "Performance Results:\n";
        std::cout << "  Jobs: " << perf_jobs << "\n";
        std::cout << "  Time: " << perf_duration.count() << " ms\n";
        std::cout << "  Throughput: " << (perf_jobs * 1000 / perf_duration.count()) << " jobs/sec\n";

        perf_pool->stop();
        std::cout << "\n=== All demos completed successfully! ===\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}