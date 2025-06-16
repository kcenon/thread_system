#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>

#include "logger/core/logger.h"
#include "utilities/core/formatter.h"

using namespace std::chrono;
using namespace utility_module;

void run_throughput_test() {
    std::cout << "\n=== Logger Throughput Test (adaptive_job_queue) ===\n";
    
    // Configure logger
    log_module::stop();
    log_module::set_title("perf_test");
    log_module::file_target(log_module::log_types::Information);
    log_module::console_target(log_module::log_types::None);
    log_module::callback_target(log_module::log_types::None);
    
    auto start_result = log_module::start();
    if (start_result.has_value()) {
        std::cout << "Failed to start logger: " << start_result.value() << std::endl;
        return;
    }
    
    const size_t num_messages = 100000;
    
    auto start = high_resolution_clock::now();
    
    for (size_t i = 0; i < num_messages; ++i) {
        log_module::write_information("Performance test message {}: adaptive_job_queue enabled", i);
    }
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::seconds(2));
    log_module::stop();
    
    auto end = high_resolution_clock::now();
    double elapsed_ms = duration_cast<milliseconds>(end - start).count();
    double throughput = (num_messages * 1000.0) / elapsed_ms;
    
    std::cout << "Messages: " << num_messages << std::endl;
    std::cout << "Time: " << elapsed_ms << " ms" << std::endl;
    std::cout << "Throughput: " << static_cast<int>(throughput) << " msg/s" << std::endl;
}

void run_concurrent_test() {
    std::cout << "\n=== Concurrent Logging Test ===\n";
    
    std::vector<size_t> thread_counts = {1, 2, 4, 8};
    
    for (size_t num_threads : thread_counts) {
        log_module::stop();
        log_module::set_title("concurrent_test");
        log_module::file_target(log_module::log_types::Information);
        log_module::console_target(log_module::log_types::None);
        log_module::callback_target(log_module::log_types::None);
        
        auto start_result = log_module::start();
        if (start_result.has_value()) {
            std::cout << "Failed to start logger: " << start_result.value() << std::endl;
            continue;
        }
        
        const size_t messages_per_thread = 10000;
        std::atomic<size_t> total_messages{0};
        
        auto start = high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        for (size_t t = 0; t < num_threads; ++t) {
            threads.emplace_back([t, messages_per_thread, &total_messages] {
                for (size_t i = 0; i < messages_per_thread; ++i) {
                    log_module::write_information("Thread {} message {}: concurrent test", t, i);
                    total_messages.fetch_add(1);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Wait for processing
        std::this_thread::sleep_for(std::chrono::seconds(1));
        log_module::stop();
        
        auto end = high_resolution_clock::now();
        double elapsed_ms = duration_cast<milliseconds>(end - start).count();
        double throughput = (total_messages.load() * 1000.0) / elapsed_ms;
        
        std::cout << num_threads << " threads: " << static_cast<int>(throughput) << " msg/s" << std::endl;
    }
}

void run_latency_test() {
    std::cout << "\n=== Latency Test ===\n";
    
    log_module::stop();
    log_module::set_title("latency_test");
    log_module::file_target(log_module::log_types::Information);
    log_module::console_target(log_module::log_types::None);
    log_module::callback_target(log_module::log_types::None);
    
    auto start_result = log_module::start();
    if (start_result.has_value()) {
        std::cout << "Failed to start logger: " << start_result.value() << std::endl;
        return;
    }
    
    std::vector<double> latencies;
    const size_t num_samples = 1000;
    
    for (size_t i = 0; i < num_samples; ++i) {
        auto start = high_resolution_clock::now();
        
        log_module::write_information("Latency test message {}", i);
        
        auto end = high_resolution_clock::now();
        double latency_us = duration_cast<microseconds>(end - start).count();
        latencies.push_back(latency_us);
        
        std::this_thread::sleep_for(microseconds(100));
    }
    
    log_module::stop();
    
    // Calculate statistics
    std::sort(latencies.begin(), latencies.end());
    
    double avg_latency = 0;
    for (double lat : latencies) {
        avg_latency += lat;
    }
    avg_latency /= latencies.size();
    
    double p50 = latencies[latencies.size() * 50 / 100];
    double p90 = latencies[latencies.size() * 90 / 100];
    double p99 = latencies[latencies.size() * 99 / 100];
    
    std::cout << "Average: " << static_cast<int>(avg_latency) << " μs" << std::endl;
    std::cout << "P50: " << static_cast<int>(p50) << " μs" << std::endl;
    std::cout << "P90: " << static_cast<int>(p90) << " μs" << std::endl;
    std::cout << "P99: " << static_cast<int>(p99) << " μs" << std::endl;
}

int main() {
    std::cout << "\n=== Logger Performance Benchmark ===\n";
    std::cout << "Using adaptive_job_queue implementation\n";
    std::cout << "Platform: Apple M1 (8-core) @ 3.2GHz\n";
    std::cout << "Compiler: Apple Clang 17.0.0\n";
    
    run_throughput_test();
    run_concurrent_test();
    run_latency_test();
    
    std::cout << "\n=== Benchmark Complete ===\n";
    return 0;
}