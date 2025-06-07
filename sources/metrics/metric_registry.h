#pragma once

#include "metric_types.h"
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <functional>
#include <regex>

namespace metrics {

/**
 * @brief Central registry for all metrics
 * 
 * Thread-safe registry that manages metric lifecycle and provides
 * centralized access to all registered metrics.
 */
class metric_registry {
private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<metric_interface>> metrics_;
    
    // Singleton instance
    metric_registry() = default;
    
public:
    /**
     * @brief Get singleton instance
     * @return Reference to the global metric registry
     */
    static metric_registry& instance() {
        static metric_registry instance;
        return instance;
    }
    
    // Delete copy/move constructors
    metric_registry(const metric_registry&) = delete;
    metric_registry& operator=(const metric_registry&) = delete;
    metric_registry(metric_registry&&) = delete;
    metric_registry& operator=(metric_registry&&) = delete;
    
    /**
     * @brief Register a new metric
     * @tparam MetricType Type of metric to register
     * @param name Unique metric name
     * @param args Arguments forwarded to metric constructor
     * @return Shared pointer to the created metric
     */
    template<typename MetricType, typename... Args>
    std::shared_ptr<MetricType> register_metric(const std::string& name, Args&&... args) {
        auto metric = std::make_shared<MetricType>(name, std::forward<Args>(args)...);
        
        std::unique_lock lock(mutex_);
        auto [it, inserted] = metrics_.try_emplace(name, metric);
        
        if (!inserted) {
            // Metric already exists, return existing one if type matches
            auto existing = std::dynamic_pointer_cast<MetricType>(it->second);
            if (existing) {
                return existing;
            }
            throw std::runtime_error("Metric '" + name + "' already exists with different type");
        }
        
        return metric;
    }
    
    /**
     * @brief Get existing metric
     * @tparam MetricType Expected metric type
     * @param name Metric name
     * @return Shared pointer to metric or nullptr if not found
     */
    template<typename MetricType>
    std::shared_ptr<MetricType> get_metric(const std::string& name) const {
        std::shared_lock lock(mutex_);
        auto it = metrics_.find(name);
        if (it != metrics_.end()) {
            return std::dynamic_pointer_cast<MetricType>(it->second);
        }
        return nullptr;
    }
    
    /**
     * @brief Remove a metric from registry
     * @param name Metric name to remove
     * @return true if metric was removed, false if not found
     */
    bool remove_metric(const std::string& name) {
        std::unique_lock lock(mutex_);
        return metrics_.erase(name) > 0;
    }
    
    /**
     * @brief Clear all metrics
     */
    void clear() {
        std::unique_lock lock(mutex_);
        metrics_.clear();
    }
    
    /**
     * @brief Collect all metrics as JSON
     * @return JSON object containing all metrics
     */
    json collect_all() const {
        std::shared_lock lock(mutex_);
        json result;
        
        for (const auto& [name, metric] : metrics_) {
            result[name] = metric->to_json();
        }
        
        return result;
    }
    
    /**
     * @brief Collect metrics matching a pattern
     * @param pattern Pattern to match (supports * wildcard)
     * @return JSON object containing matching metrics
     */
    json collect_pattern(const std::string& pattern) const {
        std::shared_lock lock(mutex_);
        json result;
        
        std::regex regex_pattern(
            std::regex_replace(pattern, std::regex("\\*"), ".*")
        );
        
        for (const auto& [name, metric] : metrics_) {
            if (std::regex_match(name, regex_pattern)) {
                result[name] = metric->to_json();
            }
        }
        
        return result;
    }
    
    /**
     * @brief Get list of all metric names
     * @return Vector of metric names
     */
    std::vector<std::string> list_metrics() const {
        std::shared_lock lock(mutex_);
        std::vector<std::string> names;
        names.reserve(metrics_.size());
        
        for (const auto& [name, _] : metrics_) {
            names.push_back(name);
        }
        
        return names;
    }
    
    /**
     * @brief Apply function to all metrics
     * @param fn Function to apply to each metric
     */
    void for_each(const std::function<void(const std::string&, const metric_interface&)>& fn) const {
        std::shared_lock lock(mutex_);
        for (const auto& [name, metric] : metrics_) {
            fn(name, *metric);
        }
    }
};

// Convenience functions
template<typename MetricType, typename... Args>
std::shared_ptr<MetricType> make_metric(const std::string& name, Args&&... args) {
    return metric_registry::instance().register_metric<MetricType>(name, std::forward<Args>(args)...);
}

template<typename MetricType>
std::shared_ptr<MetricType> get_metric(const std::string& name) {
    return metric_registry::instance().get_metric<MetricType>(name);
}

}  // namespace metrics