/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, kcenon
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
 * @file work_stealing_pool_policy_test.cpp
 * @brief Unit tests for work_stealing_pool_policy (Issue #491)
 *
 * This file tests the work_stealing_pool_policy class which extracts
 * work-stealing functionality from thread_pool into a composable policy.
 */

#include "gtest/gtest.h"

#include <kcenon/thread/pool_policies/work_stealing_pool_policy.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/thread_worker.h>
#include <kcenon/thread/core/callback_job.h>

#include <atomic>
#include <chrono>
#include <thread>

using namespace kcenon::thread;
using namespace kcenon;

// ============================================================================
// Construction Tests
// ============================================================================

TEST(WorkStealingPoolPolicyTest, DefaultConstruction) {
    work_stealing_pool_policy policy;

    EXPECT_EQ(policy.get_name(), "work_stealing_pool_policy");
    // Default worker_policy has work_stealing disabled
    EXPECT_FALSE(policy.is_enabled());
}

TEST(WorkStealingPoolPolicyTest, ConstructWithConfig) {
    worker_policy config;
    config.enable_work_stealing = true;
    config.victim_selection = steal_policy::adaptive;
    config.max_steal_attempts = 10;
    config.steal_backoff = std::chrono::microseconds{100};

    work_stealing_pool_policy policy(config);

    EXPECT_TRUE(policy.is_enabled());
    EXPECT_EQ(policy.get_steal_policy(), steal_policy::adaptive);
    EXPECT_EQ(policy.get_max_steal_attempts(), 10);
    EXPECT_EQ(policy.get_steal_backoff(), std::chrono::microseconds{100});
}

TEST(WorkStealingPoolPolicyTest, ConstructWithHighPerformanceConfig) {
    work_stealing_pool_policy policy(worker_policy::high_performance());

    EXPECT_TRUE(policy.is_enabled());
}

TEST(WorkStealingPoolPolicyTest, ConstructWithPowerEfficientConfig) {
    work_stealing_pool_policy policy(worker_policy::power_efficient());

    EXPECT_FALSE(policy.is_enabled());
}

// ============================================================================
// Enable/Disable Tests
// ============================================================================

TEST(WorkStealingPoolPolicyTest, EnableDisable) {
    work_stealing_pool_policy policy;

    EXPECT_FALSE(policy.is_enabled());

    policy.set_enabled(true);
    EXPECT_TRUE(policy.is_enabled());

    policy.set_enabled(false);
    EXPECT_FALSE(policy.is_enabled());
}

TEST(WorkStealingPoolPolicyTest, EnableDisablePolicySync) {
    worker_policy config;
    config.enable_work_stealing = true;

    work_stealing_pool_policy policy(config);
    EXPECT_TRUE(policy.is_enabled());
    EXPECT_TRUE(policy.get_policy().enable_work_stealing);

    policy.set_enabled(false);
    EXPECT_FALSE(policy.is_enabled());
    EXPECT_FALSE(policy.get_policy().enable_work_stealing);
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST(WorkStealingPoolPolicyTest, SetPolicy) {
    work_stealing_pool_policy policy;

    worker_policy new_config;
    new_config.enable_work_stealing = true;
    new_config.victim_selection = steal_policy::round_robin;
    new_config.max_steal_attempts = 7;

    policy.set_policy(new_config);

    EXPECT_TRUE(policy.is_enabled());
    EXPECT_EQ(policy.get_steal_policy(), steal_policy::round_robin);
    EXPECT_EQ(policy.get_max_steal_attempts(), 7);
}

TEST(WorkStealingPoolPolicyTest, SetStealPolicy) {
    work_stealing_pool_policy policy;

    policy.set_steal_policy(steal_policy::adaptive);
    EXPECT_EQ(policy.get_steal_policy(), steal_policy::adaptive);

    policy.set_steal_policy(steal_policy::round_robin);
    EXPECT_EQ(policy.get_steal_policy(), steal_policy::round_robin);

    policy.set_steal_policy(steal_policy::random);
    EXPECT_EQ(policy.get_steal_policy(), steal_policy::random);
}

TEST(WorkStealingPoolPolicyTest, SetMaxStealAttempts) {
    work_stealing_pool_policy policy;

    policy.set_max_steal_attempts(5);
    EXPECT_EQ(policy.get_max_steal_attempts(), 5);

    policy.set_max_steal_attempts(100);
    EXPECT_EQ(policy.get_max_steal_attempts(), 100);
}

TEST(WorkStealingPoolPolicyTest, SetStealBackoff) {
    work_stealing_pool_policy policy;

    policy.set_steal_backoff(std::chrono::microseconds{200});
    EXPECT_EQ(policy.get_steal_backoff(), std::chrono::microseconds{200});

    policy.set_steal_backoff(std::chrono::microseconds{0});
    EXPECT_EQ(policy.get_steal_backoff(), std::chrono::microseconds{0});
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST(WorkStealingPoolPolicyTest, InitialStatsAreZero) {
    work_stealing_pool_policy policy;

    EXPECT_EQ(policy.get_successful_steals(), 0);
    EXPECT_EQ(policy.get_failed_steals(), 0);
}

TEST(WorkStealingPoolPolicyTest, RecordSuccessfulSteals) {
    work_stealing_pool_policy policy;

    policy.record_successful_steal();
    EXPECT_EQ(policy.get_successful_steals(), 1);

    policy.record_successful_steal();
    policy.record_successful_steal();
    EXPECT_EQ(policy.get_successful_steals(), 3);
}

TEST(WorkStealingPoolPolicyTest, RecordFailedSteals) {
    work_stealing_pool_policy policy;

    policy.record_failed_steal();
    EXPECT_EQ(policy.get_failed_steals(), 1);

    policy.record_failed_steal();
    policy.record_failed_steal();
    EXPECT_EQ(policy.get_failed_steals(), 3);
}

TEST(WorkStealingPoolPolicyTest, ResetStats) {
    work_stealing_pool_policy policy;

    policy.record_successful_steal();
    policy.record_successful_steal();
    policy.record_failed_steal();

    EXPECT_EQ(policy.get_successful_steals(), 2);
    EXPECT_EQ(policy.get_failed_steals(), 1);

    policy.reset_stats();

    EXPECT_EQ(policy.get_successful_steals(), 0);
    EXPECT_EQ(policy.get_failed_steals(), 0);
}

// ============================================================================
// Pool Policy Interface Tests
// ============================================================================

TEST(WorkStealingPoolPolicyTest, OnEnqueueDoesNotRejectJobs) {
    work_stealing_pool_policy policy;

    auto job = std::make_unique<callback_job>(
        []() -> common::VoidResult { return common::ok(); },
        "test_job"
    );

    auto result = policy.on_enqueue(*job);
    EXPECT_FALSE(result.is_err());
}

TEST(WorkStealingPoolPolicyTest, OnJobStartAndCompleteDoNotThrow) {
    work_stealing_pool_policy policy;

    auto job = std::make_unique<callback_job>(
        []() -> common::VoidResult { return common::ok(); },
        "test_job"
    );

    // These should not throw
    EXPECT_NO_THROW(policy.on_job_start(*job));
    EXPECT_NO_THROW(policy.on_job_complete(*job, true));
    EXPECT_NO_THROW(policy.on_job_complete(*job, false));
    EXPECT_NO_THROW(policy.on_job_complete(*job, false, nullptr));
}

// ============================================================================
// Thread Pool Integration Tests
// ============================================================================

class WorkStealingPoolPolicyIntegrationTest : public ::testing::Test {
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

TEST_F(WorkStealingPoolPolicyIntegrationTest, AddPolicyToPool) {
    worker_policy config;
    config.enable_work_stealing = true;

    auto policy = std::make_unique<work_stealing_pool_policy>(config);
    pool_->add_policy(std::move(policy));

    // Find the policy
    auto* ws = pool_->find_policy<work_stealing_pool_policy>("work_stealing_pool_policy");
    ASSERT_NE(ws, nullptr);
    EXPECT_TRUE(ws->is_enabled());
}

TEST_F(WorkStealingPoolPolicyIntegrationTest, RemovePolicyFromPool) {
    auto policy = std::make_unique<work_stealing_pool_policy>();
    pool_->add_policy(std::move(policy));

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

TEST_F(WorkStealingPoolPolicyIntegrationTest, PolicyWorksDuringJobExecution) {
    // Add workers
    for (int i = 0; i < 4; ++i) {
        pool_->enqueue(std::make_unique<thread_worker>());
    }

    // Add work-stealing policy
    worker_policy config;
    config.enable_work_stealing = true;
    auto policy = std::make_unique<work_stealing_pool_policy>(config);
    pool_->add_policy(std::move(policy));

    // Start pool
    auto start_result = pool_->start();
    EXPECT_FALSE(start_result.is_err());

    // Submit jobs
    std::atomic<int> completed{0};
    constexpr int job_count = 50;

    for (int i = 0; i < job_count; ++i) {
        auto job = std::make_unique<callback_job>(
            [&completed]() -> common::VoidResult {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
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

    // Verify policy is still accessible
    auto* ws = pool_->find_policy<work_stealing_pool_policy>("work_stealing_pool_policy");
    ASSERT_NE(ws, nullptr);
    EXPECT_TRUE(ws->is_enabled());
}

TEST_F(WorkStealingPoolPolicyIntegrationTest, ConfigureViaPolicy) {
    // Add policy with specific configuration
    worker_policy config;
    config.enable_work_stealing = true;
    config.victim_selection = steal_policy::adaptive;
    config.max_steal_attempts = 10;

    auto policy = std::make_unique<work_stealing_pool_policy>(config);
    pool_->add_policy(std::move(policy));

    // Add workers and start
    for (int i = 0; i < 2; ++i) {
        pool_->enqueue(std::make_unique<thread_worker>());
    }
    pool_->start();

    // Verify configuration through policy
    auto* ws = pool_->find_policy<work_stealing_pool_policy>("work_stealing_pool_policy");
    ASSERT_NE(ws, nullptr);
    EXPECT_EQ(ws->get_steal_policy(), steal_policy::adaptive);
    EXPECT_EQ(ws->get_max_steal_attempts(), 10);
}

TEST_F(WorkStealingPoolPolicyIntegrationTest, DisablePolicyAtRuntime) {
    // Add workers
    for (int i = 0; i < 2; ++i) {
        pool_->enqueue(std::make_unique<thread_worker>());
    }

    // Add enabled policy
    worker_policy config;
    config.enable_work_stealing = true;
    auto policy = std::make_unique<work_stealing_pool_policy>(config);
    pool_->add_policy(std::move(policy));

    // Start pool
    pool_->start();

    // Disable policy at runtime
    auto* ws = pool_->find_policy<work_stealing_pool_policy>("work_stealing_pool_policy");
    ASSERT_NE(ws, nullptr);

    ws->set_enabled(false);
    EXPECT_FALSE(ws->is_enabled());

    // Re-enable
    ws->set_enabled(true);
    EXPECT_TRUE(ws->is_enabled());
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST(WorkStealingPoolPolicyTest, ConcurrentStatUpdates) {
    work_stealing_pool_policy policy;

    std::vector<std::thread> threads;
    constexpr int thread_count = 4;
    constexpr int updates_per_thread = 1000;

    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([&policy, i]() {
            for (int j = 0; j < updates_per_thread; ++j) {
                if (i % 2 == 0) {
                    policy.record_successful_steal();
                } else {
                    policy.record_failed_steal();
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Half threads recorded successful, half recorded failed
    EXPECT_EQ(policy.get_successful_steals(), (thread_count / 2) * updates_per_thread);
    EXPECT_EQ(policy.get_failed_steals(), (thread_count / 2) * updates_per_thread);
}

TEST(WorkStealingPoolPolicyTest, ConcurrentEnableDisable) {
    work_stealing_pool_policy policy;

    std::atomic<bool> stop{false};
    std::vector<std::thread> threads;

    // Reader threads
    for (int i = 0; i < 2; ++i) {
        threads.emplace_back([&policy, &stop]() {
            while (!stop.load()) {
                [[maybe_unused]] bool enabled = policy.is_enabled();
                std::this_thread::yield();
            }
        });
    }

    // Writer threads
    for (int i = 0; i < 2; ++i) {
        threads.emplace_back([&policy, &stop, i]() {
            while (!stop.load()) {
                policy.set_enabled(i % 2 == 0);
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    stop.store(true);

    for (auto& t : threads) {
        t.join();
    }

    // Should not crash or have undefined behavior
    SUCCEED();
}
