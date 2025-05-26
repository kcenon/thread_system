/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.
*****************************************************************************/

/**
 * @file logging_throughput_benchmark.cpp
 * @brief Comprehensive logging throughput and performance benchmark
 * 
 * Tests logging performance under various conditions including
 * high throughput, different log levels, and concurrent scenarios.
 */

#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <random>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "logger.h"
#include "formatter.h"

using namespace log_module;

class logging_throughput_benchmark {
private:
    struct benchmark_metrics {
        std::chrono::milliseconds duration;
        uint64_t messages_logged;
        uint64_t bytes_logged;
        double throughput_msgs_per_sec;
        double throughput_mb_per_sec;
        std::vector<std::chrono::nanoseconds> latencies;
    };

    std::atomic<uint64_t> total_messages_{0};
    std::atomic<uint64_t> total_bytes_{0};
    std::vector<std::string> sample_messages_;

public:
    void run_all_logging_benchmarks() {
        information("=== Logging Performance Benchmark ===");

        prepare_sample_messages();
        
        test_basic_throughput();
        test_concurrent_logging();
        test_different_log_levels();
        test_large_message_logging();
        test_formatted_message_performance();
        test_file_vs_console_performance();
        test_burst_logging_performance();
        test_sustained_load_performance();
    }

private:
    void prepare_sample_messages() {
        information("Preparing sample messages...");
        
        // Various message sizes and types
        sample_messages_ = {
            "Simple log message",
            "Medium length log message with some details about the operation being performed",
            "This is a longer log message that contains more detailed information about what happened during the execution of a complex operation that might involve multiple steps and various data processing tasks",
            "ERROR: Failed to process request - Invalid input parameter 'user_id' with value 12345",
            "INFO: User authentication successful for user@example.com from IP 192.168.1.100",
            "DEBUG: Database query executed in 15.3ms, returned 42 rows from table 'users'",
            "WARN: Memory usage is approaching threshold: 85% of available heap space used",
            "TRACE: Function call trace: process_request() -> validate_input() -> check_permissions() -> execute_query()",
        };
        
        // Add some JSON-like structured messages
        for (int i = 0; i < 10; ++i) {
            std::ostringstream oss;
            oss << R"({"event": "transaction", "id": )" << (1000 + i) 
                << R"(, "amount": )" << (100.0 + i * 10.5) 
                << R"(, "currency": "USD", "timestamp": "2024-01-01T12:)" 
                << std::setfill('0') << std::setw(2) << i 
                << R"(:00Z", "status": "completed"})";
            sample_messages_.push_back(oss.str());
        }
        
        information(format_string("Prepared {} sample messages", sample_messages_.size()));
    }

    void test_basic_throughput() {
        information("--- Basic Throughput Test ---");
        
        // Test different message counts
        std::vector<size_t> message_counts = {1000, 10000, 100000, 500000};
        
        for (auto count : message_counts) {
            auto metrics = run_basic_throughput_test(count);
            
            information(format_string("Messages: {:>7} -> {:>8.1f} msgs/sec, {:>6.1f} MB/sec, Duration: {}ms",
                      count, metrics.throughput_msgs_per_sec, metrics.throughput_mb_per_sec, metrics.duration.count()));
        }
    }

    void test_concurrent_logging() {
        information("--- Concurrent Logging Test ---");
        
        std::vector<size_t> thread_counts = {1, 2, 4, 8, 16};
        const size_t messages_per_thread = 10000;
        
        for (auto threads : thread_counts) {
            auto metrics = run_concurrent_logging_test(threads, messages_per_thread);
            
            information(format_string("Threads: {:>2} -> {:>8.1f} msgs/sec, {:>6.1f} MB/sec, Total: {} msgs",
                      threads, metrics.throughput_msgs_per_sec, metrics.throughput_mb_per_sec, metrics.messages_logged));
        }
    }

    void test_different_log_levels() {
        information("--- Log Level Performance Test ---");
        
        struct level_test {
            log_types level;
            std::string name;
        };
        
        std::vector<level_test> levels = {
            {log_types::Error, "ERROR"},
            {log_types::Warning, "WARN"},
            {log_types::Information, "INFO"},
            {log_types::Debug, "DEBUG"},
            {log_types::Trace, "TRACE"}
        };
        
        const size_t messages_per_level = 50000;
        
        for (const auto& level : levels) {
            auto metrics = run_log_level_test(level.level, messages_per_level);
            
            information(format_string("Level {:>5} -> {:>8.1f} msgs/sec, Duration: {}ms",
                      level.name, metrics.throughput_msgs_per_sec, metrics.duration.count()));
        }
    }

    void test_large_message_logging() {
        information("--- Large Message Performance Test ---");
        
        std::vector<size_t> message_sizes = {100, 1000, 10000, 100000};
        const size_t num_messages = 1000;
        
        for (auto size : message_sizes) {
            auto metrics = run_large_message_test(size, num_messages);
            
            information(format_string("Size: {:>6} bytes -> {:>6.1f} msgs/sec, {:>6.1f} MB/sec",
                      size, metrics.throughput_msgs_per_sec, metrics.throughput_mb_per_sec));
        }
    }

    void test_formatted_message_performance() {
        information("--- Formatted Message Performance Test ---");
        
        const size_t num_messages = 100000;
        
        // Test simple vs complex formatting
        auto simple_metrics = run_formatted_message_test(num_messages, false);
        auto complex_metrics = run_formatted_message_test(num_messages, true);
        
        information(format_string("Simple formatting  -> {:>8.1f} msgs/sec", simple_metrics.throughput_msgs_per_sec));
        information(format_string("Complex formatting -> {:>8.1f} msgs/sec", complex_metrics.throughput_msgs_per_sec));
        
        double overhead = ((simple_metrics.throughput_msgs_per_sec - complex_metrics.throughput_msgs_per_sec) / 
                          simple_metrics.throughput_msgs_per_sec) * 100.0;
        information(format_string("Formatting overhead: {:.1f}%", overhead));
    }

    void test_file_vs_console_performance() {
        information("--- File vs Console Performance Test ---");
        
        const size_t num_messages = 50000;
        
        auto console_metrics = run_console_logging_test(num_messages);
        auto file_metrics = run_file_logging_test(num_messages);
        
        information(format_string("Console logging -> {:>8.1f} msgs/sec", console_metrics.throughput_msgs_per_sec));
        information(format_string("File logging    -> {:>8.1f} msgs/sec", file_metrics.throughput_msgs_per_sec));
        
        double ratio = file_metrics.throughput_msgs_per_sec / console_metrics.throughput_msgs_per_sec;
        information(format_string("File/Console ratio: {:.2f}x", ratio));
    }

    void test_burst_logging_performance() {
        information("--- Burst Logging Performance Test ---");
        
        const size_t burst_size = 1000;
        const size_t num_bursts = 50;
        const auto burst_interval = std::chrono::milliseconds(100);
        
        auto metrics = run_burst_logging_test(burst_size, num_bursts, burst_interval);
        
        information(format_string("Burst test: {} bursts of {} messages", num_bursts, burst_size));
        information(format_string("Total throughput -> {:>8.1f} msgs/sec", metrics.throughput_msgs_per_sec));
        information(format_string("Total duration: {}ms", metrics.duration.count()));
    }

    void test_sustained_load_performance() {
        information("--- Sustained Load Performance Test ---");
        
        const auto test_duration = std::chrono::seconds(30);
        const auto message_interval = std::chrono::microseconds(100); // 10K msgs/sec target
        
        auto metrics = run_sustained_load_test(test_duration, message_interval);
        
        information(format_string("Sustained load: {} seconds", test_duration.count()));
        information(format_string("Achieved throughput -> {:>8.1f} msgs/sec", metrics.throughput_msgs_per_sec));
        information(format_string("Total messages: {}", metrics.messages_logged));
        
        // Analyze latency distribution
        if (!metrics.latencies.empty()) {
            analyze_latency_distribution(metrics.latencies);
        }
    }

    benchmark_metrics run_basic_throughput_test(size_t message_count) {
        setup_logger();
        
        total_messages_ = 0;
        total_bytes_ = 0;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> msg_dist(0, sample_messages_.size() - 1);
        
        for (size_t i = 0; i < message_count; ++i) {
            const auto& message = sample_messages_[msg_dist(gen)];
            write_information(message);
            
            total_messages_.fetch_add(1, std::memory_order_relaxed);
            total_bytes_.fetch_add(message.length(), std::memory_order_relaxed);
        }
        
        // Wait for logger to process all messages
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        stop();
        
        benchmark_metrics metrics;
        metrics.duration = duration;
        metrics.messages_logged = total_messages_.load();
        metrics.bytes_logged = total_bytes_.load();
        metrics.throughput_msgs_per_sec = (metrics.messages_logged * 1000.0) / duration.count();
        metrics.throughput_mb_per_sec = (metrics.bytes_logged * 1000.0) / (duration.count() * 1024 * 1024);
        
        return metrics;
    }

    benchmark_metrics run_concurrent_logging_test(size_t thread_count, size_t messages_per_thread) {
        setup_logger();
        
        total_messages_ = 0;
        total_bytes_ = 0;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        for (size_t t = 0; t < thread_count; ++t) {
            threads.emplace_back([this, messages_per_thread, t]() {
                std::random_device rd;
                std::mt19937 gen(rd() + t);
                std::uniform_int_distribution<> msg_dist(0, sample_messages_.size() - 1);
                
                for (size_t i = 0; i < messages_per_thread; ++i) {
                    const auto& message = sample_messages_[msg_dist(gen)];
                    write_information("Thread {} message {}: {}", t, i, message);
                    
                    total_messages_.fetch_add(1, std::memory_order_relaxed);
                    total_bytes_.fetch_add(message.length() + 20, std::memory_order_relaxed); // +20 for formatting
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Wait for logger to process all messages
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        stop();
        
        benchmark_metrics metrics;
        metrics.duration = duration;
        metrics.messages_logged = total_messages_.load();
        metrics.bytes_logged = total_bytes_.load();
        metrics.throughput_msgs_per_sec = (metrics.messages_logged * 1000.0) / duration.count();
        metrics.throughput_mb_per_sec = (metrics.bytes_logged * 1000.0) / (duration.count() * 1024 * 1024);
        
        return metrics;
    }

    benchmark_metrics run_log_level_test(log_types level, size_t message_count) {
        setup_logger();
        
        total_messages_ = 0;
        total_bytes_ = 0;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::string test_message = "Test message for level performance testing";
        
        for (size_t i = 0; i < message_count; ++i) {
            switch (level) {
                case log_types::Error:
                    write_error("Error message {}: {}", i, test_message);
                    break;
                case log_types::Warning:
                    write_warning("Warning message {}: {}", i, test_message);
                    break;
                case log_types::Information:
                    write_information("Info message {}: {}", i, test_message);
                    break;
                case log_types::Debug:
                    write_debug("Debug message {}: {}", i, test_message);
                    break;
                case log_types::Trace:
                    write_trace("Trace message {}: {}", i, test_message);
                    break;
            }
            
            total_messages_.fetch_add(1, std::memory_order_relaxed);
            total_bytes_.fetch_add(test_message.length() + 30, std::memory_order_relaxed);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        stop();
        
        benchmark_metrics metrics;
        metrics.duration = duration;
        metrics.messages_logged = total_messages_.load();
        metrics.bytes_logged = total_bytes_.load();
        metrics.throughput_msgs_per_sec = (metrics.messages_logged * 1000.0) / duration.count();
        metrics.throughput_mb_per_sec = (metrics.bytes_logged * 1000.0) / (duration.count() * 1024 * 1024);
        
        return metrics;
    }

    benchmark_metrics run_large_message_test(size_t message_size, size_t message_count) {
        setup_logger();
        
        // Create a large message of specified size
        std::string large_message(message_size, 'X');
        
        total_messages_ = 0;
        total_bytes_ = 0;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < message_count; ++i) {
            write_information("Large message {}: {}", i, large_message);
            
            total_messages_.fetch_add(1, std::memory_order_relaxed);
            total_bytes_.fetch_add(message_size + 20, std::memory_order_relaxed);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        stop();
        
        benchmark_metrics metrics;
        metrics.duration = duration;
        metrics.messages_logged = total_messages_.load();
        metrics.bytes_logged = total_bytes_.load();
        metrics.throughput_msgs_per_sec = (metrics.messages_logged * 1000.0) / duration.count();
        metrics.throughput_mb_per_sec = (metrics.bytes_logged * 1000.0) / (duration.count() * 1024 * 1024);
        
        return metrics;
    }

    benchmark_metrics run_formatted_message_test(size_t message_count, bool complex_formatting) {
        setup_logger();
        
        total_messages_ = 0;
        total_bytes_ = 0;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (complex_formatting) {
            for (size_t i = 0; i < message_count; ++i) {
                write_information("Complex format: User={}, ID={}, Balance={:.2f}, Timestamp={}, Status={}, "
                                "Request={}, Session={}, IP={}, UserAgent={}", 
                                "user@example.com", i, 1234.56 + i, 
                                std::chrono::system_clock::now().time_since_epoch().count(),
                                "active", "GET /api/data", "sess_" + std::to_string(i),
                                "192.168.1." + std::to_string(i % 255), 
                                "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
                
                total_messages_.fetch_add(1, std::memory_order_relaxed);
                total_bytes_.fetch_add(200, std::memory_order_relaxed); // Estimate
            }
        } else {
            for (size_t i = 0; i < message_count; ++i) {
                write_information("Simple message {}", i);
                
                total_messages_.fetch_add(1, std::memory_order_relaxed);
                total_bytes_.fetch_add(20, std::memory_order_relaxed);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        stop();
        
        benchmark_metrics metrics;
        metrics.duration = duration;
        metrics.messages_logged = total_messages_.load();
        metrics.bytes_logged = total_bytes_.load();
        metrics.throughput_msgs_per_sec = (metrics.messages_logged * 1000.0) / duration.count();
        metrics.throughput_mb_per_sec = (metrics.bytes_logged * 1000.0) / (duration.count() * 1024 * 1024);
        
        return metrics;
    }

    benchmark_metrics run_console_logging_test(size_t message_count) {
        set_title("console_benchmark");
        console_target(log_types::All);
        start();
        
        return run_basic_throughput_test(message_count);
    }

    benchmark_metrics run_file_logging_test(size_t message_count) {
        set_title("file_benchmark");
        file_target(log_types::All);
        start();
        
        return run_basic_throughput_test(message_count);
    }

    benchmark_metrics run_burst_logging_test(size_t burst_size, size_t num_bursts, 
                                           std::chrono::milliseconds burst_interval) {
        setup_logger();
        
        total_messages_ = 0;
        total_bytes_ = 0;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (size_t burst = 0; burst < num_bursts; ++burst) {
            // Send burst of messages
            for (size_t i = 0; i < burst_size; ++i) {
                write_information("Burst {} message {}: processing data", burst, i);
                
                total_messages_.fetch_add(1, std::memory_order_relaxed);
                total_bytes_.fetch_add(40, std::memory_order_relaxed);
            }
            
            // Wait between bursts
            if (burst < num_bursts - 1) {
                std::this_thread::sleep_for(burst_interval);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        stop();
        
        benchmark_metrics metrics;
        metrics.duration = duration;
        metrics.messages_logged = total_messages_.load();
        metrics.bytes_logged = total_bytes_.load();
        metrics.throughput_msgs_per_sec = (metrics.messages_logged * 1000.0) / duration.count();
        metrics.throughput_mb_per_sec = (metrics.bytes_logged * 1000.0) / (duration.count() * 1024 * 1024);
        
        return metrics;
    }

    benchmark_metrics run_sustained_load_test(std::chrono::seconds test_duration, 
                                            std::chrono::microseconds message_interval) {
        setup_logger();
        
        total_messages_ = 0;
        total_bytes_ = 0;
        std::vector<std::chrono::nanoseconds> latencies;
        latencies.reserve(static_cast<size_t>(test_duration.count() * 1000000 / message_interval.count()));
        
        auto start_time = std::chrono::high_resolution_clock::now();
        auto next_message_time = start_time;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> msg_dist(0, sample_messages_.size() - 1);
        
        while (std::chrono::high_resolution_clock::now() - start_time < test_duration) {
            auto now = std::chrono::high_resolution_clock::now();
            
            if (now >= next_message_time) {
                auto log_start = std::chrono::high_resolution_clock::now();
                
                const auto& message = sample_messages_[msg_dist(gen)];
                write_information("Sustained load message: {}", message);
                
                auto log_end = std::chrono::high_resolution_clock::now();
                auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(log_end - log_start);
                latencies.push_back(latency);
                
                total_messages_.fetch_add(1, std::memory_order_relaxed);
                total_bytes_.fetch_add(message.length() + 30, std::memory_order_relaxed);
                
                next_message_time += message_interval;
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        stop();
        
        benchmark_metrics metrics;
        metrics.duration = duration;
        metrics.messages_logged = total_messages_.load();
        metrics.bytes_logged = total_bytes_.load();
        metrics.throughput_msgs_per_sec = (metrics.messages_logged * 1000.0) / duration.count();
        metrics.throughput_mb_per_sec = (metrics.bytes_logged * 1000.0) / (duration.count() * 1024 * 1024);
        metrics.latencies = std::move(latencies);
        
        return metrics;
    }

    void setup_logger() {
        set_title("logging_benchmark");
        console_target(log_types::All);
        file_target(log_types::All);
        set_wake_interval(std::chrono::milliseconds(1)); // Frequent wake for responsiveness
        start();
    }

    void analyze_latency_distribution(const std::vector<std::chrono::nanoseconds>& latencies) {
        if (latencies.empty()) return;
        
        auto sorted_latencies = latencies;
        std::sort(sorted_latencies.begin(), sorted_latencies.end());
        
        auto percentile = [&](double p) -> uint64_t {
            size_t index = static_cast<size_t>(p * (sorted_latencies.size() - 1));
            return sorted_latencies[index].count();
        };
        
        information("Latency distribution (μs):");
        information(format_string("  P50: {:>6}, P90: {:>6}, P95: {:>6}, P99: {:>6}, Max: {:>6}",
                  percentile(0.5) / 1000, percentile(0.9) / 1000, percentile(0.95) / 1000,
                  percentile(0.99) / 1000, sorted_latencies.back().count() / 1000));
    }
};

int main() {
    try {
        logging_throughput_benchmark benchmark;
        benchmark.run_all_logging_benchmarks();
    } catch (const std::exception& e) {
        error(format_string("Logging benchmark failed: {}", e.what()));
        return 1;
    }

    return 0;
}