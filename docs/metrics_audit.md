# Metrics System Audit

## Overview

This document provides a comprehensive audit of the metrics system in thread_system,
identifying duplications, defining canonical metric names, and proposing a unified architecture.

## Metrics Classes Inventory

### Primary Classes

| Class | Location | Lines | Purpose |
|-------|----------|-------|---------|
| `ThreadPoolMetrics` | `metrics/thread_pool_metrics.h` | 75 | Lightweight metrics container |
| `EnhancedThreadPoolMetrics` | `metrics/enhanced_metrics.h/cpp` | 856 | Production-grade observability |
| `LatencyHistogram` | `metrics/latency_histogram.h/cpp` | 649 | HDR-style histogram |
| `SlidingWindowCounter` | `metrics/sliding_window_counter.h/cpp` | 499 | Throughput measurement |

### Supporting Structures

| Structure | Location | Purpose |
|-----------|----------|---------|
| `ThreadPoolMetrics::Snapshot` | `thread_pool_metrics.h` | Basic metrics snapshot |
| `EnhancedSnapshot` | `enhanced_metrics.h` | Comprehensive snapshot with percentiles |
| `WorkerMetrics` | `enhanced_metrics.h` | Per-worker tracking |
| `scaling_metrics_sample` | `scaling/scaling_metrics.h` | Autoscaling decisions |
| `work_stealing_stats_snapshot` | `stealing/work_stealing_stats.h` | NUMA-aware scheduling |

## Duplication Analysis

### Field Duplication (ThreadPoolMetrics vs EnhancedThreadPoolMetrics)

| Field | ThreadPoolMetrics | EnhancedThreadPoolMetrics | Status |
|-------|-------------------|---------------------------|--------|
| `tasks_submitted_` | Yes | Yes | **DUPLICATE** |
| `tasks_executed_` | Yes | Yes | **DUPLICATE** |
| `tasks_failed_` | Yes | Yes | **DUPLICATE** |
| `total_busy_time_ns_` | Yes | Yes | **DUPLICATE** |
| `total_idle_time_ns_` | Yes | Yes | **DUPLICATE** |
| `tasks_enqueued_` | Yes | No | Unique |

**Field Duplication Rate**: 83% (5 of 6 fields)

### Method Duplication

| Method | ThreadPoolMetrics | EnhancedThreadPoolMetrics | Status |
|--------|-------------------|---------------------------|--------|
| `record_submission()` | Yes | Yes | **DUPLICATE** |
| `record_execution()` | Yes | Yes | **DUPLICATE** |
| `reset()` | Yes | Yes | **DUPLICATE** |
| `snapshot()` | Yes | Yes | **DUPLICATE** |

**Method Duplication Rate**: 100% (4 core methods)

### Implementation Pattern Duplication

Both classes use identical patterns:
- `fetch_add()` with `memory_order_relaxed`
- `store()` with `memory_order_relaxed`
- `load()` with `memory_order_relaxed`

**Overall Duplication Estimate**: 40-50%

## Canonical Metric Names (Prometheus Convention)

### Counter Metrics

| Current Name | Canonical Name | Type | Description |
|--------------|----------------|------|-------------|
| `tasks_submitted_` | `thread_pool_tasks_submitted_total` | Counter | Total tasks submitted |
| `tasks_enqueued_` | `thread_pool_tasks_enqueued_total` | Counter | Total tasks enqueued |
| `tasks_executed_` | `thread_pool_tasks_executed_total` | Counter | Total tasks executed |
| `tasks_failed_` | `thread_pool_tasks_failed_total` | Counter | Total failed tasks |
| `total_busy_time_ns_` | `thread_pool_busy_time_nanoseconds_total` | Counter | Total busy time |
| `total_idle_time_ns_` | `thread_pool_idle_time_nanoseconds_total` | Counter | Total idle time |

### Gauge Metrics

| Current Name | Canonical Name | Type | Description |
|--------------|----------------|------|-------------|
| `current_queue_depth_` | `thread_pool_queue_depth` | Gauge | Current queue depth |
| `peak_queue_depth_` | `thread_pool_queue_depth_peak` | Gauge | Peak queue depth |
| `active_workers_` | `thread_pool_workers_active` | Gauge | Active worker count |

### Histogram Metrics

| Current Name | Canonical Name | Type | Description |
|--------------|----------------|------|-------------|
| `enqueue_latency_` | `thread_pool_enqueue_latency_seconds` | Histogram | Enqueue latency |
| `execution_latency_` | `thread_pool_execution_latency_seconds` | Histogram | Execution latency |
| `wait_time_` | `thread_pool_wait_time_seconds` | Histogram | Queue wait time |

## Recommended Architecture

### Class Hierarchy

```
MetricsBase (abstract base)
├── ThreadPoolMetrics (lightweight)
└── EnhancedThreadPoolMetrics (full-featured)
    ├── LatencyHistogram (3 instances)
    ├── SlidingWindowCounter (2 instances)
    └── WorkerMetrics[] (per-worker)
```

### MetricsBase Interface

```cpp
class MetricsBase {
public:
    virtual ~MetricsBase() = default;

    // Recording methods
    void record_submission(std::size_t count = 1);
    void record_execution(std::uint64_t duration_ns, bool success);
    void record_idle_time(std::uint64_t duration_ns);
    virtual void reset();

    // Common getters
    std::uint64_t tasks_submitted() const;
    std::uint64_t tasks_executed() const;
    std::uint64_t tasks_failed() const;
    std::uint64_t total_busy_time_ns() const;
    std::uint64_t total_idle_time_ns() const;

protected:
    std::atomic<std::uint64_t> tasks_submitted_{0};
    std::atomic<std::uint64_t> tasks_executed_{0};
    std::atomic<std::uint64_t> tasks_failed_{0};
    std::atomic<std::uint64_t> total_busy_time_ns_{0};
    std::atomic<std::uint64_t> total_idle_time_ns_{0};
};
```

### Benefits

1. **Code Reduction**: ~70% reduction in duplicated code
2. **Consistency**: Uniform metric recording across all variants
3. **Extensibility**: Easy to add new metrics types
4. **Maintainability**: Single point of change for core logic

## Migration Path

1. **Phase 3.1** (Complete): Audit and documentation
2. **Phase 3.2**: Create MetricsBase class
3. **Phase 3.3**: Implement backend interfaces
4. **Phase 3.4**: Migrate existing code

## Related Issues

- #436: Phase 3.0: Merge Duplicate Metrics Classes
- #470: Phase 3.1: Metrics Audit and Documentation
- #471: Phase 3.2: Create Unified MetricsBase Class
- #472: Phase 3.3: Implement MetricsBackend Interface
- #473: Phase 3.4: Migrate thread_pool to Unified Metrics

---

*Document created as part of Phase 3.0 refactoring*
