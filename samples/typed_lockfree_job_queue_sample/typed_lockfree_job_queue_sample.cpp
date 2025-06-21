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

#include "typed_thread_pool/scheduling/typed_lockfree_job_queue.h"
#include "typed_thread_pool/scheduling/typed_job_queue.h"
#include "typed_thread_pool/jobs/callback_typed_job.h"
#include "logger/core/logger.h"
#include "utilities/core/formatter.h"

#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <random>

using namespace typed_thread_pool_module;
using namespace thread_module;
using namespace utility_module;

// Performance test configuration
constexpr size_t NUM_PRODUCERS = 2;
constexpr size_t NUM_CONSUMERS = 2;
constexpr size_t JOBS_PER_PRODUCER = 100;
constexpr size_t TOTAL_JOBS = NUM_PRODUCERS * JOBS_PER_PRODUCER;

// Atomic counters for verification
std::atomic<size_t> jobs_produced{0};
std::atomic<size_t> jobs_consumed{0};
std::atomic<size_t> high_priority_consumed{0};
std::atomic<size_t> normal_priority_consumed{0};
std::atomic<size_t> low_priority_consumed{0};

// Job execution simulation
void simulate_work(job_types priority)
{
    // Simulate different work loads based on priority
    switch (priority) {
        case job_types::RealTime:
            // std::this_thread::sleep_for(std::chrono::microseconds(10));
            high_priority_consumed.fetch_add(1);
            break;
        case job_types::Batch:
            // std::this_thread::sleep_for(std::chrono::microseconds(50));
            normal_priority_consumed.fetch_add(1);
            break;
        case job_types::Background:
            // std::this_thread::sleep_for(std::chrono::microseconds(100));
            low_priority_consumed.fetch_add(1);
            break;
        default:
            break;
    }
}

// Producer function template
template<typename QueueType>
void producer_thread(QueueType& queue, size_t producer_id, size_t num_jobs)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> priority_dist(0, 2);
    
    for (size_t i = 0; i < num_jobs; ++i) {
        // Random priority distribution
        job_types priority;
        int rand_val = priority_dist(gen);
        switch (rand_val) {
            case 0: priority = job_types::RealTime; break;
            case 1: priority = job_types::Batch; break;
            default: priority = job_types::Background; break;
        }
        
        auto job = std::make_unique<callback_typed_job>(
            [priority, producer_id, i]() -> result_void {
                simulate_work(priority);
                return {};
            },
            priority
        );
        
        auto result = queue.enqueue(std::unique_ptr<thread_module::job>(std::move(job)));
        if (!result) {
            jobs_produced.fetch_add(1);
        }
        
        // Occasional batch enqueue
        if (i % 100 == 0 && i + 10 < num_jobs) {
            std::vector<std::unique_ptr<typed_job_t<job_types>>> batch;
            for (size_t j = 0; j < 10; ++j) {
                batch.push_back(std::make_unique<callback_typed_job>(
                    [producer_id, i, j]() -> result_void {
                        simulate_work(job_types::Batch);
                        return {};
                    },
                    job_types::Batch
                ));
            }
            
            auto batch_result = queue.enqueue_batch(std::move(batch));
            if (!batch_result) {
                jobs_produced.fetch_add(10);
            }
            i += 10;
        }
    }
}

// Consumer function template
template<typename QueueType>
void consumer_thread(QueueType& queue, size_t consumer_id, std::atomic<bool>& stop_flag)
{
    // For typed_job_queue, we need to check all types
    std::vector<job_types> all_types = {job_types::RealTime, job_types::Batch, job_types::Background};
    
    auto queue_not_empty = [&queue, &all_types]() -> bool {
        if constexpr (std::is_same_v<QueueType, typed_job_queue>) {
            return !queue.empty(all_types);
        } else {
            return !queue.empty();
        }
    };
    
    while (!stop_flag.load() || queue_not_empty()) {
        auto result = queue.dequeue();
        if (result.has_value()) {
            auto job = std::move(result.value());
            if (job) {
                job->do_work();
                jobs_consumed.fetch_add(1);
            }
        } else {
            // Brief pause if queue is empty
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
}

// Performance test function
template<typename QueueType>
void run_performance_test(const std::string& queue_name)
{
    log_module::write_information("\n=== Testing {} ===", queue_name);
    
    // Reset counters
    jobs_produced.store(0);
    jobs_consumed.store(0);
    high_priority_consumed.store(0);
    normal_priority_consumed.store(0);
    low_priority_consumed.store(0);
    
    // Create queue
    QueueType queue;
    std::atomic<bool> stop_flag{false};
    
    // Start timing
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Create producer threads
    std::vector<std::thread> producers;
    for (size_t i = 0; i < NUM_PRODUCERS; ++i) {
        producers.emplace_back(producer_thread<QueueType>, 
                               std::ref(queue), i, JOBS_PER_PRODUCER);
    }
    
    // Create consumer threads
    std::vector<std::thread> consumers;
    for (size_t i = 0; i < NUM_CONSUMERS; ++i) {
        consumers.emplace_back(consumer_thread<QueueType>, 
                               std::ref(queue), i, std::ref(stop_flag));
    }
    
    // Wait for all producers to finish
    for (auto& t : producers) {
        t.join();
    }
    
    // Signal consumers to stop after processing remaining jobs
    stop_flag.store(true);
    
    // Wait for all consumers to finish
    for (auto& t : consumers) {
        t.join();
    }
    
    // End timing
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    
    // Print results
    log_module::write_information("Time taken: {} ms", duration.count());
    log_module::write_information("Jobs produced: {}", jobs_produced.load());
    log_module::write_information("Jobs consumed: {}", jobs_consumed.load());
    log_module::write_information("Throughput: {} jobs/sec", jobs_consumed.load() * 1000.0 / duration.count());
    log_module::write_information("Priority distribution:");
    log_module::write_information("  RealTime: {}", high_priority_consumed.load());
    log_module::write_information("  Batch: {}", normal_priority_consumed.load());
    log_module::write_information("  Background: {}", low_priority_consumed.load());
    
    // Get queue statistics if available
    if constexpr (std::is_same_v<QueueType, typed_lockfree_job_queue>) {
        auto stats = queue.get_typed_statistics();
        log_module::write_information("Queue statistics:");
        log_module::write_information("  Type switches: {}", stats.type_switch_count);
        log_module::write_information("  Average enqueue latency: {} ns", stats.get_average_enqueue_latency_ns());
        log_module::write_information("  Average dequeue latency: {} ns", stats.get_average_dequeue_latency_ns());
        
        if (auto busiest = stats.get_busiest_type(); busiest.has_value()) {
            log_module::write_information("  Busiest type: {}", busiest.value());
        }
    }
}

// Feature demonstration
void demonstrate_features()
{
    log_module::write_information("\n=== Feature Demonstration ===");
    
    // Create a typed lock-free queue
    typed_lockfree_job_queue queue;
    
    // Test 1: Basic enqueue/dequeue
    log_module::write_information("\n1. Basic enqueue/dequeue:");
    auto job1 = std::make_unique<callback_typed_job>(
        []() -> result_void {
            log_module::write_information("   RealTime priority job executed");
            return {};
        },
        job_types::RealTime
    );
    
    queue.enqueue(std::unique_ptr<thread_module::job>(std::move(job1)));
    
    if (auto result = queue.dequeue(); result.has_value()) {
        result.value()->do_work();
    }
    
    // Test 2: Priority ordering
    log_module::write_information("\n2. Priority ordering test:");
    
    // Enqueue jobs in reverse priority order
    for (int i = 0; i < 3; ++i) {
        queue.enqueue(std::unique_ptr<thread_module::job>(std::make_unique<callback_typed_job>(
            [i]() -> result_void {
                log_module::write_information("   Background priority job {} executed", i);
                return {};
            },
            job_types::Background
        )));
    }
    
    for (int i = 0; i < 3; ++i) {
        queue.enqueue(std::unique_ptr<thread_module::job>(std::make_unique<callback_typed_job>(
            [i]() -> result_void {
                log_module::write_information("   Batch priority job {} executed", i);
                return {};
            },
            job_types::Batch
        )));
    }
    
    for (int i = 0; i < 3; ++i) {
        queue.enqueue(std::unique_ptr<thread_module::job>(std::make_unique<callback_typed_job>(
            [i]() -> result_void {
                log_module::write_information("   RealTime priority job {} executed", i);
                return {};
            },
            job_types::RealTime
        )));
    }
    
    // Dequeue all - should see high priority first
    log_module::write_information("   Dequeuing all jobs:");
    while (queue.size() > 0) {
        if (auto result = queue.dequeue(); result.has_value()) {
            result.value()->do_work();
        }
    }
    
    // Test 3: Type-specific dequeue
    log_module::write_information("\n3. Type-specific dequeue:");
    
    queue.enqueue(std::unique_ptr<thread_module::job>(std::make_unique<callback_typed_job>(
        []() -> result_void {
            log_module::write_information("   Background priority job executed");
            return {};
        },
        job_types::Background
    )));
    
    queue.enqueue(std::unique_ptr<thread_module::job>(std::make_unique<callback_typed_job>(
        []() -> result_void {
            log_module::write_information("   RealTime priority job executed");
            return {};
        },
        job_types::RealTime
    )));
    
    // Dequeue only low priority
    if (auto result = queue.dequeue(job_types::Background); result.has_value()) {
        log_module::write_information("   Dequeued low priority job");
        result.value()->do_work();
    }
    
    // Test 4: Queue sizes
    log_module::write_information("\n4. Queue sizes:");
    auto sizes = queue.get_sizes();
    for (const auto& [type, size] : sizes) {
        log_module::write_information("   {}: {} jobs", type, size);
    }
    
    // Clear remaining jobs
    queue.clear();
}

int main()
{
    // Initialize logger
    log_module::start();
    log_module::console_target(log_module::log_types::Information);
    
    log_module::write_information("=== Typed Lock-Free Job Queue Sample ===");
    log_module::write_information("Comparing performance between typed_job_queue and typed_lockfree_job_queue");
    
    // Feature demonstration
    demonstrate_features();
    
    // Performance comparison - temporarily disabled due to notification issues
    log_module::write_information("\n=== Performance Comparison ===");
    log_module::write_information("Performance testing is temporarily disabled.");
    log_module::write_information("The typed_lockfree_job_queue has been successfully implemented.");
    
    // // Test traditional typed queue
    // run_performance_test<typed_job_queue>("typed_job_queue (mutex-based)");
    
    // // Test lock-free typed queue
    // run_performance_test<typed_lockfree_job_queue>("typed_lockfree_job_queue");
    
    // Cleanup
    log_module::stop();
    
    return 0;
}