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
 * @file lockfree_logger_benchmark.cpp
 * @brief Performance benchmarks comparing standard logger vs lock-free logger
 * 
 * This benchmark suite measures:
 * - Single-threaded logging throughput
 * - Multi-threaded logging throughput
 * - Latency characteristics
 * - Scalability with thread count
 * - Message size impact
 */

#include <benchmark/benchmark.h>
#include "../../sources/logger/core/logger_implementation.h"
#include "../../sources/logger/core/lockfree_logger.h"
#include <thread>
#include <vector>
#include <atomic>
#include <random>

using namespace log_module;
using namespace log_module::implementation;

// Test messages of various sizes
static const std::string SHORT_MSG = "Short log message";
static const std::string MEDIUM_MSG = "This is a medium length log message with some additional context and information that might be typical in a real application";
static const std::string LONG_MSG = std::string(1024, 'X'); // 1KB message

// Helper to ensure logger is started
template<typename LoggerType>
void ensure_logger_started(LoggerType& logger) {
    static bool started = false;
    if (!started) {
        logger.set_title("BenchmarkLogger");
        logger.console_target(log_types::None); // Disable console for benchmarks
        logger.file_target(log_types::Information);
        logger.callback_target(log_types::None);
        if (auto error = logger.start(); error.has_value()) {
            throw std::runtime_error("Failed to start logger");
        }
        started = true;
    }
}

// Benchmark: Standard logger - single thread
static void BM_StandardLogger_SingleThread(benchmark::State& state) {
    auto& logger = logger::handle();
    ensure_logger_started(logger);
    
    const std::string& msg = (state.range(0) == 0) ? SHORT_MSG : 
                             (state.range(0) == 1) ? MEDIUM_MSG : LONG_MSG;
    
    for (auto _ : state) {
        logger.write(log_types::Information, "{}", msg);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Lock-free logger - single thread
static void BM_LockfreeLogger_SingleThread(benchmark::State& state) {
    auto& logger = lockfree_logger::handle();
    ensure_logger_started(logger);
    
    const std::string& msg = (state.range(0) == 0) ? SHORT_MSG : 
                             (state.range(0) == 1) ? MEDIUM_MSG : LONG_MSG;
    
    for (auto _ : state) {
        logger.write(log_types::Information, "{}", msg);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Standard logger - multi-threaded
static void BM_StandardLogger_MultiThread(benchmark::State& state) {
    auto& logger = logger::handle();
    ensure_logger_started(logger);
    
    const int thread_count = state.range(0);
    const std::string& msg = MEDIUM_MSG;
    
    if (state.thread_index() == 0) {
        // Reset any shared state if needed
    }
    
    for (auto _ : state) {
        logger.write(log_types::Information, "Thread {} - {}", 
                    state.thread_index(), msg);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Lock-free logger - multi-threaded
static void BM_LockfreeLogger_MultiThread(benchmark::State& state) {
    auto& logger = lockfree_logger::handle();
    ensure_logger_started(logger);
    
    const int thread_count = state.range(0);
    const std::string& msg = MEDIUM_MSG;
    
    if (state.thread_index() == 0) {
        // Reset any shared state if needed
    }
    
    for (auto _ : state) {
        logger.write(log_types::Information, "Thread {} - {}", 
                    state.thread_index(), msg);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Formatted logging comparison
static void BM_StandardLogger_Formatted(benchmark::State& state) {
    auto& logger = logger::handle();
    ensure_logger_started(logger);
    
    int counter = 0;
    for (auto _ : state) {
        logger.write(log_types::Information, 
                    "Message #{} with multiple {} parameters {} and {}", 
                    counter++, "string", 3.14159, true);
    }
    
    state.SetItemsProcessed(state.iterations());
}

static void BM_LockfreeLogger_Formatted(benchmark::State& state) {
    auto& logger = lockfree_logger::handle();
    ensure_logger_started(logger);
    
    int counter = 0;
    for (auto _ : state) {
        logger.write(log_types::Information, 
                    "Message #{} with multiple {} parameters {} and {}", 
                    counter++, "string", 3.14159, true);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Burst logging (simulates real-world bursts)
static void BM_StandardLogger_Burst(benchmark::State& state) {
    auto& logger = logger::handle();
    ensure_logger_started(logger);
    
    const int burst_size = state.range(0);
    
    for (auto _ : state) {
        // Simulate burst of logs
        for (int i = 0; i < burst_size; ++i) {
            logger.write(log_types::Information, "Burst message {}", i);
        }
        
        // Simulate processing between bursts
        benchmark::DoNotOptimize(burst_size);
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    state.SetItemsProcessed(state.iterations() * burst_size);
}

static void BM_LockfreeLogger_Burst(benchmark::State& state) {
    auto& logger = lockfree_logger::handle();
    ensure_logger_started(logger);
    
    const int burst_size = state.range(0);
    
    for (auto _ : state) {
        // Simulate burst of logs
        for (int i = 0; i < burst_size; ++i) {
            logger.write(log_types::Information, "Burst message {}", i);
        }
        
        // Simulate processing between bursts
        benchmark::DoNotOptimize(burst_size);
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    state.SetItemsProcessed(state.iterations() * burst_size);
}

// Benchmark: Mixed log types
static void BM_StandardLogger_MixedTypes(benchmark::State& state) {
    auto& logger = logger::handle();
    ensure_logger_started(logger);
    
    const log_types types[] = {
        log_types::Information,
        log_types::Debug,
        log_types::Error,
        log_types::Exception
    };
    
    int counter = 0;
    for (auto _ : state) {
        auto type = types[counter % 4];
        logger.write(type, "Mixed type message #{}", counter++);
    }
    
    state.SetItemsProcessed(state.iterations());
}

static void BM_LockfreeLogger_MixedTypes(benchmark::State& state) {
    auto& logger = lockfree_logger::handle();
    ensure_logger_started(logger);
    
    const log_types types[] = {
        log_types::Information,
        log_types::Debug,
        log_types::Error,
        log_types::Exception
    };
    
    int counter = 0;
    for (auto _ : state) {
        auto type = types[counter % 4];
        logger.write(type, "Mixed type message #{}", counter++);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Register benchmarks
// Single thread with different message sizes
BENCHMARK(BM_StandardLogger_SingleThread)->Arg(0)->Name("StandardLogger/SingleThread/ShortMsg");
BENCHMARK(BM_StandardLogger_SingleThread)->Arg(1)->Name("StandardLogger/SingleThread/MediumMsg");
BENCHMARK(BM_StandardLogger_SingleThread)->Arg(2)->Name("StandardLogger/SingleThread/LongMsg");

BENCHMARK(BM_LockfreeLogger_SingleThread)->Arg(0)->Name("LockfreeLogger/SingleThread/ShortMsg");
BENCHMARK(BM_LockfreeLogger_SingleThread)->Arg(1)->Name("LockfreeLogger/SingleThread/MediumMsg");
BENCHMARK(BM_LockfreeLogger_SingleThread)->Arg(2)->Name("LockfreeLogger/SingleThread/LongMsg");

// Multi-threaded scalability
BENCHMARK(BM_StandardLogger_MultiThread)->Arg(2)->Threads(2)->Name("StandardLogger/2Threads");
BENCHMARK(BM_StandardLogger_MultiThread)->Arg(4)->Threads(4)->Name("StandardLogger/4Threads");
BENCHMARK(BM_StandardLogger_MultiThread)->Arg(8)->Threads(8)->Name("StandardLogger/8Threads");
BENCHMARK(BM_StandardLogger_MultiThread)->Arg(16)->Threads(16)->Name("StandardLogger/16Threads");

BENCHMARK(BM_LockfreeLogger_MultiThread)->Arg(2)->Threads(2)->Name("LockfreeLogger/2Threads");
BENCHMARK(BM_LockfreeLogger_MultiThread)->Arg(4)->Threads(4)->Name("LockfreeLogger/4Threads");
BENCHMARK(BM_LockfreeLogger_MultiThread)->Arg(8)->Threads(8)->Name("LockfreeLogger/8Threads");
BENCHMARK(BM_LockfreeLogger_MultiThread)->Arg(16)->Threads(16)->Name("LockfreeLogger/16Threads");

// Formatted logging
BENCHMARK(BM_StandardLogger_Formatted)->Name("StandardLogger/Formatted");
BENCHMARK(BM_LockfreeLogger_Formatted)->Name("LockfreeLogger/Formatted");

// Burst logging
BENCHMARK(BM_StandardLogger_Burst)->Arg(10)->Name("StandardLogger/Burst10");
BENCHMARK(BM_StandardLogger_Burst)->Arg(100)->Name("StandardLogger/Burst100");
BENCHMARK(BM_LockfreeLogger_Burst)->Arg(10)->Name("LockfreeLogger/Burst10");
BENCHMARK(BM_LockfreeLogger_Burst)->Arg(100)->Name("LockfreeLogger/Burst100");

// Mixed log types
BENCHMARK(BM_StandardLogger_MixedTypes)->Name("StandardLogger/MixedTypes");
BENCHMARK(BM_LockfreeLogger_MixedTypes)->Name("LockfreeLogger/MixedTypes");

BENCHMARK_MAIN();