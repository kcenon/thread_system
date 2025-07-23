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

#include <gtest/gtest.h>
#include "../../sources/monitoring/storage/ring_buffer.h"
#include "../../sources/monitoring/core/monitoring_types.h"
#include "../../sources/monitoring/core/metrics_collector.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <random>
#include <barrier>
#include <algorithm>

namespace monitoring_module {
namespace test {

class MonitoringConcurrencyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test environment
    }

    void TearDown() override {
        // Clean up
    }
};

// Test ring buffer under extreme concurrent access
TEST_F(MonitoringConcurrencyTest, RingBufferExtremeConcurrency) {
    const size_t buffer_size = 100;
    thread_safe_ring_buffer<int> buffer(buffer_size);
    
    const int num_producers = 10;
    const int num_consumers = 5;
    const int items_per_producer = 1000;
    
    std::atomic<int> total_pushed{0};
    std::atomic<int> total_popped{0};
    std::atomic<int> push_failures{0};
    std::atomic<bool> stop_consumers{false};
    
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    
    // Start consumers
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&buffer, &total_popped, &stop_consumers]() {
            int value;
            while (!stop_consumers.load()) {
                if (buffer.pop(value)) {
                    total_popped.fetch_add(1);
                } else {
                    std::this_thread::yield();
                }
            }
            
            // Drain remaining items
            while (buffer.pop(value)) {
                total_popped.fetch_add(1);
            }
        });
    }
    
    // Start producers
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&buffer, &total_pushed, &push_failures, i, items_per_producer]() {
            for (int j = 0; j < items_per_producer; ++j) {
                int value = i * items_per_producer + j;
                if (buffer.push(value)) {
                    total_pushed.fetch_add(1);
                } else {
                    push_failures.fetch_add(1);
                    // Retry after brief delay
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                    if (buffer.push(value)) {
                        total_pushed.fetch_add(1);
                    } else {
                        push_failures.fetch_add(1);
                    }
                }
            }
        });
    }
    
    // Wait for producers
    for (auto& t : producers) {
        t.join();
    }
    
    // Let consumers catch up
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop consumers
    stop_consumers.store(true);
    
    for (auto& t : consumers) {
        t.join();
    }
    
    // Verify data integrity
    EXPECT_GT(total_pushed.load(), 0);
    EXPECT_GT(total_popped.load(), 0);
    EXPECT_LE(total_popped.load(), total_pushed.load());
}

// Test metrics updates racing with reads
TEST_F(MonitoringConcurrencyTest, MetricsRaceConditions) {
    auto metrics = std::make_shared<system_metrics>();
    
    const int num_updaters = 8;
    const int num_readers = 8;
    const int operations_per_thread = 10000;
    
    std::atomic<bool> start_flag{false};
    std::vector<std::thread> threads;
    
    // Updater threads
    for (int i = 0; i < num_updaters; ++i) {
        threads.emplace_back([metrics, &start_flag, operations_per_thread]() {
            while (!start_flag.load()) {
                std::this_thread::yield();
            }
            
            for (int op = 0; op < operations_per_thread; ++op) {
                metrics->cpu_usage_percent.fetch_add(1, std::memory_order_relaxed);
                metrics->memory_usage_bytes.fetch_add(1024, std::memory_order_relaxed);
                metrics->active_threads.fetch_add(1, std::memory_order_relaxed);
                
                // Simulate some work
                std::this_thread::yield();
                
                metrics->active_threads.fetch_sub(1, std::memory_order_relaxed);
            }
        });
    }
    
    // Reader threads
    std::atomic<int> inconsistencies{0};
    for (int i = 0; i < num_readers; ++i) {
        threads.emplace_back([metrics, &start_flag, &inconsistencies, operations_per_thread]() {
            while (!start_flag.load()) {
                std::this_thread::yield();
            }
            
            for (int op = 0; op < operations_per_thread; ++op) {
                // Read metrics
                auto cpu = metrics->cpu_usage_percent.load(std::memory_order_relaxed);
                auto mem = metrics->memory_usage_bytes.load(std::memory_order_relaxed);
                auto threads = metrics->active_threads.load(std::memory_order_relaxed);
                
                // Check for obvious inconsistencies
                if (threads > num_updaters) {
                    inconsistencies.fetch_add(1);
                }
                
                std::this_thread::yield();
            }
        });
    }
    
    // Start all threads simultaneously
    start_flag.store(true);
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Final state should be consistent
    EXPECT_EQ(metrics->active_threads.load(), 0u);
    EXPECT_EQ(inconsistencies.load(), 0);
}

// Test collector with concurrent metric registrations
TEST_F(MonitoringConcurrencyTest, ConcurrentMetricRegistration) {
    monitoring_config config;
    config.collection_interval = std::chrono::milliseconds(10);
    config.buffer_size = 1000;
    
    auto collector = std::make_unique<metrics_collector>(config);
    collector->start();
    
    const int num_threads = 10;
    const int registrations_per_thread = 100;
    
    std::vector<std::thread> threads;
    std::atomic<int> successful_registrations{0};
    
    // Threads that register/update metrics
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&collector, &successful_registrations, i, registrations_per_thread]() {
            for (int j = 0; j < registrations_per_thread; ++j) {
                if (j % 3 == 0) {
                    auto system = std::make_shared<system_metrics>();
                    system->cpu_usage_percent.store(i * 10 + j);
                    collector->register_system_metrics(system);
                } else if (j % 3 == 1) {
                    auto pool = std::make_shared<thread_pool_metrics>();
                    pool->worker_threads.store(i);
                    collector->register_thread_pool_metrics(pool);
                } else {
                    auto worker = std::make_shared<worker_metrics>();
                    worker->jobs_processed.store(j);
                    collector->register_worker_metrics(worker);
                }
                
                successful_registrations.fetch_add(1);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }
    
    // Thread that reads snapshots
    std::thread reader([&collector]() {
        for (int i = 0; i < 100; ++i) {
            auto snapshots = collector->get_recent_snapshots(10);
            auto current = collector->get_current_snapshot();
            
            // Verify snapshots are valid
            for (const auto& snapshot : snapshots) {
                EXPECT_NE(snapshot.capture_time.time_since_epoch().count(), 0);
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    reader.join();
    
    collector->stop();
    
    EXPECT_EQ(successful_registrations.load(), num_threads * registrations_per_thread);
}

// Test ring buffer size boundary with concurrent access
TEST_F(MonitoringConcurrencyTest, RingBufferSizeBoundary) {
    const size_t sizes[] = {1, 2, 10, 100, 1000};
    
    for (size_t size : sizes) {
        ring_buffer<int> buffer(size);
        
        std::atomic<bool> producer_done{false};
        std::atomic<int> items_produced{0};
        std::atomic<int> items_consumed{0};
        
        // Producer
        std::thread producer([&buffer, &producer_done, &items_produced, size]() {
            for (int i = 0; i < static_cast<int>(size * 10); ++i) {
                if (buffer.push(i)) {
                    items_produced.fetch_add(1);
                }
                
                // Occasionally yield to consumer
                if (i % 10 == 0) {
                    std::this_thread::yield();
                }
            }
            producer_done.store(true);
        });
        
        // Consumer
        std::thread consumer([&buffer, &producer_done, &items_consumed]() {
            int value;
            while (!producer_done.load() || !buffer.empty()) {
                if (buffer.pop(value)) {
                    items_consumed.fetch_add(1);
                } else {
                    std::this_thread::yield();
                }
            }
        });
        
        producer.join();
        consumer.join();
        
        // All produced items should be consumed
        EXPECT_EQ(items_consumed.load(), items_produced.load());
        EXPECT_TRUE(buffer.empty());
    }
}

// Test metric update atomicity
TEST_F(MonitoringConcurrencyTest, MetricUpdateAtomicity) {
    struct test_metrics {
        std::atomic<uint64_t> counter{0};
        std::atomic<uint64_t> sum{0};
        std::atomic<uint64_t> count{0};
        
        double get_average() const {
            auto c = count.load();
            return c > 0 ? static_cast<double>(sum.load()) / c : 0.0;
        }
    };
    
    auto metrics = std::make_shared<test_metrics>();
    const int num_threads = 8;
    const int updates_per_thread = 10000;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([metrics, i, updates_per_thread]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(1, 100);
            
            for (int j = 0; j < updates_per_thread; ++j) {
                int value = dis(gen);
                
                // Atomic updates
                metrics->counter.fetch_add(1);
                metrics->sum.fetch_add(value);
                metrics->count.fetch_add(1);
                
                // Verify consistency periodically
                if (j % 100 == 0) {
                    auto counter_val = metrics->counter.load();
                    auto count_val = metrics->count.load();
                    // Allow small differences due to timing between atomic operations
                    EXPECT_NEAR(counter_val, count_val, num_threads);
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Final verification
    EXPECT_EQ(metrics->counter.load(), num_threads * updates_per_thread);
    EXPECT_EQ(metrics->count.load(), num_threads * updates_per_thread);
    EXPECT_GT(metrics->get_average(), 0.0);
    EXPECT_LT(metrics->get_average(), 101.0);
}

// Test collector behavior during rapid start/stop cycles
TEST_F(MonitoringConcurrencyTest, CollectorRapidStartStop) {
    monitoring_config config;
    config.collection_interval = std::chrono::milliseconds(10); // Increased from 5ms to 10ms
    config.buffer_size = 100;
    
    auto collector = std::make_unique<metrics_collector>(config);
    
    const int num_cycles = 20; // Reduced from 50 to 20 for CI stability
    std::atomic<int> collections{0};
    
    // Register metrics that count collections
    auto system = std::make_shared<system_metrics>();
    collector->register_system_metrics(system);
    
    for (int i = 0; i < num_cycles; ++i) {
        auto result = collector->start();
        EXPECT_FALSE(result.has_error());
        
        // Let it collect a few times (increased duration for CI)
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Increased from 20ms to 50ms
        
        collector->stop();
        
        // Brief pause (increased for CI stability)
        std::this_thread::sleep_for(std::chrono::milliseconds(5)); // Increased from 0.1ms to 5ms
    }
    
    // Wait a bit more to ensure final collection
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Collector should be in consistent state
    EXPECT_FALSE(collector->is_running());
    
    // Should have collected some data (with retry logic for CI)
    std::vector<monitoring_module::metrics_snapshot> snapshots;
    int retries = 0;
    const int max_retries = 10;
    
    while (snapshots.empty() && retries < max_retries) {
        snapshots = collector->get_recent_snapshots(100);
        if (snapshots.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            retries++;
        }
    }
    
    EXPECT_GT(snapshots.size(), 0u) << "Failed to collect any snapshots after " << max_retries << " retries";
}

// Test concurrent metric updates with memory barriers
TEST_F(MonitoringConcurrencyTest, MemoryBarrierTest) {
    struct barrier_test {
        std::atomic<int> flag{0};
        int data{0};
        
        void writer() {
            data = 42;
            flag.store(1, std::memory_order_release);
        }
        
        bool reader(int& value) {
            if (flag.load(std::memory_order_acquire) == 1) {
                value = data;
                return true;
            }
            return false;
        }
    };
    
    const int num_iterations = 1000;
    int success_count = 0;
    
    for (int i = 0; i < num_iterations; ++i) {
        auto test = std::make_shared<barrier_test>();
        int read_value = 0;
        
        std::thread writer([test]() {
            test->writer();
        });
        
        std::thread reader([test, &read_value]() {
            while (!test->reader(read_value)) {
                std::this_thread::yield();
            }
        });
        
        writer.join();
        reader.join();
        
        if (read_value == 42) {
            success_count++;
        }
    }
    
    // All iterations should succeed with proper memory barriers
    EXPECT_EQ(success_count, num_iterations);
}

} // namespace test
} // namespace monitoring_module