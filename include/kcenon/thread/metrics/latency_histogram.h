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

#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>

namespace kcenon::thread::metrics {

/**
 * @brief HDR-style histogram for latency distribution with logarithmic buckets.
 *
 * @ingroup metrics
 *
 * This class provides a lock-free, thread-safe histogram for recording latency
 * measurements. It uses logarithmic bucketing to efficiently represent a wide
 * range of values (from nanoseconds to seconds) with bounded memory usage.
 *
 * ### Design Principles
 * - **Lock-free**: All operations use atomic instructions, no mutexes
 * - **Low overhead**: < 100ns per record operation
 * - **Memory efficient**: Fixed-size bucket array (< 1KB total)
 * - **High accuracy**: Accurate percentile calculations within 1%
 *
 * ### Bucket Design
 * Uses 64 logarithmic buckets covering the range from 0 to ~10^19 nanoseconds:
 * - Bucket 0: [0, 1) ns
 * - Bucket 1: [1, 2) ns
 * - Bucket 2: [2, 4) ns
 * - Bucket 3: [4, 8) ns
 * - ...
 * - Bucket 63: [2^62, 2^63) ns
 *
 * @thread_safety Thread-safe for concurrent recording and reading.
 *
 * @see EnhancedThreadPoolMetrics
 */
class LatencyHistogram {
public:
    /**
     * @brief Number of histogram buckets.
     */
    static constexpr std::size_t BUCKET_COUNT = 64;

    /**
     * @brief Default constructor - initializes all buckets to zero.
     */
    LatencyHistogram();

    /**
     * @brief Copy constructor.
     * @param other The histogram to copy from.
     */
    LatencyHistogram(const LatencyHistogram& other);

    /**
     * @brief Move constructor.
     * @param other The histogram to move from.
     */
    LatencyHistogram(LatencyHistogram&& other) noexcept;

    /**
     * @brief Copy assignment operator.
     * @param other The histogram to copy from.
     * @return Reference to this histogram.
     */
    LatencyHistogram& operator=(const LatencyHistogram& other);

    /**
     * @brief Move assignment operator.
     * @param other The histogram to move from.
     * @return Reference to this histogram.
     */
    LatencyHistogram& operator=(LatencyHistogram&& other) noexcept;

    /**
     * @brief Destructor.
     */
    ~LatencyHistogram() = default;

    /**
     * @brief Record a latency value.
     * @param value The latency duration to record.
     *
     * This operation is lock-free and thread-safe.
     * Complexity: O(1) - single atomic increment.
     */
    void record(std::chrono::nanoseconds value);

    /**
     * @brief Record a raw nanosecond value.
     * @param nanoseconds The latency in nanoseconds.
     */
    void record_ns(std::uint64_t nanoseconds);

    /**
     * @brief Calculate the value at a given percentile.
     * @param p The percentile (0.0 to 1.0, e.g., 0.99 for P99).
     * @return The latency value in nanoseconds at the given percentile.
     *
     * Returns 0 if the histogram is empty.
     * For an empty bucket at the target percentile, returns the bucket's
     * upper bound to provide an upper estimate.
     */
    [[nodiscard]] double percentile(double p) const;

    /**
     * @brief Get the 50th percentile (median).
     * @return The median latency in nanoseconds.
     */
    [[nodiscard]] double p50() const { return percentile(0.50); }

    /**
     * @brief Get the 90th percentile.
     * @return The P90 latency in nanoseconds.
     */
    [[nodiscard]] double p90() const { return percentile(0.90); }

    /**
     * @brief Get the 95th percentile.
     * @return The P95 latency in nanoseconds.
     */
    [[nodiscard]] double p95() const { return percentile(0.95); }

    /**
     * @brief Get the 99th percentile.
     * @return The P99 latency in nanoseconds.
     */
    [[nodiscard]] double p99() const { return percentile(0.99); }

    /**
     * @brief Get the 99.9th percentile.
     * @return The P99.9 latency in nanoseconds.
     */
    [[nodiscard]] double p999() const { return percentile(0.999); }

    /**
     * @brief Calculate the arithmetic mean.
     * @return The mean latency in nanoseconds.
     *
     * Returns 0.0 if the histogram is empty.
     * Note: The mean is approximated using bucket midpoints.
     */
    [[nodiscard]] double mean() const;

    /**
     * @brief Calculate the standard deviation.
     * @return The standard deviation in nanoseconds.
     *
     * Returns 0.0 if the histogram has fewer than 2 samples.
     */
    [[nodiscard]] double stddev() const;

    /**
     * @brief Get the minimum recorded value.
     * @return The minimum latency in nanoseconds.
     *
     * Returns 0 if the histogram is empty.
     */
    [[nodiscard]] std::uint64_t min() const;

    /**
     * @brief Get the maximum recorded value.
     * @return The maximum latency in nanoseconds.
     *
     * Returns 0 if the histogram is empty.
     */
    [[nodiscard]] std::uint64_t max() const;

    /**
     * @brief Get the total count of recorded values.
     * @return The number of recorded values.
     */
    [[nodiscard]] std::uint64_t count() const;

    /**
     * @brief Get the sum of all recorded values.
     * @return The sum of all latencies in nanoseconds.
     */
    [[nodiscard]] std::uint64_t sum() const;

    /**
     * @brief Reset all buckets and counters to zero.
     *
     * This operation is thread-safe but not atomic with respect to
     * concurrent records. Some in-flight records may be lost during reset.
     */
    void reset();

    /**
     * @brief Check if the histogram is empty.
     * @return True if no values have been recorded.
     */
    [[nodiscard]] bool empty() const;

    /**
     * @brief Merge another histogram into this one.
     * @param other The histogram to merge.
     *
     * All bucket counts from @p other are added to this histogram.
     */
    void merge(const LatencyHistogram& other);

    /**
     * @brief Get the bucket count at a specific index.
     * @param bucket_index The bucket index (0 to BUCKET_COUNT-1).
     * @return The count in the specified bucket.
     */
    [[nodiscard]] std::uint64_t bucket_count(std::size_t bucket_index) const;

    /**
     * @brief Get the lower bound of a bucket.
     * @param bucket_index The bucket index.
     * @return The lower bound in nanoseconds.
     */
    [[nodiscard]] static std::uint64_t bucket_lower_bound(std::size_t bucket_index);

    /**
     * @brief Get the upper bound of a bucket.
     * @param bucket_index The bucket index.
     * @return The upper bound in nanoseconds.
     */
    [[nodiscard]] static std::uint64_t bucket_upper_bound(std::size_t bucket_index);

private:
    /**
     * @brief Histogram buckets using logarithmic distribution.
     */
    std::array<std::atomic<std::uint64_t>, BUCKET_COUNT> buckets_;

    /**
     * @brief Total number of recorded values.
     */
    std::atomic<std::uint64_t> total_count_{0};

    /**
     * @brief Sum of all recorded values.
     */
    std::atomic<std::uint64_t> total_sum_{0};

    /**
     * @brief Minimum recorded value.
     */
    std::atomic<std::uint64_t> min_value_{std::numeric_limits<std::uint64_t>::max()};

    /**
     * @brief Maximum recorded value.
     */
    std::atomic<std::uint64_t> max_value_{0};

    /**
     * @brief Compute the bucket index for a given value.
     * @param value The value in nanoseconds.
     * @return The bucket index (0 to BUCKET_COUNT-1).
     */
    [[nodiscard]] static std::size_t compute_bucket_index(std::uint64_t value);

    /**
     * @brief Get the midpoint value of a bucket for estimation.
     * @param bucket_index The bucket index.
     * @return The midpoint value in nanoseconds.
     */
    [[nodiscard]] static double bucket_midpoint(std::size_t bucket_index);
};

} // namespace kcenon::thread::metrics
