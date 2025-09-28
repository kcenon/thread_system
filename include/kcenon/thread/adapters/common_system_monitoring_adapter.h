/*
 * BSD 3-Clause License
 * Copyright (c) 2025, kcenon
 */

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

// Check if common_system is available
#ifdef USE_COMMON_SYSTEM
#include <kcenon/common/interfaces/monitoring_interface.h>
#include <kcenon/common/patterns/result.h>
#endif

#include "../interfaces/monitorable_interface.h"
#include "../interfaces/monitoring_interface.h"

namespace kcenon::thread::adapters {

#ifdef USE_COMMON_SYSTEM

/**
 * @brief Adapter to expose thread_system monitorable as common::interfaces::IMonitorable
 *
 * This adapter allows thread_system's monitorable objects to be monitored
 * through the standard common_system monitoring interface.
 */
class common_system_monitorable_adapter : public ::common::interfaces::IMonitorable {
public:
    /**
     * @brief Construct adapter with thread_system monitorable
     * @param monitorable Shared pointer to thread_system monitorable
     * @param name Component name
     */
    explicit common_system_monitorable_adapter(
        std::shared_ptr<monitorable_interface> monitorable,
        const std::string& name = "thread_component")
        : monitorable_(monitorable), component_name_(name) {}

    ~common_system_monitorable_adapter() override = default;

    /**
     * @brief Get monitoring data
     * @return Result containing metrics snapshot or error
     */
    ::common::Result<::common::interfaces::metrics_snapshot> get_monitoring_data() override {
        if (!monitorable_) {
            return ::common::error_info(1, "Monitorable not initialized", "thread_system");
        }

        auto thread_data = monitorable_->get_monitoring_data();
        if (!thread_data) {
            return ::common::error_info(2, "Failed to get monitoring data", "thread_system");
        }

        // Convert thread_system monitoring_data to common metrics_snapshot
        ::common::interfaces::metrics_snapshot snapshot;
        snapshot.source_id = component_name_;

        // Extract metrics from thread_system monitoring_data
        if (thread_data->active_threads.has_value()) {
            snapshot.add_metric("active_threads",
                              static_cast<double>(thread_data->active_threads.value()));
        }
        if (thread_data->pending_tasks.has_value()) {
            snapshot.add_metric("pending_tasks",
                              static_cast<double>(thread_data->pending_tasks.value()));
        }
        if (thread_data->total_tasks.has_value()) {
            snapshot.add_metric("total_tasks",
                              static_cast<double>(thread_data->total_tasks.value()));
        }
        if (thread_data->failed_tasks.has_value()) {
            snapshot.add_metric("failed_tasks",
                              static_cast<double>(thread_data->failed_tasks.value()));
        }

        return snapshot;
    }

    /**
     * @brief Check health status
     * @return Result containing health check result or error
     */
    ::common::Result<::common::interfaces::health_check_result> health_check() override {
        if (!monitorable_) {
            return ::common::error_info(1, "Monitorable not initialized", "thread_system");
        }

        auto thread_health = monitorable_->health_check();

        // Convert thread_system health to common health_check_result
        ::common::interfaces::health_check_result result;

        if (thread_health.is_healthy) {
            result.status = ::common::interfaces::health_status::healthy;
            result.message = thread_health.message.value_or("Healthy");
        } else if (thread_health.is_operational) {
            result.status = ::common::interfaces::health_status::degraded;
            result.message = thread_health.message.value_or("Degraded");
        } else {
            result.status = ::common::interfaces::health_status::unhealthy;
            result.message = thread_health.message.value_or("Unhealthy");
        }

        return result;
    }

    /**
     * @brief Get component name for monitoring
     * @return Component identifier
     */
    std::string get_component_name() const override {
        return component_name_;
    }

private:
    std::shared_ptr<monitorable_interface> monitorable_;
    std::string component_name_;
};

/**
 * @brief Adapter to expose common::interfaces::IMonitorable as thread_system monitorable
 *
 * This adapter allows a common_system monitorable to be used
 * through the thread_system monitorable interface.
 */
class monitorable_from_common_adapter : public monitorable_interface {
public:
    /**
     * @brief Construct adapter with common monitorable
     * @param monitorable Shared pointer to common monitorable
     */
    explicit monitorable_from_common_adapter(
        std::shared_ptr<::common::interfaces::IMonitorable> monitorable)
        : monitorable_(monitorable) {}

    ~monitorable_from_common_adapter() override = default;

    /**
     * @brief Get monitoring data
     */
    std::optional<monitoring_data> get_monitoring_data() const override {
        if (!monitorable_) {
            return std::nullopt;
        }

        auto result = monitorable_->get_monitoring_data();
        if (::common::is_error(result)) {
            return std::nullopt;
        }

        const auto& snapshot = ::common::get_value(result);
        monitoring_data data;

        // Convert common metrics_snapshot to thread_system monitoring_data
        for (const auto& metric : snapshot.metrics) {
            if (metric.name == "active_threads") {
                data.active_threads = static_cast<std::size_t>(metric.value);
            } else if (metric.name == "pending_tasks") {
                data.pending_tasks = static_cast<std::size_t>(metric.value);
            } else if (metric.name == "total_tasks") {
                data.total_tasks = static_cast<std::size_t>(metric.value);
            } else if (metric.name == "failed_tasks") {
                data.failed_tasks = static_cast<std::size_t>(metric.value);
            }
        }

        data.timestamp = std::chrono::steady_clock::now();
        return data;
    }

    /**
     * @brief Check health status
     */
    health_status health_check() const override {
        if (!monitorable_) {
            return health_status{false, false, "Monitorable not initialized"};
        }

        auto result = monitorable_->health_check();
        if (::common::is_error(result)) {
            return health_status{false, false, "Health check failed"};
        }

        const auto& health = ::common::get_value(result);
        return health_status{
            health.is_healthy(),
            health.is_operational(),
            health.message
        };
    }

    /**
     * @brief Reset monitoring statistics
     */
    void reset_stats() override {
        // Common system doesn't have reset in IMonitorable interface
        // This is a no-op
    }

private:
    std::shared_ptr<::common::interfaces::IMonitorable> monitorable_;
};

#endif // USE_COMMON_SYSTEM

} // namespace kcenon::thread::adapters