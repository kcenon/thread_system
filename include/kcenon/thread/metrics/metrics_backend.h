/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, kcenon
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

#include <kcenon/thread/metrics/enhanced_metrics.h>
#include <kcenon/thread/metrics/metrics_base.h>

#include <functional>
#include <map>
#include <memory>
#include <string>

namespace kcenon::thread::metrics {

/**
 * @brief Abstract interface for metrics export backends.
 *
 * @ingroup metrics
 *
 * This interface defines the contract for exporting metrics to various
 * monitoring systems (Prometheus, JSON, logging, etc.).
 *
 * ### Implementation Guidelines
 * - Implementations should be thread-safe
 * - Export methods should not throw exceptions
 * - Use canonical metric naming conventions
 *
 * ### Example Implementation
 * @code
 * class CustomBackend : public MetricsBackend {
 * public:
 *     std::string name() const override { return "custom"; }
 *
 *     std::string export_base(const BaseSnapshot& snap) const override {
 *         // Export base metrics in custom format
 *     }
 *
 *     std::string export_enhanced(const EnhancedSnapshot& snap) const override {
 *         // Export enhanced metrics in custom format
 *     }
 * };
 * @endcode
 *
 * @see PrometheusBackend
 * @see JsonBackend
 * @see LoggingBackend
 */
class MetricsBackend {
public:
    /**
     * @brief Virtual destructor for proper cleanup.
     */
    virtual ~MetricsBackend() = default;

    /**
     * @brief Get the backend name.
     * @return Human-readable backend identifier (e.g., "prometheus", "json").
     */
    [[nodiscard]] virtual std::string name() const = 0;

    /**
     * @brief Export base metrics snapshot.
     * @param snapshot The base metrics snapshot to export.
     * @return Formatted string representation.
     */
    [[nodiscard]] virtual std::string export_base(
        const BaseSnapshot& snapshot) const = 0;

    /**
     * @brief Export enhanced metrics snapshot.
     * @param snapshot The enhanced metrics snapshot to export.
     * @return Formatted string representation.
     */
    [[nodiscard]] virtual std::string export_enhanced(
        const EnhancedSnapshot& snapshot) const = 0;

    /**
     * @brief Set metric name prefix.
     * @param prefix Prefix to prepend to all metric names.
     */
    virtual void set_prefix(const std::string& prefix) {
        prefix_ = prefix;
    }

    /**
     * @brief Get current metric name prefix.
     * @return Current prefix string.
     */
    [[nodiscard]] const std::string& prefix() const {
        return prefix_;
    }

    /**
     * @brief Add a label to all exported metrics.
     * @param key Label key.
     * @param value Label value.
     */
    virtual void add_label(const std::string& key, const std::string& value) {
        labels_[key] = value;
    }

    /**
     * @brief Get all configured labels.
     * @return Map of label key-value pairs.
     */
    [[nodiscard]] const std::map<std::string, std::string>& labels() const {
        return labels_;
    }

protected:
    /**
     * @brief Default constructor.
     */
    MetricsBackend() = default;

    /**
     * @brief Metric name prefix.
     */
    std::string prefix_{"thread_pool"};

    /**
     * @brief Labels to attach to all metrics.
     */
    std::map<std::string, std::string> labels_;
};

/**
 * @brief Prometheus/OpenMetrics format backend.
 *
 * Exports metrics in Prometheus exposition format, suitable for
 * scraping by Prometheus servers.
 *
 * ### Output Format
 * @code
 * # HELP thread_pool_tasks_submitted_total Total tasks submitted
 * # TYPE thread_pool_tasks_submitted_total counter
 * thread_pool_tasks_submitted_total 1234
 * @endcode
 */
class PrometheusBackend : public MetricsBackend {
public:
    /**
     * @brief Default constructor.
     */
    PrometheusBackend() = default;

    /**
     * @brief Destructor.
     */
    ~PrometheusBackend() override = default;

    [[nodiscard]] std::string name() const override {
        return "prometheus";
    }

    [[nodiscard]] std::string export_base(
        const BaseSnapshot& snapshot) const override;

    [[nodiscard]] std::string export_enhanced(
        const EnhancedSnapshot& snapshot) const override;

private:
    /**
     * @brief Format labels for Prometheus output.
     * @return Formatted label string (e.g., {key="value"}).
     */
    [[nodiscard]] std::string format_labels() const;
};

/**
 * @brief JSON format backend.
 *
 * Exports metrics as JSON objects, suitable for REST APIs,
 * logging systems, or web dashboards.
 *
 * ### Output Format
 * @code
 * {
 *   "tasks": {
 *     "submitted": 1234,
 *     "executed": 1200,
 *     "failed": 5
 *   }
 * }
 * @endcode
 */
class JsonBackend : public MetricsBackend {
public:
    /**
     * @brief Default constructor.
     */
    JsonBackend() = default;

    /**
     * @brief Destructor.
     */
    ~JsonBackend() override = default;

    [[nodiscard]] std::string name() const override {
        return "json";
    }

    [[nodiscard]] std::string export_base(
        const BaseSnapshot& snapshot) const override;

    [[nodiscard]] std::string export_enhanced(
        const EnhancedSnapshot& snapshot) const override;

    /**
     * @brief Enable or disable pretty printing.
     * @param pretty True for indented output, false for compact.
     */
    void set_pretty(bool pretty) {
        pretty_ = pretty;
    }

private:
    bool pretty_{true};
};

/**
 * @brief Logging backend for debugging and diagnostics.
 *
 * Exports metrics in a human-readable format suitable for
 * log files and console output.
 */
class LoggingBackend : public MetricsBackend {
public:
    /**
     * @brief Default constructor.
     */
    LoggingBackend() = default;

    /**
     * @brief Destructor.
     */
    ~LoggingBackend() override = default;

    [[nodiscard]] std::string name() const override {
        return "logging";
    }

    [[nodiscard]] std::string export_base(
        const BaseSnapshot& snapshot) const override;

    [[nodiscard]] std::string export_enhanced(
        const EnhancedSnapshot& snapshot) const override;
};

/**
 * @brief Backend registry for managing multiple export formats.
 *
 * Allows registration and retrieval of metrics backends by name.
 */
class BackendRegistry {
public:
    /**
     * @brief Get the singleton instance.
     * @return Reference to the global registry.
     */
    static BackendRegistry& instance() {
        static BackendRegistry registry;
        return registry;
    }

    /**
     * @brief Register a backend.
     * @param backend Shared pointer to the backend.
     */
    void register_backend(std::shared_ptr<MetricsBackend> backend) {
        if (backend) {
            backends_[backend->name()] = std::move(backend);
        }
    }

    /**
     * @brief Get a backend by name.
     * @param name Backend identifier.
     * @return Shared pointer to the backend, or nullptr if not found.
     */
    [[nodiscard]] std::shared_ptr<MetricsBackend> get(
        const std::string& name) const {
        auto it = backends_.find(name);
        return (it != backends_.end()) ? it->second : nullptr;
    }

    /**
     * @brief Check if a backend is registered.
     * @param name Backend identifier.
     * @return True if registered.
     */
    [[nodiscard]] bool has(const std::string& name) const {
        return backends_.find(name) != backends_.end();
    }

private:
    BackendRegistry() {
        // Register default backends
        register_backend(std::make_shared<PrometheusBackend>());
        register_backend(std::make_shared<JsonBackend>());
        register_backend(std::make_shared<LoggingBackend>());
    }

    std::map<std::string, std::shared_ptr<MetricsBackend>> backends_;
};

} // namespace kcenon::thread::metrics
