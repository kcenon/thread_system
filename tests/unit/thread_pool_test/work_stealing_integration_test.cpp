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
 * @brief Unit tests for thread_pool work-stealing integration using work_stealing_pool_policy
 *
 * This file tests the integration of work-stealing features via the policy pattern,
 * including:
 * - Adding work_stealing_pool_policy to thread_pool
 * - Enabling/disabling work stealing at runtime
 * - Work stealing with job execution
 */

#include "gtest/gtest.h"

#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/thread_worker.h>
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/pool_policies/work_stealing_pool_policy.h>

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
// Policy Addition Tests
// ============================================================================

TEST_F(WorkStealingIntegrationTest, AddWorkStealingPolicy) {
    worker_policy config;
    config.enable_work_stealing = true;

    auto policy = std::make_unique<work_stealing_pool_policy>(config);
    pool_->add_policy(std::move(policy));

    auto* ws = pool_->find_policy<work_stealing_pool_policy>("work_stealing_pool_policy");
    ASSERT_NE(ws, nullptr);
    EXPECT_TRUE(ws->is_enabled());
}

TEST_F(WorkStealingIntegrationTest, PolicyDisabledByDefault) {
    auto policy = std::make_unique<work_stealing_pool_policy>();
    pool_->add_policy(std::move(policy));

    auto* ws = pool_->find_policy<work_stealing_pool_policy>("work_stealing_pool_policy");
    ASSERT_NE(ws, nullptr);
    EXPECT_FALSE(ws->is_enabled());
}

TEST_F(WorkStealingIntegrationTest, EnableDisableAtRuntime) {
    auto policy = std::make_unique<work_stealing_pool_policy>();
    pool_->add_policy(std::move(policy));

    auto* ws = pool_->find_policy<work_stealing_pool_policy>("work_stealing_pool_policy");
    ASSERT_NE(ws, nullptr);

    // Initially disabled
    EXPECT_FALSE(ws->is_enabled());

    // Enable
    ws->set_enabled(true);
    EXPECT_TRUE(ws->is_enabled());

    // Disable
    ws->set_enabled(false);
    EXPECT_FALSE(ws->is_enabled());
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(WorkStealingIntegrationTest, ConfigureStealPolicy) {
    worker_policy config;
    config.enable_work_stealing = true;
    config.victim_selection = steal_policy::adaptive;

    auto policy = std::make_unique<work_stealing_pool_policy>(config);
    pool_->add_policy(std::move(policy));

    auto* ws = pool_->find_policy<work_stealing_pool_policy>("work_stealing_pool_policy");
    ASSERT_NE(ws, nullptr);
    EXPECT_EQ(ws->get_steal_policy(), steal_policy::adaptive);

    // Change policy at runtime
    ws->set_steal_policy(steal_policy::round_robin);
    EXPECT_EQ(ws->get_steal_policy(), steal_policy::round_robin);
}

TEST_F(WorkStealingIntegrationTest, ConfigureMaxStealAttempts) {
    worker_policy config;
    config.enable_work_stealing = true;
    config.max_steal_attempts = 10;

    auto policy = std::make_unique<work_stealing_pool_policy>(config);
    pool_->add_policy(std::move(policy));

    auto* ws = pool_->find_policy<work_stealing_pool_policy>("work_stealing_pool_policy");
    ASSERT_NE(ws, nullptr);
    EXPECT_EQ(ws->get_max_steal_attempts(), 10);

    // Change at runtime
    ws->set_max_steal_attempts(5);
    EXPECT_EQ(ws->get_max_steal_attempts(), 5);
}

TEST_F(WorkStealingIntegrationTest, ConfigureStealBackoff) {
    worker_policy config;
    config.enable_work_stealing = true;
    config.steal_backoff = std::chrono::microseconds(500);

    auto policy = std::make_unique<work_stealing_pool_policy>(config);
    pool_->add_policy(std::move(policy));

    auto* ws = pool_->find_policy<work_stealing_pool_policy>("work_stealing_pool_policy");
    ASSERT_NE(ws, nullptr);
    EXPECT_EQ(ws->get_steal_backoff(), std::chrono::microseconds(500));
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(WorkStealingIntegrationTest, StatsInitiallyZero) {
    auto policy = std::make_unique<work_stealing_pool_policy>();
    pool_->add_policy(std::move(policy));

    auto* ws = pool_->find_policy<work_stealing_pool_policy>("work_stealing_pool_policy");
    ASSERT_NE(ws, nullptr);

    EXPECT_EQ(ws->get_successful_steals(), 0);
    EXPECT_EQ(ws->get_failed_steals(), 0);
}

TEST_F(WorkStealingIntegrationTest, RecordAndResetStats) {
    auto policy = std::make_unique<work_stealing_pool_policy>();
    pool_->add_policy(std::move(policy));

    auto* ws = pool_->find_policy<work_stealing_pool_policy>("work_stealing_pool_policy");
    ASSERT_NE(ws, nullptr);

    // Record some steals
    ws->record_successful_steal();
    ws->record_successful_steal();
    ws->record_failed_steal();

    EXPECT_EQ(ws->get_successful_steals(), 2);
    EXPECT_EQ(ws->get_failed_steals(), 1);

    // Reset
    ws->reset_stats();
    EXPECT_EQ(ws->get_successful_steals(), 0);
    EXPECT_EQ(ws->get_failed_steals(), 0);
}

// ============================================================================
// Functional Tests with Jobs
// ============================================================================

TEST_F(WorkStealingIntegrationTest, WorkStealingWithJobs) {
    // Add work stealing policy
    worker_policy config;
    config.enable_work_stealing = true;
    config.victim_selection = steal_policy::adaptive;
    pool_->add_policy(std::make_unique<work_stealing_pool_policy>(config));

    // Add multiple workers
    for (int i = 0; i < 4; ++i) {
        pool_->enqueue(std::make_unique<thread_worker>());
    }

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
    // Add work stealing policy
    worker_policy config;
    config.enable_work_stealing = true;
    pool_->add_policy(std::make_unique<work_stealing_pool_policy>(config));

    // Add workers
    for (int i = 0; i < 4; ++i) {
        pool_->enqueue(std::make_unique<thread_worker>());
    }

    // Start and immediately stop
    pool_->start();
    auto stop_result = pool_->stop();
    EXPECT_FALSE(stop_result.is_err());
    EXPECT_FALSE(pool_->is_running());
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(WorkStealingIntegrationTest, PolicyBeforeWorkers) {
    // Add policy before workers
    worker_policy config;
    config.enable_work_stealing = true;
    pool_->add_policy(std::make_unique<work_stealing_pool_policy>(config));

    // Add workers after
    for (int i = 0; i < 2; ++i) {
        pool_->enqueue(std::make_unique<thread_worker>());
    }

    // Start should still work
    auto result = pool_->start();
    EXPECT_FALSE(result.is_err());
}

TEST_F(WorkStealingIntegrationTest, RemovePolicy) {
    // Add policy
    worker_policy config;
    config.enable_work_stealing = true;
    pool_->add_policy(std::make_unique<work_stealing_pool_policy>(config));

    // Verify it exists
    auto* ws = pool_->find_policy<work_stealing_pool_policy>("work_stealing_pool_policy");
    ASSERT_NE(ws, nullptr);

    // Remove it
    bool removed = pool_->remove_policy("work_stealing_pool_policy");
    EXPECT_TRUE(removed);

    // Verify it's gone
    ws = pool_->find_policy<work_stealing_pool_policy>("work_stealing_pool_policy");
    EXPECT_EQ(ws, nullptr);
}

TEST_F(WorkStealingIntegrationTest, UpdatePolicyConfig) {
    // Add policy with initial config
    worker_policy config;
    config.enable_work_stealing = true;
    config.max_steal_attempts = 3;
    pool_->add_policy(std::make_unique<work_stealing_pool_policy>(config));

    // Start pool
    for (int i = 0; i < 2; ++i) {
        pool_->enqueue(std::make_unique<thread_worker>());
    }
    pool_->start();

    // Update config while running
    auto* ws = pool_->find_policy<work_stealing_pool_policy>("work_stealing_pool_policy");
    ASSERT_NE(ws, nullptr);

    worker_policy new_config;
    new_config.enable_work_stealing = true;
    new_config.max_steal_attempts = 10;
    new_config.victim_selection = steal_policy::round_robin;
    ws->set_policy(new_config);

    // Verify changes
    EXPECT_EQ(ws->get_max_steal_attempts(), 10);
    EXPECT_EQ(ws->get_steal_policy(), steal_policy::round_robin);

    // Pool should still be functional
    EXPECT_TRUE(pool_->is_running());
}
