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
 * @file lockfree_logger_sample.cpp
 * @brief Demonstrates the high-performance lock-free logger implementation
 * 
 * This sample shows:
 * - Lock-free logger setup and configuration
 * - Performance comparison with standard logger
 * - High-concurrency logging scenarios
 * - Queue statistics monitoring
 */

#include "../../sources/logger/core/lockfree_logger.h"
#include "../../sources/logger/core/logger_implementation.h"
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>
#include <atomic>

using namespace log_module;
using namespace log_module::implementation;
using namespace std::chrono_literals;

// Performance test function
template<typename LoggerType>
auto performance_test(LoggerType& logger, const std::string& test_name, 
                     size_t thread_count, size_t messages_per_thread) -> void
{
    std::cout << "\n=== " << test_name << " Performance Test ===" << std::endl;
    std::cout << "Threads: " << thread_count << ", Messages per thread: " 
              << messages_per_thread << std::endl;
    
    std::atomic<size_t> total_messages{0};
    std::vector<std::thread> threads;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Launch threads
    for (size_t t = 0; t < thread_count; ++t)
    {
        threads.emplace_back([&logger, &total_messages, t, messages_per_thread]() {
            for (size_t i = 0; i < messages_per_thread; ++i)
            {
                logger.write(log_types::Information, 
                           "Thread {} - Message {}: High-performance logging test", 
                           t, i);
                total_messages.fetch_add(1);
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads)
    {
        t.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    
    std::cout << "Total messages: " << total_messages.load() << std::endl;
    std::cout << "Total time: " << duration.count() << " ms" << std::endl;
    std::cout << "Throughput: " 
              << (total_messages.load() * 1000.0 / duration.count()) 
              << " messages/sec" << std::endl;
}

int main()
{
    std::cout << "Lock-Free Logger Sample" << std::endl;
    std::cout << "=======================" << std::endl;
    
    // Configure lock-free logger
    auto& lockfree_log = lockfree_logger::handle();
    lockfree_log.set_title("LockFreeLoggerSample");
    lockfree_log.console_target(log_types::Information);
    lockfree_log.file_target(log_types::Information);
    
    // Configure callback for monitoring
    size_t callback_count = 0;
    lockfree_log.message_callback(
        [&callback_count](const log_types& type, 
                          const std::string& datetime, 
                          const std::string& message) {
            callback_count++;
        });
    lockfree_log.callback_target(log_types::Information);
    
    // Start the logger
    if (auto error = lockfree_log.start(); error.has_value())
    {
        std::cerr << "Failed to start lock-free logger: " << *error << std::endl;
        return 1;
    }
    
    // Basic logging test
    std::cout << "\n1. Basic Logging Test" << std::endl;
    lockfree_log.write(log_types::Information, "Lock-free logger initialized");
    lockfree_log.write(log_types::Debug, "Debug message - should not appear");
    lockfree_log.write(log_types::Error, "Error message example");
    lockfree_log.write(log_types::Exception, "Exception message example");
    
    // Give time for messages to be processed
    std::this_thread::sleep_for(100ms);
    
    // Performance comparison
    std::cout << "\n2. Performance Comparison" << std::endl;
    
    // Test lock-free logger
    performance_test(lockfree_log, "Lock-Free Logger", 8, 10000);
    
    // Wait for all messages to be processed
    std::this_thread::sleep_for(500ms);
    
    // Show statistics
    std::cout << "\n3. Lock-Free Logger Statistics" << std::endl;
    std::cout << "Callback messages received: " << callback_count << std::endl;
    
    // If we had access to the internal collector, we could show queue stats:
    // auto stats = lockfree_log.get_queue_statistics();
    // std::cout << "Average enqueue latency: " << stats.get_average_enqueue_latency_ns() << " ns" << std::endl;
    
    // Stress test with high concurrency
    std::cout << "\n4. High Concurrency Stress Test" << std::endl;
    performance_test(lockfree_log, "Lock-Free Logger (High Concurrency)", 16, 5000);
    
    // Wait for processing
    std::this_thread::sleep_for(1s);
    
    // Cleanup
    std::cout << "\n5. Cleanup" << std::endl;
    lockfree_log.stop();
    lockfree_logger::destroy();
    
    std::cout << "\nLock-free logger sample completed successfully!" << std::endl;
    std::cout << "Total callbacks received: " << callback_count << std::endl;
    
    return 0;
}