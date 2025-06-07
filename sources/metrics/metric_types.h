#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <shared_mutex>
#include <nlohmann/json.hpp>

namespace metrics {

using json = nlohmann::json;

/**
 * @brief Base interface for all metric types
 */
class metric_interface {
public:
    virtual ~metric_interface() = default;
    
    /**
     * @brief Convert metric to JSON representation
     * @return JSON object containing metric data
     */
    virtual json to_json() const = 0;
    
    /**
     * @brief Get metric type name
     * @return Type name as string
     */
    virtual std::string type_name() const = 0;
    
    /**
     * @brief Get metric name
     * @return Metric name
     */
    virtual const std::string& name() const = 0;
    
    /**
     * @brief Get metric description
     * @return Metric description
     */
    virtual const std::string& description() const = 0;
};

/**
 * @brief Counter metric - monotonically increasing value
 * @tparam T Numeric type for the counter
 */
template<typename T = uint64_t>
class counter : public metric_interface {
private:
    alignas(64) std::atomic<T> value_{0};  // Cache line aligned
    const std::string name_;
    const std::string description_;
    const std::unordered_map<std::string, std::string> labels_;
    
public:
    counter(std::string name, std::string desc, 
            std::unordered_map<std::string, std::string> labels = {})
        : name_(std::move(name))
        , description_(std::move(desc))
        , labels_(std::move(labels)) {}
    
    /**
     * @brief Increment counter by delta
     * @param delta Amount to increment (default: 1)
     */
    void increment(T delta = 1) noexcept {
        value_.fetch_add(delta, std::memory_order_relaxed);
    }
    
    /**
     * @brief Get current counter value
     * @return Current value
     */
    T get() const noexcept {
        return value_.load(std::memory_order_relaxed);
    }
    
    /**
     * @brief Reset counter to zero
     */
    void reset() noexcept {
        value_.store(0, std::memory_order_relaxed);
    }
    
    json to_json() const override {
        return {
            {"type", "counter"},
            {"name", name_},
            {"description", description_},
            {"value", get()},
            {"labels", labels_}
        };
    }
    
    std::string type_name() const override { return "counter"; }
    const std::string& name() const override { return name_; }
    const std::string& description() const override { return description_; }
};

/**
 * @brief Gauge metric - value that can go up or down
 * @tparam T Numeric type for the gauge
 */
template<typename T = double>
class gauge : public metric_interface {
private:
    alignas(64) std::atomic<T> value_{0};
    alignas(64) std::atomic<T> min_value_{std::numeric_limits<T>::max()};
    alignas(64) std::atomic<T> max_value_{std::numeric_limits<T>::lowest()};
    const std::string name_;
    const std::string description_;
    
public:
    gauge(std::string name, std::string desc)
        : name_(std::move(name))
        , description_(std::move(desc)) {}
    
    /**
     * @brief Set gauge to specific value
     * @param value New value
     */
    void set(T value) noexcept {
        value_.store(value, std::memory_order_relaxed);
        update_min_max(value);
    }
    
    /**
     * @brief Increment gauge by delta
     * @param delta Amount to increment
     */
    void increment(T delta = 1) noexcept {
        T new_value = value_.fetch_add(delta, std::memory_order_relaxed) + delta;
        update_min_max(new_value);
    }
    
    /**
     * @brief Decrement gauge by delta
     * @param delta Amount to decrement
     */
    void decrement(T delta = 1) noexcept {
        T new_value = value_.fetch_sub(delta, std::memory_order_relaxed) - delta;
        update_min_max(new_value);
    }
    
    /**
     * @brief Get current gauge value
     * @return Current value
     */
    T get() const noexcept {
        return value_.load(std::memory_order_relaxed);
    }
    
    json to_json() const override {
        return {
            {"type", "gauge"},
            {"name", name_},
            {"description", description_},
            {"value", get()},
            {"min", min_value_.load(std::memory_order_relaxed)},
            {"max", max_value_.load(std::memory_order_relaxed)}
        };
    }
    
    std::string type_name() const override { return "gauge"; }
    const std::string& name() const override { return name_; }
    const std::string& description() const override { return description_; }
    
private:
    void update_min_max(T value) noexcept {
        // Lock-free min update
        T current_min = min_value_.load(std::memory_order_relaxed);
        while (value < current_min && 
               !min_value_.compare_exchange_weak(current_min, value,
                   std::memory_order_relaxed));
        
        // Lock-free max update
        T current_max = max_value_.load(std::memory_order_relaxed);
        while (value > current_max && 
               !max_value_.compare_exchange_weak(current_max, value,
                   std::memory_order_relaxed));
    }
};

/**
 * @brief Histogram metric - distribution of values
 * @tparam T Numeric type for values
 * @tparam BucketCount Number of buckets for distribution
 */
template<typename T = double, size_t BucketCount = 64>
class histogram : public metric_interface {
private:
    struct bucket {
        alignas(64) std::atomic<uint64_t> count{0};
        alignas(64) std::atomic<T> sum{0};
    };
    
    std::array<bucket, BucketCount> buckets_;
    std::vector<T> bucket_boundaries_;
    alignas(64) std::atomic<uint64_t> total_count_{0};
    alignas(64) std::atomic<T> total_sum_{0};
    const std::string name_;
    const std::string description_;
    mutable std::shared_mutex boundaries_mutex_;
    
public:
    histogram(std::string name, std::string desc, 
              std::vector<T> boundaries = {})
        : name_(std::move(name))
        , description_(std::move(desc))
        , bucket_boundaries_(std::move(boundaries)) {
        
        if (bucket_boundaries_.empty()) {
            // Default exponential buckets: 0.001, 0.002, 0.004, ... seconds
            generate_exponential_buckets();
        }
        
        // Ensure boundaries are sorted
        std::sort(bucket_boundaries_.begin(), bucket_boundaries_.end());
    }
    
    /**
     * @brief Record a value in the histogram
     * @param value Value to record
     */
    void observe(T value) noexcept {
        total_count_.fetch_add(1, std::memory_order_relaxed);
        total_sum_.fetch_add(value, std::memory_order_relaxed);
        
        // Find appropriate bucket
        size_t bucket_idx = find_bucket(value);
        if (bucket_idx < buckets_.size()) {
            buckets_[bucket_idx].count.fetch_add(1, std::memory_order_relaxed);
            buckets_[bucket_idx].sum.fetch_add(value, std::memory_order_relaxed);
        }
    }
    
    /**
     * @brief Calculate percentile
     * @param p Percentile (0.0 to 1.0)
     * @return Estimated percentile value
     */
    T percentile(double p) const {
        std::shared_lock lock(boundaries_mutex_);
        
        uint64_t total = total_count_.load(std::memory_order_relaxed);
        if (total == 0) return 0;
        
        uint64_t target = static_cast<uint64_t>(total * p);
        uint64_t current = 0;
        
        for (size_t i = 0; i < bucket_boundaries_.size(); ++i) {
            current += buckets_[i].count.load(std::memory_order_relaxed);
            if (current >= target) {
                return bucket_boundaries_[i];
            }
        }
        
        return bucket_boundaries_.empty() ? 0 : bucket_boundaries_.back();
    }
    
    json to_json() const override {
        std::shared_lock lock(boundaries_mutex_);
        
        uint64_t total_count = total_count_.load(std::memory_order_relaxed);
        T total_sum = total_sum_.load(std::memory_order_relaxed);
        
        json buckets_json = json::array();
        for (size_t i = 0; i < bucket_boundaries_.size(); ++i) {
            uint64_t count = buckets_[i].count.load(std::memory_order_relaxed);
            if (count > 0) {
                buckets_json.push_back({
                    {"le", bucket_boundaries_[i]},
                    {"count", count}
                });
            }
        }
        
        return {
            {"type", "histogram"},
            {"name", name_},
            {"description", description_},
            {"count", total_count},
            {"sum", total_sum},
            {"mean", total_count > 0 ? total_sum / total_count : 0},
            {"p50", percentile(0.50)},
            {"p90", percentile(0.90)},
            {"p95", percentile(0.95)},
            {"p99", percentile(0.99)},
            {"p999", percentile(0.999)},
            {"buckets", buckets_json}
        };
    }
    
    std::string type_name() const override { return "histogram"; }
    const std::string& name() const override { return name_; }
    const std::string& description() const override { return description_; }
    
private:
    void generate_exponential_buckets() {
        bucket_boundaries_.clear();
        T value = 0.001;  // Start at 1ms
        for (size_t i = 0; i < std::min(size_t(20), BucketCount - 1); ++i) {
            bucket_boundaries_.push_back(value);
            value *= 2;
        }
        bucket_boundaries_.push_back(std::numeric_limits<T>::max());
    }
    
    size_t find_bucket(T value) const noexcept {
        // Binary search for efficiency
        auto it = std::lower_bound(bucket_boundaries_.begin(), 
                                  bucket_boundaries_.end(), value);
        return std::distance(bucket_boundaries_.begin(), it);
    }
};

/**
 * @brief Summary metric - sliding time window statistics
 * @tparam T Numeric type for values
 */
template<typename T = double>
class summary : public metric_interface {
private:
    struct time_window {
        std::chrono::steady_clock::time_point timestamp;
        T value;
    };
    
    mutable std::shared_mutex mutex_;
    std::deque<time_window> values_;
    const std::chrono::seconds window_duration_;
    const std::string name_;
    const std::string description_;
    
public:
    summary(std::string name, std::string desc, 
            std::chrono::seconds window = std::chrono::seconds(300))
        : window_duration_(window)
        , name_(std::move(name))
        , description_(std::move(desc)) {}
    
    /**
     * @brief Record a value
     * @param value Value to record
     */
    void observe(T value) {
        auto now = std::chrono::steady_clock::now();
        
        std::unique_lock lock(mutex_);
        values_.push_back({now, value});
        
        // Remove old values outside the window
        auto cutoff = now - window_duration_;
        while (!values_.empty() && values_.front().timestamp < cutoff) {
            values_.pop_front();
        }
    }
    
    json to_json() const override {
        std::shared_lock lock(mutex_);
        
        if (values_.empty()) {
            return {
                {"type", "summary"},
                {"name", name_},
                {"description", description_},
                {"window_seconds", window_duration_.count()},
                {"count", 0}
            };
        }
        
        // Calculate statistics
        std::vector<T> sorted_values;
        sorted_values.reserve(values_.size());
        
        T sum = 0;
        for (const auto& tw : values_) {
            sorted_values.push_back(tw.value);
            sum += tw.value;
        }
        
        T mean = sum / values_.size();
        
        // Sort for percentiles
        std::sort(sorted_values.begin(), sorted_values.end());
        
        auto percentile = [&sorted_values](double p) -> T {
            size_t idx = static_cast<size_t>(sorted_values.size() * p);
            return sorted_values[std::min(idx, sorted_values.size() - 1)];
        };
        
        return {
            {"type", "summary"},
            {"name", name_},
            {"description", description_},
            {"window_seconds", window_duration_.count()},
            {"count", values_.size()},
            {"mean", mean},
            {"p50", percentile(0.50)},
            {"p90", percentile(0.90)},
            {"p95", percentile(0.95)},
            {"p99", percentile(0.99)},
            {"p999", percentile(0.999)}
        };
    }
    
    std::string type_name() const override { return "summary"; }
    const std::string& name() const override { return name_; }
    const std::string& description() const override { return description_; }
};

}  // namespace metrics