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

#include <kcenon/thread/metrics/sliding_window_counter.h>

#include <algorithm>

namespace kcenon::thread::metrics {

SlidingWindowCounter::SlidingWindowCounter(
    std::chrono::seconds window_size,
    std::size_t buckets_per_second)
    : window_size_(window_size),
      bucket_duration_(std::chrono::milliseconds{1000 / buckets_per_second}),
      buckets_(static_cast<std::size_t>(window_size.count()) * buckets_per_second) {
    // Initialize all buckets
    for (auto& bucket : buckets_) {
        bucket.count.store(0, std::memory_order_relaxed);
        bucket.timestamp_ms.store(0, std::memory_order_relaxed);
    }
}

SlidingWindowCounter::SlidingWindowCounter(const SlidingWindowCounter& other)
    : window_size_(other.window_size_),
      bucket_duration_(other.bucket_duration_),
      buckets_(other.buckets_.size()) {
    for (std::size_t i = 0; i < buckets_.size(); ++i) {
        buckets_[i].count.store(
            other.buckets_[i].count.load(std::memory_order_relaxed),
            std::memory_order_relaxed);
        buckets_[i].timestamp_ms.store(
            other.buckets_[i].timestamp_ms.load(std::memory_order_relaxed),
            std::memory_order_relaxed);
    }
    all_time_total_.store(
        other.all_time_total_.load(std::memory_order_relaxed),
        std::memory_order_relaxed);
}

SlidingWindowCounter::SlidingWindowCounter(SlidingWindowCounter&& other) noexcept
    : window_size_(other.window_size_),
      bucket_duration_(other.bucket_duration_),
      buckets_(std::move(other.buckets_)) {
    all_time_total_.store(
        other.all_time_total_.exchange(0, std::memory_order_relaxed),
        std::memory_order_relaxed);
}

SlidingWindowCounter& SlidingWindowCounter::operator=(
    const SlidingWindowCounter& other) {
    if (this != &other) {
        window_size_ = other.window_size_;
        bucket_duration_ = other.bucket_duration_;

        // Resize if needed
        if (buckets_.size() != other.buckets_.size()) {
            buckets_.resize(other.buckets_.size());
        }

        for (std::size_t i = 0; i < buckets_.size(); ++i) {
            buckets_[i].count.store(
                other.buckets_[i].count.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            buckets_[i].timestamp_ms.store(
                other.buckets_[i].timestamp_ms.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
        }
        all_time_total_.store(
            other.all_time_total_.load(std::memory_order_relaxed),
            std::memory_order_relaxed);
    }
    return *this;
}

SlidingWindowCounter& SlidingWindowCounter::operator=(
    SlidingWindowCounter&& other) noexcept {
    if (this != &other) {
        window_size_ = other.window_size_;
        bucket_duration_ = other.bucket_duration_;
        buckets_ = std::move(other.buckets_);
        all_time_total_.store(
            other.all_time_total_.exchange(0, std::memory_order_relaxed),
            std::memory_order_relaxed);
    }
    return *this;
}

void SlidingWindowCounter::increment(std::size_t count) {
    const auto current_ms = current_time_ms();
    const auto bucket_index = bucket_index_for_time(current_ms);

    // Advance bucket if it's stale
    advance_bucket(bucket_index, current_ms);

    // Increment the bucket and all-time total
    buckets_[bucket_index].count.fetch_add(
        static_cast<std::uint64_t>(count), std::memory_order_relaxed);
    all_time_total_.fetch_add(
        static_cast<std::uint64_t>(count), std::memory_order_relaxed);
}

double SlidingWindowCounter::rate_per_second() const {
    const auto total = total_in_window();
    const auto window_seconds = static_cast<double>(window_size_.count());
    return static_cast<double>(total) / window_seconds;
}

std::uint64_t SlidingWindowCounter::total_in_window() const {
    const auto current_ms = current_time_ms();
    std::uint64_t total = 0;

    for (const auto& bucket : buckets_) {
        const auto timestamp = bucket.timestamp_ms.load(std::memory_order_relaxed);
        if (is_bucket_valid(timestamp, current_ms)) {
            total += bucket.count.load(std::memory_order_relaxed);
        }
    }

    return total;
}

std::uint64_t SlidingWindowCounter::all_time_total() const {
    return all_time_total_.load(std::memory_order_relaxed);
}

void SlidingWindowCounter::reset() {
    for (auto& bucket : buckets_) {
        bucket.count.store(0, std::memory_order_relaxed);
        bucket.timestamp_ms.store(0, std::memory_order_relaxed);
    }
    all_time_total_.store(0, std::memory_order_relaxed);
}

std::chrono::seconds SlidingWindowCounter::window_size() const {
    return window_size_;
}

std::size_t SlidingWindowCounter::bucket_count() const {
    return buckets_.size();
}

std::size_t SlidingWindowCounter::current_bucket_index() const {
    return bucket_index_for_time(current_time_ms());
}

std::size_t SlidingWindowCounter::bucket_index_for_time(
    std::uint64_t timestamp_ms) const {
    const auto bucket_duration_ms =
        static_cast<std::uint64_t>(bucket_duration_.count());
    return static_cast<std::size_t>(
        (timestamp_ms / bucket_duration_ms) % buckets_.size());
}

std::uint64_t SlidingWindowCounter::current_time_ms() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

bool SlidingWindowCounter::is_bucket_valid(
    std::uint64_t bucket_timestamp_ms,
    std::uint64_t current_ms) const {
    if (bucket_timestamp_ms == 0) {
        return false;
    }

    const auto window_ms =
        static_cast<std::uint64_t>(window_size_.count()) * 1000;
    return (current_ms - bucket_timestamp_ms) < window_ms;
}

void SlidingWindowCounter::advance_bucket(
    std::size_t bucket_index,
    std::uint64_t current_ms) {
    auto& bucket = buckets_[bucket_index];
    const auto bucket_duration_ms =
        static_cast<std::uint64_t>(bucket_duration_.count());

    // Calculate the expected timestamp for this bucket at current time
    const auto expected_timestamp =
        (current_ms / bucket_duration_ms) * bucket_duration_ms;

    auto old_timestamp = bucket.timestamp_ms.load(std::memory_order_relaxed);

    // Check if bucket needs to be advanced (stale bucket from previous window)
    if (old_timestamp < expected_timestamp) {
        // Try to update the timestamp atomically
        if (bucket.timestamp_ms.compare_exchange_strong(
                old_timestamp, expected_timestamp,
                std::memory_order_relaxed, std::memory_order_relaxed)) {
            // We successfully claimed this bucket for the new time period
            // Reset the count (old data is now invalid)
            bucket.count.store(0, std::memory_order_relaxed);
        }
        // If CAS failed, another thread already advanced it - that's fine
    }
}

} // namespace kcenon::thread::metrics
