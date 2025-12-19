/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, 🍀☀🌕🌥 🌊
All rights reserved.
*****************************************************************************/

/**
 * @file multi_process_monitoring_integration.cpp
 * @brief Example demonstrating integration with multi-process monitoring system
 *
 * This example shows how to:
 * - Use thread pools with proper instance identification
 * - Report metrics through the monitoring interface
 * - Handle multiple thread pools in the same process
 * - Integrate with process identification for multi-process scenarios
 *
 * @note Issue #312: Updated to use common::interfaces::IMonitor
 */

#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/common/interfaces/monitoring_interface.h>
#include <kcenon/thread/interfaces/thread_context.h>
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/core/thread_worker.h>
#include <kcenon/thread/utils/formatter.h>

#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
#include <random>

using namespace kcenon::thread;
using namespace utility_module;

// Implementation of IMonitor for multi-process monitoring
class sample_monitoring : public kcenon::common::interfaces::IMonitor {
public:
    using VoidResult = kcenon::common::VoidResult;

    VoidResult record_metric(const std::string& name, double value) override {
        std::cout << formatter::format("{}: {}\n", name, value);
        snapshot_.add_metric(name, value);
        return kcenon::common::ok();
    }

    VoidResult record_metric(
        const std::string& name,
        double value,
        const std::unordered_map<std::string, std::string>& tags) override {
        std::cout << formatter::format("{}: {}", name, value);
        if (!tags.empty()) {
            std::cout << " [";
            bool first = true;
            for (const auto& [k, v] : tags) {
                if (!first) std::cout << ", ";
                std::cout << k << "=" << v;
                first = false;
            }
            std::cout << "]";
        }
        std::cout << "\n";

        kcenon::common::interfaces::metric_value mv(name, value);
        mv.tags = tags;
        snapshot_.metrics.push_back(mv);
        return kcenon::common::ok();
    }

    kcenon::common::Result<kcenon::common::interfaces::metrics_snapshot> get_metrics() override {
        return kcenon::common::ok(snapshot_);
    }

    kcenon::common::Result<kcenon::common::interfaces::health_check_result> check_health() override {
        kcenon::common::interfaces::health_check_result result;
        result.status = kcenon::common::interfaces::health_status::healthy;
        result.message = "Sample monitoring active";
        return kcenon::common::ok(result);
    }

    VoidResult reset() override {
        snapshot_ = {};
        return kcenon::common::ok();
    }

private:
    kcenon::common::interfaces::metrics_snapshot snapshot_;
};

int main() {
    std::cout << "=== Multi-Process Monitoring Integration Example ===\n\n";

    // Create monitoring instance
    auto monitoring = std::make_shared<sample_monitoring>();

    // Create thread context with monitoring
    thread_context context(nullptr, monitoring);

    // Create multiple thread pools with unique names
    auto primary_pool = std::make_shared<thread_pool>("primary_pool", context);
    auto secondary_pool = std::make_shared<thread_pool>("secondary_pool", context);

    // Display pool instance IDs
    std::cout << formatter::format("Primary pool instance ID: {}\n", primary_pool->get_pool_instance_id());
    std::cout << formatter::format("Secondary pool instance ID: {}\n\n", secondary_pool->get_pool_instance_id());

    // Add workers then start pools
    {
        std::vector<std::unique_ptr<thread_worker>> workers;
        for (int i = 0; i < 3; ++i) workers.push_back(std::make_unique<thread_worker>());
        auto r = primary_pool->enqueue_batch(std::move(workers));
        if (r.is_err()) {
            std::cerr << "Failed to add workers to primary_pool: " << r.error().message << "\n";
            return 1;
        }
    }
    {
        std::vector<std::unique_ptr<thread_worker>> workers;
        for (int i = 0; i < 2; ++i) workers.push_back(std::make_unique<thread_worker>());
        auto r = secondary_pool->enqueue_batch(std::move(workers));
        if (r.is_err()) {
            std::cerr << "Failed to add workers to secondary_pool: " << r.error().message << "\n";
            return 1;
        }
    }

    auto start_primary = primary_pool->start();
    if (start_primary.is_err()) {
        std::cerr << "Failed to start primary_pool: " << start_primary.error().message << "\n";
        return 1;
    }
    auto start_secondary = secondary_pool->start();
    if (start_secondary.is_err()) {
        std::cerr << "Failed to start secondary_pool: " << start_secondary.error().message << "\n";
        return 1;
    }

    // Report initial metrics
    primary_pool->report_metrics();
    secondary_pool->report_metrics();

    std::cout << "\n--- Submitting jobs ---\n";

    // Submit jobs to primary pool
    for (int i = 0; i < 10; ++i) {
        auto job = std::make_unique<callback_job>(
            [i]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(50 + i * 10));
                std::cout << formatter::format("Primary job {} completed\n", i);
                return kcenon::common::ok();
            },
            formatter::format("primary_job_{}", i)
        );
        auto r = primary_pool->enqueue(std::move(job));
        if (r.is_err()) {
            std::cerr << "enqueue to primary_pool failed: " << r.error().message << "\n";
        }
    }

    // Submit jobs to secondary pool
    for (int i = 0; i < 5; ++i) {
        auto job = std::make_unique<callback_job>(
            [i]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                std::cout << formatter::format("Secondary job {} completed\n", i);
                return kcenon::common::ok();
            },
            formatter::format("secondary_job_{}", i)
        );
        auto r = secondary_pool->enqueue(std::move(job));
        if (r.is_err()) {
            std::cerr << "enqueue to secondary_pool failed: " << r.error().message << "\n";
        }
    }

    // Periodically report metrics while jobs are processing
    for (int i = 0; i < 3; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::cout << "\n--- Metrics Update ---\n";
        primary_pool->report_metrics();
        secondary_pool->report_metrics();
    }

    // Stop pools
    std::cout << "\n--- Stopping pools ---\n";
    auto stop_primary = primary_pool->stop();
    if (stop_primary.is_err()) {
        std::cerr << "Error stopping primary_pool: " << stop_primary.error().message << "\n";
    }
    auto stop_secondary = secondary_pool->stop();
    if (stop_secondary.is_err()) {
        std::cerr << "Error stopping secondary_pool: " << stop_secondary.error().message << "\n";
    }

    // Final metrics
    std::cout << "\n--- Final Metrics ---\n";
    primary_pool->report_metrics();
    secondary_pool->report_metrics();

    std::cout << "\n=== Example completed ===\n";

    return 0;
}
