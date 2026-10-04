// BSD 3-Clause License
// Copyright (c) 2026, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file sliding_window_counter_test.cpp
 * @brief Unit tests for SlidingWindowCounter.
 *
 * Covers increment, rate calculation, window totals, reset, copy/move
 * semantics, and configurable window/bucket sizes.
 */

#include <gtest/gtest.h>

#include <kcenon/thread/metrics/sliding_window_counter.h>

#include <chrono>
#include <thread>
#include <vector>

using kcenon::thread::metrics::SlidingWindowCounter;
using namespace std::chrono_literals;

TEST(SlidingWindowCounterTest, DefaultConstructionParameters) {
    SlidingWindowCounter counter(1s);
    EXPECT_EQ(counter.window_size(), 1s);
    EXPECT_EQ(counter.bucket_count(),
              SlidingWindowCounter::DEFAULT_BUCKETS_PER_SECOND);
    EXPECT_EQ(counter.total_in_window(), 0u);
    EXPECT_EQ(counter.all_time_total(), 0u);
}

TEST(SlidingWindowCounterTest, CustomBucketsPerSecond) {
    SlidingWindowCounter counter(10s, 4);
    // 4 buckets per second over 10s = 40 buckets total.
    EXPECT_EQ(counter.bucket_count(), 40u);
    EXPECT_EQ(counter.window_size(), 10s);
}

// -----------------------------------------------------------------------------
// Increment
// -----------------------------------------------------------------------------

TEST(SlidingWindowCounterTest, IncrementUpdatesAllTimeTotal) {
    SlidingWindowCounter counter(5s);
    counter.increment();
    counter.increment(4);
    EXPECT_EQ(counter.all_time_total(), 5u);
}

TEST(SlidingWindowCounterTest, IncrementUpdatesWindowTotal) {
    SlidingWindowCounter counter(5s);
    counter.increment();
    counter.increment(2);
    // Within the window these should all still be counted.
    EXPECT_EQ(counter.total_in_window(), 3u);
}

TEST(SlidingWindowCounterTest, IncrementZeroDoesNotCrash) {
    SlidingWindowCounter counter(5s);
    counter.increment(0);
    EXPECT_EQ(counter.all_time_total(), 0u);
}

// -----------------------------------------------------------------------------
// Rate
// -----------------------------------------------------------------------------

TEST(SlidingWindowCounterTest, RateIsZeroWhenEmpty) {
    SlidingWindowCounter counter(1s);
    EXPECT_DOUBLE_EQ(counter.rate_per_second(), 0.0);
}

TEST(SlidingWindowCounterTest, RateIsPositiveAfterIncrement) {
    SlidingWindowCounter counter(2s, 10);
    for (int i = 0; i < 20; ++i) {
        counter.increment();
    }
    const double rate = counter.rate_per_second();
    EXPECT_GT(rate, 0.0);
    // Over a 2s window with 20 increments, rate ~= 10/s. Bound loosely.
    EXPECT_LT(rate, 40.0);
}

// -----------------------------------------------------------------------------
// Reset
// -----------------------------------------------------------------------------

TEST(SlidingWindowCounterTest, ResetClearsAllState) {
    SlidingWindowCounter counter(2s);
    counter.increment(100);
    ASSERT_EQ(counter.all_time_total(), 100u);

    counter.reset();
    EXPECT_EQ(counter.all_time_total(), 0u);
    EXPECT_EQ(counter.total_in_window(), 0u);
    EXPECT_DOUBLE_EQ(counter.rate_per_second(), 0.0);
}

// -----------------------------------------------------------------------------
// Copy / move semantics
// -----------------------------------------------------------------------------

TEST(SlidingWindowCounterTest, CopyConstructorPreservesConfig) {
    SlidingWindowCounter a(3s, 5);
    a.increment(7);

    SlidingWindowCounter b(a);
    EXPECT_EQ(b.window_size(), 3s);
    EXPECT_EQ(b.bucket_count(), a.bucket_count());
    EXPECT_EQ(b.all_time_total(), 7u);
}

TEST(SlidingWindowCounterTest, CopyAssignmentPreservesConfig) {
    SlidingWindowCounter a(4s, 2);
    a.increment(9);

    SlidingWindowCounter b(1s);
    b = a;
    EXPECT_EQ(b.window_size(), 4s);
    EXPECT_EQ(b.all_time_total(), 9u);
}

TEST(SlidingWindowCounterTest, MoveConstructorTransfersState) {
    SlidingWindowCounter a(3s, 2);
    a.increment(11);

    SlidingWindowCounter b(std::move(a));
    EXPECT_EQ(b.window_size(), 3s);
    EXPECT_EQ(b.all_time_total(), 11u);
}

TEST(SlidingWindowCounterTest, MoveAssignmentTransfersState) {
    SlidingWindowCounter a(3s, 2);
    a.increment(13);

    SlidingWindowCounter b(1s);
    b = std::move(a);
    EXPECT_EQ(b.all_time_total(), 13u);
}

TEST(SlidingWindowCounterTest, CopyAssignmentSelfIsSafe) {
    SlidingWindowCounter a(1s);
    a.increment(2);
    SlidingWindowCounter& ref = a;
    a = ref;
    EXPECT_EQ(a.all_time_total(), 2u);
}

// -----------------------------------------------------------------------------
// Bucket expiration
// -----------------------------------------------------------------------------

TEST(SlidingWindowCounterTest, OldBucketsAreEvictedFromWindow) {
    // Short window so the test does not take long.
    SlidingWindowCounter counter(1s, 10);
    counter.increment(50);
    EXPECT_EQ(counter.total_in_window(), 50u);

    // Sleep longer than the window so the bucket's timestamp is stale.
    std::this_thread::sleep_for(1200ms);
    // Buckets recorded more than window_size ago should no longer be counted.
    EXPECT_EQ(counter.total_in_window(), 0u);

    // All-time total remains untouched.
    EXPECT_EQ(counter.all_time_total(), 50u);
}

// -----------------------------------------------------------------------------
// Thread safety smoke test
// -----------------------------------------------------------------------------

TEST(SlidingWindowCounterTest, ConcurrentIncrementPreservesAllTimeTotal) {
    SlidingWindowCounter counter(5s, 10);

    constexpr std::size_t kThreads = 4;
    constexpr std::size_t kIncs = 2000;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (std::size_t t = 0; t < kThreads; ++t) {
        threads.emplace_back([&counter]() {
            for (std::size_t i = 0; i < kIncs; ++i) {
                counter.increment();
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    EXPECT_EQ(counter.all_time_total(), kThreads * kIncs);
}
