/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, kcenon
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
 * @file retry_policy_test.cpp
 * @brief Standalone unit tests for retry_policy class
 * @since 4.0.0
 */

#include <gtest/gtest.h>

#include <kcenon/thread/core/retry_policy.h>

#include <chrono>
#include <string>

using namespace kcenon::thread;
using namespace std::chrono_literals;

// =============================================================================
// Default construction tests
// =============================================================================

TEST(RetryPolicyTest, DefaultConstructorIsNoRetry) {
    retry_policy policy;
    EXPECT_EQ(policy.get_strategy(), retry_strategy::none);
    EXPECT_EQ(policy.get_max_attempts(), 1u);
    EXPECT_EQ(policy.get_initial_delay(), 0ms);
    EXPECT_EQ(policy.get_current_attempt(), 0u);
    EXPECT_FALSE(policy.is_retry_enabled());
    EXPECT_FALSE(policy.uses_jitter());
}

// =============================================================================
// Factory method tests: no_retry
// =============================================================================

TEST(RetryPolicyTest, NoRetryFactory) {
    auto policy = retry_policy::no_retry();
    EXPECT_EQ(policy.get_strategy(), retry_strategy::none);
    EXPECT_FALSE(policy.is_retry_enabled());
    EXPECT_EQ(policy.get_max_attempts(), 1u);
}

// =============================================================================
// Factory method tests: fixed
// =============================================================================

TEST(RetryPolicyTest, FixedFactory) {
    auto policy = retry_policy::fixed(3, 100ms);
    EXPECT_EQ(policy.get_strategy(), retry_strategy::fixed);
    EXPECT_TRUE(policy.is_retry_enabled());
    EXPECT_EQ(policy.get_max_attempts(), 3u);
    EXPECT_EQ(policy.get_initial_delay(), 100ms);
    EXPECT_EQ(policy.get_max_delay(), 100ms);
}

TEST(RetryPolicyTest, FixedWithOneAttemptIsEffectivelyNoRetry) {
    auto policy = retry_policy::fixed(1, 100ms);
    EXPECT_EQ(policy.get_strategy(), retry_strategy::fixed);
    // strategy is fixed but max_attempts is 1, so no actual retry
    EXPECT_FALSE(policy.is_retry_enabled());
}

// =============================================================================
// Factory method tests: linear
// =============================================================================

TEST(RetryPolicyTest, LinearFactory) {
    auto policy = retry_policy::linear(5, 200ms);
    EXPECT_EQ(policy.get_strategy(), retry_strategy::linear);
    EXPECT_TRUE(policy.is_retry_enabled());
    EXPECT_EQ(policy.get_max_attempts(), 5u);
    EXPECT_EQ(policy.get_initial_delay(), 200ms);
}

TEST(RetryPolicyTest, LinearWithCustomMaxDelay) {
    auto policy = retry_policy::linear(5, 100ms, 500ms);
    EXPECT_EQ(policy.get_max_delay(), 500ms);
}

// =============================================================================
// Factory method tests: exponential_backoff
// =============================================================================

TEST(RetryPolicyTest, ExponentialBackoffDefaults) {
    auto policy = retry_policy::exponential_backoff(4);
    EXPECT_EQ(policy.get_strategy(), retry_strategy::exponential_backoff);
    EXPECT_TRUE(policy.is_retry_enabled());
    EXPECT_EQ(policy.get_max_attempts(), 4u);
    EXPECT_EQ(policy.get_initial_delay(), 100ms);
    EXPECT_DOUBLE_EQ(policy.get_multiplier(), 2.0);
    EXPECT_EQ(policy.get_max_delay(), 30000ms);
    EXPECT_FALSE(policy.uses_jitter());
}

TEST(RetryPolicyTest, ExponentialBackoffCustomParams) {
    auto policy = retry_policy::exponential_backoff(5, 50ms, 3.0, 10000ms, true);
    EXPECT_EQ(policy.get_max_attempts(), 5u);
    EXPECT_EQ(policy.get_initial_delay(), 50ms);
    EXPECT_DOUBLE_EQ(policy.get_multiplier(), 3.0);
    EXPECT_EQ(policy.get_max_delay(), 10000ms);
    EXPECT_TRUE(policy.uses_jitter());
}

// =============================================================================
// Attempt tracking tests
// =============================================================================

TEST(RetryPolicyTest, InitialAttemptIsZero) {
    auto policy = retry_policy::fixed(3, 100ms);
    EXPECT_EQ(policy.get_current_attempt(), 0u);
}

TEST(RetryPolicyTest, RecordAttemptIncrements) {
    auto policy = retry_policy::fixed(3, 100ms);
    policy.record_attempt();
    EXPECT_EQ(policy.get_current_attempt(), 1u);
    policy.record_attempt();
    EXPECT_EQ(policy.get_current_attempt(), 2u);
}

TEST(RetryPolicyTest, HasAttemptsRemainingTracksCorrectly) {
    auto policy = retry_policy::fixed(3, 100ms);
    // max_attempts = 3, current = 0 -> remaining = true
    EXPECT_TRUE(policy.has_attempts_remaining());

    policy.record_attempt();  // attempt 1
    EXPECT_TRUE(policy.has_attempts_remaining());

    policy.record_attempt();  // attempt 2
    EXPECT_FALSE(policy.has_attempts_remaining());  // current(2) >= max(3) - 1
}

TEST(RetryPolicyTest, ResetClearsAttemptCounter) {
    auto policy = retry_policy::fixed(3, 100ms);
    policy.record_attempt();
    policy.record_attempt();
    EXPECT_EQ(policy.get_current_attempt(), 2u);

    policy.reset();
    EXPECT_EQ(policy.get_current_attempt(), 0u);
    EXPECT_TRUE(policy.has_attempts_remaining());
}

// =============================================================================
// Delay calculation tests: fixed
// =============================================================================

TEST(RetryPolicyTest, FixedDelayIsConstant) {
    auto policy = retry_policy::fixed(5, 200ms);

    // Attempt 0: no delay (first attempt)
    EXPECT_EQ(policy.get_delay_for_current_attempt(), 0ms);

    policy.record_attempt();
    EXPECT_EQ(policy.get_delay_for_current_attempt(), 200ms);

    policy.record_attempt();
    EXPECT_EQ(policy.get_delay_for_current_attempt(), 200ms);

    policy.record_attempt();
    EXPECT_EQ(policy.get_delay_for_current_attempt(), 200ms);
}

// =============================================================================
// Delay calculation tests: linear
// =============================================================================

TEST(RetryPolicyTest, LinearDelayIncreasesLinearly) {
    auto policy = retry_policy::linear(5, 100ms);

    // Attempt 0: no delay
    EXPECT_EQ(policy.get_delay_for_current_attempt(), 0ms);

    policy.record_attempt();  // attempt 1
    EXPECT_EQ(policy.get_delay_for_current_attempt(), 100ms);

    policy.record_attempt();  // attempt 2
    EXPECT_EQ(policy.get_delay_for_current_attempt(), 200ms);

    policy.record_attempt();  // attempt 3
    EXPECT_EQ(policy.get_delay_for_current_attempt(), 300ms);
}

TEST(RetryPolicyTest, LinearDelayRespectsCap) {
    auto policy = retry_policy::linear(10, 100ms, 250ms);

    policy.record_attempt();  // 1 -> 100ms
    EXPECT_EQ(policy.get_delay_for_current_attempt(), 100ms);

    policy.record_attempt();  // 2 -> 200ms
    EXPECT_EQ(policy.get_delay_for_current_attempt(), 200ms);

    policy.record_attempt();  // 3 -> 300ms capped to 250ms
    EXPECT_EQ(policy.get_delay_for_current_attempt(), 250ms);

    policy.record_attempt();  // 4 -> 400ms capped to 250ms
    EXPECT_EQ(policy.get_delay_for_current_attempt(), 250ms);
}

// =============================================================================
// Delay calculation tests: exponential
// =============================================================================

TEST(RetryPolicyTest, ExponentialDelayDoublesEachAttempt) {
    auto policy = retry_policy::exponential_backoff(5, 100ms, 2.0, 30000ms, false);

    EXPECT_EQ(policy.get_delay_for_current_attempt(), 0ms);  // attempt 0

    policy.record_attempt();  // attempt 1: 100 * 2^0 = 100ms
    EXPECT_EQ(policy.get_delay_for_current_attempt(), 100ms);

    policy.record_attempt();  // attempt 2: 100 * 2^1 = 200ms
    EXPECT_EQ(policy.get_delay_for_current_attempt(), 200ms);

    policy.record_attempt();  // attempt 3: 100 * 2^2 = 400ms
    EXPECT_EQ(policy.get_delay_for_current_attempt(), 400ms);
}

TEST(RetryPolicyTest, ExponentialDelayRespectsCap) {
    auto policy = retry_policy::exponential_backoff(10, 100ms, 2.0, 500ms, false);

    policy.record_attempt();  // 100ms
    policy.record_attempt();  // 200ms
    policy.record_attempt();  // 400ms
    policy.record_attempt();  // 800ms -> capped to 500ms
    EXPECT_EQ(policy.get_delay_for_current_attempt(), 500ms);
}

// =============================================================================
// Delay for none strategy
// =============================================================================

TEST(RetryPolicyTest, NoneStrategyAlwaysReturnsZeroDelay) {
    retry_policy policy;  // none
    EXPECT_EQ(policy.get_delay_for_current_attempt(), 0ms);

    policy.record_attempt();
    EXPECT_EQ(policy.get_delay_for_current_attempt(), 0ms);
}

// =============================================================================
// to_string tests
// =============================================================================

TEST(RetryPolicyTest, ToStringNone) {
    auto policy = retry_policy::no_retry();
    EXPECT_EQ(policy.to_string(), "retry_policy(none)");
}

TEST(RetryPolicyTest, ToStringFixed) {
    auto policy = retry_policy::fixed(3, 100ms);
    auto str = policy.to_string();
    EXPECT_NE(str.find("fixed"), std::string::npos);
    EXPECT_NE(str.find("attempts=3"), std::string::npos);
    EXPECT_NE(str.find("100"), std::string::npos);
}

TEST(RetryPolicyTest, ToStringLinear) {
    auto policy = retry_policy::linear(5, 200ms);
    auto str = policy.to_string();
    EXPECT_NE(str.find("linear"), std::string::npos);
    EXPECT_NE(str.find("attempts=5"), std::string::npos);
}

TEST(RetryPolicyTest, ToStringExponential) {
    auto policy = retry_policy::exponential_backoff(4);
    auto str = policy.to_string();
    EXPECT_NE(str.find("exponential"), std::string::npos);
    EXPECT_NE(str.find("attempts=4"), std::string::npos);
}

// =============================================================================
// Copy and assignment
// =============================================================================

TEST(RetryPolicyTest, CopyPreservesState) {
    auto original = retry_policy::fixed(3, 100ms);
    original.record_attempt();

    retry_policy copy = original;
    EXPECT_EQ(copy.get_strategy(), retry_strategy::fixed);
    EXPECT_EQ(copy.get_max_attempts(), 3u);
    EXPECT_EQ(copy.get_initial_delay(), 100ms);
    EXPECT_EQ(copy.get_current_attempt(), 1u);
}
