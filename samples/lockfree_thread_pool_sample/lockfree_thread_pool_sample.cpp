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
#include "thread_pool/core/thread_pool.h"
#include "thread_base/lockfree/queues/lockfree_job_queue.h"

using namespace utility_module;
using namespace thread_pool_module;
using namespace thread_module;

int main() {
    // Initialize logger
    log_module::start();
    log_module::console_target(log_module::log_types::Information);
    
    log_module::write_information("Lock-Free Thread Pool Sample");
    log_module::write_information("===========================");

    try {
        // Test 1: Basic Thread Pool Usage
        log_module::write_information("\n=== Basic Thread Pool Usage ===");
        
        auto pool = std::make_unique<thread_pool>();
        pool->start();
        log_module::write_information("Created thread pool");

        std::atomic<int> completed_jobs{0};
        const int total_jobs = 20;

        auto start_time = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < total_jobs; ++i) {
            auto job = std::make_unique<callback_job>([&completed_jobs, i]() -> result_void {
                // Simulate some work
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                completed_jobs.fetch_add(1);
                std::ostringstream oss;
                oss << std::this_thread::get_id();
                log_module::write_information("Completed job {} on thread {}", 
                          i, oss.str());
                return result_void();
            });

            pool->enqueue(std::move(job));
        }

        // Wait for completion
        while (completed_jobs.load() < total_jobs) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        log_module::write_information("All {} jobs completed in {} ms", total_jobs, duration.count());
        pool->stop();
        log_module::write_information("Thread pool stopped gracefully");

        // Test 2: Lock-Free MPMC Queue Direct Usage
        log_module::write_information("\n=== Lock-Free MPMC Queue Direct Usage ===");
        
        auto lockfree_queue = std::make_unique<lockfree_job_queue>();
        const int test_jobs = 1000;

        auto queue_start = std::chrono::high_resolution_clock::now();

        // Enqueue jobs
        for (int i = 0; i < test_jobs; ++i) {
            auto test_job = std::make_unique<callback_job>([i]() -> result_void {
                return result_void();
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

        log_module::write_information("Lock-Free Queue Performance:");
        log_module::write_information("  Jobs processed: {}/{}", dequeued_count, test_jobs);
        log_module::write_information("  Total time: {} μs", queue_duration.count());
        log_module::write_information("  Throughput: {} ops/sec", test_jobs * 1000000 / queue_duration.count());

        // Display queue statistics
        auto stats = lockfree_queue->get_statistics();
        log_module::write_information("  Enqueue count: {}", stats.enqueue_count);
        log_module::write_information("  Dequeue count: {}", stats.dequeue_count);
        log_module::write_information("  Average enqueue latency: {:.2f} ns", stats.get_average_enqueue_latency_ns());
        log_module::write_information("  Average dequeue latency: {:.2f} ns", stats.get_average_dequeue_latency_ns());

        // Test 3: Performance Test
        log_module::write_information("\n=== Performance Test ===");
        
        auto perf_pool = std::make_unique<thread_pool>("perf_worker");
        perf_pool->start();

        const int perf_jobs = 50000;
        std::atomic<int> perf_completed{0};

        auto perf_start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < perf_jobs; ++i) {
            auto job = std::make_unique<callback_job>([&perf_completed]() -> result_void {
                perf_completed.fetch_add(1);
                return result_void();
            });

            perf_pool->enqueue(std::move(job));
        }

        // Wait for completion
        while (perf_completed.load() < perf_jobs) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        auto perf_end = std::chrono::high_resolution_clock::now();
        auto perf_duration = std::chrono::duration_cast<std::chrono::milliseconds>(perf_end - perf_start);

        log_module::write_information("Performance Results:");
        log_module::write_information("  Jobs: {}", perf_jobs);
        log_module::write_information("  Time: {} ms", perf_duration.count());
        log_module::write_information("  Throughput: {} jobs/sec", perf_jobs * 1000 / perf_duration.count());

        perf_pool->stop();
        log_module::write_information("\n=== All demos completed successfully! ===");
        
        log_module::stop();
        return 0;
    } catch (const std::exception& e) {
        log_module::write_error("Error: {}", e.what());
        log_module::stop();
        return 1;
    }
}