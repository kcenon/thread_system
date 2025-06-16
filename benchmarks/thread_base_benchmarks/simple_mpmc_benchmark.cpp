/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.

Simple MPMC Performance Comparison
*****************************************************************************/

#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <iomanip>
#include "../../sources/thread_base/jobs/job_queue.h"
#include "../../sources/thread_base/lockfree/queues/lockfree_mpmc_queue.h"
#include "../../sources/thread_base/lockfree/queues/adaptive_job_queue.h"
#include "../../sources/thread_base/jobs/callback_job.h"

using namespace thread_module;
using namespace std::chrono;

struct TestResult {
    std::string name;
    double throughput;
    double latency_ns;
    double cpu_efficiency;
};

template<typename QueueType>
TestResult run_test(const std::string& name, size_t num_producers, size_t num_consumers, size_t operations_per_thread) {
    QueueType queue;
    std::atomic<size_t> produced{0};
    std::atomic<size_t> consumed{0};
    std::atomic<size_t> counter{0};
    
    auto start = high_resolution_clock::now();
    
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    
    // Start producers
    for (size_t i = 0; i < num_producers; ++i) {
        producers.emplace_back([&]() {
            for (size_t j = 0; j < operations_per_thread; ++j) {
                auto job = std::make_unique<callback_job>([&counter]() -> result_void {
                    counter.fetch_add(1, std::memory_order_relaxed);
                    return result_void();
                });
                queue.enqueue(std::move(job));
                produced.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    
    // Start consumers
    for (size_t i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&]() {
            size_t total_operations = num_producers * operations_per_thread;
            while (consumed.load(std::memory_order_relaxed) < total_operations) {
                auto result = queue.dequeue();
                if (result.has_value()) {
                    result.value()->do_work();
                    consumed.fetch_add(1, std::memory_order_relaxed);
                } else if (produced.load(std::memory_order_relaxed) >= total_operations) {
                    // All items produced, check if we're done
                    if (consumed.load(std::memory_order_relaxed) >= total_operations) {
                        break;
                    }
                    std::this_thread::yield();
                }
            }
        });
    }
    
    // Wait for completion
    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<nanoseconds>(end - start);
    
    size_t total_operations = num_producers * operations_per_thread;
    double throughput = static_cast<double>(total_operations) / duration.count() * 1e9;
    double latency = static_cast<double>(duration.count()) / total_operations;
    double cpu_efficiency = static_cast<double>(counter.load()) / total_operations * 100.0;
    
    return TestResult{name, throughput, latency, cpu_efficiency};
}

void print_results(const std::vector<TestResult>& results) {
    std::cout << std::left;
    std::cout << std::setw(25) << "Queue Type" 
              << std::setw(20) << "Throughput (ops/s)" 
              << std::setw(20) << "Latency (ns/op)"
              << std::setw(20) << "CPU Efficiency (%)" << std::endl;
    std::cout << std::string(85, '-') << std::endl;
    
    for (const auto& result : results) {
        std::cout << std::setw(25) << result.name
                  << std::setw(20) << std::fixed << std::setprecision(2) << result.throughput
                  << std::setw(20) << std::fixed << std::setprecision(2) << result.latency_ns
                  << std::setw(20) << std::fixed << std::setprecision(2) << result.cpu_efficiency
                  << std::endl;
    }
}

int main() {
    std::cout << "MPMC Queue Performance Comparison\n";
    std::cout << "==================================\n\n";
    
    // Test configurations
    struct TestConfig {
        std::string name;
        size_t producers;
        size_t consumers;
        size_t ops_per_thread;
    };
    
    std::vector<TestConfig> configs = {
        {"SPSC (1P-1C)", 1, 1, 10000},
        {"Low Contention (2P-2C)", 2, 2, 5000},
        {"Medium Contention (4P-4C)", 4, 4, 2500},
        {"High Contention (8P-8C)", 8, 8, 1250}
    };
    
    for (const auto& config : configs) {
        std::cout << "\nTest: " << config.name << " - " 
                  << config.ops_per_thread * config.producers << " total operations\n";
        std::cout << std::string(85, '=') << std::endl;
        
        std::vector<TestResult> results;
        
        // Test mutex-based queue
        results.push_back(run_test<job_queue>(
            "Mutex-based Queue", 
            config.producers, 
            config.consumers, 
            config.ops_per_thread
        ));
        
        // Test lock-free MPMC queue
        results.push_back(run_test<lockfree_mpmc_queue>(
            "Lock-free MPMC Queue", 
            config.producers, 
            config.consumers, 
            config.ops_per_thread
        ));
        
        // Test adaptive queue
        results.push_back(run_test<adaptive_job_queue>(
            "Adaptive Queue", 
            config.producers, 
            config.consumers, 
            config.ops_per_thread
        ));
        
        print_results(results);
        
        // Calculate improvement
        if (results.size() >= 2) {
            double improvement = (results[1].throughput / results[0].throughput - 1) * 100;
            std::cout << "\nLock-free improvement over mutex-based: " 
                      << std::fixed << std::setprecision(1) << improvement << "%\n";
        }
    }
    
    std::cout << "\n\nNote: Results may vary based on system load and CPU characteristics.\n";
    
    return 0;
}