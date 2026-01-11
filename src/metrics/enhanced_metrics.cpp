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

#include <kcenon/thread/metrics/enhanced_metrics.h>
#include <kcenon/thread/metrics/metrics_backend.h>

namespace kcenon::thread::metrics {

EnhancedThreadPoolMetrics::EnhancedThreadPoolMetrics(std::size_t worker_count)
    : throughput_1s_(std::chrono::seconds{1}),
      throughput_1m_(std::chrono::seconds{60}),
      per_worker_metrics_(worker_count) {
    for (std::size_t i = 0; i < worker_count; ++i) {
        per_worker_metrics_[i].worker_id = i;
    }
}

void EnhancedThreadPoolMetrics::record_submission() {
    tasks_submitted_.fetch_add(1, std::memory_order_relaxed);
}

void EnhancedThreadPoolMetrics::record_enqueue(std::chrono::nanoseconds latency) {
    enqueue_latency_.record(latency);
}

void EnhancedThreadPoolMetrics::record_execution(
    std::chrono::nanoseconds latency,
    bool success) {
    execution_latency_.record(latency);

    if (success) {
        tasks_executed_.fetch_add(1, std::memory_order_relaxed);
    } else {
        tasks_failed_.fetch_add(1, std::memory_order_relaxed);
    }

    // Update throughput counters
    throughput_1s_.increment();
    throughput_1m_.increment();
}

void EnhancedThreadPoolMetrics::record_wait_time(std::chrono::nanoseconds wait) {
    wait_time_.record(wait);
}

void EnhancedThreadPoolMetrics::record_queue_depth(std::size_t depth) {
    current_queue_depth_.store(depth, std::memory_order_relaxed);

    // Update peak
    auto current_peak = peak_queue_depth_.load(std::memory_order_relaxed);
    while (depth > current_peak) {
        if (peak_queue_depth_.compare_exchange_weak(
                current_peak, depth,
                std::memory_order_relaxed,
                std::memory_order_relaxed)) {
            break;
        }
    }

    // Update average calculation
    queue_depth_sum_.fetch_add(depth, std::memory_order_relaxed);
    queue_depth_samples_.fetch_add(1, std::memory_order_relaxed);
}

void EnhancedThreadPoolMetrics::record_worker_state(
    std::size_t worker_id,
    bool busy,
    std::uint64_t duration_ns) {
    // Update global totals
    if (busy) {
        total_busy_time_ns_.fetch_add(duration_ns, std::memory_order_relaxed);
    } else {
        total_idle_time_ns_.fetch_add(duration_ns, std::memory_order_relaxed);
    }

    // Update per-worker metrics
    std::lock_guard<std::mutex> lock(workers_mutex_);
    if (worker_id < per_worker_metrics_.size()) {
        auto& worker = per_worker_metrics_[worker_id];
        worker.is_busy = busy;
        if (busy) {
            worker.busy_time_ns += duration_ns;
        } else {
            worker.idle_time_ns += duration_ns;
        }
        if (!busy && duration_ns > 0) {
            // Task completed
            worker.tasks_executed++;
        }
    }
}

void EnhancedThreadPoolMetrics::set_active_workers(std::size_t count) {
    active_workers_.store(count, std::memory_order_relaxed);
}

EnhancedSnapshot EnhancedThreadPoolMetrics::snapshot() const {
    EnhancedSnapshot snap;
    snap.snapshot_time = std::chrono::steady_clock::now();

    // Basic counters (from MetricsBase)
    snap.tasks_submitted = tasks_submitted();
    snap.tasks_executed = tasks_executed();
    snap.tasks_failed = tasks_failed();

    // Latency percentiles (convert ns to μs)
    snap.enqueue_latency_p50_us = ns_to_us(enqueue_latency_.p50());
    snap.enqueue_latency_p90_us = ns_to_us(enqueue_latency_.p90());
    snap.enqueue_latency_p99_us = ns_to_us(enqueue_latency_.p99());

    snap.execution_latency_p50_us = ns_to_us(execution_latency_.p50());
    snap.execution_latency_p90_us = ns_to_us(execution_latency_.p90());
    snap.execution_latency_p99_us = ns_to_us(execution_latency_.p99());

    snap.wait_time_p50_us = ns_to_us(wait_time_.p50());
    snap.wait_time_p90_us = ns_to_us(wait_time_.p90());
    snap.wait_time_p99_us = ns_to_us(wait_time_.p99());

    // Throughput
    snap.throughput_1s = throughput_1s_.rate_per_second();
    snap.throughput_1m = throughput_1m_.rate_per_second();

    // Queue health
    snap.current_queue_depth = current_queue_depth_.load(std::memory_order_relaxed);
    snap.peak_queue_depth = peak_queue_depth_.load(std::memory_order_relaxed);

    auto samples = queue_depth_samples_.load(std::memory_order_relaxed);
    if (samples > 0) {
        snap.avg_queue_depth =
            static_cast<double>(queue_depth_sum_.load(std::memory_order_relaxed)) /
            static_cast<double>(samples);
    }

    // Worker utilization (from MetricsBase)
    snap.total_busy_time_ns = total_busy_time_ns();
    snap.total_idle_time_ns = total_idle_time_ns();
    snap.active_workers = active_workers_.load(std::memory_order_relaxed);

    auto total_time = snap.total_busy_time_ns + snap.total_idle_time_ns;
    if (total_time > 0) {
        snap.worker_utilization =
            static_cast<double>(snap.total_busy_time_ns) /
            static_cast<double>(total_time);
    }

    // Per-worker utilization
    {
        std::lock_guard<std::mutex> lock(workers_mutex_);
        snap.per_worker_utilization.reserve(per_worker_metrics_.size());
        for (const auto& worker : per_worker_metrics_) {
            auto worker_total = worker.busy_time_ns + worker.idle_time_ns;
            if (worker_total > 0) {
                snap.per_worker_utilization.push_back(
                    static_cast<double>(worker.busy_time_ns) /
                    static_cast<double>(worker_total));
            } else {
                snap.per_worker_utilization.push_back(0.0);
            }
        }
    }

    return snap;
}

const LatencyHistogram& EnhancedThreadPoolMetrics::enqueue_latency() const {
    return enqueue_latency_;
}

const LatencyHistogram& EnhancedThreadPoolMetrics::execution_latency() const {
    return execution_latency_;
}

const LatencyHistogram& EnhancedThreadPoolMetrics::wait_time() const {
    return wait_time_;
}

std::vector<WorkerMetrics> EnhancedThreadPoolMetrics::worker_metrics() const {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    return per_worker_metrics_;
}

const SlidingWindowCounter& EnhancedThreadPoolMetrics::throughput_1s() const {
    return throughput_1s_;
}

const SlidingWindowCounter& EnhancedThreadPoolMetrics::throughput_1m() const {
    return throughput_1m_;
}

void EnhancedThreadPoolMetrics::reset() {
    // Reset base class counters first
    MetricsBase::reset();

    // Reset histograms
    enqueue_latency_.reset();
    execution_latency_.reset();
    wait_time_.reset();

    // Reset throughput counters
    throughput_1s_.reset();
    throughput_1m_.reset();

    // Reset queue depth tracking
    current_queue_depth_.store(0, std::memory_order_relaxed);
    peak_queue_depth_.store(0, std::memory_order_relaxed);
    queue_depth_sum_.store(0, std::memory_order_relaxed);
    queue_depth_samples_.store(0, std::memory_order_relaxed);

    // Reset per-worker metrics
    {
        std::lock_guard<std::mutex> lock(workers_mutex_);
        for (auto& worker : per_worker_metrics_) {
            worker.tasks_executed = 0;
            worker.busy_time_ns = 0;
            worker.idle_time_ns = 0;
            worker.is_busy = false;
        }
    }
}

void EnhancedThreadPoolMetrics::update_worker_count(std::size_t count) {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    if (count > per_worker_metrics_.size()) {
        auto old_size = per_worker_metrics_.size();
        per_worker_metrics_.resize(count);
        for (std::size_t i = old_size; i < count; ++i) {
            per_worker_metrics_[i].worker_id = i;
        }
    }
}

std::string EnhancedThreadPoolMetrics::to_json() const {
    // Delegate to JsonBackend for consistent output format
    auto backend = BackendRegistry::instance().get("json");
    if (backend) {
        return backend->export_enhanced(snapshot());
    }

    // Fallback if registry is not initialized (shouldn't happen)
    JsonBackend fallback;
    return fallback.export_enhanced(snapshot());
}

std::string EnhancedThreadPoolMetrics::to_prometheus(
    const std::string& prefix) const {
    // Create a PrometheusBackend with the specified prefix
    PrometheusBackend backend;
    backend.set_prefix(prefix);
    return backend.export_enhanced(snapshot());
}

} // namespace kcenon::thread::metrics
