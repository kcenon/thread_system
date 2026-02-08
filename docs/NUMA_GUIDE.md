# NUMA Topology and Work-Stealing Guide

> **Language:** **English**

A comprehensive guide to thread_system's NUMA-aware thread pool and work-stealing subsystem, covering topology detection, NUMA-local scheduling, and cross-node work stealing.

## Table of Contents

1. [Overview](#overview)
2. [NUMA Concepts](#numa-concepts)
3. [Topology Detection](#topology-detection)
4. [NUMA Thread Pool](#numa-thread-pool)
5. [Work Stealing](#work-stealing)
6. [Configuration Reference](#configuration-reference)
7. [Metrics and Observability](#metrics-and-observability)
8. [Usage Examples](#usage-examples)
9. [Performance Tuning](#performance-tuning)
10. [Anti-Patterns](#anti-patterns)
11. [Troubleshooting](#troubleshooting)

---

## Overview

The NUMA subsystem enables thread_system to optimize work distribution on Non-Uniform Memory Access (NUMA) architectures. It consists of three cooperating layers:

```
┌─────────────────────────────────────────────────┐
│              numa_thread_pool                    │  User-facing API
│         (extends thread_pool)                    │
├─────────────────────────────────────────────────┤
│           numa_work_stealer                      │  Stealing engine
│     (6 victim selection policies)                │
├──────────────────┬──────────────────────────────┤
│  numa_topology   │  work_affinity_tracker        │  Infrastructure
│  (HW detection)  │  (cooperation tracking)       │
└──────────────────┴──────────────────────────────┘
```

### Source Files

| Header | Description |
|--------|-------------|
| [`numa_topology.h`](../include/kcenon/thread/stealing/numa_topology.h) | NUMA topology detection and queries |
| [`numa_thread_pool.h`](../include/kcenon/thread/core/numa_thread_pool.h) | NUMA-aware thread pool (extends `thread_pool`) |
| [`numa_work_stealer.h`](../include/kcenon/thread/stealing/numa_work_stealer.h) | NUMA-aware work-stealing engine |
| [`enhanced_work_stealing_config.h`](../include/kcenon/thread/stealing/enhanced_work_stealing_config.h) | Configuration with factory presets |
| [`enhanced_steal_policy.h`](../include/kcenon/thread/stealing/enhanced_steal_policy.h) | Victim selection policy enum |
| [`work_stealing_stats.h`](../include/kcenon/thread/stealing/work_stealing_stats.h) | Atomic statistics and snapshots |
| [`work_affinity_tracker.h`](../include/kcenon/thread/stealing/work_affinity_tracker.h) | Worker cooperation tracking |
| [`steal_backoff_strategy.h`](../include/kcenon/thread/stealing/steal_backoff_strategy.h) | Backoff strategies for failed steals |
| [`worker_policy.h`](../include/kcenon/thread/core/worker_policy.h) | Worker behavior and CPU pinning |

### When to Use

| Scenario | Recommendation |
|----------|---------------|
| Single-socket workstation | Use base `thread_pool` — NUMA overhead unnecessary |
| Multi-socket server with memory-intensive work | Use `numa_thread_pool` |
| CPU-bound tasks with low memory access | Base `thread_pool` is sufficient |
| Heterogeneous workloads with cache sensitivity | Use `numa_thread_pool` with `locality_aware` policy |
| Need to monitor NUMA performance metrics | Use `numa_thread_pool` with statistics enabled |

---

## NUMA Concepts

### What is NUMA?

NUMA (Non-Uniform Memory Access) is a memory architecture where each CPU socket has its own local memory. Accessing local memory is fast; accessing another socket's memory is slower.

```
┌──────────────────────┐     ┌──────────────────────┐
│     NUMA Node 0      │     │     NUMA Node 1      │
│                      │     │                      │
│  ┌────┐  ┌────┐     │     │     ┌────┐  ┌────┐  │
│  │CPU0│  │CPU1│     │     │     │CPU4│  │CPU5│  │
│  └────┘  └────┘     │     │     └────┘  └────┘  │
│  ┌────┐  ┌────┐     │     │     ┌────┐  ┌────┐  │
│  │CPU2│  │CPU3│     │     │     │CPU6│  │CPU7│  │
│  └────┘  └────┘     │     │     └────┘  └────┘  │
│                      │     │                      │
│  ┌──────────────┐    │     │    ┌──────────────┐  │
│  │ Local Memory │    │     │    │ Local Memory │  │
│  │    (fast)    │    │     │    │    (fast)    │  │
│  └──────────────┘    │     │    └──────────────┘  │
│                      │     │                      │
└──────────┬───────────┘     └───────────┬──────────┘
           │       Interconnect          │
           │   (slower cross-node)       │
           └─────────────────────────────┘
```

### Why NUMA Matters for Thread Pools

| Access Pattern | Latency | Impact |
|----------------|---------|--------|
| **Local memory** (same node) | ~100 ns | Optimal |
| **Remote memory** (cross node) | ~200-300 ns | 2-3x slower |

Without NUMA awareness, a thread pool may schedule work on CPU 4 (Node 1) that accesses data allocated by CPU 0 (Node 0), causing every memory access to cross the interconnect. NUMA-aware scheduling keeps work close to its data.

### NUMA Distance

thread_system uses the Linux NUMA distance convention:

| Distance | Meaning |
|:--------:|---------|
| **10** | Local (same node) |
| **20** | Adjacent node (typical 2-socket) |
| **30+** | Farther nodes (4+ socket systems) |

---

## Topology Detection

### `numa_topology` Class

The `numa_topology` class detects the system's NUMA layout at runtime and provides an immutable, thread-safe view of the topology.

```cpp
#include <kcenon/thread/stealing/numa_topology.h>

auto topology = numa_topology::detect();
```

### Detection Behavior

| Platform | Method | Result |
|----------|--------|--------|
| **Linux** | Reads `/sys/devices/system/node/` | Full NUMA topology (nodes, CPUs, distances) |
| **macOS** | Fallback | Single node with all CPUs (no NUMA on macOS) |
| **Windows** | Fallback | Single node with all CPUs |

The detection runs once and the result is immutable. It is safe to query from any thread after construction.

### API Reference

| Method | Returns | Description |
|--------|---------|-------------|
| `detect()` | `numa_topology` | Static factory: detects system topology |
| `is_numa_available()` | `bool` | `true` if system has multiple NUMA nodes |
| `node_count()` | `size_t` | Number of NUMA nodes |
| `cpu_count()` | `size_t` | Total number of CPUs |
| `get_nodes()` | `const vector<numa_node>&` | All NUMA nodes |
| `get_node_for_cpu(cpu_id)` | `int` | NUMA node for a CPU (-1 if invalid) |
| `get_cpus_for_node(node_id)` | `vector<int>` | CPUs on a node (empty if invalid) |
| `get_distance(node1, node2)` | `int` | Distance between nodes (10 = local, -1 if invalid) |
| `is_same_node(cpu1, cpu2)` | `bool` | Whether two CPUs share a NUMA node |

### `numa_node` Structure

```cpp
struct numa_node {
    int node_id{-1};                   // NUMA node identifier
    std::vector<int> cpu_ids;          // CPUs belonging to this node
    std::size_t memory_size_bytes{0};  // Total memory on this node
};
```

### Example: Querying Topology

```cpp
auto topology = numa_topology::detect();

if (topology.is_numa_available()) {
    std::cout << "NUMA nodes: " << topology.node_count() << "\n";
    std::cout << "Total CPUs: " << topology.cpu_count() << "\n";

    for (const auto& node : topology.get_nodes()) {
        std::cout << "Node " << node.node_id << ": "
                  << node.cpu_ids.size() << " CPUs, "
                  << (node.memory_size_bytes / (1024 * 1024)) << " MB\n";
    }

    // Check distance between nodes
    int dist = topology.get_distance(0, 1);
    std::cout << "Distance between node 0 and 1: " << dist << "\n";

    // Check CPU locality
    if (topology.is_same_node(0, 1)) {
        std::cout << "CPU 0 and 1 are on the same NUMA node\n";
    }
} else {
    std::cout << "No NUMA detected (single-node system)\n";
}
```

---

## NUMA Thread Pool

### `numa_thread_pool` Class

`numa_thread_pool` extends `thread_pool` with NUMA-specific features. It inherits all base functionality (job enqueue, workers, policies) and adds NUMA-aware work stealing.

```cpp
#include <kcenon/thread/core/numa_thread_pool.h>

auto pool = std::make_shared<numa_thread_pool>("numa_workers");
```

### Construction

Three constructors are available:

```cpp
// 1. Default: auto-detects topology
numa_thread_pool(const std::string& title = "numa_thread_pool",
                 const thread_context& context = thread_context());

// 2. With custom job queue
numa_thread_pool(const std::string& title,
                 std::shared_ptr<job_queue> custom_queue,
                 const thread_context& context = thread_context());

// 3. With policy_queue adapter
numa_thread_pool(const std::string& title,
                 std::unique_ptr<pool_queue_adapter_interface> queue_adapter,
                 const thread_context& context = thread_context());
```

Topology detection happens automatically on construction (lazy, triggered on first access).

### NUMA-Specific API

| Method | Returns | Description |
|--------|---------|-------------|
| `configure_numa_work_stealing(config)` | `void` | Set NUMA work-stealing configuration |
| `numa_work_stealing_config()` | `const enhanced_work_stealing_config&` | Get current config |
| `numa_work_stealing_stats()` | `work_stealing_stats_snapshot` | Get statistics snapshot |
| `numa_topology_info()` | `const numa_topology&` | Get detected topology |
| `is_numa_system()` | `bool` | Check if system has NUMA |
| `enable_numa_work_stealing()` | `void` | Enable with `numa_optimized()` defaults |
| `disable_numa_work_stealing()` | `void` | Revert to basic work stealing |
| `is_numa_work_stealing_enabled()` | `bool` | Check if NUMA stealing is active |

### Migration from `thread_pool`

If you were using deprecated NUMA methods on `thread_pool`:

```cpp
// Old (deprecated):
auto pool = std::make_shared<thread_pool>("pool");
pool->set_work_stealing_config(config);
auto stats = pool->get_work_stealing_stats();

// New (recommended):
auto pool = std::make_shared<numa_thread_pool>("pool");
pool->configure_numa_work_stealing(config);
auto stats = pool->numa_work_stealing_stats();
```

---

## Work Stealing

### How Work Stealing Works

When a worker's local queue is empty, it attempts to **steal** jobs from other workers' queues:

```
Worker 0 (idle)          Worker 1 (busy)         Worker 2 (busy)
┌──────────────┐        ┌──────────────┐        ┌──────────────┐
│  (empty)     │ ──steal──> │ Job A      │        │ Job D        │
│              │        │ Job B        │        │ Job E        │
│              │        │ Job C        │        │ Job F        │
└──────────────┘        └──────────────┘        └──────────────┘
      NUMA Node 0              NUMA Node 0            NUMA Node 1
```

With NUMA awareness, Worker 0 prefers stealing from Worker 1 (same node) over Worker 2 (cross-node), reducing cross-node memory traffic.

### `numa_work_stealer` Class

The core work-stealing engine that coordinates theft operations across workers.

```cpp
#include <kcenon/thread/stealing/numa_work_stealer.h>

// Accessors provide worker information
auto deque_accessor = [&](std::size_t id) { return workers[id]->get_local_deque(); };
auto cpu_accessor = [&](std::size_t id) { return workers[id]->get_policy().preferred_cpu; };

// Create stealer with NUMA-optimized config
auto config = enhanced_work_stealing_config::numa_optimized();
numa_work_stealer stealer(worker_count, deque_accessor, cpu_accessor, config);
```

### Steal Operations

| Method | Returns | Description |
|--------|---------|-------------|
| `steal_for(worker_id)` | `job*` | Steal a single job for the worker (nullptr if none) |
| `steal_batch_for(worker_id, max)` | `vector<job*>` | Steal up to `max` jobs (may return fewer) |

```cpp
// Single steal
if (auto* j = stealer.steal_for(0)) {
    j->do_work();
}

// Batch steal (more efficient for multiple transfers)
auto batch = stealer.steal_batch_for(0, 4);
for (auto* j : batch) {
    j->do_work();
}
```

### Victim Selection Policies

The `enhanced_steal_policy` enum defines how victims are selected:

| Policy | Algorithm | Best For |
|--------|-----------|----------|
| `random` | Random victim selection | General use, good load distribution |
| `round_robin` | Sequential selection across workers | Deterministic, fair distribution |
| `adaptive` | Prefer workers with largest queues | Uneven workloads (default) |
| `numa_aware` | Prefer same NUMA node, penalty for cross-node | NUMA systems with memory locality |
| `locality_aware` | Prefer workers with recent cooperation history | Cache-sensitive workloads |
| `hierarchical` | Select NUMA node first, then random within | Large NUMA systems (4+ sockets) |

#### Policy Selection Flowchart

```
Is this a NUMA system?
├── NO → Use 'adaptive' (default)
└── YES
    ├── 2 sockets? → Use 'numa_aware'
    ├── 4+ sockets? → Use 'hierarchical'
    └── Cache-sensitive work? → Use 'locality_aware'
```

### NUMA-Aware Stealing

When `numa_aware` is `true`, the stealer applies a **penalty factor** to cross-node steals:

```
Same-node steal cost:  1.0
Cross-node steal cost: 1.0 × numa_penalty_factor (default: 2.0)
```

This makes the stealer naturally prefer same-node victims. The penalty does not block cross-node steals — it only makes them less preferred when same-node alternatives exist.

### Batch Stealing

Batch stealing transfers multiple jobs at once, reducing the overhead of individual steal operations:

```cpp
enhanced_work_stealing_config config;
config.min_steal_batch = 1;          // Minimum jobs per batch (default: 1)
config.max_steal_batch = 4;          // Maximum jobs per batch (default: 4)
config.adaptive_batch_size = true;   // Adjust based on victim queue depth (default)
```

When `adaptive_batch_size` is `true`, the actual batch size is dynamically adjusted based on the victim's queue depth. A half-full queue donates fewer jobs than a full one.

### Backoff Strategies

When steal attempts fail, workers use backoff to reduce contention:

| Strategy | Formula | Best For |
|----------|---------|----------|
| `fixed` | `delay = initial_backoff` | Predictable, constant workloads |
| `linear` | `delay = initial * (attempt + 1)` | Moderate contention |
| `exponential` | `delay = initial * 2^attempt` | High contention (default) |
| `adaptive_jitter` | `exponential + random(±50%)` | Variable workloads |

```cpp
config.backoff_strategy = steal_backoff_strategy::exponential;
config.initial_backoff = std::chrono::microseconds{50};    // 50µs initial delay
config.max_backoff = std::chrono::microseconds{1000};      // 1ms maximum delay
config.backoff_multiplier = 2.0;                           // Double per attempt
```

### Work Affinity Tracking

The `work_affinity_tracker` records cooperation patterns between workers. Workers that frequently steal from each other develop higher affinity scores, making them preferred victims.

```cpp
config.track_locality = true;           // Enable affinity tracking
config.locality_history_size = 16;      // Track last 16 interactions (default)
```

**Design rationale**: When workers frequently cooperate, they likely share related work that benefits from cache locality. Preferring high-affinity victims improves cache utilization.

The affinity tracker uses an upper-triangular cooperation matrix with atomic operations for lock-free updates. Memory usage scales as `O(worker_count^2)`.

---

## Configuration Reference

### `enhanced_work_stealing_config` Structure

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `enabled` | `bool` | `false` | Master switch for work stealing |
| `policy` | `enhanced_steal_policy` | `adaptive` | Victim selection policy |
| `numa_aware` | `bool` | `false` | Enable NUMA-aware stealing |
| `numa_penalty_factor` | `double` | `2.0` | Cross-node steal cost multiplier |
| `prefer_same_node` | `bool` | `true` | Prefer same NUMA node |
| `min_steal_batch` | `size_t` | `1` | Minimum batch size |
| `max_steal_batch` | `size_t` | `4` | Maximum batch size |
| `adaptive_batch_size` | `bool` | `true` | Dynamic batch sizing |
| `max_steal_attempts` | `size_t` | `3` | Max attempts per stealing round |
| `max_consecutive_failures` | `size_t` | `10` | Failures before yielding |
| `backoff_strategy` | `steal_backoff_strategy` | `exponential` | Backoff strategy |
| `initial_backoff` | `microseconds` | `50µs` | Initial backoff delay |
| `max_backoff` | `microseconds` | `1000µs` | Maximum backoff delay |
| `backoff_multiplier` | `double` | `2.0` | Exponential backoff multiplier |
| `track_locality` | `bool` | `false` | Enable affinity tracking |
| `locality_history_size` | `size_t` | `16` | Affinity history window |
| `collect_statistics` | `bool` | `false` | Enable statistics collection |

### Factory Presets

| Preset | `enabled` | `policy` | `numa_aware` | `track_locality` | `collect_statistics` | Use Case |
|--------|:-:|--------|:-:|:-:|:-:|----------|
| `default_config()` | false | adaptive | false | false | false | Disabled baseline |
| `numa_optimized()` | true | numa_aware | true | false | true | NUMA systems (2-socket) |
| `locality_optimized()` | true | locality_aware | false | true | true | Cache-sensitive work |
| `batch_optimized()` | true | adaptive | false | false | false | High-throughput batch |
| `hierarchical_numa()` | true | hierarchical | true | true | true | Large NUMA (4+ sockets) |

### Factory Preset Details

```cpp
// NUMA optimized (most common for NUMA systems)
auto config = enhanced_work_stealing_config::numa_optimized();
// Sets: enabled=true, policy=numa_aware, numa_aware=true,
//       prefer_same_node=true, numa_penalty_factor=2.0,
//       collect_statistics=true

// Locality optimized (for cache-sensitive workloads)
auto config = enhanced_work_stealing_config::locality_optimized();
// Sets: enabled=true, policy=locality_aware, track_locality=true,
//       locality_history_size=32, collect_statistics=true

// Batch optimized (for high-throughput scenarios)
auto config = enhanced_work_stealing_config::batch_optimized();
// Sets: enabled=true, policy=adaptive, min_steal_batch=2,
//       max_steal_batch=8, adaptive_batch_size=true

// Hierarchical NUMA (for 4+ socket systems)
auto config = enhanced_work_stealing_config::hierarchical_numa();
// Sets: enabled=true, policy=hierarchical, numa_aware=true,
//       prefer_same_node=true, numa_penalty_factor=3.0,
//       track_locality=true, collect_statistics=true
```

---

## Metrics and Observability

### Statistics Structure

Work-stealing statistics are collected with atomic operations for lock-free concurrent updates.

| Field | Type | Description |
|-------|------|-------------|
| `steal_attempts` | `uint64_t` | Total steal attempts |
| `successful_steals` | `uint64_t` | Successful steal operations |
| `failed_steals` | `uint64_t` | Failed steal operations |
| `jobs_stolen` | `uint64_t` | Total jobs stolen |
| `same_node_steals` | `uint64_t` | Steals from same NUMA node |
| `cross_node_steals` | `uint64_t` | Steals from different NUMA node |
| `batch_steals` | `uint64_t` | Batch steal operations |
| `total_batch_size` | `uint64_t` | Total jobs across all batches |
| `total_steal_time_ns` | `uint64_t` | Time spent stealing (ns) |
| `total_backoff_time_ns` | `uint64_t` | Time spent in backoff (ns) |

### Computed Metrics

| Method | Formula | Interpretation |
|--------|---------|----------------|
| `steal_success_rate()` | `successful / attempts` | 0.0–1.0; higher = less contention |
| `avg_batch_size()` | `total_batch_size / batch_steals` | Jobs per batch operation |
| `cross_node_ratio()` | `cross / (same + cross)` | 0.0–1.0; **lower = better NUMA locality** |
| `avg_steal_time_ns()` | `total_time / attempts` | Average nanoseconds per attempt |

### Reading Statistics

```cpp
// Via numa_thread_pool (recommended)
auto stats = pool->numa_work_stealing_stats();

// Via numa_work_stealer directly
auto snap = stealer.get_stats_snapshot();  // Non-atomic snapshot (safe to iterate)
auto& live = stealer.get_stats();          // Live atomic counters

// Key metrics
std::cout << "Success rate: " << stats.steal_success_rate() * 100 << "%\n";
std::cout << "Cross-node ratio: " << stats.cross_node_ratio() * 100 << "%\n";
std::cout << "Avg batch size: " << stats.avg_batch_size() << "\n";
std::cout << "Avg steal time: " << stats.avg_steal_time_ns() << " ns\n";
```

### Memory Ordering

- **Counter updates**: `relaxed` ordering — eventual consistency is acceptable for statistics
- **Snapshot reads**: `acquire` ordering — consistent view of all counters
- **Reset**: Not atomic across all counters; avoid calling `reset()` during active stealing

---

## Usage Examples

### Basic NUMA-Aware Pool

```cpp
#include <kcenon/thread/core/numa_thread_pool.h>
#include <kcenon/thread/core/thread_worker.h>
#include <kcenon/thread/core/job_builder.h>

// Create NUMA-aware pool
auto pool = std::make_shared<numa_thread_pool>("numa_workers");

// Add workers
for (int i = 0; i < 8; ++i) {
    pool->enqueue(std::make_unique<thread_worker>(true));
}

// Enable NUMA-optimized work stealing (convenience method)
pool->enable_numa_work_stealing();
pool->start();

// Check NUMA topology
if (pool->is_numa_system()) {
    const auto& topo = pool->numa_topology_info();
    std::cout << "Running on " << topo.node_count() << " NUMA nodes\n";
}

// Submit work using job_builder
for (int i = 0; i < 1000; ++i) {
    auto j = job_builder()
        .name("task_" + std::to_string(i))
        .work([]() -> common::VoidResult {
            // Work is automatically distributed across NUMA nodes
            return common::ok();
        })
        .build();
    pool->enqueue(std::move(j));
}

// Monitor NUMA performance
auto stats = pool->numa_work_stealing_stats();
std::cout << "Cross-node steal ratio: " << stats.cross_node_ratio() * 100 << "%\n";

pool->stop();
```

### Custom NUMA Configuration

```cpp
#include <kcenon/thread/core/numa_thread_pool.h>
#include <kcenon/thread/stealing/enhanced_work_stealing_config.h>

auto pool = std::make_shared<numa_thread_pool>("custom_numa");

// Custom configuration
enhanced_work_stealing_config config;
config.enabled = true;
config.policy = enhanced_steal_policy::numa_aware;
config.numa_aware = true;
config.prefer_same_node = true;
config.numa_penalty_factor = 3.0;       // Strong preference for same-node
config.max_steal_batch = 8;              // Larger batches for throughput
config.backoff_strategy = steal_backoff_strategy::adaptive_jitter;
config.collect_statistics = true;

pool->configure_numa_work_stealing(config);

// Add workers and start
for (int i = 0; i < 16; ++i) {
    pool->enqueue(std::make_unique<thread_worker>(true));
}
pool->start();

// ... submit work ...

pool->stop();
```

### Hierarchical NUMA (4+ Sockets)

```cpp
auto pool = std::make_shared<numa_thread_pool>("hierarchical_pool");

// Use hierarchical preset for large NUMA systems
pool->configure_numa_work_stealing(
    enhanced_work_stealing_config::hierarchical_numa()
);

// hierarchical_numa() sets:
// - policy: hierarchical (select node first, then random within)
// - numa_penalty_factor: 3.0 (stronger cross-node penalty)
// - track_locality: true
// - collect_statistics: true
```

### Monitoring and Diagnostics

```cpp
auto pool = std::make_shared<numa_thread_pool>("monitored_pool");
pool->enable_numa_work_stealing();

// ... run workload ...

// Periodic monitoring
auto stats = pool->numa_work_stealing_stats();
const auto& topo = pool->numa_topology_info();

std::cout << "=== NUMA Work-Stealing Report ===\n";
std::cout << "NUMA nodes: " << topo.node_count() << "\n";
std::cout << "Total CPUs: " << topo.cpu_count() << "\n";
std::cout << "\n--- Stealing Statistics ---\n";
std::cout << "Attempts: " << stats.steal_attempts << "\n";
std::cout << "Success rate: " << stats.steal_success_rate() * 100 << "%\n";
std::cout << "Same-node steals: " << stats.same_node_steals << "\n";
std::cout << "Cross-node steals: " << stats.cross_node_steals << "\n";
std::cout << "Cross-node ratio: " << stats.cross_node_ratio() * 100 << "%\n";
std::cout << "Avg batch size: " << stats.avg_batch_size() << "\n";
std::cout << "Avg steal time: " << stats.avg_steal_time_ns() << " ns\n";
```

### Topology Query (Standalone)

```cpp
#include <kcenon/thread/stealing/numa_topology.h>

auto topology = numa_topology::detect();

// Print system layout
for (const auto& node : topology.get_nodes()) {
    std::cout << "Node " << node.node_id << " CPUs: [";
    for (std::size_t i = 0; i < node.cpu_ids.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << node.cpu_ids[i];
    }
    std::cout << "]\n";
}

// Print distance matrix
std::cout << "\nDistance matrix:\n";
for (std::size_t i = 0; i < topology.node_count(); ++i) {
    for (std::size_t j = 0; j < topology.node_count(); ++j) {
        std::cout << topology.get_distance(
            static_cast<int>(i), static_cast<int>(j)) << "\t";
    }
    std::cout << "\n";
}
```

---

## Performance Tuning

### Choosing the Right Policy

| Workload | Recommended Policy | Why |
|----------|-------------------|-----|
| **General purpose** | `adaptive` | Steals from largest queues, good balance |
| **2-socket NUMA server** | `numa_aware` | Minimizes cross-node memory traffic |
| **4+ socket NUMA server** | `hierarchical` | Two-level selection reduces search space |
| **Shared data structures** | `locality_aware` | Keeps related work on cooperating workers |
| **Fair distribution needed** | `round_robin` | Deterministic, equal treatment |
| **Simple/testing** | `random` | Low overhead, reasonable distribution |

### NUMA Penalty Factor Guidelines

| Factor | Effect | Use Case |
|:------:|--------|----------|
| **1.0** | No NUMA preference | Testing, or NUMA overhead is negligible |
| **2.0** (default) | Moderate preference for same-node | Typical 2-socket servers |
| **3.0** | Strong preference for same-node | Memory-intensive workloads |
| **5.0+** | Very strong — almost never cross-node | Only when cross-node is very expensive |

> **Warning**: Very high penalty factors can cause load imbalance if one node has more work than others. Monitor `cross_node_ratio()` — a value near 0% may indicate underutilization of some nodes.

### Batch Size Tuning

| Batch Size | Trade-off |
|:----------:|-----------|
| **1** (min) | Low per-steal cost, but many steal operations |
| **4** (default) | Good balance for most workloads |
| **8** | Reduces steal frequency, good for high-throughput |
| **16+** | Risk of over-stealing; victim may become idle |

Use `adaptive_batch_size = true` (default) to let the system auto-adjust based on queue depth.

### Backoff Tuning

| Parameter | Low Value | High Value |
|-----------|-----------|------------|
| `initial_backoff` | 10µs (responsive) | 100µs (reduces contention) |
| `max_backoff` | 500µs (responsive) | 5000µs (reduces CPU waste) |
| `max_steal_attempts` | 1-2 (yield quickly) | 5-10 (try harder) |
| `max_consecutive_failures` | 5 (yield early) | 20 (persistent) |

### Performance Indicators

| Metric | Healthy Range | Action if Outside |
|--------|:-------------:|-------------------|
| `steal_success_rate()` | 30-70% | < 30%: too much contention; > 70%: imbalanced work |
| `cross_node_ratio()` | < 20% | > 20%: increase `numa_penalty_factor` or check affinity |
| `avg_batch_size()` | 2-6 | < 2: increase `max_steal_batch`; > 8: may be over-stealing |
| `avg_steal_time_ns()` | < 1000 ns | > 1000: high contention, increase backoff |

---

## Anti-Patterns

### 1. Enabling NUMA on Single-Socket Systems

```cpp
// Unnecessary: NUMA overhead without benefit
auto pool = std::make_shared<numa_thread_pool>("pool");
pool->enable_numa_work_stealing();

// Better: Check first
if (pool->is_numa_system()) {
    pool->enable_numa_work_stealing();
} else {
    // Base thread_pool work stealing is sufficient
}
```

### 2. Forgetting to Enable Work Stealing

```cpp
// WRONG: config created but enabled is false (default)
enhanced_work_stealing_config config;
config.policy = enhanced_steal_policy::numa_aware;
config.numa_aware = true;
// config.enabled = false;  // Default! No stealing happens

pool->configure_numa_work_stealing(config);
```

Always set `config.enabled = true`, or use factory presets which set it automatically.

### 3. Very High Penalty Factor Causing Starvation

```cpp
// WRONG: Cross-node stealing effectively disabled
config.numa_penalty_factor = 100.0;
// Workers on underloaded nodes starve while other nodes are overloaded
```

### 4. Collecting Statistics in Production Without Need

```cpp
// Overhead: atomic increments on every steal operation
config.collect_statistics = true;  // Only enable when diagnosing
```

Statistics collection adds atomic operations to every steal attempt. Disable in production unless actively monitoring.

### 5. Using locality_aware Without Understanding Memory Impact

```cpp
// Memory: O(worker_count^2) for cooperation matrix
config.track_locality = true;
config.locality_history_size = 1024;  // Excessive history
// With 64 workers: 64^2 * 1024 = 4 million entries
```

Keep `locality_history_size` small (16-32). Large values consume memory without proportional benefit.

---

## Troubleshooting

### Work Stealing Not Working

1. **Check `enabled` flag**: `config.enabled` defaults to `false`
2. **Verify pool type**: Must be `numa_thread_pool`, not base `thread_pool`
3. **Check worker count**: Need at least 2 workers (can't steal from self)
4. **Verify `start()` called**: Pool must be started before stealing begins

### High Cross-Node Ratio

1. **Increase `numa_penalty_factor`**: Default 2.0 may not be enough for memory-intensive work
2. **Switch to `numa_aware` or `hierarchical` policy**: `adaptive` and `random` don't consider NUMA
3. **Check CPU pinning**: Ensure workers have `preferred_cpu` set for NUMA detection
4. **Verify topology detection**: Call `numa_topology_info()` to confirm multi-node detection

### Low Steal Success Rate

1. **All workers busy**: Not a problem — indicates balanced load
2. **Workers not producing enough work**: Add more jobs or reduce worker count
3. **Increase `max_steal_attempts`**: More attempts per round
4. **Try `adaptive` policy**: Targets workers with largest queues

### Fallback Behavior on Non-NUMA Systems

On macOS or single-socket Linux systems, `numa_topology::detect()` returns a single-node topology with all CPUs. Work stealing still functions but without NUMA-specific optimizations. The `is_numa_available()` method returns `false` and `node_count()` returns `1`.

```cpp
// Safe on all platforms
auto pool = std::make_shared<numa_thread_pool>("pool");
pool->enable_numa_work_stealing();
// On non-NUMA: works as basic work stealing
// On NUMA: applies NUMA-aware victim selection
```

---

## Related Documentation

- [Architecture Overview](ARCHITECTURE.md) — System structure and design principles
- [Autoscaler Guide](AUTOSCALER_GUIDE.md) — Dynamic worker scaling
- [Policy Queue Guide](POLICY_QUEUE_GUIDE.md) — Queue-level policy composition
- [API Reference](API_REFERENCE.md) — Complete API documentation
- [Benchmarks](BENCHMARKS.md) — Performance measurements
