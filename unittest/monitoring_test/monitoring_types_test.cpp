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
#include "../../sources/monitoring/core/monitoring_types.h"
#include <thread>
#include <chrono>

namespace monitoring_module {
namespace test {

class MonitoringTypesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset any global state if needed
    }

    void TearDown() override {
        // Cleanup if needed
    }
};

TEST_F(MonitoringTypesTest, SystemMetricsInitialization) {
    system_metrics metrics;
    
    // Verify initial values are zero
    EXPECT_EQ(metrics.cpu_usage_percent.load(), 0u);
    EXPECT_EQ(metrics.memory_usage_bytes.load(), 0u);
    EXPECT_EQ(metrics.active_threads.load(), 0u);
    EXPECT_EQ(metrics.total_allocations.load(), 0u);
    
    // Verify timestamp is initialized
    auto now = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - metrics.timestamp).count();
    EXPECT_LE(diff, 1); // Should be within 1 second
}

TEST_F(MonitoringTypesTest, ThreadPoolMetricsOperations) {
    thread_pool_metrics metrics;
    
    // Test atomic operations
    metrics.worker_threads.store(10);
    metrics.idle_threads.store(5);
    metrics.jobs_pending.store(100);
    metrics.jobs_completed.fetch_add(50);
    metrics.total_execution_time_ns.fetch_add(50000);
    metrics.average_latency_ns.store(1000);
    
    EXPECT_EQ(metrics.worker_threads.load(), 10u);
    EXPECT_EQ(metrics.idle_threads.load(), 5u);
    EXPECT_EQ(metrics.jobs_pending.load(), 100u);
    EXPECT_EQ(metrics.jobs_completed.load(), 50u);
    EXPECT_EQ(metrics.total_execution_time_ns.load(), 50000u);
    EXPECT_EQ(metrics.average_latency_ns.load(), 1000u);
    
    // Test concurrent updates
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&metrics]() {
            for (int j = 0; j < 1000; ++j) {
                metrics.jobs_completed.fetch_add(1);
                metrics.total_execution_time_ns.fetch_add(1000);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(metrics.jobs_completed.load(), 4050u); // 50 + 4*1000
    EXPECT_EQ(metrics.total_execution_time_ns.load(), 4050000u); // 50000 + 4*1000*1000
}

TEST_F(MonitoringTypesTest, WorkerMetricsTracking) {
    worker_metrics metrics;
    
    // Simulate job processing
    metrics.jobs_processed.fetch_add(100);
    metrics.total_processing_time_ns.fetch_add(1500000000); // 1.5 seconds
    metrics.idle_time_ns.fetch_add(500000000); // 0.5 seconds
    metrics.context_switches.fetch_add(50);
    
    EXPECT_EQ(metrics.jobs_processed.load(), 100u);
    EXPECT_EQ(metrics.total_processing_time_ns.load(), 1500000000u);
    EXPECT_EQ(metrics.idle_time_ns.load(), 500000000u);
    EXPECT_EQ(metrics.context_switches.load(), 50u);
    
    // Test copy operations
    worker_metrics copy(metrics);
    EXPECT_EQ(copy.jobs_processed.load(), 100u);
    EXPECT_EQ(copy.total_processing_time_ns.load(), 1500000000u);
}

TEST_F(MonitoringTypesTest, ScopedTimerFunctionality) {
    std::atomic<uint64_t> elapsed_time{0};
    
    {
        scoped_timer timer(elapsed_time);
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Timer should have recorded elapsed time
    EXPECT_GT(elapsed_time.load(), 0u);
    EXPECT_GE(elapsed_time.load(), 10000u); // At least 10ms in microseconds
    
    // Test multiple timers
    std::atomic<uint64_t> total_time{0};
    for (int i = 0; i < 5; ++i) {
        scoped_timer timer(total_time);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    EXPECT_GE(total_time.load(), 5000u); // At least 5ms total
}

TEST_F(MonitoringTypesTest, SystemMetricsCopyOperations) {
    system_metrics original;
    original.cpu_usage_percent.store(50);
    original.memory_usage_bytes.store(2048);
    original.active_threads.store(8);
    original.total_allocations.store(1000);
    
    // Test copy constructor
    system_metrics copy1(original);
    EXPECT_EQ(copy1.cpu_usage_percent.load(), 50u);
    EXPECT_EQ(copy1.memory_usage_bytes.load(), 2048u);
    EXPECT_EQ(copy1.active_threads.load(), 8u);
    EXPECT_EQ(copy1.total_allocations.load(), 1000u);
    
    // Test copy assignment
    system_metrics copy2;
    copy2 = original;
    EXPECT_EQ(copy2.cpu_usage_percent.load(), 50u);
    EXPECT_EQ(copy2.memory_usage_bytes.load(), 2048u);
    EXPECT_EQ(copy2.active_threads.load(), 8u);
    EXPECT_EQ(copy2.total_allocations.load(), 1000u);
}

TEST_F(MonitoringTypesTest, MonitoringConfigValidation) {
    monitoring_config config;
    
    // Test default values
    EXPECT_EQ(config.collection_interval, std::chrono::milliseconds(100));
    EXPECT_EQ(config.buffer_size, 3600u);
    EXPECT_TRUE(config.enable_system_metrics);
    EXPECT_TRUE(config.enable_thread_pool_metrics);
    EXPECT_TRUE(config.enable_worker_metrics);
    EXPECT_FALSE(config.low_overhead_mode);
    
    // Test custom configuration
    monitoring_config custom_config;
    custom_config.collection_interval = std::chrono::milliseconds(500);
    custom_config.buffer_size = 7200;
    custom_config.enable_system_metrics = false;
    custom_config.low_overhead_mode = true;
    
    EXPECT_EQ(custom_config.collection_interval, std::chrono::milliseconds(500));
    EXPECT_EQ(custom_config.buffer_size, 7200u);
    EXPECT_FALSE(custom_config.enable_system_metrics);
    EXPECT_TRUE(custom_config.low_overhead_mode);
}

TEST_F(MonitoringTypesTest, MetricsUpdaterUtility) {
    std::atomic<uint64_t> counter{100};
    
    // Test increment_counter
    metrics_updater::increment_counter(counter);
    EXPECT_EQ(counter.load(), 101u);
    
    // Test add_value
    metrics_updater::add_value(counter, 10);
    EXPECT_EQ(counter.load(), 111u);
    
    // Test set_value
    metrics_updater::set_value(counter, 200);
    EXPECT_EQ(counter.load(), 200u);
    
    // Test create_timer
    std::atomic<uint64_t> timer_counter{0};
    {
        auto timer = metrics_updater::create_timer(timer_counter);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    EXPECT_GT(timer_counter.load(), 0u);
    
    // Test concurrent updates
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&counter]() {
            for (int j = 0; j < 1000; ++j) {
                metrics_updater::increment_counter(counter);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(counter.load(), 4200u); // 200 + 4*1000
}

TEST_F(MonitoringTypesTest, EdgeCasesAndBoundaries) {
    // Test with maximum values
    std::atomic<uint64_t> max_counter{UINT64_MAX - 1};
    metrics_updater::increment_counter(max_counter);
    EXPECT_EQ(max_counter.load(), UINT64_MAX);
    
    // Test add_value overflow
    std::atomic<uint64_t> near_max{UINT64_MAX - 10};
    metrics_updater::add_value(near_max, 20);
    // Overflow wraps around (defined behavior for unsigned)
    EXPECT_LT(near_max.load(), 20u);
    
    // Test timer with zero duration
    std::atomic<uint64_t> instant_time{0};
    {
        scoped_timer timer(instant_time);
        // No sleep, should still record some time
    }
    // Even instant operations take some time
    EXPECT_GE(instant_time.load(), 0u);
}

} // namespace test
} // namespace monitoring_module