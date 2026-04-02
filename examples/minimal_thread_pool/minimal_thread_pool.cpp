// BSD 3-Clause License
// Copyright (c) 2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/thread_worker.h>
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/interfaces/thread_context.h>

using namespace kcenon::thread;
using namespace kcenon::thread;

int main() {
    std::cout << "=== Minimal Thread Pool Sample (No Logger) ===" << std::endl;
    
    // Create thread pool
    thread_context context;
    auto pool = std::make_shared<thread_pool>("MinimalPool", context);
    
    // Create workers
    const size_t worker_count = 4;
    std::vector<std::unique_ptr<thread_worker>> workers;
    
    for (size_t i = 0; i < worker_count; ++i) {
        workers.push_back(std::make_unique<thread_worker>(false, context));
    }
    
    // Add workers to pool
    auto result = pool->enqueue_batch(std::move(workers));
    if (result.is_err()) {
        std::cerr << "Error adding workers: " << result.error().message << std::endl;
        return 1;
    }

    // Start the pool
    result = pool->start();
    if (result.is_err()) {
        std::cerr << "Error starting pool: " << result.error().message << std::endl;
        return 1;
    }
    
    std::cout << "Thread pool started with " << worker_count << " workers" << std::endl;
    
    // Submit some jobs
    std::atomic<int> completed_jobs{0};
    const int total_jobs = 20;
    
    std::cout << "Submitting " << total_jobs << " jobs..." << std::endl;
    
    for (int i = 0; i < total_jobs; ++i) {
        auto job = std::make_unique<callback_job>(
            [i, &completed_jobs, total_jobs]() -> kcenon::common::VoidResult {
                // Simulate some work
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                // Print progress (thread-safe)
                int current = completed_jobs.fetch_add(1) + 1;
                std::cout << "Job " << i << " completed. Total: "
                         << current << "/" << total_jobs << std::endl;

                return kcenon::common::ok();
            },
            "job_" + std::to_string(i)
        );

        result = pool->enqueue(std::move(job));
        if (result.is_err()) {
            std::cerr << "Error enqueuing job: " << result.error().message << std::endl;
        }
    }
    
    // Wait for all jobs to complete
    std::cout << "Waiting for jobs to complete..." << std::endl;
    while (completed_jobs.load() < total_jobs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "All jobs completed!" << std::endl;
    
    // Stop the pool
    auto stop_result = pool->stop();
    if (stop_result.is_err()) {
        std::cerr << "Error stopping pool: " << stop_result.error().message << std::endl;
    }
    std::cout << "Thread pool stopped." << std::endl;
    
    return 0;
}
