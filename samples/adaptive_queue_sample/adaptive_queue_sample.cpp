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

#include "../../sources/thread_base/adaptive_job_queue.h"
#include "../../sources/thread_base/callback_job.h"
#include "../../sources/logger/logger.h"

#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>

using namespace thread_module;
using namespace log_module;
using namespace std::chrono_literals;

// Example 1: Basic queue strategies comparison
void strategy_comparison_example()
{
    logger::handle().log(log_types::information, "[Example 1] Queue Strategy Comparison");
    
    const int num_jobs = 10000;
    const int num_producers = 4;
    const int num_consumers = 4;
    
    // Test each strategy
    for (auto strategy : {adaptive_job_queue::queue_strategy::MUTEX_BASED,
                         adaptive_job_queue::queue_strategy::LOCK_FREE,
                         adaptive_job_queue::queue_strategy::ADAPTIVE})
    {
        adaptive_job_queue queue(strategy);
        std::atomic<int> produced{0};
        std::atomic<int> consumed{0};
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> producers;
        std::vector<std::thread> consumers;
        
        // Start producers
        for (int p = 0; p < num_producers; ++p) {
            producers.emplace_back([&queue, &produced, p, num_jobs]() {
                for (int i = 0; i < num_jobs / num_producers; ++i) {
                    auto job = std::make_unique<callback_job>(
                        [p, i]() -> std::optional<std::string> {
                            return std::nullopt;
                        });
                    
                    while (!queue.enqueue(std::move(job))) {
                        std::this_thread::yield();
                    }
                    produced.fetch_add(1);
                }
            });
        }
        
        // Start consumers
        for (int c = 0; c < num_consumers; ++c) {
            consumers.emplace_back([&queue, &consumed, num_jobs]() {
                while (consumed.load() < num_jobs) {
                    auto result = queue.dequeue();
                    if (result.has_value()) {
                        auto& job = result.value();
                        job->do_work();
                        consumed.fetch_add(1);
                    } else {
                        std::this_thread::yield();
                    }
                }
            });
        }
        
        // Wait for completion
        for (auto& t : producers) t.join();
        for (auto& t : consumers) t.join();
        
        auto duration = std::chrono::high_resolution_clock::now() - start;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        
        std::string strategy_name;
        switch (strategy) {
            case adaptive_job_queue::queue_strategy::MUTEX_BASED:
                strategy_name = "Mutex-based";
                break;
            case adaptive_job_queue::queue_strategy::LOCK_FREE:
                strategy_name = "Lock-free";
                break;
            case adaptive_job_queue::queue_strategy::ADAPTIVE:
                strategy_name = "Adaptive";
                break;
        }
        
        logger::handle().log(log_types::information, 
            formatter::format("{} strategy: {} jobs in {} ms = {} ops/sec",
                strategy_name, num_jobs, ms, 
                num_jobs * 1000.0 / ms));
    }
}

// Example 2: Adaptive strategy behavior under varying contention
void adaptive_behavior_example()
{
    logger::handle().log(log_types::information, "\n[Example 2] Adaptive Strategy Behavior");
    
    adaptive_job_queue queue(adaptive_job_queue::queue_strategy::ADAPTIVE);
    
    // Low contention phase (1 producer, 1 consumer)
    logger::handle().log(log_types::information, "Phase 1: Low contention (1P-1C)");
    {
        std::atomic<bool> running{true};
        std::atomic<int> jobs_processed{0};
        
        std::thread producer([&queue, &running]() {
            while (running) {
                auto job = std::make_unique<callback_job>(
                    []() -> std::optional<std::string> { return std::nullopt; });
                queue.enqueue(std::move(job));
                std::this_thread::sleep_for(1ms);
            }
        });
        
        std::thread consumer([&queue, &running, &jobs_processed]() {
            while (running) {
                auto result = queue.dequeue();
                if (result.has_value()) {
                    result.value()->do_work();
                    jobs_processed.fetch_add(1);
                }
                std::this_thread::sleep_for(1ms);
            }
        });
        
        std::this_thread::sleep_for(2s);
        running = false;
        producer.join();
        consumer.join();
        
        auto strategy = queue.get_current_strategy();
        logger::handle().log(log_types::information, 
            formatter::format("  Current strategy: {}, Jobs processed: {}",
                strategy == adaptive_job_queue::queue_strategy::MUTEX_BASED ? "Mutex-based" : "Lock-free",
                jobs_processed.load()));
    }
    
    // High contention phase (8 producers, 8 consumers)
    logger::handle().log(log_types::information, "Phase 2: High contention (8P-8C)");
    {
        std::atomic<bool> running{true};
        std::atomic<int> jobs_processed{0};
        std::vector<std::thread> threads;
        
        // Start producers
        for (int i = 0; i < 8; ++i) {
            threads.emplace_back([&queue, &running]() {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dist(0, 100);
                
                while (running) {
                    auto job = std::make_unique<callback_job>(
                        []() -> std::optional<std::string> { return std::nullopt; });
                    queue.enqueue(std::move(job));
                    if (dist(gen) < 10) {  // 10% chance of sleep
                        std::this_thread::sleep_for(std::chrono::microseconds(dist(gen)));
                    }
                }
            });
        }
        
        // Start consumers
        for (int i = 0; i < 8; ++i) {
            threads.emplace_back([&queue, &running, &jobs_processed]() {
                while (running) {
                    auto result = queue.dequeue();
                    if (result.has_value()) {
                        result.value()->do_work();
                        jobs_processed.fetch_add(1);
                    }
                }
            });
        }
        
        std::this_thread::sleep_for(2s);
        running = false;
        for (auto& t : threads) t.join();
        
        auto strategy = queue.get_current_strategy();
        logger::handle().log(log_types::information, 
            formatter::format("  Current strategy: {}, Jobs processed: {}",
                strategy == adaptive_job_queue::queue_strategy::MUTEX_BASED ? "Mutex-based" : "Lock-free",
                jobs_processed.load()));
    }
}

// Example 3: Manual strategy switching
void manual_switching_example()
{
    logger::handle().log(log_types::information, "\n[Example 3] Manual Strategy Switching");
    
    adaptive_job_queue queue(adaptive_job_queue::queue_strategy::ADAPTIVE);
    
    // Force mutex-based strategy
    queue.set_strategy(adaptive_job_queue::queue_strategy::MUTEX_BASED);
    logger::handle().log(log_types::information, 
        formatter::format("Forced strategy: {}",
            queue.get_current_strategy() == adaptive_job_queue::queue_strategy::MUTEX_BASED ? 
            "Mutex-based" : "Lock-free"));
    
    // Perform some operations
    std::vector<std::unique_ptr<job>> jobs;
    for (int i = 0; i < 100; ++i) {
        jobs.push_back(std::make_unique<callback_job>(
            [i]() -> std::optional<std::string> {
                return formatter::format("Job {}", i);
            }));
    }
    
    auto enqueue_result = queue.enqueue_batch(std::move(jobs));
    if (enqueue_result) {
        logger::handle().log(log_types::information, "Batch enqueue successful");
    }
    
    // Force lock-free strategy
    queue.set_strategy(adaptive_job_queue::queue_strategy::LOCK_FREE);
    logger::handle().log(log_types::information, 
        formatter::format("Forced strategy: {}",
            queue.get_current_strategy() == adaptive_job_queue::queue_strategy::MUTEX_BASED ? 
            "Mutex-based" : "Lock-free"));
    
    // Dequeue jobs
    auto dequeued = queue.dequeue_batch();
    logger::handle().log(log_types::information, 
        formatter::format("Dequeued {} jobs", dequeued.size()));
    
    // Process dequeued jobs
    for (auto& job : dequeued) {
        auto result = job->do_work();
        if (result.has_value()) {
            logger::handle().log(log_types::debug, result.value());
        }
    }
}

// Example 4: Performance monitoring
void performance_monitoring_example()
{
    logger::handle().log(log_types::information, "\n[Example 4] Performance Monitoring");
    
    adaptive_job_queue queue(adaptive_job_queue::queue_strategy::ADAPTIVE);
    
    const int num_operations = 50000;
    std::atomic<bool> running{true};
    std::atomic<int> enqueued{0};
    std::atomic<int> dequeued{0};
    
    // Producer thread
    std::thread producer([&queue, &running, &enqueued, num_operations]() {
        for (int i = 0; i < num_operations; ++i) {
            auto job = std::make_unique<callback_job>(
                []() -> std::optional<std::string> { return std::nullopt; });
            
            while (!queue.enqueue(std::move(job))) {
                std::this_thread::yield();
            }
            enqueued.fetch_add(1);
        }
    });
    
    // Consumer thread
    std::thread consumer([&queue, &running, &dequeued, num_operations]() {
        while (dequeued.load() < num_operations) {
            auto result = queue.dequeue();
            if (result.has_value()) {
                result.value()->do_work();
                dequeued.fetch_add(1);
            }
        }
    });
    
    // Monitor thread
    std::thread monitor([&queue, &running, &enqueued, &dequeued, num_operations]() {
        auto start = std::chrono::steady_clock::now();
        
        while (dequeued.load() < num_operations) {
            std::this_thread::sleep_for(500ms);
            
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration<double>(now - start).count();
            
            auto current_strategy = queue.get_current_strategy();
            std::string strategy_name = current_strategy == adaptive_job_queue::queue_strategy::MUTEX_BASED ?
                "Mutex-based" : "Lock-free";
            
            logger::handle().log(log_types::information,
                formatter::format("Status: {} strategy, Enqueued: {}, Dequeued: {}, Rate: {:.0f} ops/sec",
                    strategy_name, enqueued.load(), dequeued.load(),
                    dequeued.load() / elapsed));
        }
    });
    
    producer.join();
    consumer.join();
    running = false;
    monitor.join();
    
    logger::handle().log(log_types::information, 
        formatter::format("Completed {} operations", num_operations));
}

// Example 5: Real-world scenario - Web server simulation
void web_server_simulation()
{
    logger::handle().log(log_types::information, "\n[Example 5] Web Server Simulation");
    
    adaptive_job_queue request_queue(adaptive_job_queue::queue_strategy::ADAPTIVE);
    std::atomic<bool> server_running{true};
    std::atomic<int> requests_handled{0};
    std::atomic<int> requests_failed{0};
    
    // Request types
    enum class request_type { GET, POST, PUT, DELETE };
    
    // Simulate incoming requests
    std::vector<std::thread> clients;
    for (int client_id = 0; client_id < 5; ++client_id) {
        clients.emplace_back([&request_queue, &server_running, client_id]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> type_dist(0, 3);
            std::uniform_int_distribution<> delay_dist(10, 100);
            
            while (server_running) {
                auto type = static_cast<request_type>(type_dist(gen));
                
                auto request = std::make_unique<callback_job>(
                    [client_id, type]() -> std::optional<std::string> {
                        // Simulate request processing
                        std::this_thread::sleep_for(std::chrono::microseconds(
                            type == request_type::GET ? 10 : 50));
                        
                        std::string type_str;
                        switch (type) {
                            case request_type::GET: type_str = "GET"; break;
                            case request_type::POST: type_str = "POST"; break;
                            case request_type::PUT: type_str = "PUT"; break;
                            case request_type::DELETE: type_str = "DELETE"; break;
                        }
                        
                        return formatter::format("Client {} {} request completed", 
                            client_id, type_str);
                    });
                
                if (!request_queue.enqueue(std::move(request))) {
                    requests_failed.fetch_add(1);
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));
            }
        });
    }
    
    // Worker threads (server handlers)
    std::vector<std::thread> workers;
    for (int worker_id = 0; worker_id < 3; ++worker_id) {
        workers.emplace_back([&request_queue, &server_running, &requests_handled, worker_id]() {
            while (server_running) {
                auto request = request_queue.dequeue();
                if (request.has_value()) {
                    auto result = request.value()->do_work();
                    if (result.has_value()) {
                        logger::handle().log(log_types::debug, 
                            formatter::format("Worker {}: {}", worker_id, result.value()));
                    }
                    requests_handled.fetch_add(1);
                } else {
                    std::this_thread::sleep_for(1ms);
                }
            }
        });
    }
    
    // Run simulation for 5 seconds
    std::this_thread::sleep_for(5s);
    server_running = false;
    
    // Cleanup
    for (auto& t : clients) t.join();
    for (auto& t : workers) t.join();
    
    logger::handle().log(log_types::information,
        formatter::format("Server simulation complete: {} requests handled, {} failed",
            requests_handled.load(), requests_failed.load()));
}

int main()
{
    logger::handle().start();
    logger::handle().set_log_level(log_types::debug);
    
    logger::handle().log(log_types::information, 
        "Adaptive Job Queue Sample\n"
        "=========================");
    
    try {
        strategy_comparison_example();
        adaptive_behavior_example();
        manual_switching_example();
        performance_monitoring_example();
        web_server_simulation();
    } catch (const std::exception& e) {
        logger::handle().log(log_types::error, 
            formatter::format("Exception: {}", e.what()));
    }
    
    logger::handle().log(log_types::information, "\nAll examples completed!");
    
    logger::handle().stop();
    return 0;
}