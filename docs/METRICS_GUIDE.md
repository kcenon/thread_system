---
doc_id: "THR-GUID-002b"
doc_title: "Metrics Guide"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "thread_system"
category: "GUID"
---

# Metrics Guide

> **SSOT**: This document is the single source of truth for **Metrics Guide** (health monitoring, Prometheus integration, Kubernetes probes, metrics backends).

> **Language:** **English**

> **See also**: [Diagnostics Guide](DIAGNOSTICS_GUIDE.md) for thread dump, job inspection, bottleneck detection, and event tracing.

A comprehensive guide to thread_system's health monitoring, metrics framework, latency histograms, throughput counters, and pluggable export backends.

## Table of Contents

1. [Health Monitoring](#health-monitoring)
2. [Metrics Framework](#metrics-framework)
3. [Latency Histograms](#latency-histograms)
4. [Sliding Window Counters](#sliding-window-counters)
5. [Metrics Service](#metrics-service)
6. [Metrics Backends](#metrics-backends)
7. [Usage Examples](#usage-examples)
8. [Configuration Reference](#configuration-reference)
9. [Anti-Patterns](#anti-patterns)
10. [Troubleshooting](#troubleshooting)

---

## Health Monitoring

### Health States

The health system uses four states compatible with Kubernetes probes and standard health check frameworks:

| State | HTTP Code | Meaning |
|-------|-----------|---------|
| `healthy` | 200 | Component is fully operational |
| `degraded` | 200 | Operational but with reduced capacity/performance |
| `unhealthy` | 503 | Not operational or failing |
| `unknown` | 503 | Health state cannot be determined |

### Health Check API

```cpp
auto health = pool->diagnostics().health_check();

// Quick boolean check
if (pool->diagnostics().is_healthy()) {
    // All good
}

// HTTP endpoint integration
auto status_code = health.http_status_code();
auto body = health.to_json();
return http_response(status_code, body);
```

### Health Components

The health check evaluates three independent components:

| Component | What It Checks |
|-----------|---------------|
| **workers** | Worker utilization, idle worker count |
| **queue** | Queue saturation level |
| **metrics** | Success rate, average latency |

### Overall Status Aggregation

```
calculate_overall_status():
  If any component unhealthy -> overall = unhealthy
  Else if any component degraded -> overall = degraded
  Else if any component unknown -> overall = degraded
  Else all healthy -> overall = healthy
```

### Health Thresholds

Default thresholds (all configurable via `health_thresholds`):

| Threshold | Default | Description |
|-----------|---------|-------------|
| `min_success_rate` | `0.95` | Below this, pool is degraded |
| `unhealthy_success_rate` | `0.8` | Below this, pool is unhealthy |
| `max_healthy_latency_ms` | `100.0` | Max latency for healthy status |
| `degraded_latency_ms` | `500.0` | Latency above which pool is degraded |
| `queue_saturation_warning` | `0.8` | Queue saturation for degraded status |
| `queue_saturation_critical` | `0.95` | Queue saturation for unhealthy status |
| `worker_utilization_warning` | `0.9` | Worker utilization for degraded status |
| `min_idle_workers` | `0` | Minimum idle workers for healthy (0 = disabled) |

### Customizing Thresholds

```cpp
diagnostics_config config;
config.health_thresholds_config.min_success_rate = 0.99;
config.health_thresholds_config.max_healthy_latency_ms = 50.0;
config.health_thresholds_config.queue_saturation_warning = 0.7;
config.health_thresholds_config.min_idle_workers = 2;

thread_pool_diagnostics diag(*pool, config);
auto health = diag.health_check();
```

### Kubernetes Integration

```cpp
// Liveness probe
auto handle_liveness = [&]() {
    auto health = pool->diagnostics().health_check();
    return http_response(health.http_status_code(), health.to_json());
};

// Readiness probe (stricter: only 200 if fully healthy)
auto handle_readiness = [&]() {
    auto health = pool->diagnostics().health_check();
    int code = (health.overall_status == health_state::healthy) ? 200 : 503;
    return http_response(code, health.to_json());
};
```

### Prometheus Health Export

```cpp
std::string prom = health.to_prometheus("my_pool");
```

Output:

```
# HELP thread_pool_health_status Health status (1=healthy, 0.5=degraded, 0=unhealthy)
# TYPE thread_pool_health_status gauge
thread_pool_health_status{pool="my_pool"} 1

# HELP thread_pool_uptime_seconds Total uptime in seconds
# TYPE thread_pool_uptime_seconds counter
thread_pool_uptime_seconds{pool="my_pool"} 3600.50

# HELP thread_pool_success_rate Ratio of successful jobs (0.0 to 1.0)
# TYPE thread_pool_success_rate gauge
thread_pool_success_rate{pool="my_pool"} 0.9987

# HELP thread_pool_workers_active Number of active workers
# TYPE thread_pool_workers_active gauge
thread_pool_workers_active{pool="my_pool"} 5

# HELP thread_pool_queue_depth Current queue depth
# TYPE thread_pool_queue_depth gauge
thread_pool_queue_depth{pool="my_pool"} 42
```

### JSON Health Output

```json
{
  "status": "healthy",
  "message": "All components are healthy",
  "http_code": 200,
  "metrics": {
    "uptime_seconds": 3600.50,
    "total_jobs_processed": 125000,
    "success_rate": 0.9987,
    "avg_latency_ms": 2.450
  },
  "workers": {
    "total": 8,
    "active": 5,
    "idle": 3
  },
  "queue": {
    "depth": 42,
    "capacity": 1024
  },
  "components": [
    {
      "name": "workers",
      "status": "healthy",
      "message": "All workers operational"
    },
    {
      "name": "queue",
      "status": "healthy",
      "message": "Queue within normal limits"
    },
    {
      "name": "metrics",
      "status": "healthy",
      "message": "Success rate and latency within thresholds"
    }
  ]
}
```

---

## Metrics Framework

### Class Hierarchy

```
MetricsBase (abstract)
  |
  +-- ThreadPoolMetrics        (lightweight: 6 atomic counters)
  |
  +-- EnhancedThreadPoolMetrics (full-featured: histograms + counters)
```

### MetricsBase

The abstract base class providing 5 lock-free atomic counters:

| Counter | Method | Description |
|---------|--------|-------------|
| `tasks_submitted_` | `record_submission(count)` | Tasks submitted to pool |
| `tasks_executed_` | `record_execution(ns, success)` | Successfully completed tasks |
| `tasks_failed_` | `record_execution(ns, false)` | Failed tasks |
| `total_busy_time_ns_` | `record_execution(ns, *)` | Cumulative execution time |
| `total_idle_time_ns_` | `record_idle_time(ns)` | Cumulative idle time |

Computed properties:
- `utilization()`: `busy_time / (busy_time + idle_time)`, 0.0 to 1.0
- `success_rate()`: `executed / (executed + failed)`, 0.0 to 1.0
- `base_snapshot()`: Returns `BaseSnapshot` struct

**Performance**: < 50ns per record, 40 bytes memory (5 atomic counters).

### ThreadPoolMetrics

Extends `MetricsBase` with one additional counter:

```cpp
auto metrics = std::make_shared<ThreadPoolMetrics>();

// Record operations
metrics->record_submission();
metrics->record_enqueue();
metrics->record_execution(50000, true);   // 50us execution, success
metrics->record_execution(100000, false); // 100us execution, failure
metrics->record_idle_time(200000);        // 200us idle

// Query individual values
auto submitted = metrics->tasks_submitted();
auto enqueued  = metrics->tasks_enqueued();
auto executed  = metrics->tasks_executed();
auto failed    = metrics->tasks_failed();
auto util      = metrics->utilization();
auto rate      = metrics->success_rate();

// Point-in-time snapshot
auto snap = metrics->snapshot();
// snap.tasks_submitted, snap.tasks_enqueued, snap.tasks_executed,
// snap.tasks_failed, snap.total_busy_time_ns, snap.total_idle_time_ns
```

**Performance**: < 50ns per record, 48 bytes memory (6 atomic counters).

### EnhancedThreadPoolMetrics

Full-featured metrics with histograms, throughput counters, and per-worker tracking:

```cpp
auto metrics = std::make_shared<EnhancedThreadPoolMetrics>(8); // 8 workers

// Record metrics (called internally by thread_pool)
metrics->record_submission();
metrics->record_enqueue(std::chrono::nanoseconds{1000});
metrics->record_execution(std::chrono::nanoseconds{50000}, true);
metrics->record_wait_time(std::chrono::nanoseconds{5000});
metrics->record_queue_depth(42);
metrics->record_worker_state(0, true, 50000);  // worker 0 busy for 50us
metrics->set_active_workers(6);

// Get comprehensive snapshot
auto snap = metrics->snapshot();
LOG_INFO("P99 execution: {:.2f}us", snap.execution_latency_p99_us);
LOG_INFO("Throughput: {:.1f} ops/sec", snap.throughput_1s);
LOG_INFO("Queue depth: {} (peak: {})", snap.current_queue_depth, snap.peak_queue_depth);
LOG_INFO("Worker utilization: {:.1f}%", snap.worker_utilization * 100.0);

// Access individual histograms
const auto& exec_hist = metrics->execution_latency();
LOG_INFO("Exec P50={:.0f}ns P99={:.0f}ns mean={:.0f}ns",
         exec_hist.p50(), exec_hist.p99(), exec_hist.mean());

// Per-worker analysis
auto workers = metrics->worker_metrics();
for (const auto& w : workers) {
    LOG_INFO("Worker {}: {} tasks, busy={:.1f}%",
             w.worker_id, w.tasks_executed,
             static_cast<double>(w.busy_time_ns) /
             (w.busy_time_ns + w.idle_time_ns + 1) * 100.0);
}

// Export
std::string json = metrics->to_json();
std::string prom = metrics->to_prometheus("my_pool");

// Scale worker tracking
metrics->update_worker_count(12);  // Pool scaled to 12 workers
```

**Performance**: < 100ns per record, < 10us snapshot, < 1KB per histogram, < 4KB per counter (60s window).

### EnhancedSnapshot Fields

| Category | Field | Unit | Description |
|----------|-------|------|-------------|
| **Counters** | `tasks_submitted` | count | Total tasks submitted |
| | `tasks_executed` | count | Successful completions |
| | `tasks_failed` | count | Failed tasks |
| **Enqueue Latency** | `enqueue_latency_p50_us` | us | Median enqueue time |
| | `enqueue_latency_p90_us` | us | P90 enqueue time |
| | `enqueue_latency_p99_us` | us | P99 enqueue time |
| **Execution Latency** | `execution_latency_p50_us` | us | Median execution time |
| | `execution_latency_p90_us` | us | P90 execution time |
| | `execution_latency_p99_us` | us | P99 execution time |
| **Wait Time** | `wait_time_p50_us` | us | Median queue wait |
| | `wait_time_p90_us` | us | P90 queue wait |
| | `wait_time_p99_us` | us | P99 queue wait |
| **Throughput** | `throughput_1s` | ops/sec | 1-second window rate |
| | `throughput_1m` | ops/sec | 1-minute window average |
| **Queue** | `current_queue_depth` | count | Current queue size |
| | `peak_queue_depth` | count | Peak since reset |
| | `avg_queue_depth` | count | Sampled average |
| **Workers** | `worker_utilization` | ratio | Overall utilization (0.0-1.0) |
| | `per_worker_utilization` | vector | Per-worker ratios |
| | `active_workers` | count | Currently active workers |
| **Timing** | `total_busy_time_ns` | ns | Cumulative busy time |
| | `total_idle_time_ns` | ns | Cumulative idle time |
| | `snapshot_time` | time_point | When snapshot was taken |

---

## Latency Histograms

### Design

`LatencyHistogram` is an HDR-style histogram using 64 logarithmic buckets to efficiently capture latency distributions across a wide range (nanoseconds to seconds):

```
Bucket  Range (nanoseconds)        Resolution
  0     [0, 1)                     1 ns
  1     [1, 2)                     1 ns
  2     [2, 4)                     2 ns
  3     [4, 8)                     4 ns
  ...
  10    [512, 1024)                512 ns (~1us)
  20    [524288, 1048576)          ~500us (~1ms)
  30    [~500M, ~1B)              ~500ms (~1s)
  ...
  63    [2^62, 2^63)              ~10^18 ns
```

### Properties

| Property | Value |
|----------|-------|
| Bucket count | 64 (fixed) |
| Memory footprint | < 1KB |
| Record overhead | < 100ns (lock-free) |
| Range | 0 to ~10^19 nanoseconds |
| Accuracy | Within 1% for percentiles |
| Thread safety | Fully lock-free |

### API

```cpp
LatencyHistogram histogram;

// Record values
histogram.record(std::chrono::nanoseconds{1500});
histogram.record_ns(2500);

// Percentiles (returned in nanoseconds)
double median = histogram.p50();
double p90    = histogram.p90();
double p95    = histogram.p95();
double p99    = histogram.p99();
double p999   = histogram.p999();
double custom = histogram.percentile(0.75);  // P75

// Statistics
double avg     = histogram.mean();
double sd      = histogram.stddev();
auto   min_val = histogram.min();
auto   max_val = histogram.max();
auto   total   = histogram.count();
auto   sum_val = histogram.sum();

// State management
bool is_empty = histogram.empty();
histogram.reset();

// Merge another histogram
LatencyHistogram other;
// ... record into other ...
histogram.merge(other);

// Bucket inspection
for (std::size_t i = 0; i < LatencyHistogram::BUCKET_COUNT; ++i) {
    auto count = histogram.bucket_count(i);
    if (count > 0) {
        LOG_DEBUG("Bucket {} [{}, {}): {} samples",
                  i,
                  LatencyHistogram::bucket_lower_bound(i),
                  LatencyHistogram::bucket_upper_bound(i),
                  count);
    }
}
```

---

## Sliding Window Counters

### Design

`SlidingWindowCounter` tracks event rates using a lock-free circular buffer of time buckets. As time advances, old buckets are automatically invalidated and reused.

```
Window: 1 second, 10 buckets per second

Time:    [0-100ms] [100-200ms] [200-300ms] ... [900-1000ms]
Buckets:  [  5  ]   [  3  ]    [  7  ]    ...  [  4  ]
                                                    ^
                                              current bucket
```

### Properties

| Property | Value |
|----------|-------|
| Default buckets per second | 10 (`DEFAULT_BUCKETS_PER_SECOND`) |
| Increment overhead | O(1), lock-free |
| Rate calculation | O(bucket_count) |
| Memory (1s window) | 10 buckets x 16 bytes = 160 bytes |
| Memory (60s window) | 600 buckets x 16 bytes = ~10KB |
| Thread safety | Fully lock-free |

### API

```cpp
using namespace std::chrono_literals;

// 1-second window with default 10 buckets/second
SlidingWindowCounter counter_1s(1s);

// 1-minute window with 10 buckets/second
SlidingWindowCounter counter_1m(60s);

// Custom precision: 100 buckets/second for 5-second window
SlidingWindowCounter precise_counter(5s, 100);

// Increment
counter_1s.increment();       // Add 1
counter_1s.increment(10);     // Add 10

// Query rates
double rate = counter_1s.rate_per_second();
auto in_window = counter_1s.total_in_window();
auto all_time = counter_1s.all_time_total();

// Metadata
auto window = counter_1s.window_size();    // 1s
auto buckets = counter_1s.bucket_count();  // 10

// Reset
counter_1s.reset();
```

---

## Metrics Service

### Overview

`metrics_service` is the centralized entry point for metrics management, owned by `thread_pool` and shared with `thread_worker` instances.

### Ownership Model

```
thread_pool
    |
    +-- owns --> metrics_service (shared_ptr)
                    |
                    +-- basic_metrics_ (always initialized)
                    +-- enhanced_metrics_ (lazily initialized)

thread_worker
    |
    +-- non-owning pointer --> metrics_service
```

### API

```cpp
auto svc = std::make_shared<metrics_service>();

// === Recording (called by thread_pool and thread_worker) ===
svc->record_submission();
svc->record_enqueue();
svc->record_enqueue_with_latency(std::chrono::nanoseconds{500});
svc->record_execution(50000, true);  // 50us, success
svc->record_execution_with_wait_time(
    std::chrono::nanoseconds{50000},  // execution
    std::chrono::nanoseconds{5000},   // wait
    true);                            // success
svc->record_idle_time(200000);
svc->record_queue_depth(42);
svc->record_worker_state(0, true, 50000);

// === Enhanced Metrics Control ===
svc->set_enhanced_metrics_enabled(true, 8);  // Enable with 8 workers
bool enabled = svc->is_enhanced_metrics_enabled();
svc->update_worker_count(12);  // Pool scaled
svc->set_active_workers(10);

// === Query ===
const auto& basic = svc->basic_metrics();     // Always available
auto basic_ptr = svc->get_basic_metrics();     // shared_ptr for workers
const auto& enhanced = svc->enhanced_metrics(); // Throws if not enabled
auto snap = svc->enhanced_snapshot();           // Empty snapshot if disabled

// === Management ===
svc->reset();  // Reset all metrics
```

### Thread Safety

| Property | Mechanism |
|----------|-----------|
| Atomic counters | `std::memory_order_relaxed` for writes |
| Enhanced toggle | `std::atomic<bool>` |
| Lazy initialization | `std::mutex` for one-time init |
| Ownership | Non-copyable, non-movable |

---

## Metrics Backends

### MetricsBackend Interface

The `MetricsBackend` abstract class defines the contract for exporting metrics to monitoring systems:

```cpp
class MetricsBackend {
public:
    virtual ~MetricsBackend() = default;

    // Required overrides
    virtual std::string name() const = 0;
    virtual std::string export_base(const BaseSnapshot& snapshot) const = 0;
    virtual std::string export_enhanced(const EnhancedSnapshot& snapshot) const = 0;

    // Optional overrides (with defaults)
    virtual void set_prefix(const std::string& prefix);  // default: "thread_pool"
    virtual void add_label(const std::string& key, const std::string& value);
};
```

### Built-in Backends

#### PrometheusBackend

Exports metrics in Prometheus/OpenMetrics exposition format:

```cpp
auto backend = std::make_shared<PrometheusBackend>();
backend->set_prefix("myapp_pool");
backend->add_label("instance", "worker-01");

auto snap = metrics->base_snapshot();
std::string output = backend->export_base(snap);
```

Output:

```
# HELP myapp_pool_tasks_submitted_total Total tasks submitted
# TYPE myapp_pool_tasks_submitted_total counter
myapp_pool_tasks_submitted_total{instance="worker-01"} 1234
```

#### JsonBackend

Exports metrics as structured JSON:

```cpp
auto backend = std::make_shared<JsonBackend>();
backend->set_pretty(true);   // Indented output (default)
backend->set_pretty(false);  // Compact output

auto snap = metrics->base_snapshot();
std::string output = backend->export_base(snap);
```

Output (pretty):

```json
{
  "tasks": {
    "submitted": 1234,
    "executed": 1200,
    "failed": 5
  }
}
```

#### LoggingBackend

Exports metrics in human-readable format for log files:

```cpp
auto backend = std::make_shared<LoggingBackend>();
std::string output = backend->export_base(snap);
```

### BackendRegistry

The singleton `BackendRegistry` manages available backends:

```cpp
auto& registry = BackendRegistry::instance();

// All 3 defaults are auto-registered
bool has_prom = registry.has("prometheus");  // true
bool has_json = registry.has("json");        // true
bool has_log  = registry.has("logging");     // true

// Get a backend by name
auto prom = registry.get("prometheus");
if (prom) {
    std::string output = prom->export_base(snap);
}

// Register a custom backend
registry.register_backend(std::make_shared<MyCustomBackend>());
```

### Implementing a Custom Backend

```cpp
#include <kcenon/thread/metrics/metrics_backend.h>
using namespace kcenon::thread::metrics;

class StatsDBBackend : public MetricsBackend {
public:
    std::string name() const override {
        return "statsdb";
    }

    std::string export_base(const BaseSnapshot& snap) const override {
        std::ostringstream oss;
        oss << prefix() << ".submitted " << snap.tasks_submitted << "\n";
        oss << prefix() << ".executed "  << snap.tasks_executed  << "\n";
        oss << prefix() << ".failed "    << snap.tasks_failed    << "\n";

        // Include configured labels
        for (const auto& [key, value] : labels()) {
            oss << "# label " << key << "=" << value << "\n";
        }
        return oss.str();
    }

    std::string export_enhanced(const EnhancedSnapshot& snap) const override {
        std::ostringstream oss;
        oss << prefix() << ".exec_p99_us "   << snap.execution_latency_p99_us << "\n";
        oss << prefix() << ".throughput_1s " << snap.throughput_1s             << "\n";
        oss << prefix() << ".queue_depth "   << snap.current_queue_depth       << "\n";
        return oss.str();
    }
};

// Register it
BackendRegistry::instance().register_backend(
    std::make_shared<StatsDBBackend>());
```

---

## Usage Examples

### Complete Diagnostics + Metrics Setup

```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/diagnostics/thread_pool_diagnostics.h>
#include <kcenon/thread/metrics/metrics_backend.h>

using namespace kcenon::thread;
using namespace kcenon::thread::diagnostics;
using namespace kcenon::thread::metrics;

// === 1. Create and configure the pool ===
auto pool = std::make_shared<thread_pool>("WorkerPool");
pool->start(8);  // 8 workers

// === 2. Enable enhanced metrics ===
pool->set_enhanced_metrics_enabled(true);

// === 3. Configure diagnostics ===
diagnostics_config diag_config;
diag_config.enable_tracing = true;
diag_config.recent_jobs_capacity = 5000;
diag_config.queue_saturation_high = 0.75;
diag_config.health_thresholds_config.min_success_rate = 0.99;

// === 4. Add event listener ===
class AlertListener : public execution_event_listener {
public:
    void on_event(const job_execution_event& event) override {
        if (event.type == event_type::failed) {
            // Alert on failures
            LOG_ERROR("Job {} failed: {}",
                      event.job_name,
                      event.error_message.value_or("unknown error"));
        }
    }
};
pool->diagnostics().add_event_listener(std::make_shared<AlertListener>());

// === 5. Submit work ===
for (int i = 0; i < 1000; ++i) {
    pool->enqueue(make_job()
        .name("Task-" + std::to_string(i))
        .work([]() -> common::VoidResult {
            // ... do work ...
            return common::ok();
        })
        .build());
}

// === 6. Monitor health ===
auto health = pool->diagnostics().health_check();
LOG_INFO("Pool health: {} (HTTP {})",
         health_state_to_string(health.overall_status),
         health.http_status_code());

// === 7. Check for bottlenecks ===
auto report = pool->diagnostics().detect_bottlenecks();
if (report.has_bottleneck) {
    LOG_WARN("Bottleneck detected: {} (severity: {})",
             bottleneck_type_to_string(report.type),
             report.severity_string());
}

// === 8. Export metrics ===
auto& registry = BackendRegistry::instance();

// Prometheus format
auto prom = registry.get("prometheus");
prom->set_prefix("myapp_worker_pool");
prom->add_label("service", "order-processor");
// ... use prom->export_enhanced(snap) in your /metrics endpoint

// JSON format for dashboards
auto json_backend = registry.get("json");
// ... use json_backend->export_enhanced(snap) in your REST API

// === 9. Thread dump for debugging ===
std::cout << pool->diagnostics().format_thread_dump() << std::endl;
```

### Periodic Monitoring Loop

```cpp
void monitoring_loop(std::shared_ptr<thread_pool> pool) {
    while (pool->is_running()) {
        // Health check
        auto health = pool->diagnostics().health_check();

        if (!health.is_healthy()) {
            LOG_WARN("Pool degraded: {}", health.status_message);

            // Detailed bottleneck analysis
            auto report = pool->diagnostics().detect_bottlenecks();
            if (report.severity() >= 2) {
                for (const auto& rec : report.recommendations) {
                    LOG_WARN("  Action: {}", rec);
                }
            }
        }

        // Metrics snapshot
        auto snap = pool->metrics_service()->enhanced_snapshot();
        LOG_INFO("Throughput: {:.0f}/s  P99: {:.1f}us  Queue: {}",
                 snap.throughput_1s,
                 snap.execution_latency_p99_us,
                 snap.current_queue_depth);

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}
```

### HTTP Health Endpoint

```cpp
// Kubernetes liveness probe
auto handle_healthz = [&pool]() -> HttpResponse {
    auto health = pool->diagnostics().health_check();
    return HttpResponse{
        health.http_status_code(),
        "application/json",
        health.to_json()
    };
};

// Prometheus metrics scrape endpoint
auto handle_metrics = [&pool]() -> HttpResponse {
    auto snap = pool->metrics_service()->enhanced_snapshot();
    auto prom = BackendRegistry::instance().get("prometheus");
    return HttpResponse{
        200,
        "text/plain; version=0.0.4; charset=utf-8",
        prom->export_enhanced(snap)
    };
};
```

---

## Configuration Reference

### diagnostics_config Defaults

| Field | Default | Description |
|-------|---------|-------------|
| `recent_jobs_capacity` | `1000` | Max recent jobs in ring buffer |
| `event_history_size` | `1000` | Max events in history buffer |
| `enable_tracing` | `false` | Auto-enable event tracing |
| `queue_saturation_high` | `0.8` | High watermark for queue saturation |
| `utilization_high_threshold` | `0.9` | Worker utilization warning threshold |
| `wait_time_threshold_ms` | `100.0` | Wait time for slow consumer detection |
| `health_thresholds_config` | `{}` | Nested health threshold defaults |

### health_thresholds Defaults

| Field | Default | Description |
|-------|---------|-------------|
| `min_success_rate` | `0.95` | Below = degraded |
| `unhealthy_success_rate` | `0.8` | Below = unhealthy |
| `max_healthy_latency_ms` | `100.0` | Above = not fully healthy |
| `degraded_latency_ms` | `500.0` | Above = degraded |
| `queue_saturation_warning` | `0.8` | Queue warning threshold |
| `queue_saturation_critical` | `0.95` | Queue critical threshold |
| `worker_utilization_warning` | `0.9` | Worker warning threshold |
| `min_idle_workers` | `0` | Min idle workers (0 = disabled) |

### Memory Budget

| Component | Memory Usage |
|-----------|-------------|
| `MetricsBase` | 40 bytes (5 atomic counters) |
| `ThreadPoolMetrics` | 48 bytes (6 atomic counters) |
| `LatencyHistogram` | < 1KB (64 buckets + counters) |
| `SlidingWindowCounter` (1s) | ~160 bytes (10 buckets) |
| `SlidingWindowCounter` (60s) | ~10KB (600 buckets) |
| `EnhancedThreadPoolMetrics` | ~25KB base + ~32 bytes per worker |
| `diagnostics_config` | ~100 bytes |

---

## Anti-Patterns

### 1. Polling Diagnostics in a Tight Loop

```cpp
// BAD: Wastes CPU on constant diagnostics polling
while (true) {
    auto health = diag.health_check();  // O(n) per call
    // No sleep!
}

// GOOD: Poll at reasonable intervals
while (true) {
    auto health = diag.health_check();
    std::this_thread::sleep_for(std::chrono::seconds(5));
}
```

### 2. Slow Event Listeners

```cpp
// BAD: Blocking I/O in event listener (blocks worker thread)
class SlowListener : public execution_event_listener {
    void on_event(const job_execution_event& event) override {
        write_to_database(event);  // 10ms+ blocking call
    }
};

// GOOD: Buffer events and process asynchronously
class AsyncListener : public execution_event_listener {
    void on_event(const job_execution_event& event) override {
        event_queue_.push(event.to_json());  // < 1us
    }
private:
    concurrent_queue<std::string> event_queue_;
};
```

### 3. Ignoring Enhanced Metrics Lifecycle

```cpp
// BAD: Accessing enhanced metrics without checking
try {
    const auto& em = svc->enhanced_metrics();  // Throws if not enabled!
} catch (...) { }

// GOOD: Check first or use snapshot
if (svc->is_enhanced_metrics_enabled()) {
    const auto& em = svc->enhanced_metrics();
}
// Or: always-safe snapshot (returns empty if disabled)
auto snap = svc->enhanced_snapshot();
```

### 4. Creating Multiple Diagnostics Instances

```cpp
// BAD: Multiple diagnostics for the same pool (wasted memory)
thread_pool_diagnostics diag1(*pool);
thread_pool_diagnostics diag2(*pool);

// GOOD: Use the pool's built-in diagnostics
auto& diag = pool->diagnostics();
```

### 5. Not Resetting Metrics After Scale Events

```cpp
// BAD: Metrics from old worker count skew per-worker stats
pool->resize(16);
// ... per_worker_utilization still has 8 entries ...

// GOOD: Update worker count in metrics
pool->resize(16);
svc->update_worker_count(16);
```

---

## Troubleshooting

### Health Check Returns `unknown`

**Cause**: No components registered, or pool not started.

**Solution**:
```cpp
// Ensure pool is started before health checks
pool->start();
// Then check health
auto health = pool->diagnostics().health_check();
```

### Bottleneck Detection Shows `none` Despite Slow Tasks

**Cause**: Thresholds may be too lenient for your workload.

**Solution**:
```cpp
diagnostics_config config;
config.queue_saturation_high = 0.5;      // More sensitive
config.utilization_high_threshold = 0.7;  // More sensitive
config.wait_time_threshold_ms = 10.0;     // Stricter wait time
```

### Enhanced Metrics Snapshot Returns All Zeros

**Cause**: Enhanced metrics not enabled, or no data recorded yet.

**Solution**:
```cpp
// 1. Enable enhanced metrics BEFORE submitting work
svc->set_enhanced_metrics_enabled(true, worker_count);

// 2. Submit work and wait for processing

// 3. Then take snapshot
auto snap = svc->enhanced_snapshot();
```

### Prometheus Metrics Missing Labels

**Cause**: Labels must be set on the backend before exporting.

**Solution**:
```cpp
auto prom = BackendRegistry::instance().get("prometheus");
prom->set_prefix("myapp_pool");
prom->add_label("service", "api");
prom->add_label("env", "production");
// NOW export
auto output = prom->export_enhanced(snap);
```

### Event Listener Not Receiving Events

**Cause**: Tracing may not be enabled, or listener was not registered.

**Solution**:
```cpp
// 1. Enable tracing
pool->diagnostics().enable_tracing(true);

// 2. Register listener BEFORE submitting work
pool->diagnostics().add_event_listener(my_listener);

// 3. Verify tracing is enabled
assert(pool->diagnostics().is_tracing_enabled());
```

### Per-Worker Utilization Shows 0% for All Workers

**Cause**: `record_worker_state()` is not being called with duration data.

**Solution**: Ensure the thread pool implementation calls `record_worker_state(worker_id, busy, duration_ns)` on each state transition with the duration spent in the previous state.

---

## Related Documentation

- [Diagnostics Guide](DIAGNOSTICS_GUIDE.md) - Thread dump, job inspection, bottleneck detection, event tracing
- [Architecture Overview](ARCHITECTURE_OVERVIEW.md) - System-wide architecture
- [Policy Queue Combinations Guide](POLICY_QUEUE_GUIDE.md) - Queue and policy configurations
- [NUMA Work-Stealing Guide](NUMA_GUIDE.md) - NUMA topology and work stealing
- [Autoscaler Guide](AUTOSCALER_GUIDE.md) - Dynamic worker pool sizing
