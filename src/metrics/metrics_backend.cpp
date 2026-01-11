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

#include <kcenon/thread/metrics/metrics_backend.h>

#include <iomanip>
#include <sstream>

namespace kcenon::thread::metrics {

// =============================================================================
// PrometheusBackend Implementation
// =============================================================================

std::string PrometheusBackend::format_labels() const {
    if (labels_.empty()) {
        return "";
    }

    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& [key, value] : labels_) {
        if (!first) {
            oss << ",";
        }
        oss << key << "=\"" << value << "\"";
        first = false;
    }
    oss << "}";
    return oss.str();
}

std::string PrometheusBackend::export_base(const BaseSnapshot& snapshot) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    const auto& p = prefix_;
    const auto labels = format_labels();

    // Task counters
    oss << "# HELP " << p << "_tasks_submitted_total Total tasks submitted\n";
    oss << "# TYPE " << p << "_tasks_submitted_total counter\n";
    oss << p << "_tasks_submitted_total" << labels << " " << snapshot.tasks_submitted << "\n\n";

    oss << "# HELP " << p << "_tasks_executed_total Total tasks executed\n";
    oss << "# TYPE " << p << "_tasks_executed_total counter\n";
    oss << p << "_tasks_executed_total" << labels << " " << snapshot.tasks_executed << "\n\n";

    oss << "# HELP " << p << "_tasks_failed_total Total tasks failed\n";
    oss << "# TYPE " << p << "_tasks_failed_total counter\n";
    oss << p << "_tasks_failed_total" << labels << " " << snapshot.tasks_failed << "\n\n";

    // Time counters
    oss << "# HELP " << p << "_busy_time_nanoseconds_total Total busy time\n";
    oss << "# TYPE " << p << "_busy_time_nanoseconds_total counter\n";
    oss << p << "_busy_time_nanoseconds_total" << labels << " " << snapshot.total_busy_time_ns << "\n\n";

    oss << "# HELP " << p << "_idle_time_nanoseconds_total Total idle time\n";
    oss << "# TYPE " << p << "_idle_time_nanoseconds_total counter\n";
    oss << p << "_idle_time_nanoseconds_total" << labels << " " << snapshot.total_idle_time_ns << "\n";

    return oss.str();
}

std::string PrometheusBackend::export_enhanced(const EnhancedSnapshot& snapshot) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    const auto& p = prefix_;
    const auto labels = format_labels();

    // Task counters
    oss << "# HELP " << p << "_tasks_submitted_total Total tasks submitted\n";
    oss << "# TYPE " << p << "_tasks_submitted_total counter\n";
    oss << p << "_tasks_submitted_total" << labels << " " << snapshot.tasks_submitted << "\n\n";

    oss << "# HELP " << p << "_tasks_executed_total Total tasks executed\n";
    oss << "# TYPE " << p << "_tasks_executed_total counter\n";
    oss << p << "_tasks_executed_total" << labels << " " << snapshot.tasks_executed << "\n\n";

    oss << "# HELP " << p << "_tasks_failed_total Total tasks failed\n";
    oss << "# TYPE " << p << "_tasks_failed_total counter\n";
    oss << p << "_tasks_failed_total" << labels << " " << snapshot.tasks_failed << "\n\n";

    // Latency summary (enqueue)
    oss << "# HELP " << p << "_enqueue_latency_us Enqueue latency in microseconds\n";
    oss << "# TYPE " << p << "_enqueue_latency_us summary\n";
    oss << p << "_enqueue_latency_us{quantile=\"0.5\"" << (labels.empty() ? "" : "," + labels.substr(1, labels.size() - 2)) << "} " << snapshot.enqueue_latency_p50_us << "\n";
    oss << p << "_enqueue_latency_us{quantile=\"0.9\"" << (labels.empty() ? "" : "," + labels.substr(1, labels.size() - 2)) << "} " << snapshot.enqueue_latency_p90_us << "\n";
    oss << p << "_enqueue_latency_us{quantile=\"0.99\"" << (labels.empty() ? "" : "," + labels.substr(1, labels.size() - 2)) << "} " << snapshot.enqueue_latency_p99_us << "\n\n";

    // Latency summary (execution)
    oss << "# HELP " << p << "_execution_latency_us Execution latency in microseconds\n";
    oss << "# TYPE " << p << "_execution_latency_us summary\n";
    oss << p << "_execution_latency_us{quantile=\"0.5\"" << (labels.empty() ? "" : "," + labels.substr(1, labels.size() - 2)) << "} " << snapshot.execution_latency_p50_us << "\n";
    oss << p << "_execution_latency_us{quantile=\"0.9\"" << (labels.empty() ? "" : "," + labels.substr(1, labels.size() - 2)) << "} " << snapshot.execution_latency_p90_us << "\n";
    oss << p << "_execution_latency_us{quantile=\"0.99\"" << (labels.empty() ? "" : "," + labels.substr(1, labels.size() - 2)) << "} " << snapshot.execution_latency_p99_us << "\n\n";

    // Latency summary (wait time)
    oss << "# HELP " << p << "_wait_time_us Queue wait time in microseconds\n";
    oss << "# TYPE " << p << "_wait_time_us summary\n";
    oss << p << "_wait_time_us{quantile=\"0.5\"" << (labels.empty() ? "" : "," + labels.substr(1, labels.size() - 2)) << "} " << snapshot.wait_time_p50_us << "\n";
    oss << p << "_wait_time_us{quantile=\"0.9\"" << (labels.empty() ? "" : "," + labels.substr(1, labels.size() - 2)) << "} " << snapshot.wait_time_p90_us << "\n";
    oss << p << "_wait_time_us{quantile=\"0.99\"" << (labels.empty() ? "" : "," + labels.substr(1, labels.size() - 2)) << "} " << snapshot.wait_time_p99_us << "\n\n";

    // Throughput
    oss << "# HELP " << p << "_throughput_1s Tasks per second (1s window)\n";
    oss << "# TYPE " << p << "_throughput_1s gauge\n";
    oss << p << "_throughput_1s" << labels << " " << snapshot.throughput_1s << "\n\n";

    oss << "# HELP " << p << "_throughput_1m Tasks per second (1m window)\n";
    oss << "# TYPE " << p << "_throughput_1m gauge\n";
    oss << p << "_throughput_1m" << labels << " " << snapshot.throughput_1m << "\n\n";

    // Queue depth
    oss << "# HELP " << p << "_queue_depth_current Current queue depth\n";
    oss << "# TYPE " << p << "_queue_depth_current gauge\n";
    oss << p << "_queue_depth_current" << labels << " " << snapshot.current_queue_depth << "\n\n";

    oss << "# HELP " << p << "_queue_depth_peak Peak queue depth\n";
    oss << "# TYPE " << p << "_queue_depth_peak gauge\n";
    oss << p << "_queue_depth_peak" << labels << " " << snapshot.peak_queue_depth << "\n\n";

    // Worker utilization
    oss << "# HELP " << p << "_worker_utilization Overall worker utilization\n";
    oss << "# TYPE " << p << "_worker_utilization gauge\n";
    oss << p << "_worker_utilization" << labels << " " << snapshot.worker_utilization << "\n\n";

    oss << "# HELP " << p << "_active_workers Number of active workers\n";
    oss << "# TYPE " << p << "_active_workers gauge\n";
    oss << p << "_active_workers" << labels << " " << snapshot.active_workers << "\n\n";

    // Per-worker utilization
    oss << "# HELP " << p << "_worker_utilization_per_worker Per-worker utilization\n";
    oss << "# TYPE " << p << "_worker_utilization_per_worker gauge\n";
    for (std::size_t i = 0; i < snapshot.per_worker_utilization.size(); ++i) {
        oss << p << "_worker_utilization_per_worker{worker=\"" << i << "\"";
        if (!labels_.empty()) {
            for (const auto& [key, value] : labels_) {
                oss << "," << key << "=\"" << value << "\"";
            }
        }
        oss << "} " << snapshot.per_worker_utilization[i] << "\n";
    }

    return oss.str();
}

// =============================================================================
// JsonBackend Implementation
// =============================================================================

std::string JsonBackend::export_base(const BaseSnapshot& snapshot) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    if (pretty_) {
        oss << "{\n";
        oss << "  \"tasks\": {\n";
        oss << "    \"submitted\": " << snapshot.tasks_submitted << ",\n";
        oss << "    \"executed\": " << snapshot.tasks_executed << ",\n";
        oss << "    \"failed\": " << snapshot.tasks_failed << "\n";
        oss << "  },\n";
        oss << "  \"timing_ns\": {\n";
        oss << "    \"busy\": " << snapshot.total_busy_time_ns << ",\n";
        oss << "    \"idle\": " << snapshot.total_idle_time_ns << "\n";
        oss << "  }\n";
        oss << "}";
    } else {
        oss << "{\"tasks\":{\"submitted\":" << snapshot.tasks_submitted
            << ",\"executed\":" << snapshot.tasks_executed
            << ",\"failed\":" << snapshot.tasks_failed
            << "},\"timing_ns\":{\"busy\":" << snapshot.total_busy_time_ns
            << ",\"idle\":" << snapshot.total_idle_time_ns << "}}";
    }

    return oss.str();
}

std::string JsonBackend::export_enhanced(const EnhancedSnapshot& snapshot) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    if (pretty_) {
        oss << "{\n";
        oss << "  \"tasks\": {\n";
        oss << "    \"submitted\": " << snapshot.tasks_submitted << ",\n";
        oss << "    \"executed\": " << snapshot.tasks_executed << ",\n";
        oss << "    \"failed\": " << snapshot.tasks_failed << "\n";
        oss << "  },\n";

        oss << "  \"latency_us\": {\n";
        oss << "    \"enqueue\": { \"p50\": " << snapshot.enqueue_latency_p50_us
            << ", \"p90\": " << snapshot.enqueue_latency_p90_us
            << ", \"p99\": " << snapshot.enqueue_latency_p99_us << " },\n";
        oss << "    \"execution\": { \"p50\": " << snapshot.execution_latency_p50_us
            << ", \"p90\": " << snapshot.execution_latency_p90_us
            << ", \"p99\": " << snapshot.execution_latency_p99_us << " },\n";
        oss << "    \"wait_time\": { \"p50\": " << snapshot.wait_time_p50_us
            << ", \"p90\": " << snapshot.wait_time_p90_us
            << ", \"p99\": " << snapshot.wait_time_p99_us << " }\n";
        oss << "  },\n";

        oss << "  \"throughput\": {\n";
        oss << "    \"rate_1s\": " << snapshot.throughput_1s << ",\n";
        oss << "    \"rate_1m\": " << snapshot.throughput_1m << "\n";
        oss << "  },\n";

        oss << "  \"queue\": {\n";
        oss << "    \"current_depth\": " << snapshot.current_queue_depth << ",\n";
        oss << "    \"peak_depth\": " << snapshot.peak_queue_depth << ",\n";
        oss << "    \"avg_depth\": " << snapshot.avg_queue_depth << "\n";
        oss << "  },\n";

        oss << "  \"workers\": {\n";
        oss << "    \"active\": " << snapshot.active_workers << ",\n";
        oss << "    \"utilization\": " << snapshot.worker_utilization << ",\n";
        oss << "    \"per_worker_utilization\": [";
        for (std::size_t i = 0; i < snapshot.per_worker_utilization.size(); ++i) {
            if (i > 0) {
                oss << ", ";
            }
            oss << snapshot.per_worker_utilization[i];
        }
        oss << "]\n";
        oss << "  }\n";

        oss << "}";
    } else {
        oss << "{\"tasks\":{\"submitted\":" << snapshot.tasks_submitted
            << ",\"executed\":" << snapshot.tasks_executed
            << ",\"failed\":" << snapshot.tasks_failed
            << "},\"latency_us\":{\"enqueue\":{\"p50\":" << snapshot.enqueue_latency_p50_us
            << ",\"p90\":" << snapshot.enqueue_latency_p90_us
            << ",\"p99\":" << snapshot.enqueue_latency_p99_us
            << "},\"execution\":{\"p50\":" << snapshot.execution_latency_p50_us
            << ",\"p90\":" << snapshot.execution_latency_p90_us
            << ",\"p99\":" << snapshot.execution_latency_p99_us
            << "},\"wait_time\":{\"p50\":" << snapshot.wait_time_p50_us
            << ",\"p90\":" << snapshot.wait_time_p90_us
            << ",\"p99\":" << snapshot.wait_time_p99_us
            << "}},\"throughput\":{\"rate_1s\":" << snapshot.throughput_1s
            << ",\"rate_1m\":" << snapshot.throughput_1m
            << "},\"queue\":{\"current_depth\":" << snapshot.current_queue_depth
            << ",\"peak_depth\":" << snapshot.peak_queue_depth
            << ",\"avg_depth\":" << snapshot.avg_queue_depth
            << "},\"workers\":{\"active\":" << snapshot.active_workers
            << ",\"utilization\":" << snapshot.worker_utilization
            << ",\"per_worker_utilization\":[";
        for (std::size_t i = 0; i < snapshot.per_worker_utilization.size(); ++i) {
            if (i > 0) {
                oss << ",";
            }
            oss << snapshot.per_worker_utilization[i];
        }
        oss << "]}}";
    }

    return oss.str();
}

// =============================================================================
// LoggingBackend Implementation
// =============================================================================

std::string LoggingBackend::export_base(const BaseSnapshot& snapshot) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    oss << "[" << prefix_ << "] Metrics Summary:\n";
    oss << "  Tasks: submitted=" << snapshot.tasks_submitted
        << ", executed=" << snapshot.tasks_executed
        << ", failed=" << snapshot.tasks_failed << "\n";

    auto total = snapshot.total_busy_time_ns + snapshot.total_idle_time_ns;
    double utilization = (total > 0)
        ? 100.0 * static_cast<double>(snapshot.total_busy_time_ns) / static_cast<double>(total)
        : 0.0;
    oss << "  Time: busy=" << (snapshot.total_busy_time_ns / 1'000'000.0) << "ms"
        << ", idle=" << (snapshot.total_idle_time_ns / 1'000'000.0) << "ms"
        << " (utilization=" << utilization << "%)";

    return oss.str();
}

std::string LoggingBackend::export_enhanced(const EnhancedSnapshot& snapshot) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    oss << "[" << prefix_ << "] Enhanced Metrics:\n";
    oss << "  Tasks: submitted=" << snapshot.tasks_submitted
        << ", executed=" << snapshot.tasks_executed
        << ", failed=" << snapshot.tasks_failed << "\n";

    oss << "  Latency (P50/P90/P99):\n";
    oss << "    Enqueue: " << snapshot.enqueue_latency_p50_us << "/"
        << snapshot.enqueue_latency_p90_us << "/" << snapshot.enqueue_latency_p99_us << " us\n";
    oss << "    Execute: " << snapshot.execution_latency_p50_us << "/"
        << snapshot.execution_latency_p90_us << "/" << snapshot.execution_latency_p99_us << " us\n";
    oss << "    Wait:    " << snapshot.wait_time_p50_us << "/"
        << snapshot.wait_time_p90_us << "/" << snapshot.wait_time_p99_us << " us\n";

    oss << "  Throughput: " << snapshot.throughput_1s << " ops/sec (1s), "
        << snapshot.throughput_1m << " ops/sec (1m)\n";

    oss << "  Queue: depth=" << snapshot.current_queue_depth
        << ", peak=" << snapshot.peak_queue_depth
        << ", avg=" << snapshot.avg_queue_depth << "\n";

    oss << "  Workers: active=" << snapshot.active_workers
        << ", utilization=" << (snapshot.worker_utilization * 100.0) << "%";

    return oss.str();
}

} // namespace kcenon::thread::metrics
