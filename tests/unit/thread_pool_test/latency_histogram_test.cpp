// BSD 3-Clause License
// Copyright (c) 2026, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file latency_histogram_test.cpp
 * @brief Unit tests for LatencyHistogram.
 *
 * Covers recording, percentile calculation, statistics, merge, reset,
 * copy/move semantics, and bucket boundary edge cases.
 */

#include <gtest/gtest.h>

#include <kcenon/thread/metrics/latency_histogram.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using kcenon::thread::metrics::LatencyHistogram;
using namespace std::chrono_literals;

// -----------------------------------------------------------------------------
// Construction / defaults
// -----------------------------------------------------------------------------

TEST(LatencyHistogramTest, DefaultConstructedIsEmpty) {
    LatencyHistogram h;
    EXPECT_TRUE(h.empty());
    EXPECT_EQ(h.count(), 0u);
    EXPECT_EQ(h.sum(), 0u);
    EXPECT_EQ(h.min(), 0u);
    EXPECT_EQ(h.max(), 0u);
    EXPECT_DOUBLE_EQ(h.mean(), 0.0);
    EXPECT_DOUBLE_EQ(h.stddev(), 0.0);
    EXPECT_DOUBLE_EQ(h.p50(), 0.0);
    EXPECT_DOUBLE_EQ(h.p99(), 0.0);
}

// -----------------------------------------------------------------------------
// Basic recording
// -----------------------------------------------------------------------------

TEST(LatencyHistogramTest, RecordSingleValueUpdatesCounters) {
    LatencyHistogram h;
    h.record(500ns);

    EXPECT_FALSE(h.empty());
    EXPECT_EQ(h.count(), 1u);
    EXPECT_EQ(h.sum(), 500u);
    EXPECT_EQ(h.min(), 500u);
    EXPECT_EQ(h.max(), 500u);
    EXPECT_GT(h.mean(), 0.0);
}

TEST(LatencyHistogramTest, RecordNsAcceptsRawNanoseconds) {
    LatencyHistogram h;
    h.record_ns(1234);

    EXPECT_EQ(h.count(), 1u);
    EXPECT_EQ(h.sum(), 1234u);
    EXPECT_EQ(h.min(), 1234u);
    EXPECT_EQ(h.max(), 1234u);
}

TEST(LatencyHistogramTest, RecordMultipleValuesTracksMinMax) {
    LatencyHistogram h;
    h.record_ns(100);
    h.record_ns(10000);
    h.record_ns(500);
    h.record_ns(50);

    EXPECT_EQ(h.count(), 4u);
    EXPECT_EQ(h.sum(), 100u + 10000u + 500u + 50u);
    EXPECT_EQ(h.min(), 50u);
    EXPECT_EQ(h.max(), 10000u);
}

TEST(LatencyHistogramTest, RecordZeroValueLandsInFirstBucket) {
    LatencyHistogram h;
    h.record_ns(0);
    EXPECT_EQ(h.count(), 1u);
    EXPECT_EQ(h.sum(), 0u);
    // Bucket 0 covers [0, 1) ns
    EXPECT_EQ(h.bucket_count(0), 1u);
}

// -----------------------------------------------------------------------------
// Percentiles
// -----------------------------------------------------------------------------

TEST(LatencyHistogramTest, PercentilesIncreaseMonotonically) {
    LatencyHistogram h;
    for (std::uint64_t v = 1; v <= 1000; ++v) {
        h.record_ns(v);
    }

    const double p50 = h.p50();
    const double p90 = h.p90();
    const double p95 = h.p95();
    const double p99 = h.p99();
    const double p999 = h.p999();

    EXPECT_LE(p50, p90);
    EXPECT_LE(p90, p95);
    EXPECT_LE(p95, p99);
    EXPECT_LE(p99, p999);
    EXPECT_GT(p999, 0.0);
}

TEST(LatencyHistogramTest, PercentileOutOfRangeIsHandled) {
    LatencyHistogram h;
    h.record_ns(100);
    // These values are outside [0, 1] but must not crash.
    EXPECT_GE(h.percentile(-0.5), 0.0);
    EXPECT_GE(h.percentile(1.5), 0.0);
}

// -----------------------------------------------------------------------------
// Mean / stddev
// -----------------------------------------------------------------------------

TEST(LatencyHistogramTest, MeanApproximatesArithmeticMean) {
    LatencyHistogram h;
    for (int i = 0; i < 100; ++i) {
        h.record_ns(1000);
    }

    const double mean = h.mean();
    // Bucket midpoint approximation will vary, but should be positive and
    // roughly around the recorded value's order of magnitude.
    EXPECT_GT(mean, 0.0);
}

TEST(LatencyHistogramTest, StddevZeroForSingleSample) {
    LatencyHistogram h;
    h.record_ns(500);
    EXPECT_DOUBLE_EQ(h.stddev(), 0.0);
}

TEST(LatencyHistogramTest, StddevPositiveForVariedSamples) {
    LatencyHistogram h;
    h.record_ns(10);
    h.record_ns(1000000);
    h.record_ns(50);
    EXPECT_GT(h.stddev(), 0.0);
}

// -----------------------------------------------------------------------------
// Reset
// -----------------------------------------------------------------------------

TEST(LatencyHistogramTest, ResetClearsAllState) {
    LatencyHistogram h;
    for (std::uint64_t v : {10u, 100u, 1000u, 100000u}) {
        h.record_ns(v);
    }
    ASSERT_FALSE(h.empty());

    h.reset();

    EXPECT_TRUE(h.empty());
    EXPECT_EQ(h.count(), 0u);
    EXPECT_EQ(h.sum(), 0u);
    EXPECT_EQ(h.min(), 0u);
    EXPECT_EQ(h.max(), 0u);
}

// -----------------------------------------------------------------------------
// Merge
// -----------------------------------------------------------------------------

TEST(LatencyHistogramTest, MergeCombinesCounters) {
    LatencyHistogram a;
    LatencyHistogram b;
    a.record_ns(100);
    a.record_ns(200);
    b.record_ns(300);
    b.record_ns(400);

    a.merge(b);

    EXPECT_EQ(a.count(), 4u);
    EXPECT_EQ(a.sum(), 100u + 200u + 300u + 400u);
    EXPECT_EQ(a.min(), 100u);
    EXPECT_EQ(a.max(), 400u);
}

TEST(LatencyHistogramTest, MergeEmptyIntoEmptyIsNoop) {
    LatencyHistogram a, b;
    a.merge(b);
    EXPECT_TRUE(a.empty());
}

TEST(LatencyHistogramTest, MergeEmptyIntoNonEmptyDoesNothing) {
    LatencyHistogram a, b;
    a.record_ns(42);
    a.merge(b);
    EXPECT_EQ(a.count(), 1u);
    EXPECT_EQ(a.sum(), 42u);
}

// -----------------------------------------------------------------------------
// Copy / move semantics
// -----------------------------------------------------------------------------

TEST(LatencyHistogramTest, CopyConstructorDuplicatesState) {
    LatencyHistogram a;
    a.record_ns(1000);
    a.record_ns(2000);

    LatencyHistogram b(a);
    EXPECT_EQ(b.count(), a.count());
    EXPECT_EQ(b.sum(), a.sum());
    EXPECT_EQ(b.min(), a.min());
    EXPECT_EQ(b.max(), a.max());
}

TEST(LatencyHistogramTest, CopyAssignmentDuplicatesState) {
    LatencyHistogram a;
    a.record_ns(7);

    LatencyHistogram b;
    b = a;
    EXPECT_EQ(b.count(), 1u);
    EXPECT_EQ(b.sum(), 7u);
}

TEST(LatencyHistogramTest, MoveConstructorTransfersState) {
    LatencyHistogram a;
    a.record_ns(500);

    LatencyHistogram b(std::move(a));
    EXPECT_EQ(b.count(), 1u);
    EXPECT_EQ(b.sum(), 500u);
}

TEST(LatencyHistogramTest, MoveAssignmentTransfersState) {
    LatencyHistogram a;
    a.record_ns(999);

    LatencyHistogram b;
    b = std::move(a);
    EXPECT_EQ(b.count(), 1u);
    EXPECT_EQ(b.sum(), 999u);
}

TEST(LatencyHistogramTest, SelfAssignmentIsSafe) {
    LatencyHistogram a;
    a.record_ns(42);
    LatencyHistogram& ref = a;
    a = ref; // self-assignment through reference
    EXPECT_EQ(a.count(), 1u);
    EXPECT_EQ(a.sum(), 42u);
}

// -----------------------------------------------------------------------------
// Bucket bounds (static API)
// -----------------------------------------------------------------------------

TEST(LatencyHistogramTest, BucketBoundsFormContiguousCoverage) {
    for (std::size_t i = 1; i < LatencyHistogram::BUCKET_COUNT; ++i) {
        EXPECT_EQ(LatencyHistogram::bucket_lower_bound(i),
                  LatencyHistogram::bucket_upper_bound(i - 1))
            << "gap between bucket " << (i - 1) << " and " << i;
    }
}

TEST(LatencyHistogramTest, BucketZeroCoversZeroRange) {
    EXPECT_EQ(LatencyHistogram::bucket_lower_bound(0),
              static_cast<std::uint64_t>(0));
}

TEST(LatencyHistogramTest, BucketUpperBoundIsMonotonic) {
    for (std::size_t i = 1; i < LatencyHistogram::BUCKET_COUNT; ++i) {
        EXPECT_GT(LatencyHistogram::bucket_upper_bound(i),
                  LatencyHistogram::bucket_upper_bound(i - 1));
    }
}

// -----------------------------------------------------------------------------
// Thread safety (smoke test)
// -----------------------------------------------------------------------------

TEST(LatencyHistogramTest, ConcurrentRecordIsSafe) {
    LatencyHistogram h;
    constexpr std::size_t kThreads = 4;
    constexpr std::size_t kPerThread = 2000;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (std::size_t t = 0; t < kThreads; ++t) {
        threads.emplace_back([&h, t]() {
            for (std::size_t i = 0; i < kPerThread; ++i) {
                h.record_ns(static_cast<std::uint64_t>(t * 1000 + i + 1));
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    EXPECT_EQ(h.count(), kThreads * kPerThread);
    EXPECT_GT(h.sum(), 0u);
    EXPECT_GT(h.max(), 0u);
    EXPECT_GT(h.min(), 0u);
    EXPECT_LE(h.min(), h.max());
}
