/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, DongCheol Shin
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
 * @file queue_factory_sample.cpp
 * @brief Demonstrates queue_factory usage for convenient queue creation
 *
 * This sample shows how to use the queue_factory class to create different
 * queue types based on requirements, either at runtime or compile-time.
 */

#include <kcenon/thread/queue/queue_factory.h>
#include <kcenon/thread/core/callback_job.h>

#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>

using namespace kcenon::thread;

/**
 * @brief Example 1: Simple factory usage
 *
 * Demonstrates the basic factory methods for creating different queue types.
 */
void simple_factory_usage()
{
    std::cout << "=== Example 1: Simple Factory Usage ===" << std::endl;

    // Create standard queue (job_queue) - exact size, batch operations, blocking dequeue
    auto standard = queue_factory::create_standard_queue();
    std::cout << "Standard queue (job_queue):" << std::endl;
    std::cout << "  - has_exact_size: " << std::boolalpha << standard->has_exact_size() << std::endl;
    std::cout << "  - is_lock_free: " << standard->is_lock_free() << std::endl;

    // Create lock-free queue - maximum throughput, non-blocking
    auto lockfree = queue_factory::create_lockfree_queue();
    std::cout << "Lock-free queue (lockfree_job_queue):" << std::endl;
    std::cout << "  - has_exact_size: " << lockfree->has_exact_size() << std::endl;
    std::cout << "  - is_lock_free: " << lockfree->is_lock_free() << std::endl;

    // Create adaptive queue - auto-optimizing based on workload
    auto adaptive = queue_factory::create_adaptive_queue();
    std::cout << "Adaptive queue (adaptive_job_queue):" << std::endl;
    std::cout << "  - has_exact_size: " << adaptive->has_exact_size() << std::endl;
    std::cout << "  - is_lock_free: " << adaptive->is_lock_free() << std::endl;
    std::cout << "  - auto-switching enabled for balanced performance" << std::endl;

    std::cout << std::endl;
}

/**
 * @brief Example 2: Requirements-based selection
 *
 * Shows how to use requirements to automatically select the appropriate queue type.
 * Returns scheduler_interface which provides schedule() and get_next_job() methods.
 */
void requirements_based_selection()
{
    std::cout << "=== Example 2: Requirements-Based Selection ===" << std::endl;

    // Scenario: Monitoring system needs exact counts
    std::cout << "Monitoring queue (need_exact_size=true):" << std::endl;
    queue_factory::requirements monitoring_reqs;
    monitoring_reqs.need_exact_size = true;
    auto monitoring_queue = queue_factory::create_for_requirements(monitoring_reqs);
    std::cout << "  - Returns job_queue via scheduler_interface" << std::endl;
    std::cout << "  - Provides exact size() and empty() operations" << std::endl;

    // Scenario: High-performance logging prefers lock-free
    std::cout << "Logging queue (prefer_lock_free=true):" << std::endl;
    queue_factory::requirements logging_reqs;
    logging_reqs.prefer_lock_free = true;
    auto logging_queue = queue_factory::create_for_requirements(logging_reqs);
    std::cout << "  - Returns lockfree_job_queue via scheduler_interface" << std::endl;
    std::cout << "  - Maximum throughput for high-volume logging" << std::endl;

    // Scenario: Batch processing needs batch operations
    std::cout << "Batch queue (need_batch_operations=true):" << std::endl;
    queue_factory::requirements batch_reqs;
    batch_reqs.need_batch_operations = true;
    auto batch_queue = queue_factory::create_for_requirements(batch_reqs);
    std::cout << "  - Returns job_queue for batch operation support" << std::endl;

    // Scenario: No specific requirements - gets adaptive queue
    std::cout << "Default queue (no specific requirements):" << std::endl;
    queue_factory::requirements default_reqs;
    auto default_queue = queue_factory::create_for_requirements(default_reqs);
    std::cout << "  - Returns adaptive_job_queue for flexibility" << std::endl;

    // Demonstrate basic scheduler_interface usage
    std::cout << "\nUsing scheduler_interface:" << std::endl;
    auto job = std::make_unique<callback_job>(
        []() -> result_void {
            std::cout << "  - Job executed!" << std::endl;
            return result_void();
        });
    auto schedule_result = monitoring_queue->schedule(std::move(job));
    if (!schedule_result.has_error()) {
        auto next_job = monitoring_queue->get_next_job();
        if (next_job.has_value()) {
            auto work_result = next_job.value()->do_work();
            (void)work_result;
        }
    }

    std::cout << std::endl;
}

/**
 * @brief Example 3: Optimal queue selection
 *
 * Demonstrates automatic queue selection based on the runtime environment.
 */
void optimal_selection()
{
    std::cout << "=== Example 3: Optimal Queue Selection ===" << std::endl;

    auto optimal = queue_factory::create_optimal();

    std::cout << "Optimal queue selected for this system:" << std::endl;
    std::cout << "  Selection criteria:" << std::endl;
    std::cout << "  - Hardware concurrency: " << std::thread::hardware_concurrency() << " cores" << std::endl;
#if defined(__aarch64__) || defined(_M_ARM64)
    std::cout << "  - Architecture: ARM (weak memory model)" << std::endl;
    std::cout << "  - Selection: job_queue (safety priority)" << std::endl;
#else
    std::cout << "  - Architecture: x86 (strong memory model)" << std::endl;
    if (std::thread::hardware_concurrency() <= 2) {
        std::cout << "  - Selection: job_queue (mutex efficient for low core count)" << std::endl;
    } else {
        std::cout << "  - Selection: adaptive_job_queue (best of both worlds)" << std::endl;
    }
#endif

    // Demonstrate usage through scheduler_interface
    std::cout << "\nUsing optimal queue:" << std::endl;
    std::atomic<int> job_count{0};
    const int num_jobs = 5;

    for (int i = 0; i < num_jobs; ++i) {
        auto job = std::make_unique<callback_job>(
            [&job_count]() -> result_void {
                job_count.fetch_add(1);
                return result_void();
            });
        optimal->schedule(std::move(job));
    }

    // Process all jobs
    for (int i = 0; i < num_jobs; ++i) {
        auto result = optimal->get_next_job();
        if (result.has_value()) {
            auto work_result = result.value()->do_work();
            (void)work_result;
        }
    }
    std::cout << "  Processed " << job_count.load() << " jobs" << std::endl;

    std::cout << std::endl;
}

/**
 * @brief Example 4: Compile-time selection
 *
 * Shows how to use template-based type selection for zero-overhead queue selection.
 */
void compile_time_selection()
{
    std::cout << "=== Example 4: Compile-Time Selection ===" << std::endl;

    // Type aliases for common use cases
    std::cout << "Pre-defined type aliases:" << std::endl;
    std::cout << "  - accurate_queue_t = job_queue (exact size/empty)" << std::endl;
    std::cout << "  - fast_queue_t = lockfree_job_queue (maximum throughput)" << std::endl;
    std::cout << "  - balanced_queue_t = adaptive_job_queue (auto-tuning)" << std::endl;

    // Demonstrate usage
    accurate_queue_t accurate;
    fast_queue_t fast;
    balanced_queue_t balanced;

    std::cout << "\nInstantiated queues:" << std::endl;
    std::cout << "  - accurate_queue_t has_exact_size: " << std::boolalpha << accurate.has_exact_size() << std::endl;
    std::cout << "  - fast_queue_t is_lock_free: " << fast.is_lock_free() << std::endl;
    std::cout << "  - balanced_queue_t (adaptive mode)" << std::endl;

    // Show template-based selection
    std::cout << "\nTemplate-based selection (queue_t<NeedExactSize, PreferLockFree>):" << std::endl;
    std::cout << "  - queue_t<true, false>  -> job_queue" << std::endl;
    std::cout << "  - queue_t<false, true>  -> lockfree_job_queue" << std::endl;
    std::cout << "  - queue_t<false, false> -> adaptive_job_queue" << std::endl;
    std::cout << "  - queue_t<true, true>   -> compile error (mutually exclusive)" << std::endl;

    std::cout << std::endl;
}

/**
 * @brief Example 5: Practical use cases
 *
 * Demonstrates real-world scenarios where different queue types are appropriate.
 */
void practical_use_cases()
{
    std::cout << "=== Example 5: Practical Use Cases ===" << std::endl;

    // Financial system: needs exact counts for audit
    std::cout << "\n[Financial System - Audit Queue]" << std::endl;
    std::cout << "  Requirements: exact_size + batch_operations" << std::endl;
    std::cout << "  Selected: job_queue (mutex-based for accuracy)" << std::endl;
    auto financial_queue = queue_factory::create_standard_queue();

    // High-frequency trading: needs maximum speed
    std::cout << "\n[High-Frequency Trading - Order Queue]" << std::endl;
    std::cout << "  Requirements: prefer_lock_free" << std::endl;
    std::cout << "  Selected: lockfree_job_queue (maximum throughput)" << std::endl;
    auto hft_queue = queue_factory::create_lockfree_queue();

    // Web server: balanced workload
    std::cout << "\n[Web Server - Request Queue]" << std::endl;
    std::cout << "  Requirements: variable load, auto-tuning" << std::endl;
    std::cout << "  Selected: adaptive_job_queue with balanced policy" << std::endl;
    auto web_queue = queue_factory::create_adaptive_queue(adaptive_job_queue::policy::balanced);

    // Demonstrate actual usage with financial queue (has exact size)
    std::cout << "\n[Demo: Processing jobs through financial queue]" << std::endl;
    std::atomic<int> processed{0};

    // Enqueue some jobs
    for (int i = 0; i < 5; ++i) {
        auto job = std::make_unique<callback_job>(
            [i, &processed]() -> result_void {
                processed.fetch_add(1);
                return result_void();
            });
        auto result = financial_queue->enqueue(std::move(job));
        if (!result.has_error()) {
            std::cout << "  Enqueued job " << i << ", queue size: " << financial_queue->size() << std::endl;
        }
    }

    // Process jobs
    while (!financial_queue->empty()) {
        auto result = financial_queue->dequeue();
        if (result.has_value()) {
            auto work_result = result.value()->do_work();
            (void)work_result;
        }
    }
    std::cout << "  Processed " << processed.load() << " jobs" << std::endl;

    // Demonstrate HFT queue (lock-free)
    std::cout << "\n[Demo: High-frequency trading simulation]" << std::endl;
    std::atomic<int> orders_processed{0};
    const int order_count = 1000;

    auto start = std::chrono::high_resolution_clock::now();

    // Enqueue orders
    for (int i = 0; i < order_count; ++i) {
        auto job = std::make_unique<callback_job>(
            [&orders_processed]() -> result_void {
                orders_processed.fetch_add(1);
                return result_void();
            });
        auto enqueue_result = hft_queue->enqueue(std::move(job));
        (void)enqueue_result;
    }

    // Process orders
    while (true) {
        auto result = hft_queue->dequeue();
        if (!result.has_value()) break;
        auto work_result = result.value()->do_work();
        (void)work_result;
    }

    auto duration = std::chrono::high_resolution_clock::now() - start;
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    std::cout << "  Processed " << orders_processed.load() << " orders in " << us << " us" << std::endl;
    if (us > 0) {
        std::cout << "  Throughput: " << (order_count * 1000000.0 / us) << " ops/sec" << std::endl;
    }

    std::cout << std::endl;
}

int main()
{
    std::cout << "Queue Factory Sample" << std::endl;
    std::cout << "====================" << std::endl;
    std::cout << std::endl;

    try {
        simple_factory_usage();
        requirements_based_selection();
        optimal_selection();
        compile_time_selection();
        practical_use_cases();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "All examples completed successfully!" << std::endl;

    return 0;
}
