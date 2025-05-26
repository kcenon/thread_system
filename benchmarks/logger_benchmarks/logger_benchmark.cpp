/**
 * @file logger_benchmark.cpp
 * @brief Performance benchmarks for the logging system
 */

#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <iomanip>
#include <locale>
#include <codecvt>

#include "logger.h"
#include "formatter.h"

// Separate logger for test output to avoid interference with benchmark
namespace test_output {
    void print_info(const std::string& message) {
        log_module::information(message);
    }
}

using namespace std::chrono;
using namespace utility_module;

class LoggerBenchmark {
public:
    LoggerBenchmark() {
        // Logger will be configured per test
    }
    
    void run_all_benchmarks() {
        test_output::print_info("\n=== Logger Performance Benchmarks ===");
        
        benchmark_throughput();
        benchmark_latency();
        benchmark_concurrent_logging();
        benchmark_different_targets();
        
        test_output::print_info("\n=== Logger Benchmark Complete ===");
    }
    
private:
    void benchmark_throughput() {
        test_output::print_info("\n1. Logger Throughput by Log Level");
        test_output::print_info("---------------------------------");
        
        struct LevelTest {
            log_module::log_types level;
            std::wstring name;
            std::function<void(const std::wstring&)> log_func;
        };
        
        std::vector<LevelTest> levels = {
            {log_module::log_types::Debug, L"Debug", 
             [](const std::wstring& msg) { log_module::debug(msg); }},
            {log_module::log_types::Information, L"Info", 
             [](const std::wstring& msg) { log_module::info(msg); }},
            {log_module::log_types::Warning, L"Warning", 
             [](const std::wstring& msg) { log_module::warning(msg); }},
            {log_module::log_types::Error, L"Error", 
             [](const std::wstring& msg) { log_module::error(msg); }}
        };
        
        for (const auto& level : levels) {
            // Configure logger for this test
            log_module::stop();
            log_module::set_title("throughput_test");
            log_module::file_target(level.level);
            log_module::console_target(log_module::log_types::None);
            log_module::start();
            
            const size_t num_messages = 100000;
            
            auto start = high_resolution_clock::now();
            
            for (size_t i = 0; i < num_messages; ++i) {
                level.log_func(formatter::format(L"Test message {}: {}", 
                    i, L"Performance benchmark"));
            }
            
            // Wait for all messages to be processed
            log_module::stop();
            
            auto end = high_resolution_clock::now();
            double elapsed_ms = duration_cast<milliseconds>(end - start).count();
            double throughput = (num_messages * 1000.0) / elapsed_ms;
            
            // Convert wstring to string for logging
            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
            std::string level_name = converter.to_bytes(level.name);
            test_output::print_info(formatter::format("{}: {:.0f} msg/s", level_name, throughput));
        }
    }
    
    void benchmark_latency() {
        test_output::print_info("\n2. Logger Latency Analysis");
        test_output::print_info("--------------------------");
        
        // Configure for latency testing
        log_module::stop();
        log_module::set_title("latency_test");
        log_module::file_target(log_module::log_types::All);
        log_module::console_target(log_module::log_types::None);
        log_module::start();
        
        std::vector<double> latencies;
        const size_t num_samples = 10000;
        
        for (size_t i = 0; i < num_samples; ++i) {
            auto start = high_resolution_clock::now();
            
            log_module::info(L"Latency test message {}", i);
            
            auto end = high_resolution_clock::now();
            double latency_us = duration_cast<microseconds>(end - start).count();
            latencies.push_back(latency_us);
            
            // Small delay to avoid overwhelming the system
            if (i % 100 == 0) {
                std::this_thread::sleep_for(microseconds(10));
            }
        }
        
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
        double p999 = latencies[latencies.size() * 999 / 1000];
        
        test_output::print_info(formatter::format("Average: {:.1f} μs", avg_latency));
        test_output::print_info(formatter::format("P50: {:.1f} μs", p50));
        test_output::print_info(formatter::format("P90: {:.1f} μs", p90));
        test_output::print_info(formatter::format("P99: {:.1f} μs", p99));
        test_output::print_info(formatter::format("P99.9: {:.1f} μs", p999));
        
        log_module::stop();
    }
    
    void benchmark_concurrent_logging() {
        test_output::print_info("\n3. Concurrent Logging Performance");
        test_output::print_info("---------------------------------");
        
        std::vector<size_t> thread_counts = {1, 2, 4, 8, 16};
        
        for (size_t num_threads : thread_counts) {
            log_module::stop();
            log_module::set_title("concurrent_test");
            log_module::file_target(log_module::log_types::All);
            log_module::console_target(log_module::log_types::None);
            log_module::start();
            
            const size_t messages_per_thread = 10000;
            std::atomic<size_t> total_messages{0};
            
            auto start = high_resolution_clock::now();
            
            std::vector<std::thread> threads;
            for (size_t t = 0; t < num_threads; ++t) {
                threads.emplace_back([t, messages_per_thread, &total_messages] {
                    for (size_t i = 0; i < messages_per_thread; ++i) {
                        log_module::info(L"Thread {} message {}", t, i);
                        total_messages.fetch_add(1);
                    }
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
            
            log_module::stop();
            
            auto end = high_resolution_clock::now();
            double elapsed_ms = duration_cast<milliseconds>(end - start).count();
            double throughput = (total_messages.load() * 1000.0) / elapsed_ms;
            
            test_output::print_info(formatter::format("{} threads: {:.0f} msg/s", num_threads, throughput));
        }
    }
    
    void benchmark_different_targets() {
        test_output::print_info("\n4. Performance by Output Target");
        test_output::print_info("-------------------------------");
        
        struct TargetTest {
            std::string name;
            std::function<void()> configure;
        };
        
        std::vector<TargetTest> targets = {
            {"Console only", [] {
                log_module::file_target(log_module::log_types::None);
                log_module::console_target(log_module::log_types::All);
                log_module::callback_target(log_module::log_types::None);
            }},
            {"File only", [] {
                log_module::file_target(log_module::log_types::All);
                log_module::console_target(log_module::log_types::None);
                log_module::callback_target(log_module::log_types::None);
            }},
            {"Both console & file", [] {
                log_module::file_target(log_module::log_types::All);
                log_module::console_target(log_module::log_types::All);
                log_module::callback_target(log_module::log_types::None);
            }},
            {"Callback only", [] {
                log_module::file_target(log_module::log_types::None);
                log_module::console_target(log_module::log_types::None);
                log_module::callback_target(log_module::log_types::All);
                log_module::message_callback(
                    [](const log_module::log_types&, const std::string&, 
                       const std::string&) {
                        // Empty callback to measure overhead
                    });
            }}
        };
        
        const size_t num_messages = 50000;
        
        for (const auto& target : targets) {
            log_module::stop();
            log_module::set_title("target_test");
            target.configure();
            log_module::start();
            
            auto start = high_resolution_clock::now();
            
            for (size_t i = 0; i < num_messages; ++i) {
                log_module::info(L"Target benchmark message {}", i);
            }
            
            log_module::stop();
            
            auto end = high_resolution_clock::now();
            double elapsed_ms = duration_cast<milliseconds>(end - start).count();
            double throughput = (num_messages * 1000.0) / elapsed_ms;
            
            test_output::print_info(formatter::format("{}: {:.0f} msg/s", target.name, throughput));
        }
    }
};

int main() {
    LoggerBenchmark benchmark;
    benchmark.run_all_benchmarks();
    
    return 0;
}