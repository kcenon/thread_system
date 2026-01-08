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

#include <kcenon/thread/metrics/latency_histogram.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace kcenon::thread::metrics {

LatencyHistogram::LatencyHistogram() {
    for (auto& bucket : buckets_) {
        bucket.store(0, std::memory_order_relaxed);
    }
}

LatencyHistogram::LatencyHistogram(const LatencyHistogram& other) {
    for (std::size_t i = 0; i < BUCKET_COUNT; ++i) {
        buckets_[i].store(other.buckets_[i].load(std::memory_order_relaxed),
                          std::memory_order_relaxed);
    }
    total_count_.store(other.total_count_.load(std::memory_order_relaxed),
                       std::memory_order_relaxed);
    total_sum_.store(other.total_sum_.load(std::memory_order_relaxed),
                     std::memory_order_relaxed);
    min_value_.store(other.min_value_.load(std::memory_order_relaxed),
                     std::memory_order_relaxed);
    max_value_.store(other.max_value_.load(std::memory_order_relaxed),
                     std::memory_order_relaxed);
}

LatencyHistogram::LatencyHistogram(LatencyHistogram&& other) noexcept {
    for (std::size_t i = 0; i < BUCKET_COUNT; ++i) {
        buckets_[i].store(other.buckets_[i].load(std::memory_order_relaxed),
                          std::memory_order_relaxed);
        other.buckets_[i].store(0, std::memory_order_relaxed);
    }
    total_count_.store(other.total_count_.exchange(0, std::memory_order_relaxed),
                       std::memory_order_relaxed);
    total_sum_.store(other.total_sum_.exchange(0, std::memory_order_relaxed),
                     std::memory_order_relaxed);
    min_value_.store(other.min_value_.exchange(
                         std::numeric_limits<std::uint64_t>::max(),
                         std::memory_order_relaxed),
                     std::memory_order_relaxed);
    max_value_.store(other.max_value_.exchange(0, std::memory_order_relaxed),
                     std::memory_order_relaxed);
}

LatencyHistogram& LatencyHistogram::operator=(const LatencyHistogram& other) {
    if (this != &other) {
        for (std::size_t i = 0; i < BUCKET_COUNT; ++i) {
            buckets_[i].store(other.buckets_[i].load(std::memory_order_relaxed),
                              std::memory_order_relaxed);
        }
        total_count_.store(other.total_count_.load(std::memory_order_relaxed),
                           std::memory_order_relaxed);
        total_sum_.store(other.total_sum_.load(std::memory_order_relaxed),
                         std::memory_order_relaxed);
        min_value_.store(other.min_value_.load(std::memory_order_relaxed),
                         std::memory_order_relaxed);
        max_value_.store(other.max_value_.load(std::memory_order_relaxed),
                         std::memory_order_relaxed);
    }
    return *this;
}

LatencyHistogram& LatencyHistogram::operator=(LatencyHistogram&& other) noexcept {
    if (this != &other) {
        for (std::size_t i = 0; i < BUCKET_COUNT; ++i) {
            buckets_[i].store(other.buckets_[i].load(std::memory_order_relaxed),
                              std::memory_order_relaxed);
            other.buckets_[i].store(0, std::memory_order_relaxed);
        }
        total_count_.store(other.total_count_.exchange(0, std::memory_order_relaxed),
                           std::memory_order_relaxed);
        total_sum_.store(other.total_sum_.exchange(0, std::memory_order_relaxed),
                         std::memory_order_relaxed);
        min_value_.store(other.min_value_.exchange(
                             std::numeric_limits<std::uint64_t>::max(),
                             std::memory_order_relaxed),
                         std::memory_order_relaxed);
        max_value_.store(other.max_value_.exchange(0, std::memory_order_relaxed),
                         std::memory_order_relaxed);
    }
    return *this;
}

void LatencyHistogram::record(std::chrono::nanoseconds value) {
    record_ns(static_cast<std::uint64_t>(value.count()));
}

void LatencyHistogram::record_ns(std::uint64_t nanoseconds) {
    const auto bucket_index = compute_bucket_index(nanoseconds);
    buckets_[bucket_index].fetch_add(1, std::memory_order_relaxed);
    total_count_.fetch_add(1, std::memory_order_relaxed);
    total_sum_.fetch_add(nanoseconds, std::memory_order_relaxed);

    // Update min atomically using CAS
    auto current_min = min_value_.load(std::memory_order_relaxed);
    while (nanoseconds < current_min) {
        if (min_value_.compare_exchange_weak(current_min, nanoseconds,
                                              std::memory_order_relaxed,
                                              std::memory_order_relaxed)) {
            break;
        }
    }

    // Update max atomically using CAS
    auto current_max = max_value_.load(std::memory_order_relaxed);
    while (nanoseconds > current_max) {
        if (max_value_.compare_exchange_weak(current_max, nanoseconds,
                                              std::memory_order_relaxed,
                                              std::memory_order_relaxed)) {
            break;
        }
    }
}

double LatencyHistogram::percentile(double p) const {
    const auto total = total_count_.load(std::memory_order_relaxed);
    if (total == 0) {
        return 0.0;
    }

    // Clamp percentile to valid range
    p = std::clamp(p, 0.0, 1.0);
    const auto target_count = static_cast<std::uint64_t>(std::ceil(p * total));

    std::uint64_t cumulative = 0;
    for (std::size_t i = 0; i < BUCKET_COUNT; ++i) {
        cumulative += buckets_[i].load(std::memory_order_relaxed);
        if (cumulative >= target_count) {
            // Linear interpolation within the bucket
            const auto bucket_count_val = buckets_[i].load(std::memory_order_relaxed);
            if (bucket_count_val == 0) {
                return bucket_midpoint(i);
            }

            const auto prev_cumulative = cumulative - bucket_count_val;
            const auto target_in_bucket = target_count - prev_cumulative;
            const auto fraction =
                static_cast<double>(target_in_bucket) / bucket_count_val;

            const auto lower = bucket_lower_bound(i);
            const auto upper = bucket_upper_bound(i);

            return lower + fraction * (upper - lower);
        }
    }

    // Should not reach here, but return max bucket value
    return static_cast<double>(bucket_upper_bound(BUCKET_COUNT - 1));
}

double LatencyHistogram::mean() const {
    const auto total = total_count_.load(std::memory_order_relaxed);
    if (total == 0) {
        return 0.0;
    }
    return static_cast<double>(total_sum_.load(std::memory_order_relaxed)) / total;
}

double LatencyHistogram::stddev() const {
    const auto total = total_count_.load(std::memory_order_relaxed);
    if (total < 2) {
        return 0.0;
    }

    const auto mean_val = mean();
    double sum_squared_diff = 0.0;

    for (std::size_t i = 0; i < BUCKET_COUNT; ++i) {
        const auto bucket_count_val = buckets_[i].load(std::memory_order_relaxed);
        if (bucket_count_val > 0) {
            const auto midpoint = bucket_midpoint(i);
            const auto diff = midpoint - mean_val;
            sum_squared_diff += bucket_count_val * diff * diff;
        }
    }

    return std::sqrt(sum_squared_diff / (total - 1));
}

std::uint64_t LatencyHistogram::min() const {
    const auto total = total_count_.load(std::memory_order_relaxed);
    if (total == 0) {
        return 0;
    }
    return min_value_.load(std::memory_order_relaxed);
}

std::uint64_t LatencyHistogram::max() const {
    const auto total = total_count_.load(std::memory_order_relaxed);
    if (total == 0) {
        return 0;
    }
    return max_value_.load(std::memory_order_relaxed);
}

std::uint64_t LatencyHistogram::count() const {
    return total_count_.load(std::memory_order_relaxed);
}

std::uint64_t LatencyHistogram::sum() const {
    return total_sum_.load(std::memory_order_relaxed);
}

void LatencyHistogram::reset() {
    for (auto& bucket : buckets_) {
        bucket.store(0, std::memory_order_relaxed);
    }
    total_count_.store(0, std::memory_order_relaxed);
    total_sum_.store(0, std::memory_order_relaxed);
    min_value_.store(std::numeric_limits<std::uint64_t>::max(),
                     std::memory_order_relaxed);
    max_value_.store(0, std::memory_order_relaxed);
}

bool LatencyHistogram::empty() const {
    return total_count_.load(std::memory_order_relaxed) == 0;
}

void LatencyHistogram::merge(const LatencyHistogram& other) {
    for (std::size_t i = 0; i < BUCKET_COUNT; ++i) {
        buckets_[i].fetch_add(other.buckets_[i].load(std::memory_order_relaxed),
                              std::memory_order_relaxed);
    }
    total_count_.fetch_add(other.total_count_.load(std::memory_order_relaxed),
                           std::memory_order_relaxed);
    total_sum_.fetch_add(other.total_sum_.load(std::memory_order_relaxed),
                         std::memory_order_relaxed);

    // Update min atomically
    auto other_min = other.min_value_.load(std::memory_order_relaxed);
    auto current_min = min_value_.load(std::memory_order_relaxed);
    while (other_min < current_min) {
        if (min_value_.compare_exchange_weak(current_min, other_min,
                                              std::memory_order_relaxed,
                                              std::memory_order_relaxed)) {
            break;
        }
    }

    // Update max atomically
    auto other_max = other.max_value_.load(std::memory_order_relaxed);
    auto current_max = max_value_.load(std::memory_order_relaxed);
    while (other_max > current_max) {
        if (max_value_.compare_exchange_weak(current_max, other_max,
                                              std::memory_order_relaxed,
                                              std::memory_order_relaxed)) {
            break;
        }
    }
}

std::uint64_t LatencyHistogram::bucket_count(std::size_t bucket_index) const {
    if (bucket_index >= BUCKET_COUNT) {
        return 0;
    }
    return buckets_[bucket_index].load(std::memory_order_relaxed);
}

std::uint64_t LatencyHistogram::bucket_lower_bound(std::size_t bucket_index) {
    if (bucket_index == 0) {
        return 0;
    }
    if (bucket_index >= BUCKET_COUNT) {
        return std::numeric_limits<std::uint64_t>::max();
    }
    return static_cast<std::uint64_t>(1ULL << (bucket_index - 1));
}

std::uint64_t LatencyHistogram::bucket_upper_bound(std::size_t bucket_index) {
    if (bucket_index >= BUCKET_COUNT - 1) {
        return std::numeric_limits<std::uint64_t>::max();
    }
    return static_cast<std::uint64_t>(1ULL << bucket_index);
}

std::size_t LatencyHistogram::compute_bucket_index(std::uint64_t value) {
    if (value == 0) {
        return 0;
    }

    // Use bit manipulation to find the highest set bit (floor of log2)
    // This gives us the bucket index directly
#if defined(__GNUC__) || defined(__clang__)
    const auto leading_zeros = __builtin_clzll(value);
    const auto highest_bit = 63 - leading_zeros;
#elif defined(_MSC_VER)
    unsigned long highest_bit;
    _BitScanReverse64(&highest_bit, value);
#else
    // Fallback: portable implementation
    std::size_t highest_bit = 0;
    auto temp = value;
    while (temp >>= 1) {
        ++highest_bit;
    }
#endif

    // Bucket index is highest_bit + 1, capped at BUCKET_COUNT - 1
    return std::min(static_cast<std::size_t>(highest_bit + 1), BUCKET_COUNT - 1);
}

double LatencyHistogram::bucket_midpoint(std::size_t bucket_index) {
    const auto lower = bucket_lower_bound(bucket_index);
    const auto upper = bucket_upper_bound(bucket_index);

    if (upper == std::numeric_limits<std::uint64_t>::max()) {
        // For the last bucket, use lower bound as estimate
        return static_cast<double>(lower);
    }

    return (static_cast<double>(lower) + static_cast<double>(upper)) / 2.0;
}

} // namespace kcenon::thread::metrics
