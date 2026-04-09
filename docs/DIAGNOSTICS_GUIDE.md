---
doc_id: "THR-GUID-002a"
doc_title: "Diagnostics Guide"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "thread_system"
category: "GUID"
---

# Diagnostics Guide

> **SSOT**: This document is the single source of truth for **Diagnostics Guide** (thread dump, job inspection, bottleneck detection, event tracing).

> **Language:** **English**

> **See also**: [Metrics Guide](METRICS_GUIDE.md) for health monitoring, Prometheus integration, and metrics backends.

A comprehensive guide to thread_system's diagnostics subsystem, covering thread dumps, job inspection, bottleneck detection, and event tracing.

## Table of Contents

1. [Overview](#overview)
2. [Diagnostics Subsystem](#diagnostics-subsystem)
3. [Bottleneck Detection](#bottleneck-detection)
4. [Event Tracing](#event-tracing)

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

## Related Documentation

- [Metrics Guide](METRICS_GUIDE.md) - Health monitoring, Prometheus, metrics backends
- [Architecture Overview](ARCHITECTURE_OVERVIEW.md) - System-wide architecture
- [Policy Queue Combinations Guide](POLICY_QUEUE_GUIDE.md) - Queue and policy configurations
- [NUMA Work-Stealing Guide](NUMA_GUIDE.md) - NUMA topology and work stealing
- [Autoscaler Guide](AUTOSCALER_GUIDE.md) - Dynamic worker pool sizing
