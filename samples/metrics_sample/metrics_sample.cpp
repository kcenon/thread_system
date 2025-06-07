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
#include <thread>
#include <random>
#include <atomic>
#include <iomanip>

#include "formatter.h"
#include "logger.h"
#include "thread_pool/monitored_thread_pool.h"
#include "typed_thread_pool/monitored_typed_thread_pool.h"
#include "metrics/metric_registry.h"

using job_types = typed_thread_pool_module::job_types;

using namespace std::chrono_literals;

/**
 * @brief Simulate CPU-intensive work
 * @param duration_ms Duration in milliseconds
 */
void simulate_work(int duration_ms)
{
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(duration_ms))
    {
        // Busy wait to simulate CPU load
        volatile int dummy = 0;
        for (int i = 0; i < 1000; ++i)
        {
            dummy += i;
        }
    }
}

/**
 * @brief Display metrics in a formatted table
 */
void display_metrics(const std::string& title, const json& metrics)
{
    std::cout << "\n=== " << title << " ===" << std::endl;
    
    // Jobs summary
    if (metrics.contains("jobs"))
    {
        std::cout << "\nJob Statistics:" << std::endl;
        std::cout << std::setw(20) << "Submitted:" 
                  << metrics["jobs"]["submitted"]["value"] << std::endl;
        std::cout << std::setw(20) << "Completed:" 
                  << metrics["jobs"]["completed"]["value"] << std::endl;
        std::cout << std::setw(20) << "Failed:" 
                  << metrics["jobs"]["failed"]["value"] << std::endl;
        std::cout << std::setw(20) << "Rejected:" 
                  << metrics["jobs"]["rejected"]["value"] << std::endl;
    }
    
    // Queue statistics
    if (metrics.contains("queue"))
    {
        std::cout << "\nQueue Statistics:" << std::endl;
        std::cout << std::setw(20) << "Current Depth:" 
                  << metrics["queue"]["depth"]["value"] << std::endl;
        
        auto& wait_time = metrics["queue"]["wait_time"];
        std::cout << std::setw(20) << "Wait Time P50:" 
                  << std::fixed << std::setprecision(3) 
                  << wait_time["p50"].get<double>() * 1000 << " ms" << std::endl;
        std::cout << std::setw(20) << "Wait Time P99:" 
                  << wait_time["p99"].get<double>() * 1000 << " ms" << std::endl;
    }
    
    // Worker statistics
    if (metrics.contains("workers"))
    {
        std::cout << "\nWorker Statistics:" << std::endl;
        std::cout << std::setw(20) << "Total Workers:" 
                  << metrics["workers"]["total"]["value"] << std::endl;
        std::cout << std::setw(20) << "Active Workers:" 
                  << metrics["workers"]["active"]["value"] << std::endl;
        std::cout << std::setw(20) << "Idle Workers:" 
                  << metrics["workers"]["idle"]["value"] << std::endl;
    }
    
    // Performance metrics
    if (metrics.contains("performance"))
    {
        std::cout << "\nPerformance Metrics:" << std::endl;
        
        auto& duration = metrics["performance"]["job_duration"];
        std::cout << std::setw(20) << "Job Duration P50:" 
                  << std::fixed << std::setprecision(3) 
                  << duration["p50"].get<double>() * 1000 << " ms" << std::endl;
        std::cout << std::setw(20) << "Job Duration P99:" 
                  << duration["p99"].get<double>() * 1000 << " ms" << std::endl;
        
        if (metrics["performance"].contains("throughput"))
        {
            auto& throughput = metrics["performance"]["throughput"];
            if (throughput.contains("mean"))
            {
                std::cout << std::setw(20) << "Throughput:" 
                          << std::fixed << std::setprecision(1) 
                          << throughput["mean"].get<double>() << " jobs/sec" << std::endl;
            }
        }
    }
}

/**
 * @brief Demonstrate basic thread pool with metrics
 */
void demo_monitored_thread_pool()
{
    std::cout << "\n============================================" << std::endl;
    std::cout << "Demo: Monitored Thread Pool" << std::endl;
    std::cout << "============================================" << std::endl;
    
    // Create a monitored thread pool with 4 workers
    auto pool = thread_pool_module::make_monitored_thread_pool(4, "demo_pool");
    
    if (auto error = pool->start())
    {
        std::cerr << "Failed to start pool: " << *error << std::endl;
        return;
    }
    
    std::cout << "Thread pool started with 4 workers" << std::endl;
    
    // Random number generator for varying work loads
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> work_dist(5, 50);  // 5-50ms work
    std::uniform_int_distribution<> batch_dist(10, 30); // batch sizes
    
    // Submit jobs in batches
    std::atomic<int> completed_jobs{0};
    int total_jobs = 0;
    
    for (int batch = 0; batch < 5; ++batch)
    {
        int batch_size = batch_dist(gen);
        total_jobs += batch_size;
        
        std::cout << "\nSubmitting batch " << (batch + 1) 
                  << " with " << batch_size << " jobs..." << std::endl;
        
        for (int i = 0; i < batch_size; ++i)
        {
            int work_ms = work_dist(gen);
            
            auto job = std::make_unique<thread_module::callback_job>(
                [work_ms, &completed_jobs]() -> std::optional<std::string> {
                    simulate_work(work_ms);
                    completed_jobs.fetch_add(1);
                    return std::nullopt;
                }
            );
            
            if (auto error = pool->enqueue(std::move(job)))
            {
                std::cerr << "Failed to enqueue job: " << *error << std::endl;
            }
        }
        
        // Update stats after each batch
        pool->update_stats();
        
        // Give some time for processing
        std::this_thread::sleep_for(100ms);
    }
    
    // Wait for all jobs to complete
    std::cout << "\nWaiting for jobs to complete..." << std::endl;
    while (completed_jobs.load() < total_jobs)
    {
        std::this_thread::sleep_for(50ms);
        pool->update_stats();
    }
    
    // Display final metrics
    if (auto metrics = pool->get_metrics())
    {
        display_metrics("Final Thread Pool Metrics", metrics->to_json());
    }
    
    pool->stop();
    std::cout << "\nThread pool stopped" << std::endl;
}

/**
 * @brief Demonstrate typed thread pool with per-type metrics
 */
void demo_monitored_typed_thread_pool()
{
    std::cout << "\n============================================" << std::endl;
    std::cout << "Demo: Monitored Typed Thread Pool" << std::endl;
    std::cout << "============================================" << std::endl;
    
    // Create a typed pool with 6 workers
    auto pool = typed_thread_pool_module::make_monitored_typed_thread_pool<typed_thread_pool_module::job_types>(
        6, "typed_demo_pool"
    );
    
    auto start_result = pool->start();
    if (start_result.has_error())
    {
        std::cerr << "Failed to start pool: " << start_result.get_error().to_string() << std::endl;
        return;
    }
    
    std::cout << "Typed thread pool started with 6 workers" << std::endl;
    std::cout << "All workers handle all job types with type-based scheduling" << std::endl;
    
    // Random generators
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> type_dist(0, 2);  // job types
    std::uniform_int_distribution<> realtime_work(1, 10);   // 1-10ms for realtime
    std::uniform_int_distribution<> normal_work(10, 50);    // 10-50ms for normal
    std::uniform_int_distribution<> background_work(50, 100); // 50-100ms for background
    
    // Counters
    std::atomic<int> completed_by_type[3] = {0, 0, 0};
    int submitted_by_type[3] = {0, 0, 0};
    
    // Submit mixed job types
    for (int wave = 0; wave < 3; ++wave)
    {
        std::cout << "\n--- Wave " << (wave + 1) << " ---" << std::endl;
        
        // Submit different distributions of job types
        int job_counts[3];
        if (wave == 0)
        {
            // First wave: mostly realtime
            job_counts[0] = 20;  // RealTime
            job_counts[1] = 10;  // Batch
            job_counts[2] = 5;   // Background
        }
        else if (wave == 1)
        {
            // Second wave: balanced
            job_counts[0] = 10;
            job_counts[1] = 15;
            job_counts[2] = 10;
        }
        else
        {
            // Third wave: mostly background
            job_counts[0] = 5;
            job_counts[1] = 10;
            job_counts[2] = 20;
        }
        
        // Submit jobs
        for (int type_idx = 0; type_idx < 3; ++type_idx)
        {
            auto job_type = static_cast<job_types>(type_idx);
            int count = job_counts[type_idx];
            submitted_by_type[type_idx] += count;
            
            std::cout << "Submitting " << count << " ";
            switch (job_type)
            {
                case job_types::RealTime: std::cout << "RealTime"; break;
                case job_types::Batch: std::cout << "Batch"; break;
                case job_types::Background: std::cout << "Background"; break;
                default: break;
            }
            std::cout << " jobs" << std::endl;
            
            for (int i = 0; i < count; ++i)
            {
                int work_ms;
                switch (job_type)
                {
                    case job_types::RealTime: work_ms = realtime_work(gen); break;
                    case job_types::Batch: work_ms = normal_work(gen); break;
                    case job_types::Background: work_ms = background_work(gen); break;
                    default: work_ms = 10;
                }
                
                auto job = std::make_unique<typed_thread_pool_module::callback_typed_job_t<job_types>>(
                    [work_ms, type_idx, &completed_by_type]() -> result_void {
                        simulate_work(work_ms);
                        completed_by_type[type_idx].fetch_add(1);
                        return result_void{};
                    },
                    job_type
                );
                
                auto result = pool->enqueue(std::move(job));
                if (result.has_error())
                {
                    std::cerr << "Failed to enqueue job: " << result.get_error().to_string() << std::endl;
                }
            }
        }
        
        // Update stats
        pool->update_stats();
        
        // Wait a bit between waves
        std::this_thread::sleep_for(200ms);
        
        // Show progress
        std::cout << "Progress: ";
        for (int i = 0; i < 3; ++i)
        {
            std::cout << completed_by_type[i].load() << "/" << submitted_by_type[i] << " ";
        }
        std::cout << std::endl;
    }
    
    // Wait for all jobs to complete
    std::cout << "\nWaiting for all jobs to complete..." << std::endl;
    int total_submitted = submitted_by_type[0] + submitted_by_type[1] + submitted_by_type[2];
    int total_completed = 0;
    
    while (total_completed < total_submitted)
    {
        total_completed = completed_by_type[0].load() + 
                         completed_by_type[1].load() + 
                         completed_by_type[2].load();
        
        std::this_thread::sleep_for(50ms);
        pool->update_stats();
    }
    
    // Display final metrics
    if (auto metrics = pool->get_metrics())
    {
        display_metrics("Final Typed Thread Pool Metrics", metrics->to_json());
    }
    
    // Display type-specific metrics
    std::cout << "\nType-Specific Metrics:" << std::endl;
    for (int i = 0; i < 3; ++i)
    {
        auto job_type = static_cast<job_types>(i);
        auto type_metrics = pool->get_type_metrics(job_type);
        
        std::cout << "\n";
        switch (job_type)
        {
            case job_types::RealTime: std::cout << "RealTime Jobs:"; break;
            case job_types::Batch: std::cout << "Batch Jobs:"; break;
            case job_types::Background: std::cout << "Background Jobs:"; break;
            default: break;
        }
        
        if (type_metrics.contains("submitted"))
        {
            std::cout << "\n  Submitted: " << type_metrics["submitted"];
        }
        if (type_metrics.contains("latency"))
        {
            auto& latency = type_metrics["latency"];
            std::cout << "\n  Latency P50: " << std::fixed << std::setprecision(3) 
                      << latency["p50"].get<double>() * 1000 << " ms";
            std::cout << "\n  Latency P99: " 
                      << latency["p99"].get<double>() * 1000 << " ms";
        }
        std::cout << std::endl;
    }
    
    auto stop_result = pool->stop();
    if (stop_result.has_error())
    {
        std::cerr << "Error stopping pool: " << stop_result.get_error().to_string() << std::endl;
    }
    std::cout << "\nTyped thread pool stopped" << std::endl;
}

/**
 * @brief Display all registered metrics
 */
void display_all_metrics()
{
    std::cout << "\n============================================" << std::endl;
    std::cout << "All Registered Metrics" << std::endl;
    std::cout << "============================================" << std::endl;
    
    auto& registry = metrics::metric_registry::instance();
    auto all_metrics = registry.collect_all();
    
    std::cout << all_metrics.dump(2) << std::endl;
}

int main()
{
    // Initialize logging
    log_module::start();
    // Set log level - commenting out as set_log_level might not exist
    // log_module::set_log_level(log_module::log_types::info);
    
    std::cout << "Thread System Metrics Demonstration" << std::endl;
    std::cout << "===================================" << std::endl;
    
    // Run demonstrations
    demo_monitored_thread_pool();
    demo_monitored_typed_thread_pool();
    
    // Display all collected metrics
    display_all_metrics();
    
    // Cleanup
    log_module::stop();
    
    std::cout << "\nMetrics demonstration completed!" << std::endl;
    
    return 0;
}