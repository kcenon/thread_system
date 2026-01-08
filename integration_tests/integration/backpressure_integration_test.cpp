/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, DongCheol Shin
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

#include <kcenon/thread/core/backpressure_job_queue.h>
#include <kcenon/thread/core/backpressure_config.h>
#include <kcenon/thread/core/token_bucket.h>
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/core/thread_pool.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace kcenon::thread;

/**
 * @brief Integration tests for backpressure mechanisms
 *
 * Goal: Verify backpressure_job_queue behavior under various scenarios
 * Expected time: < 30 seconds
 * Test scenarios:
 *   1. Token bucket rate limiting
 *   2. Watermark-based pressure detection
 *   3. Various backpressure policies
 *   4. Thread pool integration with backpressure queue
 */
class BackpressureIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        completed_jobs_.store(0);
        rejected_jobs_.store(0);
    }

    void TearDown() override {
        std::this_thread::yield();
    }

    template <typename Predicate>
    bool WaitForCondition(Predicate pred,
                          std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
        auto start = std::chrono::steady_clock::now();
        while (!pred()) {
            if (std::chrono::steady_clock::now() - start > timeout) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return true;
    }

    std::atomic<int> completed_jobs_{0};
    std::atomic<int> rejected_jobs_{0};
};

// ============================================================================
// Token Bucket Tests
// ============================================================================

TEST_F(BackpressureIntegrationTest, TokenBucketBasicAcquisition) {
    // Create bucket: 100 tokens/sec, burst of 10
    token_bucket bucket(100, 10);

    // Should be able to acquire all burst tokens immediately
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(bucket.try_acquire());
    }

    // Next acquisition should fail (bucket empty)
    EXPECT_FALSE(bucket.try_acquire());

    // Wait for refill (1 token = 10ms at 100 tokens/sec)
    std::this_thread::sleep_for(std::chrono::milliseconds(15));

    // Should be able to acquire again
    EXPECT_TRUE(bucket.try_acquire());
}

TEST_F(BackpressureIntegrationTest, TokenBucketWithTimeout) {
    // Create bucket: 1000 tokens/sec, burst of 5
    token_bucket bucket(1000, 5);

    // Exhaust bucket
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(bucket.try_acquire());
    }

    // Try to acquire with timeout (should succeed after refill)
    auto start = std::chrono::steady_clock::now();
    EXPECT_TRUE(bucket.try_acquire_for(1, std::chrono::milliseconds(100)));
    auto elapsed = std::chrono::steady_clock::now() - start;

    // Should have waited some time (but not too long)
    EXPECT_GE(elapsed, std::chrono::milliseconds(1));
    EXPECT_LT(elapsed, std::chrono::milliseconds(50));
}

// ============================================================================
// Backpressure Queue Tests
// ============================================================================

TEST_F(BackpressureIntegrationTest, BackpressureQueuePressureLevels) {
    backpressure_config config;
    config.policy = backpressure_policy::drop_newest;
    config.high_watermark = 0.8;
    config.low_watermark = 0.5;

    auto queue = std::make_shared<backpressure_job_queue>(10, config);

    // Initially, pressure should be none
    EXPECT_EQ(queue->get_pressure_level(), pressure_level::none);
    EXPECT_LT(queue->get_pressure_ratio(), 0.1);

    // Add jobs to reach low watermark (50% = 5 jobs)
    for (int i = 0; i < 5; ++i) {
        auto job_ptr = std::make_unique<callback_job>(
            [this]() -> std::optional<std::string> { completed_jobs_.fetch_add(1); return std::nullopt; });
        EXPECT_TRUE(queue->enqueue(std::move(job_ptr)).is_ok());
    }

    // Should be at low pressure
    EXPECT_EQ(queue->get_pressure_level(), pressure_level::low);

    // Add more jobs to reach high watermark (80% = 8 jobs)
    for (int i = 0; i < 3; ++i) {
        auto job_ptr = std::make_unique<callback_job>(
            [this]() -> std::optional<std::string> { completed_jobs_.fetch_add(1); return std::nullopt; });
        EXPECT_TRUE(queue->enqueue(std::move(job_ptr)).is_ok());
    }

    // Should be at high pressure
    EXPECT_EQ(queue->get_pressure_level(), pressure_level::high);

    // Fill to capacity
    for (int i = 0; i < 2; ++i) {
        auto job_ptr = std::make_unique<callback_job>(
            [this]() -> std::optional<std::string> { completed_jobs_.fetch_add(1); return std::nullopt; });
        EXPECT_TRUE(queue->enqueue(std::move(job_ptr)).is_ok());
    }

    // Should be critical
    EXPECT_EQ(queue->get_pressure_level(), pressure_level::critical);
}

TEST_F(BackpressureIntegrationTest, DropNewestPolicy) {
    backpressure_config config;
    config.policy = backpressure_policy::drop_newest;

    auto queue = std::make_shared<backpressure_job_queue>(5, config);

    // Fill the queue
    for (int i = 0; i < 5; ++i) {
        auto job_ptr = std::make_unique<callback_job>(
            [this]() -> std::optional<std::string> { completed_jobs_.fetch_add(1); return std::nullopt; });
        EXPECT_TRUE(queue->enqueue(std::move(job_ptr)).is_ok());
    }

    // Next enqueue should fail (drop_newest = reject new job)
    auto job_ptr = std::make_unique<callback_job>(
        [this]() -> std::optional<std::string> { rejected_jobs_.fetch_add(1); return std::nullopt; });
    auto result = queue->enqueue(std::move(job_ptr));
    EXPECT_TRUE(result.is_err());

    // Queue size should still be 5
    EXPECT_EQ(queue->size(), 5);
}

TEST_F(BackpressureIntegrationTest, DropOldestPolicy) {
    backpressure_config config;
    config.policy = backpressure_policy::drop_oldest;

    auto queue = std::make_shared<backpressure_job_queue>(5, config);

    // Fill the queue with marker jobs
    for (int i = 0; i < 5; ++i) {
        auto job_ptr = std::make_unique<callback_job>(
            [i, this]() -> std::optional<std::string> { completed_jobs_.store(i); return std::nullopt; });
        EXPECT_TRUE(queue->enqueue(std::move(job_ptr)).is_ok());
    }

    // Add new job (should drop oldest)
    auto job_ptr = std::make_unique<callback_job>(
        [this]() -> std::optional<std::string> { completed_jobs_.store(100); return std::nullopt; });
    auto result = queue->enqueue(std::move(job_ptr));
    EXPECT_TRUE(result.is_ok());

    // Queue size should still be 5
    EXPECT_EQ(queue->size(), 5);

    // Stats should show dropped job
    auto stats = queue->get_backpressure_stats();
    EXPECT_GE(stats.jobs_dropped, 1);
}

TEST_F(BackpressureIntegrationTest, RateLimitingIntegration) {
    backpressure_config config;
    config.policy = backpressure_policy::block;
    config.enable_rate_limiting = true;
    config.rate_limit_tokens_per_second = 100;
    config.rate_limit_burst_size = 5;
    config.block_timeout = std::chrono::milliseconds(500);

    auto queue = std::make_shared<backpressure_job_queue>(100, config);

    // Enqueue burst (should succeed immediately)
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 5; ++i) {
        auto job_ptr = std::make_unique<callback_job>(
            [this]() -> std::optional<std::string> { completed_jobs_.fetch_add(1); return std::nullopt; });
        EXPECT_TRUE(queue->enqueue(std::move(job_ptr)).is_ok());
    }
    auto burst_time = std::chrono::steady_clock::now() - start;

    // Burst should be fast
    EXPECT_LT(burst_time, std::chrono::milliseconds(50));

    // Next enqueue should wait for rate limit refill
    start = std::chrono::steady_clock::now();
    auto job_ptr = std::make_unique<callback_job>(
        [this]() -> std::optional<std::string> { completed_jobs_.fetch_add(1); return std::nullopt; });
    EXPECT_TRUE(queue->enqueue(std::move(job_ptr)).is_ok());
    auto wait_time = std::chrono::steady_clock::now() - start;

    // Should have waited for token refill
    EXPECT_GE(wait_time, std::chrono::milliseconds(5));

    // Check stats
    auto stats = queue->get_backpressure_stats();
    EXPECT_GE(stats.rate_limit_waits, 1);
}

TEST_F(BackpressureIntegrationTest, StatisticsTracking) {
    backpressure_config config;
    config.policy = backpressure_policy::drop_newest;

    auto queue = std::make_shared<backpressure_job_queue>(5, config);

    // Enqueue some jobs
    for (int i = 0; i < 5; ++i) {
        auto job_ptr = std::make_unique<callback_job>(
            [this]() -> std::optional<std::string> { completed_jobs_.fetch_add(1); return std::nullopt; });
        EXPECT_TRUE(queue->enqueue(std::move(job_ptr)).is_ok());
    }

    // Try to enqueue when full (should be rejected)
    for (int i = 0; i < 3; ++i) {
        auto job_ptr = std::make_unique<callback_job>(
            [this]() -> std::optional<std::string> { rejected_jobs_.fetch_add(1); return std::nullopt; });
        auto result = queue->enqueue(std::move(job_ptr));
        EXPECT_TRUE(result.is_err());
    }

    // Check statistics
    auto stats = queue->get_backpressure_stats();
    EXPECT_EQ(stats.jobs_accepted, 5);
    EXPECT_EQ(stats.jobs_rejected, 3);

    // Acceptance rate should be 5/8 = 0.625
    double expected_rate = 5.0 / 8.0;
    EXPECT_NEAR(stats.acceptance_rate(), expected_rate, 0.01);

    // Reset and verify
    queue->reset_stats();
    stats = queue->get_backpressure_stats();
    EXPECT_EQ(stats.jobs_accepted, 0);
    EXPECT_EQ(stats.jobs_rejected, 0);
}

// ============================================================================
// Thread Pool Integration Tests
// ============================================================================

TEST_F(BackpressureIntegrationTest, ThreadPoolWithBackpressureQueue) {
    // Create backpressure queue
    backpressure_config config;
    config.policy = backpressure_policy::drop_newest;
    config.high_watermark = 0.8;

    auto bp_queue = std::make_shared<backpressure_job_queue>(100, config);

    // Create thread pool with custom queue
    auto pool = std::make_shared<thread_pool>("BackpressurePool", bp_queue);

    // Add workers
    for (int i = 0; i < 4; ++i) {
        auto worker = std::make_unique<thread_worker>();
        worker->set_job_queue(bp_queue);
        pool->enqueue(std::move(worker));
    }

    // Start pool
    ASSERT_TRUE(pool->start().is_ok());

    // Submit jobs
    for (int i = 0; i < 50; ++i) {
        auto job_ptr = std::make_unique<callback_job>(
            [this]() -> std::optional<std::string> {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                completed_jobs_.fetch_add(1);
                return std::nullopt;
            });
        pool->enqueue(std::move(job_ptr));
    }

    // Wait for completion
    EXPECT_TRUE(WaitForCondition([this]() {
        return completed_jobs_.load() >= 50;
    }, std::chrono::seconds(10)));

    // Stop pool
    EXPECT_TRUE(pool->stop().is_ok());

    // Verify all jobs completed
    EXPECT_GE(completed_jobs_.load(), 50);
}

TEST_F(BackpressureIntegrationTest, PressureCallbackInvocation) {
    std::atomic<int> callback_count{0};
    std::atomic<double> last_ratio{0.0};

    backpressure_config config;
    config.policy = backpressure_policy::drop_newest;
    config.high_watermark = 0.8;
    config.low_watermark = 0.5;
    config.pressure_callback = [&callback_count, &last_ratio](
        std::size_t /*depth*/, double ratio) {
        callback_count.fetch_add(1);
        last_ratio.store(ratio);
    };

    auto queue = std::make_shared<backpressure_job_queue>(10, config);

    // Fill to high watermark
    for (int i = 0; i < 9; ++i) {
        auto job_ptr = std::make_unique<callback_job>(
            []() -> std::optional<std::string> { return std::nullopt; });
        auto result = queue->enqueue(std::move(job_ptr));
        (void)result;  // Suppress nodiscard warning
    }

    // Callback should have been invoked when crossing high watermark
    EXPECT_GE(callback_count.load(), 1);
    EXPECT_GT(last_ratio.load(), 0.7);
}

// ============================================================================
// Configuration Validation Tests
// ============================================================================

TEST_F(BackpressureIntegrationTest, ConfigurationValidation) {
    backpressure_config config;

    // Valid configuration
    config.low_watermark = 0.5;
    config.high_watermark = 0.8;
    EXPECT_TRUE(config.is_valid());

    // Invalid: low >= high
    config.low_watermark = 0.9;
    config.high_watermark = 0.8;
    EXPECT_FALSE(config.is_valid());

    // Invalid: out of range
    config.low_watermark = 1.5;
    EXPECT_FALSE(config.is_valid());

    // Invalid: callback policy without callback
    config.low_watermark = 0.5;
    config.high_watermark = 0.8;
    config.policy = backpressure_policy::callback;
    config.decision_callback = nullptr;
    EXPECT_FALSE(config.is_valid());

    // Valid callback policy with callback
    config.decision_callback = [](std::unique_ptr<job>&) {
        return backpressure_decision::accept;
    };
    EXPECT_TRUE(config.is_valid());
}

TEST_F(BackpressureIntegrationTest, ToStringOutput) {
    backpressure_config config;
    config.policy = backpressure_policy::adaptive;

    auto queue = std::make_shared<backpressure_job_queue>(100, config);

    std::string str = queue->to_string();

    // Should contain key information
    EXPECT_NE(str.find("backpressure_job_queue"), std::string::npos);
    EXPECT_NE(str.find("adaptive"), std::string::npos);
}
