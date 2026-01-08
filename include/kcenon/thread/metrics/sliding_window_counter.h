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

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

namespace kcenon::thread::metrics {

/**
 * @brief Sliding window counter for throughput measurement.
 *
 * @ingroup metrics
 *
 * This class provides a lock-free, thread-safe counter that tracks events
 * within a sliding time window. It uses a circular buffer of time buckets
 * to maintain accurate rate calculations.
 *
 * ### Design Principles
 * - **Lock-free**: All operations use atomic instructions
 * - **Low overhead**: O(1) increment, O(bucket_count) for rate calculation
 * - **Configurable**: Window size and bucket granularity are configurable
 * - **Accurate**: Sub-second precision with configurable bucket size
 *
 * ### Implementation Details
 * The window is divided into multiple buckets (default: 10 per second).
 * Each bucket covers a fixed time interval. As time advances, old buckets
 * are automatically invalidated and reused for new time periods.
 *
 * Example for a 1-second window with 10 buckets:
 * ```
 * Time:    [0ms - 100ms] [100ms - 200ms] ... [900ms - 1000ms]
 * Buckets:   bucket[0]      bucket[1]    ...    bucket[9]
 * ```
 *
 * @thread_safety Thread-safe for concurrent increment and rate queries.
 *
 * @see EnhancedThreadPoolMetrics
 */
class SlidingWindowCounter {
public:
    /**
     * @brief Default number of buckets per second.
     */
    static constexpr std::size_t DEFAULT_BUCKETS_PER_SECOND = 10;

    /**
     * @brief Constructs a sliding window counter.
     * @param window_size The duration of the sliding window.
     * @param buckets_per_second Number of buckets per second (default: 10).
     *
     * Higher buckets_per_second provides more precision but uses more memory.
     * For a 60-second window with 10 buckets/second, uses 600 * 16 bytes = ~10KB.
     */
    explicit SlidingWindowCounter(
        std::chrono::seconds window_size,
        std::size_t buckets_per_second = DEFAULT_BUCKETS_PER_SECOND);

    /**
     * @brief Copy constructor.
     * @param other The counter to copy from.
     */
    SlidingWindowCounter(const SlidingWindowCounter& other);

    /**
     * @brief Move constructor.
     * @param other The counter to move from.
     */
    SlidingWindowCounter(SlidingWindowCounter&& other) noexcept;

    /**
     * @brief Copy assignment operator.
     * @param other The counter to copy from.
     * @return Reference to this counter.
     */
    SlidingWindowCounter& operator=(const SlidingWindowCounter& other);

    /**
     * @brief Move assignment operator.
     * @param other The counter to move from.
     * @return Reference to this counter.
     */
    SlidingWindowCounter& operator=(SlidingWindowCounter&& other) noexcept;

    /**
     * @brief Destructor.
     */
    ~SlidingWindowCounter() = default;

    /**
     * @brief Increment the counter.
     * @param count The value to add (default: 1).
     *
     * This operation is lock-free and thread-safe.
     * Complexity: O(1).
     */
    void increment(std::size_t count = 1);

    /**
     * @brief Get the current rate per second.
     * @return The average rate over the sliding window.
     *
     * This calculates the total count in the window divided by window duration.
     */
    [[nodiscard]] double rate_per_second() const;

    /**
     * @brief Get the total count within the current window.
     * @return The sum of all counts in valid buckets.
     */
    [[nodiscard]] std::uint64_t total_in_window() const;

    /**
     * @brief Get the all-time total count.
     * @return The cumulative count since creation or last reset.
     */
    [[nodiscard]] std::uint64_t all_time_total() const;

    /**
     * @brief Reset the counter.
     *
     * Clears all buckets and resets the all-time total.
     */
    void reset();

    /**
     * @brief Get the window size.
     * @return The configured sliding window duration.
     */
    [[nodiscard]] std::chrono::seconds window_size() const;

    /**
     * @brief Get the number of buckets.
     * @return The total number of time buckets.
     */
    [[nodiscard]] std::size_t bucket_count() const;

private:
    /**
     * @brief Time bucket structure.
     *
     * Note: Custom constructors are needed because std::atomic is not
     * move/copy constructible by default.
     */
    struct Bucket {
        std::atomic<std::uint64_t> count{0};
        std::atomic<std::uint64_t> timestamp_ms{0};  // Bucket start time

        Bucket() = default;

        Bucket(const Bucket& other)
            : count(other.count.load(std::memory_order_relaxed)),
              timestamp_ms(other.timestamp_ms.load(std::memory_order_relaxed)) {}

        Bucket(Bucket&& other) noexcept
            : count(other.count.load(std::memory_order_relaxed)),
              timestamp_ms(other.timestamp_ms.load(std::memory_order_relaxed)) {}

        Bucket& operator=(const Bucket& other) {
            if (this != &other) {
                count.store(other.count.load(std::memory_order_relaxed),
                            std::memory_order_relaxed);
                timestamp_ms.store(other.timestamp_ms.load(std::memory_order_relaxed),
                                   std::memory_order_relaxed);
            }
            return *this;
        }

        Bucket& operator=(Bucket&& other) noexcept {
            if (this != &other) {
                count.store(other.count.load(std::memory_order_relaxed),
                            std::memory_order_relaxed);
                timestamp_ms.store(other.timestamp_ms.load(std::memory_order_relaxed),
                                   std::memory_order_relaxed);
            }
            return *this;
        }
    };

    /**
     * @brief The sliding window duration.
     */
    std::chrono::seconds window_size_;

    /**
     * @brief Duration of each bucket.
     */
    std::chrono::milliseconds bucket_duration_;

    /**
     * @brief Circular buffer of time buckets.
     */
    std::vector<Bucket> buckets_;

    /**
     * @brief All-time total count.
     */
    std::atomic<std::uint64_t> all_time_total_{0};

    /**
     * @brief Get the current bucket index based on current time.
     * @return The index into the circular buffer.
     */
    [[nodiscard]] std::size_t current_bucket_index() const;

    /**
     * @brief Get the bucket index for a specific timestamp.
     * @param timestamp_ms Timestamp in milliseconds since epoch.
     * @return The bucket index.
     */
    [[nodiscard]] std::size_t bucket_index_for_time(std::uint64_t timestamp_ms) const;

    /**
     * @brief Get current time in milliseconds since epoch.
     * @return Current timestamp in milliseconds.
     */
    [[nodiscard]] static std::uint64_t current_time_ms();

    /**
     * @brief Check if a bucket is within the current window.
     * @param bucket_timestamp_ms The bucket's timestamp.
     * @param current_ms The current timestamp.
     * @return True if the bucket is valid (within window).
     */
    [[nodiscard]] bool is_bucket_valid(
        std::uint64_t bucket_timestamp_ms,
        std::uint64_t current_ms) const;

    /**
     * @brief Advance bucket to current time period if needed.
     * @param bucket_index The bucket to advance.
     * @param current_ms The current timestamp.
     */
    void advance_bucket(std::size_t bucket_index, std::uint64_t current_ms);
};

} // namespace kcenon::thread::metrics
