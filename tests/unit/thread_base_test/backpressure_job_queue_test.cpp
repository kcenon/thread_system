// BSD 3-Clause License
// Copyright (c) 2026, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file backpressure_job_queue_test.cpp
 * @brief Unit tests for backpressure_job_queue.
 *
 * Targets paths not exercised by the existing
 * integration_tests/integration/backpressure_integration_test.cpp:
 *   - callback policy (all four decisions)
 *   - adaptive policy under varying pressure
 *   - block policy timeout handling
 *   - enqueue_batch for each policy (happy path + oversized batch)
 *   - set_backpressure_config with toggled rate limiting
 *   - to_string, reset_stats
 *   - null/empty argument error paths
 *   - get_available_tokens and is_rate_limited
 */

#include <gtest/gtest.h>

#include <kcenon/thread/core/backpressure_config.h>
#include <kcenon/thread/core/backpressure_job_queue.h>
#include <kcenon/thread/core/callback_job.h>

#include <chrono>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using kcenon::thread::backpressure_config;
using kcenon::thread::backpressure_decision;
using kcenon::thread::backpressure_job_queue;
using kcenon::thread::backpressure_policy;
using kcenon::thread::backpressure_policy_to_string;
using kcenon::thread::callback_job;
using kcenon::thread::job;
using kcenon::thread::pressure_level;
using kcenon::thread::pressure_level_to_string;

namespace {

std::unique_ptr<job> make_noop_job() {
    return std::make_unique<callback_job>(
        []() -> std::optional<std::string> { return std::nullopt; });
}

std::vector<std::unique_ptr<job>> make_noop_batch(std::size_t n) {
    std::vector<std::unique_ptr<job>> batch;
    batch.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        batch.push_back(make_noop_job());
    }
    return batch;
}

backpressure_config make_drop_newest_config() {
    backpressure_config cfg;
    cfg.policy = backpressure_policy::drop_newest;
    cfg.high_watermark = 0.8;
    cfg.low_watermark = 0.5;
    return cfg;
}

} // namespace

// -----------------------------------------------------------------------------
// String conversion helpers
// -----------------------------------------------------------------------------

TEST(BackpressureHelpersTest, PressureLevelToString) {
    EXPECT_EQ(pressure_level_to_string(pressure_level::none), "none");
    EXPECT_EQ(pressure_level_to_string(pressure_level::low), "low");
    EXPECT_EQ(pressure_level_to_string(pressure_level::high), "high");
    EXPECT_EQ(pressure_level_to_string(pressure_level::critical), "critical");
}

TEST(BackpressureHelpersTest, BackpressurePolicyToString) {
    EXPECT_EQ(backpressure_policy_to_string(backpressure_policy::block), "block");
    EXPECT_EQ(backpressure_policy_to_string(backpressure_policy::drop_oldest), "drop_oldest");
    EXPECT_EQ(backpressure_policy_to_string(backpressure_policy::drop_newest), "drop_newest");
    EXPECT_EQ(backpressure_policy_to_string(backpressure_policy::callback), "callback");
    EXPECT_EQ(backpressure_policy_to_string(backpressure_policy::adaptive), "adaptive");
}

// -----------------------------------------------------------------------------
// Config validation
// -----------------------------------------------------------------------------

TEST(BackpressureConfigTest, DefaultsAreValid) {
    backpressure_config cfg;
    EXPECT_TRUE(cfg.is_valid());
}

TEST(BackpressureConfigTest, NegativeLowWatermarkIsInvalid) {
    backpressure_config cfg;
    cfg.low_watermark = -0.1;
    EXPECT_FALSE(cfg.is_valid());
}

TEST(BackpressureConfigTest, HighWatermarkAboveOneIsInvalid) {
    backpressure_config cfg;
    cfg.high_watermark = 1.2;
    EXPECT_FALSE(cfg.is_valid());
}

TEST(BackpressureConfigTest, CallbackPolicyRequiresCallback) {
    backpressure_config cfg;
    cfg.policy = backpressure_policy::callback;
    cfg.decision_callback = nullptr;
    EXPECT_FALSE(cfg.is_valid());

    cfg.decision_callback =
        [](std::unique_ptr<job>&) { return backpressure_decision::reject; };
    EXPECT_TRUE(cfg.is_valid());
}

TEST(BackpressureConfigTest, RateLimitingRequiresPositiveParameters) {
    backpressure_config cfg;
    cfg.enable_rate_limiting = true;
    cfg.rate_limit_tokens_per_second = 0;
    cfg.rate_limit_burst_size = 10;
    EXPECT_FALSE(cfg.is_valid());

    cfg.rate_limit_tokens_per_second = 100;
    cfg.rate_limit_burst_size = 0;
    EXPECT_FALSE(cfg.is_valid());

    cfg.rate_limit_burst_size = 10;
    EXPECT_TRUE(cfg.is_valid());
}

// -----------------------------------------------------------------------------
// Stats snapshot helpers
// -----------------------------------------------------------------------------

TEST(BackpressureStatsTest, AcceptanceRateForEmptyIsOne) {
    kcenon::thread::backpressure_stats_snapshot snap;
    EXPECT_DOUBLE_EQ(snap.acceptance_rate(), 1.0);
    EXPECT_DOUBLE_EQ(snap.avg_block_time_ms(), 0.0);
}

TEST(BackpressureStatsTest, AcceptanceRateRespectsCounters) {
    kcenon::thread::backpressure_stats_snapshot snap;
    snap.jobs_accepted = 75;
    snap.jobs_rejected = 25;
    EXPECT_DOUBLE_EQ(snap.acceptance_rate(), 0.75);
}

TEST(BackpressureStatsTest, AvgBlockTimeUsesRateLimitWaits) {
    kcenon::thread::backpressure_stats_snapshot snap;
    snap.rate_limit_waits = 2;
    snap.total_block_time_ns = 3'000'000; // 3ms
    EXPECT_NEAR(snap.avg_block_time_ms(), 1.5, 1e-6);
}

// -----------------------------------------------------------------------------
// Null / empty arguments
// -----------------------------------------------------------------------------

TEST(BackpressureJobQueueTest, EnqueueNullJobFails) {
    backpressure_job_queue q(10);
    auto result = q.enqueue(nullptr);
    ASSERT_TRUE(result.is_err());
}

TEST(BackpressureJobQueueTest, EnqueueEmptyBatchFails) {
    backpressure_job_queue q(10);
    std::vector<std::unique_ptr<job>> empty;
    auto result = q.enqueue_batch(std::move(empty));
    EXPECT_TRUE(result.is_err());
}

TEST(BackpressureJobQueueTest, EnqueueBatchWithNullJobFails) {
    backpressure_job_queue q(10);
    std::vector<std::unique_ptr<job>> batch;
    batch.push_back(make_noop_job());
    batch.push_back(nullptr);
    batch.push_back(make_noop_job());
    auto result = q.enqueue_batch(std::move(batch));
    EXPECT_TRUE(result.is_err());
}

TEST(BackpressureJobQueueTest, EnqueueAfterStopFails) {
    backpressure_job_queue q(10);
    q.stop();

    auto enq_result = q.enqueue(make_noop_job());
    EXPECT_TRUE(enq_result.is_err());

    auto batch_result = q.enqueue_batch(make_noop_batch(2));
    EXPECT_TRUE(batch_result.is_err());
}

// -----------------------------------------------------------------------------
// Batch enqueue: happy path (fits)
// -----------------------------------------------------------------------------

TEST(BackpressureJobQueueTest, BatchFitsWithinCapacity) {
    backpressure_job_queue q(10, make_drop_newest_config());
    auto result = q.enqueue_batch(make_noop_batch(5));
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(q.size(), 5u);

    auto stats = q.get_backpressure_stats();
    EXPECT_EQ(stats.jobs_accepted, 5u);
}

TEST(BackpressureJobQueueTest, BatchExceedingCapacityRejectedForDropNewest) {
    backpressure_job_queue q(3, make_drop_newest_config());
    auto result = q.enqueue_batch(make_noop_batch(5));
    EXPECT_TRUE(result.is_err());

    auto stats = q.get_backpressure_stats();
    EXPECT_GE(stats.jobs_rejected, 5u);
}

TEST(BackpressureJobQueueTest, BatchDropOldestMakesRoom) {
    backpressure_config cfg;
    cfg.policy = backpressure_policy::drop_oldest;
    backpressure_job_queue q(4, cfg);

    ASSERT_TRUE(q.enqueue_batch(make_noop_batch(4)).is_ok());
    ASSERT_EQ(q.size(), 4u);

    // A 2-job batch needs to evict 2 old jobs.
    auto result = q.enqueue_batch(make_noop_batch(2));
    EXPECT_TRUE(result.is_ok());
    EXPECT_LE(q.size(), 4u);

    auto stats = q.get_backpressure_stats();
    EXPECT_GE(stats.jobs_dropped, 2u);
}

TEST(BackpressureJobQueueTest, BatchBlockPolicyTimesOutWhenFull) {
    backpressure_config cfg;
    cfg.policy = backpressure_policy::block;
    cfg.block_timeout = std::chrono::milliseconds(50);

    backpressure_job_queue q(2, cfg);
    ASSERT_TRUE(q.enqueue_batch(make_noop_batch(2)).is_ok());

    auto before = std::chrono::steady_clock::now();
    auto result = q.enqueue_batch(make_noop_batch(3));
    auto elapsed = std::chrono::steady_clock::now() - before;

    EXPECT_TRUE(result.is_err());
    // Should have waited at least the configured timeout.
    EXPECT_GE(elapsed, std::chrono::milliseconds(40));
}

// -----------------------------------------------------------------------------
// Block policy: single enqueue timeout
// -----------------------------------------------------------------------------

TEST(BackpressureJobQueueTest, BlockPolicyTimesOutWhenFull) {
    backpressure_config cfg;
    cfg.policy = backpressure_policy::block;
    cfg.block_timeout = std::chrono::milliseconds(30);

    backpressure_job_queue q(1, cfg);
    ASSERT_TRUE(q.enqueue(make_noop_job()).is_ok());

    auto before = std::chrono::steady_clock::now();
    auto result = q.enqueue(make_noop_job());
    auto elapsed = std::chrono::steady_clock::now() - before;

    EXPECT_TRUE(result.is_err());
    EXPECT_GE(elapsed, std::chrono::milliseconds(20));
}

// -----------------------------------------------------------------------------
// Callback policy: all four decisions
// -----------------------------------------------------------------------------

TEST(BackpressureJobQueueTest, CallbackPolicyRejectDecision) {
    backpressure_config cfg;
    cfg.policy = backpressure_policy::callback;
    cfg.decision_callback =
        [](std::unique_ptr<job>&) { return backpressure_decision::reject; };

    backpressure_job_queue q(1, cfg);
    ASSERT_TRUE(q.enqueue(make_noop_job()).is_ok());
    ASSERT_TRUE(q.is_full());

    auto result = q.enqueue(make_noop_job());
    EXPECT_TRUE(result.is_err());

    auto stats = q.get_backpressure_stats();
    EXPECT_GE(stats.jobs_rejected, 1u);
}

TEST(BackpressureJobQueueTest, CallbackPolicyAcceptDecisionForcesAccept) {
    backpressure_config cfg;
    cfg.policy = backpressure_policy::callback;
    cfg.decision_callback =
        [](std::unique_ptr<job>&) { return backpressure_decision::accept; };

    backpressure_job_queue q(1, cfg);
    ASSERT_TRUE(q.enqueue(make_noop_job()).is_ok());

    // Accept decision forces enqueue despite queue being full.
    auto result = q.enqueue(make_noop_job());
    EXPECT_TRUE(result.is_ok());
    EXPECT_GE(q.size(), 1u);
}

TEST(BackpressureJobQueueTest, CallbackPolicyDropAndAcceptEvictsOldest) {
    backpressure_config cfg;
    cfg.policy = backpressure_policy::callback;
    cfg.decision_callback =
        [](std::unique_ptr<job>&) {
            return backpressure_decision::drop_and_accept;
        };

    backpressure_job_queue q(2, cfg);
    ASSERT_TRUE(q.enqueue(make_noop_job()).is_ok());
    ASSERT_TRUE(q.enqueue(make_noop_job()).is_ok());
    ASSERT_TRUE(q.is_full());

    auto result = q.enqueue(make_noop_job());
    EXPECT_TRUE(result.is_ok());

    auto stats = q.get_backpressure_stats();
    EXPECT_GE(stats.jobs_dropped, 1u);
}

TEST(BackpressureJobQueueTest, CallbackPolicyDelayDecisionReturnsBusy) {
    backpressure_config cfg;
    cfg.policy = backpressure_policy::callback;
    cfg.decision_callback =
        [](std::unique_ptr<job>&) { return backpressure_decision::delay; };

    backpressure_job_queue q(1, cfg);
    ASSERT_TRUE(q.enqueue(make_noop_job()).is_ok());

    auto result = q.enqueue(make_noop_job());
    EXPECT_TRUE(result.is_err());
}

// -----------------------------------------------------------------------------
// Adaptive policy
// -----------------------------------------------------------------------------

TEST(BackpressureJobQueueTest, AdaptivePolicyAcceptsWhileBelowHighWatermark) {
    backpressure_config cfg;
    cfg.policy = backpressure_policy::adaptive;
    cfg.high_watermark = 0.8;
    cfg.low_watermark = 0.5;

    backpressure_job_queue q(10, cfg);
    for (int i = 0; i < 4; ++i) {
        EXPECT_TRUE(q.enqueue(make_noop_job()).is_ok());
    }
    EXPECT_EQ(q.size(), 4u);
}

TEST(BackpressureJobQueueTest, AdaptivePolicyRejectsOrAcceptsAboveWatermark) {
    backpressure_config cfg;
    cfg.policy = backpressure_policy::adaptive;
    cfg.high_watermark = 0.5;
    cfg.low_watermark = 0.25;

    backpressure_job_queue q(4, cfg);
    // Fill beyond watermark.
    std::size_t accepted = 0;
    for (int i = 0; i < 40; ++i) {
        if (q.enqueue(make_noop_job()).is_ok()) {
            ++accepted;
        }
    }
    // Some jobs must be accepted (below watermark), but not all 40 can be.
    EXPECT_GT(accepted, 0u);
    EXPECT_LT(accepted, 40u);
}

// -----------------------------------------------------------------------------
// Pressure level / ratio on empty or misconfigured queues
// -----------------------------------------------------------------------------

TEST(BackpressureJobQueueTest, EmptyQueueReportsNonePressure) {
    backpressure_job_queue q(5);
    EXPECT_EQ(q.get_pressure_level(), pressure_level::none);
    EXPECT_DOUBLE_EQ(q.get_pressure_ratio(), 0.0);
}

TEST(BackpressureJobQueueTest, FullQueueReportsCritical) {
    backpressure_config cfg;
    cfg.policy = backpressure_policy::drop_newest;

    backpressure_job_queue q(2, cfg);
    ASSERT_TRUE(q.enqueue(make_noop_job()).is_ok());
    ASSERT_TRUE(q.enqueue(make_noop_job()).is_ok());
    EXPECT_EQ(q.get_pressure_level(), pressure_level::critical);
    EXPECT_GE(q.get_pressure_ratio(), 1.0 - 1e-9);
}

// -----------------------------------------------------------------------------
// Statistics reset / to_string
// -----------------------------------------------------------------------------

TEST(BackpressureJobQueueTest, ResetStatsClearsCounters) {
    backpressure_job_queue q(2, make_drop_newest_config());
    ASSERT_TRUE(q.enqueue(make_noop_job()).is_ok());
    ASSERT_TRUE(q.enqueue(make_noop_job()).is_ok());
    auto bad = q.enqueue(make_noop_job()); // triggers reject under drop_newest
    EXPECT_TRUE(bad.is_err());

    auto before = q.get_backpressure_stats();
    EXPECT_GT(before.jobs_accepted + before.jobs_rejected, 0u);

    q.reset_stats();

    auto after = q.get_backpressure_stats();
    EXPECT_EQ(after.jobs_accepted, 0u);
    EXPECT_EQ(after.jobs_rejected, 0u);
    EXPECT_EQ(after.jobs_dropped, 0u);
    EXPECT_EQ(after.pressure_events, 0u);
    EXPECT_EQ(after.rate_limit_waits, 0u);
    EXPECT_EQ(after.total_block_time_ns, 0u);
}

TEST(BackpressureJobQueueTest, ToStringContainsQueueState) {
    backpressure_job_queue q(4, make_drop_newest_config());
    ASSERT_TRUE(q.enqueue(make_noop_job()).is_ok());

    const auto str = q.to_string();
    EXPECT_FALSE(str.empty());
    EXPECT_NE(str.find("backpressure_job_queue"), std::string::npos);
    EXPECT_NE(str.find("drop_newest"), std::string::npos);
}

// -----------------------------------------------------------------------------
// set_backpressure_config
// -----------------------------------------------------------------------------

TEST(BackpressureJobQueueTest, SetConfigSwitchesPolicy) {
    backpressure_job_queue q(2);
    ASSERT_EQ(q.get_backpressure_config().policy, backpressure_policy::block);

    backpressure_config updated = make_drop_newest_config();
    q.set_backpressure_config(updated);
    EXPECT_EQ(q.get_backpressure_config().policy, backpressure_policy::drop_newest);
}

TEST(BackpressureJobQueueTest, SetConfigEnablesAndDisablesRateLimiter) {
    backpressure_job_queue q(5);
    EXPECT_FALSE(q.is_rate_limited());
    EXPECT_EQ(q.get_available_tokens(), std::numeric_limits<std::size_t>::max());

    backpressure_config cfg;
    cfg.policy = backpressure_policy::drop_newest;
    cfg.enable_rate_limiting = true;
    cfg.rate_limit_tokens_per_second = 1000;
    cfg.rate_limit_burst_size = 100;
    q.set_backpressure_config(cfg);

    // Now a token count exists; cannot exceed burst size.
    EXPECT_LE(q.get_available_tokens(), cfg.rate_limit_burst_size);

    // Disable again.
    cfg.enable_rate_limiting = false;
    q.set_backpressure_config(cfg);
    EXPECT_FALSE(q.is_rate_limited());
}

// -----------------------------------------------------------------------------
// Rate limiter single-job path
// -----------------------------------------------------------------------------

TEST(BackpressureJobQueueTest, RateLimitRejectsWhenBurstExhausted) {
    backpressure_config cfg;
    cfg.policy = backpressure_policy::drop_newest;
    cfg.enable_rate_limiting = true;
    cfg.rate_limit_tokens_per_second = 1; // Very slow refill
    cfg.rate_limit_burst_size = 2;
    cfg.block_timeout = std::chrono::milliseconds(20);

    backpressure_job_queue q(100, cfg);

    // Burst allows first 2.
    EXPECT_TRUE(q.enqueue(make_noop_job()).is_ok());
    EXPECT_TRUE(q.enqueue(make_noop_job()).is_ok());

    // Next attempts must wait for tokens; with a tiny timeout, they fail.
    bool any_failed = false;
    for (int i = 0; i < 5; ++i) {
        if (q.enqueue(make_noop_job()).is_err()) {
            any_failed = true;
            break;
        }
    }
    EXPECT_TRUE(any_failed);

    auto stats = q.get_backpressure_stats();
    EXPECT_GE(stats.rate_limit_waits, 1u);
}
