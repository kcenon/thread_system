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

/**
 * @file logger_comparison_benchmark.cpp
 * @brief Comprehensive comparison of Thread System loggers vs popular logging libraries
 * 
 * This benchmark compares:
 * - Thread System Standard Logger
 * - spdlog (popular C++ logging library)
 * - Simple console output (baseline)
 * 
 * Metrics measured:
 * - Single-threaded throughput
 * - Multi-threaded scalability
 * - Latency characteristics
 * - Memory usage patterns
 */

#include <benchmark/benchmark.h>
#include "../../sources/utilities/interface/logger.h"
#include "../../sources/utilities/core/formatter.h"

// Check if spdlog is available
#ifdef HAS_SPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>
#endif

#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

using namespace log_module;
using namespace utility_module;

// Test configurations
static const int WARMUP_ITERATIONS = 1000;
static constexpr const char* TEST_MESSAGE = "Benchmark log message with some data: value={}";

// Initialize loggers once
static void init_thread_system_loggers() {
    static bool initialized = false;
    if (!initialized) {
        // Standard logger
        stop();
        set_title("BenchmarkStandard");
        console_target();
        file_target(log_types::Information);
        start();
        
        initialized = true;
    }
}

#ifdef HAS_SPDLOG
static std::shared_ptr<spdlog::logger> init_spdlog() {
    static std::shared_ptr<spdlog::logger> logger = nullptr;
    if (!logger) {
        // Create basic synchronous file logger
        logger = spdlog::basic_logger_mt("benchmark_logger", "spdlog_benchmark.log");
        logger->set_level(spdlog::level::info);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        
        // Flush on info level for fair comparison
        logger->flush_on(spdlog::level::info);
    }
    return logger;
}

static std::shared_ptr<spdlog::logger> init_spdlog_async() {
    static std::shared_ptr<spdlog::logger> logger = nullptr;
    if (!logger) {
        // Create async logger with 8192 queue size
        spdlog::init_thread_pool(8192, 1);
        logger = spdlog::basic_logger_mt<spdlog::async_factory>("async_logger", "spdlog_async_benchmark.log");
        logger->set_level(spdlog::level::info);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    }
    return logger;
}
#endif

// Benchmark: Simple console output (baseline)
static void BM_ConsoleOutput(benchmark::State& state) {
    // Redirect cout to file for fair comparison
    std::ofstream file("console_benchmark.log", std::ios::app);
    std::streambuf* old_cout = std::cout.rdbuf(file.rdbuf());
    
    int counter = 0;
    for (auto _ : state) {
        std::cout << "[INFO] Benchmark log message with some data: value=" 
                  << counter++ << std::endl;
    }
    
    std::cout.rdbuf(old_cout);
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Thread System Standard Logger
static void BM_ThreadSystemStandard(benchmark::State& state) {
    init_thread_system_loggers();
    
    int counter = 0;
    for (auto _ : state) {
        write_information(L"Benchmark log message with some data: value={}", counter++);
    }
    
    state.SetItemsProcessed(state.iterations());
}

#ifdef HAS_SPDLOG
// Benchmark: spdlog (synchronous)
static void BM_Spdlog(benchmark::State& state) {
    auto logger = init_spdlog();
    
    int counter = 0;
    for (auto _ : state) {
        logger->info(TEST_MESSAGE, counter++);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: spdlog (asynchronous)
static void BM_SpdlogAsync(benchmark::State& state) {
    auto logger = init_spdlog_async();
    
    int counter = 0;
    for (auto _ : state) {
        logger->info(TEST_MESSAGE, counter++);
    }
    
    state.SetItemsProcessed(state.iterations());
}
#endif

// Multi-threaded benchmarks
static void BM_ThreadSystemStandard_MT(benchmark::State& state) {
    init_thread_system_loggers();
    
    if (state.thread_index() == 0) {
        // Setup on first thread
    }
    
    int counter = state.thread_index() * 1000000;
    for (auto _ : state) {
        write_information(L"Thread {} - {}", state.thread_index(), counter++);
    }
    
    state.SetItemsProcessed(state.iterations());
}

#ifdef HAS_SPDLOG
static void BM_Spdlog_MT(benchmark::State& state) {
    auto logger = init_spdlog();
    
    if (state.thread_index() == 0) {
        // Setup on first thread
    }
    
    int counter = state.thread_index() * 1000000;
    for (auto _ : state) {
        logger->info("Thread {} - {}", state.thread_index(), counter++);
    }
    
    state.SetItemsProcessed(state.iterations());
}

static void BM_SpdlogAsync_MT(benchmark::State& state) {
    auto logger = init_spdlog_async();
    
    if (state.thread_index() == 0) {
        // Setup on first thread
    }
    
    int counter = state.thread_index() * 1000000;
    for (auto _ : state) {
        logger->info("Thread {} - {}", state.thread_index(), counter++);
    }
    
    state.SetItemsProcessed(state.iterations());
}
#endif

// Latency-focused benchmarks (single message)
static void BM_ThreadSystemStandard_Latency(benchmark::State& state) {
    init_thread_system_loggers();
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        write_information(L"Latency test message");
        auto end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e9);
    }
    
    state.SetItemsProcessed(state.iterations());
}

#ifdef HAS_SPDLOG
static void BM_Spdlog_Latency(benchmark::State& state) {
    auto logger = init_spdlog();
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        logger->info("Latency test message");
        auto end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e9);
    }
    
    state.SetItemsProcessed(state.iterations());
}

static void BM_SpdlogAsync_Latency(benchmark::State& state) {
    auto logger = init_spdlog_async();
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        logger->info("Latency test message");
        auto end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e9);
    }
    
    state.SetItemsProcessed(state.iterations());
}
#endif

// Register benchmarks
BENCHMARK(BM_ConsoleOutput);
BENCHMARK(BM_ThreadSystemStandard);

#ifdef HAS_SPDLOG
BENCHMARK(BM_Spdlog);
BENCHMARK(BM_SpdlogAsync);
#endif

// Multi-threaded benchmarks
BENCHMARK(BM_ThreadSystemStandard_MT)->Threads(1);
BENCHMARK(BM_ThreadSystemStandard_MT)->Threads(2);
BENCHMARK(BM_ThreadSystemStandard_MT)->Threads(4);
BENCHMARK(BM_ThreadSystemStandard_MT)->Threads(8);

#ifdef HAS_SPDLOG
BENCHMARK(BM_Spdlog_MT)->Threads(1);
BENCHMARK(BM_Spdlog_MT)->Threads(2);
BENCHMARK(BM_Spdlog_MT)->Threads(4);
BENCHMARK(BM_Spdlog_MT)->Threads(8);

BENCHMARK(BM_SpdlogAsync_MT)->Threads(1);
BENCHMARK(BM_SpdlogAsync_MT)->Threads(2);
BENCHMARK(BM_SpdlogAsync_MT)->Threads(4);
BENCHMARK(BM_SpdlogAsync_MT)->Threads(8);
#endif

// Latency benchmarks
BENCHMARK(BM_ThreadSystemStandard_Latency)->UseManualTime();

#ifdef HAS_SPDLOG
BENCHMARK(BM_Spdlog_Latency)->UseManualTime();
BENCHMARK(BM_SpdlogAsync_Latency)->UseManualTime();
#endif

// Main function
BENCHMARK_MAIN();