/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.
*****************************************************************************/

/**
 * @file logging_throughput_benchmark.cpp
 * @brief Google Benchmark-based comprehensive logging throughput and performance tests
 * 
 * Tests logging performance under various conditions including
 * high throughput, different log levels, and concurrent scenarios
 * using Google Benchmark framework.
 */

#include <benchmark/benchmark.h>
#include <vector>
#include <thread>
#include <atomic>
#include <random>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "../../sources/logger/core/logger.h"
#include "../../sources/utilities/core/formatter.h"

using namespace log_module;
using namespace utility_module;

// Sample messages for benchmarking
static std::vector<std::wstring> g_sample_messages = {
    L"Simple log message",
    L"Medium length log message with some details about the operation being performed",
    L"This is a longer log message that contains more detailed information about what happened during the execution of a complex operation that might involve multiple steps and various data processing tasks",
    L"ERROR: Failed to process request - Invalid input parameter 'user_id' with value 12345",
    L"INFO: User authentication successful for user@example.com from IP 192.168.1.100",
    L"DEBUG: Database query executed in 15.3ms, returned 42 rows from table 'users'",
    L"WARN: Memory usage is approaching threshold: 85% of available heap space used",
    L"TRACE: Function call trace: process_request() -> validate_input() -> check_permissions() -> execute_query()",
};

// Initialize sample messages once
static void InitializeSampleMessages() {
    static bool initialized = false;
    if (!initialized) {
        // Add some JSON-like structured messages
        for (int i = 0; i < 10; ++i) {
            std::wostringstream woss;
            woss << L"{\"event\": \"transaction\", \"id\": " << (1000 + i) 
                 << L", \"amount\": " << (100.0 + i * 10.5) 
                 << L", \"currency\": \"USD\", \"timestamp\": \"2024-01-01T12:" 
                 << std::setfill(L'0') << std::setw(2) << i 
                 << L":00Z\", \"status\": \"completed\"}";
            g_sample_messages.push_back(woss.str());
        }
        initialized = true;
    }
}

/**
 * @brief Benchmark basic logging throughput
 * 
 * Measures raw logging performance with various message counts.
 */
static void BM_BasicThroughput(benchmark::State& state) {
    const size_t message_count = state.range(0);
    InitializeSampleMessages();
    
    // Setup logger
    log_module::stop();
    log_module::start();
    log_module::console_target(log_types::Information);
    log_module::file_target(log_types::Information);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> msg_dist(0, g_sample_messages.size() - 1);
    
    // Benchmark
    for (auto _ : state) {
        for (size_t i = 0; i < message_count; ++i) {
            const auto& message = g_sample_messages[msg_dist(gen)];
            log_module::write_information(L"{}", message);
        }
        
        // Ensure all messages are processed
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    log_module::stop();
    
    state.SetItemsProcessed(state.iterations() * message_count);
    state.counters["msg_count"] = message_count;
}
// Test with different message counts
BENCHMARK(BM_BasicThroughput)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000);

/**
 * @brief Benchmark concurrent logging performance
 * 
 * Measures throughput when multiple threads log simultaneously.
 */
static void BM_ConcurrentLogging(benchmark::State& state) {
    const size_t thread_count = state.range(0);
    const size_t messages_per_thread = state.range(1);
    InitializeSampleMessages();
    
    // Setup logger
    log_module::stop();
    log_module::start();
    log_module::console_target(log_types::Information);
    
    // Benchmark
    for (auto _ : state) {
        std::vector<std::thread> threads;
        std::atomic<size_t> total_messages{0};
        
        state.PauseTiming();
        for (size_t t = 0; t < thread_count; ++t) {
            threads.emplace_back([&total_messages, messages_per_thread, t]() {
                std::random_device rd;
                std::mt19937 gen(rd() + t);
                std::uniform_int_distribution<> msg_dist(0, g_sample_messages.size() - 1);
                
                for (size_t i = 0; i < messages_per_thread; ++i) {
                    const auto& message = g_sample_messages[msg_dist(gen)];
                    log_module::write_information(L"Thread {} message {}: {}", t, i, message);
                    total_messages.fetch_add(1);
                }
            });
        }
        state.ResumeTiming();
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Ensure all messages are processed
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    log_module::stop();
    
    state.SetItemsProcessed(state.iterations() * thread_count * messages_per_thread);
    state.counters["threads"] = thread_count;
    state.counters["msgs_per_thread"] = messages_per_thread;
}
// Test matrix: Threads x Messages per thread
BENCHMARK(BM_ConcurrentLogging)
    ->Args({1, 10000})
    ->Args({2, 10000})
    ->Args({4, 10000})
    ->Args({8, 10000})
    ->Args({16, 10000});

/**
 * @brief Benchmark different log levels
 * 
 * Measures performance differences between log levels.
 */
static void BM_LogLevelPerformance(benchmark::State& state) {
    const auto log_level = static_cast<log_types>(state.range(0));
    const size_t message_count = 50000;
    
    // Setup logger
    log_module::stop();
    log_module::start();
    log_module::console_target(log_level);
    
    const std::wstring test_message = L"Test message for level performance testing";
    
    // Benchmark
    for (auto _ : state) {
        for (size_t i = 0; i < message_count; ++i) {
            switch (log_level) {
                case log_types::Error:
                    log_module::write_error(L"Error message {}: {}", i, test_message);
                    break;
                // Warning type not available, using Error instead
                case log_types::Information:
                    log_module::write_information(L"Info message {}: {}", i, test_message);
                    break;
                case log_types::Debug:
                    log_module::write_debug(L"Debug message {}: {}", i, test_message);
                    break;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    log_module::stop();
    
    state.SetItemsProcessed(state.iterations() * message_count);
    state.counters["log_level"] = static_cast<int>(log_level);
}
// Test different log levels
BENCHMARK(BM_LogLevelPerformance)
    ->Arg(static_cast<int>(log_types::Error))
    ->Arg(static_cast<int>(log_types::Information))
    ->Arg(static_cast<int>(log_types::Debug));

/**
 * @brief Benchmark large message logging
 * 
 * Measures performance with different message sizes.
 */
static void BM_LargeMessageLogging(benchmark::State& state) {
    const size_t message_size = state.range(0);
    const size_t num_messages = 1000;
    
    // Create a large message of specified size
    std::wstring large_message(message_size, L'X');
    
    // Setup logger
    log_module::stop();
    log_module::start();
    log_module::console_target(log_types::Information);
    
    // Benchmark
    for (auto _ : state) {
        for (size_t i = 0; i < num_messages; ++i) {
            log_module::write_information(L"Large message {}: {}", i, large_message);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    log_module::stop();
    
    state.SetItemsProcessed(state.iterations() * num_messages);
    state.SetBytesProcessed(state.iterations() * num_messages * message_size);
    state.counters["msg_size"] = message_size;
}
// Test different message sizes
BENCHMARK(BM_LargeMessageLogging)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000);

/**
 * @brief Benchmark formatted vs unformatted messages
 * 
 * Measures the overhead of message formatting.
 */
static void BM_FormattedMessages(benchmark::State& state) {
    const bool complex_formatting = state.range(0);
    const size_t num_messages = 100000;
    
    // Setup logger
    log_module::stop();
    log_module::start();
    log_module::console_target(log_types::Information);
    
    // Benchmark
    for (auto _ : state) {
        if (complex_formatting) {
            for (size_t i = 0; i < num_messages; ++i) {
                log_module::write_information(
                    L"Complex format: User={}, ID={}, Balance={:.2f}, Timestamp={}, Status={}, "
                    L"Request={}, Session={}, IP={}, UserAgent={}", 
                    L"user@example.com", i, 1234.56 + i, 
                    std::chrono::system_clock::now().time_since_epoch().count(),
                    L"active", L"GET /api/data", formatter::format(L"sess_{}", i),
                    formatter::format(L"192.168.1.{}", i % 255), 
                    L"Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
            }
        } else {
            for (size_t i = 0; i < num_messages; ++i) {
                log_module::write_information(L"Simple message {}", i);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    log_module::stop();
    
    state.SetItemsProcessed(state.iterations() * num_messages);
    state.counters["complex_format"] = complex_formatting;
}
// Test simple vs complex formatting
BENCHMARK(BM_FormattedMessages)
    ->Arg(0)  // Simple
    ->Arg(1); // Complex

/**
 * @brief Benchmark file vs console logging
 * 
 * Compares performance between different logging targets.
 */
static void BM_LoggingTargets(benchmark::State& state) {
    const int target_type = state.range(0);  // 0=console, 1=file, 2=both
    const size_t num_messages = 50000;
    InitializeSampleMessages();
    
    // Setup logger
    log_module::stop();
    log_module::start();
    
    switch (target_type) {
        case 0:
            log_module::console_target(log_types::Information);
            break;
        case 1:
            log_module::file_target(log_types::Information);
            break;
        case 2:
            log_module::console_target(log_types::Information);
            log_module::file_target(log_types::Information);
            break;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> msg_dist(0, g_sample_messages.size() - 1);
    
    // Benchmark
    for (auto _ : state) {
        for (size_t i = 0; i < num_messages; ++i) {
            const auto& message = g_sample_messages[msg_dist(gen)];
            log_module::write_information(L"{}", message);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    log_module::stop();
    
    // Clean up file if created
    if (target_type >= 1) {
        std::remove("benchmark_log.txt");
    }
    
    state.SetItemsProcessed(state.iterations() * num_messages);
    state.counters["target"] = target_type;
}
// Test different targets
BENCHMARK(BM_LoggingTargets)
    ->Arg(0)  // Console
    ->Arg(1)  // File
    ->Arg(2); // Both

/**
 * @brief Benchmark burst logging performance
 * 
 * Measures performance with burst patterns of messages.
 */
static void BM_BurstLogging(benchmark::State& state) {
    const size_t burst_size = state.range(0);
    const size_t burst_interval_ms = state.range(1);
    const size_t num_bursts = 50;
    
    // Setup logger
    log_module::stop();
    log_module::start();
    log_module::console_target(log_types::Information);
    
    // Benchmark
    for (auto _ : state) {
        for (size_t burst = 0; burst < num_bursts; ++burst) {
            // Send burst of messages
            for (size_t i = 0; i < burst_size; ++i) {
                log_module::write_information(L"Burst {} message {}: processing data", burst, i);
            }
            
            // Wait between bursts
            if (burst < num_bursts - 1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(burst_interval_ms));
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    log_module::stop();
    
    state.SetItemsProcessed(state.iterations() * burst_size * num_bursts);
    state.counters["burst_size"] = burst_size;
    state.counters["interval_ms"] = burst_interval_ms;
}
// Test matrix: Burst size x Interval
BENCHMARK(BM_BurstLogging)
    ->Args({100, 10})
    ->Args({1000, 10})
    ->Args({100, 100})
    ->Args({1000, 100});

/**
 * @brief Benchmark sustained load performance
 * 
 * Measures logging performance under sustained load with latency tracking.
 */
static void BM_SustainedLoad(benchmark::State& state) {
    const size_t target_rate = state.range(0);  // Messages per second
    const size_t duration_seconds = 5;
    InitializeSampleMessages();
    
    // Setup logger
    log_module::stop();
    log_module::start();
    log_module::console_target(log_types::Information);
    
    const auto message_interval = std::chrono::microseconds(1000000 / target_rate);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> msg_dist(0, g_sample_messages.size() - 1);
    
    // Benchmark
    for (auto _ : state) {
        auto start_time = std::chrono::high_resolution_clock::now();
        auto next_message_time = start_time;
        size_t messages_sent = 0;
        
        while (std::chrono::high_resolution_clock::now() - start_time < std::chrono::seconds(duration_seconds)) {
            auto now = std::chrono::high_resolution_clock::now();
            
            if (now >= next_message_time) {
                const auto& message = g_sample_messages[msg_dist(gen)];
                log_module::write_information(L"Sustained load message: {}", message);
                
                messages_sent++;
                next_message_time += message_interval;
            } else {
                std::this_thread::yield();
            }
        }
        
        state.SetIterationTime(duration_seconds);
        state.counters["actual_rate"] = messages_sent / duration_seconds;
    }
    
    log_module::stop();
    
    state.counters["target_rate"] = target_rate;
}
// Test different target rates
BENCHMARK(BM_SustainedLoad)
    ->Arg(1000)   // 1K msgs/sec
    ->Arg(5000)   // 5K msgs/sec
    ->Arg(10000)  // 10K msgs/sec
    ->Arg(20000)  // 20K msgs/sec
    ->UseManualTime();

// Main function to run benchmarks
BENCHMARK_MAIN();