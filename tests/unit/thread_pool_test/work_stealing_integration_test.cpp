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
 * @file work_stealing_integration_test.cpp
 * @brief Unit tests for thread_pool work-stealing integration (Issue #427)
 *
 * This file tests the integration of enhanced work-stealing features
 * into thread_pool, including:
 * - set_work_stealing_config() / get_work_stealing_config()
 * - get_work_stealing_stats()
 * - get_numa_topology()
 */

#include "gtest/gtest.h"

#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/thread_worker.h>
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/stealing/enhanced_work_stealing_config.h>
#include <kcenon/thread/stealing/work_stealing_stats.h>
#include <kcenon/thread/stealing/numa_topology.h>

#include <atomic>
#include <chrono>
#include <thread>

using namespace kcenon::thread;
using namespace kcenon;

class WorkStealingIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool_ = std::make_shared<thread_pool>("TestPool");
    }

    void TearDown() override {
        if (pool_) {
            pool_->stop();
        }
    }

    std::shared_ptr<thread_pool> pool_;
};

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(WorkStealingIntegrationTest, GetDefaultConfig) {
    // Default config should have work-stealing disabled
    const auto& config = pool_->get_work_stealing_config();
    EXPECT_FALSE(config.enabled);
}

TEST_F(WorkStealingIntegrationTest, SetAndGetConfig) {
    auto config = enhanced_work_stealing_config::numa_optimized();
    pool_->set_work_stealing_config(config);

    const auto& retrieved = pool_->get_work_stealing_config();
    EXPECT_TRUE(retrieved.enabled);
    EXPECT_EQ(retrieved.policy, enhanced_steal_policy::numa_aware);
    EXPECT_TRUE(retrieved.numa_aware);
    EXPECT_TRUE(retrieved.prefer_same_node);
}

TEST_F(WorkStealingIntegrationTest, SetBatchOptimizedConfig) {
    auto config = enhanced_work_stealing_config::batch_optimized();
    pool_->set_work_stealing_config(config);

    const auto& retrieved = pool_->get_work_stealing_config();
    EXPECT_TRUE(retrieved.enabled);
    EXPECT_EQ(retrieved.policy, enhanced_steal_policy::adaptive);
    EXPECT_EQ(retrieved.min_steal_batch, 2);
    EXPECT_EQ(retrieved.max_steal_batch, 8);
    EXPECT_TRUE(retrieved.adaptive_batch_size);
}

TEST_F(WorkStealingIntegrationTest, SetLocalityOptimizedConfig) {
    auto config = enhanced_work_stealing_config::locality_optimized();
    pool_->set_work_stealing_config(config);

    const auto& retrieved = pool_->get_work_stealing_config();
    EXPECT_TRUE(retrieved.enabled);
    EXPECT_EQ(retrieved.policy, enhanced_steal_policy::locality_aware);
    EXPECT_TRUE(retrieved.track_locality);
}

TEST_F(WorkStealingIntegrationTest, DisableConfig) {
    // First enable
    pool_->set_work_stealing_config(enhanced_work_stealing_config::numa_optimized());
    EXPECT_TRUE(pool_->get_work_stealing_config().enabled);

    // Then disable
    enhanced_work_stealing_config disabled_config;
    disabled_config.enabled = false;
    pool_->set_work_stealing_config(disabled_config);
    EXPECT_FALSE(pool_->get_work_stealing_config().enabled);
}

// ============================================================================
// NUMA Topology Tests
// ============================================================================

TEST_F(WorkStealingIntegrationTest, GetNumaTopology) {
    const auto& topology = pool_->get_numa_topology();

    // Should have at least one node
    EXPECT_GE(topology.node_count(), 1);

    // Should have at least one CPU
    EXPECT_GE(topology.cpu_count(), 1);
}

TEST_F(WorkStealingIntegrationTest, NumaTopologyConsistency) {
    // Multiple calls should return consistent results
    const auto& topology1 = pool_->get_numa_topology();
    const auto& topology2 = pool_->get_numa_topology();

    EXPECT_EQ(topology1.node_count(), topology2.node_count());
    EXPECT_EQ(topology1.cpu_count(), topology2.cpu_count());
}

TEST_F(WorkStealingIntegrationTest, NumaTopologyCpuMapping) {
    const auto& topology = pool_->get_numa_topology();

    // Each CPU should map to a valid node
    for (std::size_t cpu = 0; cpu < topology.cpu_count(); ++cpu) {
        int node = topology.get_node_for_cpu(static_cast<int>(cpu));
        EXPECT_GE(node, 0);
        EXPECT_LT(node, static_cast<int>(topology.node_count()));
    }
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(WorkStealingIntegrationTest, GetDefaultStats) {
    // Default stats should be zero
    auto stats = pool_->get_work_stealing_stats();
    EXPECT_EQ(stats.steal_attempts, 0);
    EXPECT_EQ(stats.successful_steals, 0);
    EXPECT_EQ(stats.failed_steals, 0);
}

TEST_F(WorkStealingIntegrationTest, StatsAfterEnabling) {
    // Add workers first
    for (int i = 0; i < 4; ++i) {
        pool_->enqueue(std::make_unique<thread_worker>());
    }

    // Start pool
    pool_->start();

    // Enable work stealing with statistics collection
    auto config = enhanced_work_stealing_config::numa_optimized();
    config.collect_statistics = true;
    pool_->set_work_stealing_config(config);

    // Stats should be available (may still be zero if no stealing occurred)
    auto stats = pool_->get_work_stealing_stats();
    EXPECT_GE(stats.steal_attempts, 0);
}

TEST_F(WorkStealingIntegrationTest, StatsComputedMetrics) {
    auto stats = pool_->get_work_stealing_stats();

    // Computed metrics should return valid values even with zero data
    EXPECT_GE(stats.steal_success_rate(), 0.0);
    EXPECT_LE(stats.steal_success_rate(), 1.0);
    EXPECT_GE(stats.avg_batch_size(), 0.0);
    EXPECT_GE(stats.cross_node_ratio(), 0.0);
    EXPECT_LE(stats.cross_node_ratio(), 1.0);
}

// ============================================================================
// Integration with Worker Policy Tests
// ============================================================================

TEST_F(WorkStealingIntegrationTest, ConfigOverridesWorkerPolicy) {
    // Set basic work stealing via worker policy
    worker_policy policy;
    policy.enable_work_stealing = true;
    pool_->set_worker_policy(policy);
    EXPECT_TRUE(pool_->is_work_stealing_enabled());

    // Enhanced config should override
    enhanced_work_stealing_config config;
    config.enabled = false;
    pool_->set_work_stealing_config(config);
    EXPECT_FALSE(pool_->is_work_stealing_enabled());

    // Re-enable via enhanced config
    config.enabled = true;
    pool_->set_work_stealing_config(config);
    EXPECT_TRUE(pool_->is_work_stealing_enabled());
}

// ============================================================================
// Functional Tests with Jobs
// ============================================================================

TEST_F(WorkStealingIntegrationTest, WorkStealingWithJobs) {
    // Add multiple workers
    for (int i = 0; i < 4; ++i) {
        pool_->enqueue(std::make_unique<thread_worker>());
    }

    // Enable work stealing
    auto config = enhanced_work_stealing_config::batch_optimized();
    config.collect_statistics = true;
    pool_->set_work_stealing_config(config);

    // Start pool
    auto start_result = pool_->start();
    EXPECT_FALSE(start_result.is_err());

    // Submit jobs
    std::atomic<int> completed{0};
    constexpr int job_count = 100;

    for (int i = 0; i < job_count; ++i) {
        auto job = std::make_unique<callback_job>(
            [&completed]() -> common::VoidResult {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                completed.fetch_add(1, std::memory_order_relaxed);
                return common::ok();
            },
            "test_job_" + std::to_string(i)
        );
        pool_->enqueue(std::move(job));
    }

    // Wait for completion
    constexpr int max_wait_ms = 5000;
    int waited = 0;
    while (completed.load() < job_count && waited < max_wait_ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        waited += 10;
    }

    EXPECT_EQ(completed.load(), job_count);

    // Check that pool is still healthy
    EXPECT_TRUE(pool_->is_running());
}

TEST_F(WorkStealingIntegrationTest, WorkStealingDoesNotBreakShutdown) {
    // Add workers
    for (int i = 0; i < 4; ++i) {
        pool_->enqueue(std::make_unique<thread_worker>());
    }

    // Enable work stealing
    pool_->set_work_stealing_config(enhanced_work_stealing_config::numa_optimized());

    // Start and immediately stop
    pool_->start();
    auto stop_result = pool_->stop();
    EXPECT_FALSE(stop_result.is_err());
    EXPECT_FALSE(pool_->is_running());
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(WorkStealingIntegrationTest, ConfigBeforeWorkers) {
    // Set config before adding workers
    pool_->set_work_stealing_config(enhanced_work_stealing_config::numa_optimized());

    // Add workers after
    for (int i = 0; i < 2; ++i) {
        pool_->enqueue(std::make_unique<thread_worker>());
    }

    // Start should still work
    auto result = pool_->start();
    EXPECT_FALSE(result.is_err());
}

TEST_F(WorkStealingIntegrationTest, ReconfigureWhileRunning) {
    // Add workers and start
    for (int i = 0; i < 4; ++i) {
        pool_->enqueue(std::make_unique<thread_worker>());
    }
    pool_->start();

    // Reconfigure multiple times while running
    pool_->set_work_stealing_config(enhanced_work_stealing_config::numa_optimized());
    pool_->set_work_stealing_config(enhanced_work_stealing_config::batch_optimized());
    pool_->set_work_stealing_config(enhanced_work_stealing_config::locality_optimized());

    // Pool should still be functional
    EXPECT_TRUE(pool_->is_running());
    EXPECT_TRUE(pool_->get_work_stealing_config().enabled);
}
