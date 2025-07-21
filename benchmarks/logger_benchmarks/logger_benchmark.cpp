/*
 * BSD 3-Clause License
 * 
 * Copyright (c) 2024, DongCheol Shin
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file logger_benchmark.cpp
 * @brief Google Benchmark-based performance tests for the logging system
 * 
 * This file contains comprehensive benchmarks using Google Benchmark to measure:
 * - Logging throughput for different log levels
 * - Logging latency under various conditions
 * - Concurrent logging performance
 * - Format string performance
 * - Different logging targets (console, file, etc.)
 */

#include <benchmark/benchmark.h>
#include <vector>
#include <thread>
#include <atomic>
#include <sstream>
#include <fstream>

#include "../../sources/logger/core/logger.h"
#include "../../sources/utilities/core/formatter.h"

using namespace log_module;
using namespace utility_module;

/**
 * @brief Benchmark logging throughput for different log levels
 * 
 * Measures how many log messages per second can be processed
 * for each log level (Debug, Info, Warning, Error).
 */
static void BM_LoggerThroughput(benchmark::State& state) {
    const auto log_level = static_cast<log_types>(state.range(0));
    const std::wstring message = L"Benchmark log message for throughput testing";
    
    // Setup logger
    log_module::stop();
    log_module::start();
    
    // Configure null output to measure pure logging overhead
    log_module::console_target(log_level);
    
    // Select logging function based on level
    std::function<void(const std::wstring&)> log_func;
    switch (log_level) {
        case log_types::Debug:
            log_func = [](const std::wstring& msg) { log_module::write_debug(L"{}", msg); };
            break;
        case log_types::Information:
            log_func = [](const std::wstring& msg) { log_module::write_information(L"{}", msg); };
            break;
        // Warning type not available in this logging system
        case log_types::Error:
            log_func = [](const std::wstring& msg) { log_module::write_error(L"{}", msg); };
            break;
        default:
            log_func = [](const std::wstring& msg) { log_module::write_information(L"{}", msg); };
    }
    
    // Benchmark
    for (auto _ : state) {
        log_func(message);
    }
    
    // Cleanup
    log_module::stop();
    
    state.SetItemsProcessed(state.iterations());
    state.counters["log_level"] = static_cast<int>(log_level);
}
// Test all log levels
BENCHMARK(BM_LoggerThroughput)
    ->Arg(static_cast<int>(log_types::Debug))
    ->Arg(static_cast<int>(log_types::Information))
    ->Arg(static_cast<int>(log_types::Error));

/**
 * @brief Benchmark logging latency
 * 
 * Measures the time taken for a single log operation
 * including timestamp generation and formatting.
 */
static void BM_LoggerLatency(benchmark::State& state) {
    const size_t message_length = state.range(0);
    const std::wstring message(message_length, L'X');
    
    // Setup logger with console output
    log_module::stop();
    log_module::start();
    log_module::console_target(log_types::Information);
    
    // Measure latency
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        log_module::write_information(L"{}", message);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e9);
    }
    
    // Cleanup
    log_module::stop();
    
    state.SetItemsProcessed(state.iterations());
    state.counters["message_length"] = message_length;
}
// Test different message lengths
BENCHMARK(BM_LoggerLatency)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->UseManualTime();

/**
 * @brief Benchmark concurrent logging performance
 * 
 * Measures throughput when multiple threads are logging simultaneously.
 */
static void BM_ConcurrentLogging(benchmark::State& state) {
    const size_t num_threads = state.range(0);
    const size_t messages_per_thread = 1000;
    
    // Setup logger
    log_module::stop();
    log_module::start();
    log_module::console_target(log_types::Information);
    
    for (auto _ : state) {
        std::vector<std::thread> threads;
        std::atomic<size_t> total_logged{0};
        
        state.PauseTiming();
        // Create threads
        for (size_t i = 0; i < num_threads; ++i) {
            threads.emplace_back([&total_logged, i, messages_per_thread]() {
                for (size_t j = 0; j < messages_per_thread; ++j) {
                    log_module::write_information(L"Thread {} message {}", i, j);
                    total_logged.fetch_add(1);
                }
            });
        }
        state.ResumeTiming();
        
        // Wait for all threads
        for (auto& t : threads) {
            t.join();
        }
    }
    
    // Cleanup
    log_module::stop();
    
    state.SetItemsProcessed(state.iterations() * num_threads * messages_per_thread);
    state.counters["threads"] = num_threads;
}
// Test with different thread counts
BENCHMARK(BM_ConcurrentLogging)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->Arg(16);

/**
 * @brief Benchmark formatted logging performance
 * 
 * Measures the overhead of formatting complex log messages.
 */
static void BM_FormattedLogging(benchmark::State& state) {
    const size_t num_args = state.range(0);
    
    // Setup logger
    log_module::stop();
    log_module::start();
    log_module::console_target(log_types::Information);
    
    // Benchmark based on number of format arguments
    for (auto _ : state) {
        switch (num_args) {
            case 0:
                log_module::write_information(L"Simple message without formatting");
                break;
            case 1:
                log_module::write_information(L"Message with {} argument", 42);
                break;
            case 3:
                log_module::write_information(L"Message with {} {} {}", 
                    L"multiple", 42, 3.14);
                break;
            case 5:
                log_module::write_information(L"Complex {} with {} args: {}, {}, {}", 
                    L"message", 5, true, 3.14159, L"test");
                break;
        }
    }
    
    // Cleanup
    log_module::stop();
    
    state.SetItemsProcessed(state.iterations());
    state.counters["format_args"] = num_args;
}
// Test different numbers of format arguments
BENCHMARK(BM_FormattedLogging)
    ->Arg(0)
    ->Arg(1)
    ->Arg(3)
    ->Arg(5);

/**
 * @brief Benchmark different logging targets
 * 
 * Compares performance of console, file, and callback logging.
 */
static void BM_LoggingTargets(benchmark::State& state) {
    const int target_type = state.range(0);
    const std::wstring message = L"Benchmark message for target testing";
    
    // Setup logger
    log_module::stop();
    log_module::start();
    
    // Configure target based on type
    std::string temp_file = "benchmark_log.txt";
    switch (target_type) {
        case 0: // Console
            log_module::console_target(log_types::Information);
            break;
        case 1: // File
            log_module::file_target(log_types::Information);
            break;
        case 2: // Callback
            log_module::message_callback([](const log_types&, const std::string&, const std::string&) {
                // No-op callback
            });
            log_module::callback_target(log_types::Information);
            break;
    }
    
    // Benchmark
    for (auto _ : state) {
        log_module::write_information(L"{}", message);
    }
    
    // Cleanup
    log_module::stop();
    if (target_type == 1) {
        std::remove(temp_file.c_str());
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["target"] = target_type;
}
// Test different targets (0=console, 1=file, 2=callback)
BENCHMARK(BM_LoggingTargets)
    ->Arg(0)
    ->Arg(1)
    ->Arg(2);

/**
 * @brief Benchmark batch logging performance
 * 
 * Measures performance when logging multiple messages in quick succession.
 */
static void BM_BatchLogging(benchmark::State& state) {
    const size_t batch_size = state.range(0);
    
    // Setup logger
    log_module::stop();
    log_module::start();
    log_module::console_target(log_types::Information);
    
    // Prepare messages
    std::vector<std::wstring> messages;
    for (size_t i = 0; i < batch_size; ++i) {
        messages.push_back(formatter::format(L"Batch message {}", i));
    }
    
    // Benchmark
    for (auto _ : state) {
        for (const auto& msg : messages) {
            log_module::write_information(L"{}", msg);
        }
    }
    
    // Cleanup
    log_module::stop();
    
    state.SetItemsProcessed(state.iterations() * batch_size);
    state.counters["batch_size"] = batch_size;
}
// Test different batch sizes
BENCHMARK(BM_BatchLogging)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

// Main function to run benchmarks
BENCHMARK_MAIN();