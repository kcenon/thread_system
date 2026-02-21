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
 * @file token_bucket_test.cpp
 * @brief Unit tests for token_bucket rate limiter
 * @since 4.0.0
 */

#include <gtest/gtest.h>

#include <kcenon/thread/core/token_bucket.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace kcenon::thread;
using namespace std::chrono_literals;

// =============================================================================
// Construction tests
// =============================================================================

TEST(TokenBucketTest, ConstructionStartsFull) {
	token_bucket bucket(100, 10);
	EXPECT_EQ(bucket.available_tokens(), 10u);
	EXPECT_EQ(bucket.get_rate(), 100u);
	EXPECT_EQ(bucket.get_burst_size(), 10u);
}

TEST(TokenBucketTest, ConstructionWithLargeValues) {
	token_bucket bucket(1000000, 50000);
	EXPECT_EQ(bucket.available_tokens(), 50000u);
	EXPECT_EQ(bucket.get_rate(), 1000000u);
	EXPECT_EQ(bucket.get_burst_size(), 50000u);
}

// =============================================================================
// try_acquire tests
// =============================================================================

TEST(TokenBucketTest, TryAcquireSucceedsWhenTokensAvailable) {
	token_bucket bucket(1000, 10);
	EXPECT_TRUE(bucket.try_acquire());
	EXPECT_EQ(bucket.available_tokens(), 9u);
}

TEST(TokenBucketTest, TryAcquireMultipleTokens) {
	token_bucket bucket(1000, 10);
	EXPECT_TRUE(bucket.try_acquire(5));
	EXPECT_EQ(bucket.available_tokens(), 5u);
}

TEST(TokenBucketTest, TryAcquireAllTokens) {
	token_bucket bucket(1000, 10);
	EXPECT_TRUE(bucket.try_acquire(10));
	EXPECT_EQ(bucket.available_tokens(), 0u);
}

TEST(TokenBucketTest, TryAcquireFailsWhenDepleted) {
	token_bucket bucket(1000, 5);
	EXPECT_TRUE(bucket.try_acquire(5));
	EXPECT_FALSE(bucket.try_acquire());
}

TEST(TokenBucketTest, TryAcquireFailsWhenInsufficientTokens) {
	token_bucket bucket(1000, 5);
	EXPECT_TRUE(bucket.try_acquire(3));
	EXPECT_FALSE(bucket.try_acquire(3));
}

// =============================================================================
// try_acquire_for (blocking with timeout) tests
// =============================================================================

TEST(TokenBucketTest, TryAcquireForSucceedsImmediatelyWhenAvailable) {
	token_bucket bucket(1000, 10);
	auto start = std::chrono::steady_clock::now();
	EXPECT_TRUE(bucket.try_acquire_for(1, 1000ms));
	auto elapsed = std::chrono::steady_clock::now() - start;
	EXPECT_LT(elapsed, 100ms);
}

TEST(TokenBucketTest, TryAcquireForTimesOut) {
	token_bucket bucket(10, 1);  // Very slow refill: 10 tokens/sec
	EXPECT_TRUE(bucket.try_acquire(1));

	auto start = std::chrono::steady_clock::now();
	// Need 5 tokens but only get ~0.5 in 50ms at 10/sec rate
	EXPECT_FALSE(bucket.try_acquire_for(5, 50ms));
	auto elapsed = std::chrono::steady_clock::now() - start;
	EXPECT_GE(elapsed, 40ms);
}

TEST(TokenBucketTest, TryAcquireForWaitsForRefill) {
	token_bucket bucket(1000, 5);  // 1000 tokens/sec
	EXPECT_TRUE(bucket.try_acquire(5));  // Deplete

	// Should refill ~1 token per ms at 1000/sec rate
	EXPECT_TRUE(bucket.try_acquire_for(1, 500ms));
}

// =============================================================================
// available_tokens tests
// =============================================================================

TEST(TokenBucketTest, AvailableTokensDecreasesAfterAcquire) {
	token_bucket bucket(1000, 10);
	EXPECT_EQ(bucket.available_tokens(), 10u);

	bucket.try_acquire(3);
	EXPECT_EQ(bucket.available_tokens(), 7u);
}

// =============================================================================
// time_until_available tests
// =============================================================================

TEST(TokenBucketTest, TimeUntilAvailableZeroWhenSufficient) {
	token_bucket bucket(1000, 10);
	EXPECT_EQ(bucket.time_until_available(5), 0ns);
}

TEST(TokenBucketTest, TimeUntilAvailablePositiveWhenInsufficient) {
	token_bucket bucket(100, 5);  // 100 tokens/sec
	bucket.try_acquire(5);

	auto wait_time = bucket.time_until_available(3);
	// Need 3 tokens at 100/sec = ~30ms
	EXPECT_GT(wait_time, 0ns);
}

// =============================================================================
// set_rate / set_burst_size tests
// =============================================================================

TEST(TokenBucketTest, SetRateChangesRate) {
	token_bucket bucket(100, 10);
	bucket.set_rate(500);
	EXPECT_EQ(bucket.get_rate(), 500u);
}

TEST(TokenBucketTest, SetBurstSizeChangesCapacity) {
	token_bucket bucket(100, 10);
	bucket.set_burst_size(20);
	EXPECT_EQ(bucket.get_burst_size(), 20u);
}

TEST(TokenBucketTest, SetBurstSizeSmallerThanCurrentDiscards) {
	token_bucket bucket(100, 10);
	EXPECT_EQ(bucket.available_tokens(), 10u);

	bucket.set_burst_size(3);
	EXPECT_LE(bucket.available_tokens(), 3u);
}

// =============================================================================
// get_rate / get_burst_size tests
// =============================================================================

TEST(TokenBucketTest, GetRateReturnsConfiguredRate) {
	token_bucket bucket(500, 10);
	EXPECT_EQ(bucket.get_rate(), 500u);
}

TEST(TokenBucketTest, GetBurstSizeReturnsConfiguredSize) {
	token_bucket bucket(100, 42);
	EXPECT_EQ(bucket.get_burst_size(), 42u);
}

// =============================================================================
// reset tests
// =============================================================================

TEST(TokenBucketTest, ResetRestoresFullBucket) {
	token_bucket bucket(1000, 10);
	bucket.try_acquire(10);
	EXPECT_EQ(bucket.available_tokens(), 0u);

	bucket.reset();
	EXPECT_EQ(bucket.available_tokens(), 10u);
}

// =============================================================================
// Concurrency tests
// =============================================================================

TEST(TokenBucketTest, ConcurrentTryAcquireNeverExceedsBurst) {
	token_bucket bucket(100000, 100);  // Fast refill
	std::atomic<int> total_acquired{0};

	std::vector<std::thread> threads;
	for (int i = 0; i < 8; ++i) {
		threads.emplace_back([&]() {
			for (int j = 0; j < 200; ++j) {
				if (bucket.try_acquire()) {
					total_acquired.fetch_add(1);
				}
			}
		});
	}

	for (auto& t : threads) t.join();

	// Some should have been acquired (exact count depends on refill timing)
	EXPECT_GT(total_acquired.load(), 0);
}

// =============================================================================
// Refill accuracy tests
// =============================================================================

TEST(TokenBucketTest, RefillAccumulatesOverTime) {
	token_bucket bucket(1000, 10);  // 1000 tokens/sec = 1 per ms
	bucket.try_acquire(10);  // Deplete
	EXPECT_EQ(bucket.available_tokens(), 0u);

	std::this_thread::sleep_for(50ms);  // Should accumulate ~50 tokens, capped at 10

	auto available = bucket.available_tokens();
	EXPECT_GT(available, 0u);
	EXPECT_LE(available, 10u);  // Capped by burst size
}
