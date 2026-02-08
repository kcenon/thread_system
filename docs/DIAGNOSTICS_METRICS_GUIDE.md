# Diagnostics and Metrics Guide

> **Language:** **English**

A comprehensive guide to thread_system's diagnostics and metrics subsystems, covering health checks, bottleneck detection, event tracing, latency histograms, throughput counters, and pluggable export backends.

## Table of Contents

1. [Overview](#overview)
2. [Diagnostics Subsystem](#diagnostics-subsystem)
3. [Health Monitoring](#health-monitoring)
4. [Bottleneck Detection](#bottleneck-detection)
5. [Event Tracing](#event-tracing)
6. [Metrics Framework](#metrics-framework)
7. [Latency Histograms](#latency-histograms)
8. [Sliding Window Counters](#sliding-window-counters)
9. [Metrics Service](#metrics-service)
10. [Metrics Backends](#metrics-backends)
11. [Usage Examples](#usage-examples)
12. [Configuration Reference](#configuration-reference)
13. [Anti-Patterns](#anti-patterns)
14. [Troubleshooting](#troubleshooting)

---

## Overview

The diagnostics and metrics subsystems provide comprehensive observability for thread_system's thread pools. They operate as two complementary layers:

```
+-----------------------------------------------------------+
|                   Application Code                         |
+-----------------------------------------------------------+
|        thread_pool_diagnostics (qualitative)               |
|  [health check] [bottleneck detect] [event trace] [dump]   |
+-----------------------------------------------------------+
|           metrics_service (quantitative)                   |
|  [basic metrics] [enhanced metrics] [export backends]      |
+----------------------------+------------------------------+
|  ThreadPoolMetrics         |  EnhancedThreadPoolMetrics    |
|  (6 atomic counters)       |  (histograms + counters)      |
+----------------------------+------------------------------+
|       MetricsBase (5 atomic counters)                      |
+-----------------------------------------------------------+
```

### Key Features

| Feature | Description |
|---------|-------------|
| **Health Checks** | Component-level assessment with HTTP status codes and Kubernetes integration |
| **Bottleneck Detection** | Automatic identification of 7 bottleneck types with severity and recommendations |
| **Event Tracing** | Observer-pattern job lifecycle tracing with < 1us overhead |
| **Basic Metrics** | Lock-free atomic counters with < 50ns overhead per record |
| **Enhanced Metrics** | Latency histograms, throughput counters, per-worker tracking |
| **Pluggable Backends** | Prometheus, JSON, and Logging export with custom backend support |
| **Thread Dump** | Point-in-time worker state snapshots |

### Source Files

**Diagnostics** (`kcenon::thread::diagnostics` namespace):

| Header | Description |
|--------|-------------|
| [`thread_pool_diagnostics.h`](../include/kcenon/thread/diagnostics/thread_pool_diagnostics.h) | Central diagnostics API |
| [`health_status.h`](../include/kcenon/thread/diagnostics/health_status.h) | Health state, thresholds, component health |
| [`bottleneck_report.h`](../include/kcenon/thread/diagnostics/bottleneck_report.h) | Bottleneck types, severity, recommendations |
| [`execution_event.h`](../include/kcenon/thread/diagnostics/execution_event.h) | Event types, listener interface |
| [`job_info.h`](../include/kcenon/thread/diagnostics/job_info.h) | Job lifecycle and timing |
| [`thread_info.h`](../include/kcenon/thread/diagnostics/thread_info.h) | Worker state and statistics |

**Metrics** (`kcenon::thread::metrics` namespace):

| Header | Description |
|--------|-------------|
| [`metrics_service.h`](../include/kcenon/thread/metrics/metrics_service.h) | Centralized metrics management |
| [`metrics_base.h`](../include/kcenon/thread/metrics/metrics_base.h) | Abstract base with 5 atomic counters |
| [`thread_pool_metrics.h`](../include/kcenon/thread/metrics/thread_pool_metrics.h) | Lightweight metrics (6 counters) |
| [`enhanced_metrics.h`](../include/kcenon/thread/metrics/enhanced_metrics.h) | Histograms, percentiles, per-worker tracking |
| [`latency_histogram.h`](../include/kcenon/thread/metrics/latency_histogram.h) | HDR-style 64-bucket histogram |
| [`sliding_window_counter.h`](../include/kcenon/thread/metrics/sliding_window_counter.h) | Lock-free throughput counter |
| [`metrics_backend.h`](../include/kcenon/thread/metrics/metrics_backend.h) | Backend interface and built-in implementations |

### When to Use

| Scenario | Recommendation |
|----------|---------------|
| Quick health check in HTTP endpoint | `diagnostics().health_check()` |
| Production monitoring with Prometheus | Enable enhanced metrics + `PrometheusBackend` |
| Debugging slow task execution | `diagnostics().detect_bottlenecks()` + event tracing |
| Lightweight embedded deployment | Basic `ThreadPoolMetrics` only (48 bytes) |
| Per-worker performance analysis | `EnhancedThreadPoolMetrics` with `worker_metrics()` |
| Custom monitoring integration | Implement `MetricsBackend` interface |

---

## Diagnostics Subsystem

### Architecture

The `thread_pool_diagnostics` class provides a non-intrusive, read-only view into thread pool state:

```
thread_pool_diagnostics
    |
    +-- Thread Dump       (dump_thread_states, format_thread_dump)
    +-- Job Inspection    (get_active_jobs, get_pending_jobs, get_recent_jobs)
    +-- Bottleneck Detect (detect_bottlenecks -> bottleneck_report)
    +-- Health Check      (health_check -> health_status)
    +-- Event Tracing     (enable_tracing, add_event_listener)
    +-- Export            (to_json, to_string, to_prometheus)
```

### Design Principles

| Principle | Description |
|-----------|-------------|
| **Non-intrusive** | Minimal overhead when not actively queried |
| **Thread-safe** | All methods callable from any thread |
| **Read-only** | Never modifies thread pool state |
| **Snapshot-based** | Returns point-in-time copies, not live references |

### Construction

```cpp
#include <kcenon/thread/diagnostics/thread_pool_diagnostics.h>
using namespace kcenon::thread;
using namespace kcenon::thread::diagnostics;

auto pool = std::make_shared<thread_pool>("MyPool");
pool->start();

// Access diagnostics via the pool's built-in diagnostics
auto& diag = pool->diagnostics();

// Or create standalone diagnostics with custom config
diagnostics_config config;
config.recent_jobs_capacity = 5000;
config.enable_tracing = true;
thread_pool_diagnostics custom_diag(*pool, config);
```

### Thread Dump

Thread dump provides a snapshot of all worker states:

```cpp
// Structured data
auto threads = diag.dump_thread_states();
for (const auto& t : threads) {
    LOG_INFO("Worker {} ({}): {} jobs done, {:.1f}% utilization",
             t.thread_name, worker_state_to_string(t.state),
             t.jobs_completed, t.utilization * 100.0);
}

// Human-readable format
std::cout << diag.format_thread_dump() << std::endl;
```

Output format:

```
=== Thread Pool Dump: MyPool ===
Time: 2025-01-08T10:30:00Z
Workers: 8, Active: 5, Idle: 3

Worker-0 [tid:12345] ACTIVE (2.5s)
  Current Job: ProcessOrder#1234 (running 150ms)
  Jobs: 1523 completed, 2 failed
  Utilization: 87.3%
```

#### Worker States

| State | Meaning |
|-------|---------|
| `idle` | Worker is waiting for jobs |
| `active` | Worker is executing a job |
| `stopping` | Worker is in the process of stopping |
| `stopped` | Worker has stopped |

#### thread_info Fields

| Field | Type | Description |
|-------|------|-------------|
| `thread_id` | `std::thread::id` | System thread ID |
| `thread_name` | `std::string` | Human-readable name (e.g., "Worker-0") |
| `worker_id` | `std::size_t` | Pool-assigned worker ID |
| `state` | `worker_state` | Current operational state |
| `state_since` | `steady_clock::time_point` | When the current state began |
| `current_job` | `std::optional<job_info>` | Current job (if `active`) |
| `jobs_completed` | `std::uint64_t` | Total successful completions |
| `jobs_failed` | `std::uint64_t` | Total failures |
| `total_busy_time` | `std::chrono::nanoseconds` | Cumulative busy time |
| `total_idle_time` | `std::chrono::nanoseconds` | Cumulative idle time |
| `utilization` | `double` | `busy / (busy + idle)`, 0.0 to 1.0 |

### Job Inspection

```cpp
// Currently executing jobs
auto active = diag.get_active_jobs();

// Pending jobs in queue (default limit: 100)
auto pending = diag.get_pending_jobs(50);

// Recent completed/failed jobs (ring buffer, default limit: 100)
auto recent = diag.get_recent_jobs(200);

for (const auto& job : active) {
    if (job.status == job_status::running) {
        LOG_INFO("Job {} running for {:.1f}ms",
                 job.job_name, job.execution_time_ms());
    }
}
```

#### Job Lifecycle

```
enqueue_time                start_time                    end_time
    |                          |                             |
    v                          v                             v
    [=======wait_time=========][====execution_time==========]
    |<----- pending ---------->|<-------- running --------->|
```

#### job_status Values

| Status | Description |
|--------|-------------|
| `pending` | Job is waiting in the queue |
| `running` | Job is currently being executed |
| `completed` | Job completed successfully |
| `failed` | Job failed with an error |
| `cancelled` | Job was cancelled before completion |
| `timed_out` | Job exceeded its timeout limit |

### Export Formats

```cpp
// JSON export
std::string json = diag.to_json();

// Human-readable export
std::string text = diag.to_string();

// Prometheus exposition format
std::string prom = diag.to_prometheus();
```

### Performance Characteristics

| Operation | Complexity | Latency |
|-----------|-----------|---------|
| Thread dump | O(n) workers | < 1ms for 64 workers |
| Job inspection (active) | O(1) | < 100ns |
| Job inspection (history) | O(n) | < 1ms for 1000 entries |
| Bottleneck detection | O(n) workers | < 1ms for 64 workers |
| Health check | O(n) all components | < 1ms |
| Event tracing | O(1) per event | < 1us |

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

## Bottleneck Detection

### Overview

The `detect_bottlenecks()` method analyzes thread pool metrics to identify performance bottlenecks and provide actionable recommendations.

```cpp
auto report = pool->diagnostics().detect_bottlenecks();
if (report.has_bottleneck) {
    LOG_WARN("Bottleneck: {} - {}",
             bottleneck_type_to_string(report.type),
             report.description);
    for (const auto& rec : report.recommendations) {
        LOG_INFO("  Recommendation: {}", rec);
    }
}
```

### Bottleneck Types

| Type | Description | Typical Cause |
|------|-------------|---------------|
| `none` | No bottleneck detected | System operating normally |
| `queue_full` | Queue is at capacity | Production rate exceeds consumption |
| `slow_consumer` | Workers can't keep up | Tasks take too long to execute |
| `worker_starvation` | Not enough workers | Pool too small for workload |
| `lock_contention` | High mutex wait times | Shared resource contention |
| `uneven_distribution` | Work not evenly spread | Work stealing needed |
| `memory_pressure` | Excessive allocations | Memory-intensive tasks |

### Detection Logic

The detection algorithm evaluates metrics in priority order:

```
Step 1: Check queue_saturation
  queue_saturation > 0.9  -->  queue_full

Step 2: Check consumer speed
  avg_wait_time > threshold AND worker_utilization > 0.9  -->  slow_consumer

Step 3: Check worker capacity
  worker_utilization > 0.95 AND queue_saturation > 0.5  -->  worker_starvation

Step 4: Check distribution
  utilization variance high  -->  uneven_distribution
```

### Severity Levels

| Level | Name | Condition | Action |
|-------|------|-----------|--------|
| 0 | `none` | No bottleneck | No action needed |
| 1 | `low` | Bottleneck detected, metrics below warning thresholds | Monitor |
| 2 | `medium` | `queue_saturation > 0.8` OR `worker_utilization > 0.9` | Investigate |
| 3 | `critical` | `queue_saturation > 0.95` OR `worker_utilization > 0.98` | Immediate action |

### Interpreting the Report

The `bottleneck_report` contains supporting metrics for diagnosis:

| Field | Type | Description |
|-------|------|-------------|
| `has_bottleneck` | `bool` | Whether any bottleneck was detected |
| `type` | `bottleneck_type` | Classification of the bottleneck |
| `description` | `std::string` | Human-readable explanation |
| `queue_saturation` | `double` | Queue depth / capacity (0.0 to 1.0+) |
| `avg_wait_time_ms` | `double` | Average queue wait time |
| `worker_utilization` | `double` | Average worker busy ratio (0.0 to 1.0) |
| `utilization_variance` | `double` | Variance in worker utilization |
| `estimated_backlog_time_ms` | `std::size_t` | Estimated time to clear queue |
| `queue_depth` | `std::size_t` | Current items in queue |
| `idle_workers` | `std::size_t` | Number of idle workers |
| `total_workers` | `std::size_t` | Total worker count |
| `jobs_rejected` | `std::uint64_t` | Jobs rejected due to full queue |
| `recommendations` | `vector<string>` | Actionable suggestions |

### Response Guide

| Bottleneck | Recommended Actions |
|------------|-------------------|
| `queue_full` | Increase queue capacity, add workers, enable autoscaling |
| `slow_consumer` | Optimize task execution time, break large tasks, add workers |
| `worker_starvation` | Increase `min_workers`, enable autoscaling, reduce queue capacity |
| `lock_contention` | Reduce shared state, use lock-free algorithms, partition work |
| `uneven_distribution` | Enable work stealing, check task affinity, rebalance queue |
| `memory_pressure` | Pool objects, reduce allocations, use arena allocators |

### Bottleneck Report Export

```cpp
auto report = diag.detect_bottlenecks();

// JSON format
std::string json = report.to_json();

// Human-readable format
std::string text = report.to_string();

// Severity check
if (report.requires_immediate_action()) {
    alert_on_call_team(report.to_json());
}
```

Human-readable output:

```
=== Bottleneck Report ===
Status: DETECTED (medium severity)
Type: slow_consumer
Description: Workers cannot keep up with job submission rate

Metrics:
  Queue: 100 items (75.0% saturated)
  Workers: 7/8 active (1 idle)
  Utilization: 92.0% (variance: 0.0500)
  Wait time: 150.500ms avg
  Backlog: ~5000ms to clear

Recommendations:
  - Add more worker threads
  - Optimize job execution time
```

---

## Event Tracing

### Overview

Event tracing provides lifecycle tracking for individual jobs with the observer pattern. When enabled, every job state transition generates a `job_execution_event`.

### Event Lifecycle

```
enqueued --> dequeued --> started --> completed/failed/cancelled
                                       |
                                    retried --> started --> ...
```

### Event Types

| Type | Description | Timing Fields |
|------|-------------|---------------|
| `enqueued` | Job added to queue | `timestamp` only |
| `dequeued` | Job taken from queue | `wait_time` populated |
| `started` | Execution started | `wait_time` populated |
| `completed` | Successful completion | `wait_time`, `execution_time` |
| `failed` | Failed with error | `wait_time`, `execution_time`, `error_*` |
| `cancelled` | Cancelled before completion | `wait_time`, optionally `execution_time` |
| `retried` | Being retried after failure | `wait_time` |

### Enabling Tracing

```cpp
// Enable with default history size (1000 events)
pool->diagnostics().enable_tracing(true);

// Enable with custom history size
pool->diagnostics().enable_tracing(true, 5000);

// Check if tracing is enabled
if (pool->diagnostics().is_tracing_enabled()) {
    // Tracing is active
}

// Disable tracing
pool->diagnostics().enable_tracing(false);
```

### Custom Event Listener

Implement the `execution_event_listener` interface to receive real-time events:

```cpp
#include <kcenon/thread/diagnostics/execution_event.h>
using namespace kcenon::thread::diagnostics;

class JsonEventLogger : public execution_event_listener {
public:
    void on_event(const job_execution_event& event) override {
        // IMPORTANT: This is called from worker threads.
        // Keep processing fast (< 1us) to avoid impacting performance.

        if (event.is_terminal()) {
            LOG_INFO("[{}] job:{} {} wait:{:.3f}ms exec:{:.3f}ms",
                     event.format_timestamp(),
                     event.job_name,
                     event_type_to_string(event.type),
                     event.wait_time_ms(),
                     event.execution_time_ms());
        }

        if (event.is_error() && event.error_message.has_value()) {
            LOG_ERROR("Job {} failed: {}",
                      event.job_name,
                      event.error_message.value());
        }
    }
};

// Register the listener
auto logger = std::make_shared<JsonEventLogger>();
pool->diagnostics().add_event_listener(logger);

// Later: remove the listener
pool->diagnostics().remove_event_listener(logger);
```

### Querying Event History

```cpp
// Get recent events (default limit: 100)
auto events = pool->diagnostics().get_recent_events(50);
for (const auto& event : events) {
    std::cout << event.to_string() << std::endl;
}
```

Event string output:

```
[2025-01-08T10:30:00.123Z] Event#123 job:ProcessOrder#456 type:completed
  worker:0 thread:12345 wait:1.500ms exec:10.200ms
```

### job_execution_event Fields

| Field | Type | Description |
|-------|------|-------------|
| `event_id` | `std::uint64_t` | Monotonically increasing ID |
| `job_id` | `std::uint64_t` | Job identifier |
| `job_name` | `std::string` | Human-readable job name |
| `type` | `event_type` | Event classification |
| `timestamp` | `steady_clock::time_point` | Monotonic timestamp |
| `system_timestamp` | `system_clock::time_point` | Wall-clock timestamp |
| `thread_id` | `std::thread::id` | Processing thread ID |
| `worker_id` | `std::size_t` | Worker identifier |
| `wait_time` | `std::chrono::nanoseconds` | Queue wait duration |
| `execution_time` | `std::chrono::nanoseconds` | Execution duration |
| `error_code` | `std::optional<int>` | Error code (failures only) |
| `error_message` | `std::optional<std::string>` | Error message (failures only) |

### Performance Impact

| Tracing Mode | Overhead per Event |
|--------------|-------------------|
| Disabled | 0 (no overhead) |
| Enabled, no listeners | < 1us (history recording only) |
| Enabled, with listeners | < 1us + listener processing time |

> **Warning**: Listener `on_event()` implementations must be fast (< 1us). Use async logging or buffering for expensive operations.

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

- [Architecture Overview](ARCHITECTURE.md) - System-wide architecture
- [DAG Scheduler Guide](DAG_GUIDE.md) - Dependency-aware job scheduling
- [NUMA Work-Stealing Guide](NUMA_GUIDE.md) - NUMA topology and work stealing
- [Autoscaler Guide](AUTOSCALER_GUIDE.md) - Dynamic worker pool sizing
