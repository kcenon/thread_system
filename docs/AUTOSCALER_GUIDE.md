# Autoscaler and Scaling Policies Guide

> **Language:** **English**

A comprehensive guide to thread_system's autoscaling subsystem, which dynamically adjusts thread pool size based on workload metrics.

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Scaling Policies](#scaling-policies)
4. [Autoscaler API](#autoscaler-api)
5. [Pool Integration](#pool-integration)
6. [Metrics and Observability](#metrics-and-observability)
7. [Configuration Examples](#configuration-examples)
8. [Best Practices](#best-practices)
9. [Anti-Patterns](#anti-patterns)
10. [Troubleshooting](#troubleshooting)

---

## Overview

The autoscaling subsystem provides automatic thread pool worker management driven by real-time workload metrics. It monitors utilization, queue depth, and latency, then adjusts the number of workers within configured bounds.

### Key Features

| Feature | Description |
|---------|-------------|
| **3 Operating Modes** | Disabled, Manual, Automatic |
| **Asymmetric Triggers** | Scale-up: OR logic; Scale-down: AND logic |
| **Cooldown Protection** | Prevents oscillation between scaling events |
| **Sample Aggregation** | Multiple samples required before any decision |
| **Non-intrusive Design** | Integrates via pool policy pattern without modifying `thread_pool` |
| **Observable** | Statistics, metrics history, and scaling callbacks |

### Source Files

| Header | Description |
|--------|-------------|
| [`autoscaler.h`](../include/kcenon/thread/scaling/autoscaler.h) | Core autoscaling class with background monitor thread |
| [`autoscaling_policy.h`](../include/kcenon/thread/scaling/autoscaling_policy.h) | Configuration struct with thresholds and modes |
| [`scaling_metrics.h`](../include/kcenon/thread/scaling/scaling_metrics.h) | Metrics types, decisions, and statistics |
| [`autoscaling_pool_policy.h`](../include/kcenon/thread/pool_policies/autoscaling_pool_policy.h) | Pool policy adapter wrapping autoscaler |
| [`pool_policy.h`](../include/kcenon/thread/pool_policies/pool_policy.h) | Base pool policy interface |

---

## Architecture

### Monitor Loop State Machine

The autoscaler runs a background thread that continuously cycles through five phases:

```
┌─────────────────────────────────────────────────────────────┐
│                    Autoscaler Loop                           │
│                                                             │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐     │
│  │ 1. Collect  │───>│ 2. Aggregate│───>│ 3. Make     │     │
│  │    Metrics  │    │    Samples  │    │    Decision │     │
│  └─────────────┘    └─────────────┘    └─────────────┘     │
│         │                                    │              │
│         │                                    v              │
│         │                         ┌─────────────────┐       │
│         │                         │ 4. Check        │       │
│         │                         │    Cooldown     │       │
│         │                         └─────────────────┘       │
│         │                                    │              │
│         │                                    v              │
│         │                         ┌─────────────────┐       │
│         │                         │ 5. Execute      │       │
│         │                         │    Scale        │       │
│         │                         └─────────────────┘       │
│         │                                    │              │
│         v                                    │              │
│  ┌─────────────┐                             │              │
│  │   Sleep     │<────────────────────────────┘              │
│  │  Interval   │                                            │
│  └─────────────┘                                            │
└─────────────────────────────────────────────────────────────┘
```

| Phase | Description |
|-------|-------------|
| **1. Collect Metrics** | Samples current pool state: worker count, active workers, queue depth, utilization, throughput |
| **2. Aggregate Samples** | Accumulates `samples_for_decision` samples before proceeding (default: 5) |
| **3. Make Decision** | Evaluates aggregated metrics against scale-up/down thresholds |
| **4. Check Cooldown** | Ensures minimum time has elapsed since last scaling event |
| **5. Execute Scale** | Adds or removes workers from the pool, invokes callback |

### Design Principles

- **Non-intrusive**: Scaling decisions are made asynchronously in a background thread
- **Configurable**: All thresholds, cooldowns, and behaviors are user-customizable
- **Graceful**: Scale-down removes workers only when safe (no forced termination)
- **Observable**: Provides `autoscaling_stats`, metrics history, and a `scaling_callback`

### Operating Modes

```cpp
enum class mode {
    disabled,   // No automatic scaling
    manual,     // Only scale on explicit API calls (evaluate_now, scale_to, scale_up, scale_down)
    automatic   // Fully automatic scaling via background monitor thread
};
```

| Mode | Monitor Thread | Manual API | Use Case |
|------|:-:|:-:|----------|
| `disabled` | Off | Available but no automatic evaluation | Testing, fixed pools |
| `manual` | Off | Full control | Operator-driven scaling |
| `automatic` | Running | Also available for override | Production workloads |

### Thread Safety

All public methods on `autoscaler` and `autoscaling_pool_policy` are thread-safe. Internal state is protected by:

- `std::atomic<bool>` for the `running_` flag
- `std::mutex` for policy, history, and stats access
- `std::condition_variable` for graceful shutdown signaling

---

## Scaling Policies

### `autoscaling_policy` Structure

The `autoscaling_policy` struct is a plain configuration object (no virtual methods). It defines all parameters for autoscaling behavior.

#### Worker Bounds

```cpp
std::size_t min_workers = 1;                                    // Never scale below this
std::size_t max_workers = std::thread::hardware_concurrency();  // Never scale above this
```

> **Important**: `min_workers` must be >= 1. `max_workers` must be >= `min_workers`. Targets are always clamped to `[min_workers, max_workers]`.

#### Scale-Up Configuration (OR Logic)

Scale-up triggers when **ANY** threshold is exceeded:

```cpp
struct scale_up_config {
    double queue_depth_threshold = 100.0;          // Jobs per worker
    double utilization_threshold = 0.8;            // 80% worker utilization
    double latency_threshold_ms = 50.0;            // P95 latency in milliseconds
    std::size_t pending_jobs_threshold = 1000;     // Absolute pending job count
};
```

The OR logic means the system reacts quickly to **any** sign of overload:

```
IF (utilization > 0.8) OR (queue_depth_per_worker > 100.0)
   OR (p95_latency > 50ms) OR (pending_jobs > 1000)
THEN → scale UP
```

#### Scale-Down Configuration (AND Logic)

Scale-down triggers only when **ALL** conditions are met:

```cpp
struct scale_down_config {
    double utilization_threshold = 0.3;            // 30% worker utilization
    double queue_depth_threshold = 10.0;           // Jobs per worker
    std::chrono::seconds idle_duration{60};         // Worker idle time
};
```

The AND logic ensures the system only shrinks when there is **strong evidence** of underutilization:

```
IF (utilization < 0.3) AND (queue_depth_per_worker < 10.0)
   AND (idle_duration >= 60s)
THEN → scale DOWN
```

> **Rationale**: Asymmetric OR/AND logic follows a "scale up fast, scale down slow" strategy. This prevents premature scale-down during brief lulls, which is critical for production stability.

#### Scaling Behavior

```cpp
std::size_t scale_up_increment = 1;       // Workers added per scale-up event
std::size_t scale_down_increment = 1;     // Workers removed per scale-down event
double scale_up_factor = 1.5;             // Multiplicative factor (if enabled)
bool use_multiplicative_scaling = false;   // Additive (default) vs multiplicative
```

**Additive scaling** (default): Adds/removes a fixed number of workers per event.

**Multiplicative scaling**: Multiplies current worker count by `scale_up_factor` (e.g., 4 workers x 1.5 = 6 workers). Useful for burst workloads that need rapid expansion.

#### Cooldown Periods

```cpp
std::chrono::seconds scale_up_cooldown{30};    // Minimum 30s between scale-up events
std::chrono::seconds scale_down_cooldown{60};  // Minimum 60s between scale-down events
```

The asymmetric cooldowns (30s up, 60s down) reflect the design philosophy:
- **Fast scale-up**: Respond quickly to rising demand
- **Slow scale-down**: Avoid premature shrinking during workload fluctuations

#### Sampling Configuration

```cpp
std::chrono::milliseconds sample_interval{1000};  // Collect metrics every 1s
std::size_t samples_for_decision = 5;              // Require 5 samples before deciding
```

With defaults, a scaling decision requires at least 5 seconds of data (5 samples x 1s interval), smoothing out transient spikes.

#### Scaling Callback

```cpp
std::function<void(scaling_direction, scaling_reason, std::size_t, std::size_t)> scaling_callback;
// Parameters: direction, reason, from_count, to_count
```

Called on every scaling event for logging, metrics export, or alerting:

```cpp
policy.scaling_callback = [](scaling_direction dir, scaling_reason reason,
                             std::size_t from, std::size_t to) {
    std::cout << "Scaled " << (dir == scaling_direction::up ? "UP" : "DOWN")
              << ": " << from << " -> " << to << " workers" << std::endl;
};
```

#### Policy Validation

The `is_valid()` method checks:

| Condition | Rule |
|-----------|------|
| `min_workers` | Must be >= 1 |
| `max_workers` | Must be >= `min_workers` |
| `scale_up.utilization_threshold` | Must be in (0.0, 1.0] |
| `scale_down.utilization_threshold` | Must be in [0.0, 1.0) |
| Utilization gap | `scale_down.utilization_threshold` < `scale_up.utilization_threshold` |
| Increments | `scale_up_increment` and `scale_down_increment` must be >= 1 |
| Samples | `samples_for_decision` must be >= 1 |

---

## Autoscaler API

### Construction

```cpp
#include <kcenon/thread/scaling/autoscaler.h>

// Create with default policy (mode: disabled)
auto scaler = std::make_unique<autoscaler>(pool);

// Create with custom policy
autoscaling_policy policy{ .min_workers = 2, .max_workers = 16 };
auto scaler = std::make_unique<autoscaler>(pool, policy);
```

The autoscaler is **non-copyable and non-movable** to ensure the background thread and shared state remain valid.

### Lifecycle Control

| Method | Description |
|--------|-------------|
| `start()` | Starts the background monitor thread (only effective in `automatic` mode) |
| `stop()` | Stops the monitor thread; blocks until the thread exits |
| `is_active()` | Returns `true` if the monitor thread is running |

```cpp
scaler->start();
// ... autoscaler is now monitoring and scaling ...
scaler->stop();  // blocks until monitor thread completes
```

> **Note**: The destructor calls `stop()` automatically, ensuring clean shutdown.

### Manual Triggers

These methods work in **any** mode (`disabled`, `manual`, or `automatic`):

| Method | Returns | Description |
|--------|---------|-------------|
| `evaluate_now()` | `scaling_decision` | Evaluates current metrics and returns what decision **would** be made. Does **not** execute scaling. |
| `scale_to(n)` | `common::VoidResult` | Scales to exactly `n` workers (clamped to `[min, max]`) |
| `scale_up()` | `common::VoidResult` | Adds `scale_up_increment` workers |
| `scale_down()` | `common::VoidResult` | Removes `scale_down_increment` workers |

```cpp
// Inspect without acting
auto decision = scaler->evaluate_now();
if (decision.should_scale()) {
    std::cout << decision.explanation << std::endl;
}

// Manual override
auto result = scaler->scale_to(8);
if (result.is_err()) {
    // Handle error
}
```

### Configuration

| Method | Description |
|--------|-------------|
| `set_policy(policy)` | Updates the autoscaling policy (takes effect immediately) |
| `get_policy()` | Returns a const reference to the current policy |

```cpp
// Dynamic policy update
auto policy = scaler->get_policy();
// (copy)
autoscaling_policy new_policy = policy;
new_policy.max_workers = 32;
scaler->set_policy(new_policy);
```

### Metrics and Statistics

| Method | Returns | Description |
|--------|---------|-------------|
| `get_current_metrics()` | `scaling_metrics_sample` | Snapshot of current pool state |
| `get_metrics_history(count)` | `vector<scaling_metrics_sample>` | Recent metrics (up to `count`, default 60) |
| `get_stats()` | `autoscaling_stats` | Scaling event counters and timestamps |
| `reset_stats()` | `void` | Zeros all statistics |

---

## Pool Integration

### The Pool Policy Pattern

thread_system uses a **Strategy pattern** where `pool_policy` is the base interface:

```cpp
class pool_policy {
public:
    virtual auto on_enqueue(job& j) -> common::VoidResult = 0;
    virtual void on_job_start(job& j) = 0;
    virtual void on_job_complete(job& j, bool success,
                                 const std::exception* error = nullptr) = 0;
    [[nodiscard]] virtual auto get_name() const -> std::string = 0;
    [[nodiscard]] virtual auto is_enabled() const -> bool;
    virtual void set_enabled(bool enabled);
};
```

#### Lifecycle Hooks

| Hook | Called From | Thread | Can Reject? |
|------|------------|--------|:-----------:|
| `on_enqueue(job&)` | Enqueueing thread | Caller's thread | Yes (return error) |
| `on_job_start(job&)` | Worker thread | Worker's thread | No |
| `on_job_complete(job&, bool, exception*)` | Worker thread | Worker's thread | No |

### `autoscaling_pool_policy`

This class adapts `autoscaler` as a `pool_policy`, enabling non-intrusive integration with any `thread_pool`:

```cpp
#include <kcenon/thread/pool_policies/autoscaling_pool_policy.h>

// Construction option 1: New autoscaler
autoscaling_policy config;
config.min_workers = 2;
config.max_workers = 16;
config.scaling_mode = autoscaling_policy::mode::automatic;

auto as_policy = std::make_unique<autoscaling_pool_policy>(pool, config);

// Construction option 2: Shared autoscaler
auto shared_scaler = std::make_shared<autoscaler>(pool, config);
auto as_policy = std::make_unique<autoscaling_pool_policy>(shared_scaler);
```

#### Adding to a Thread Pool

```cpp
pool->add_policy(std::move(as_policy));

// Find and control later
auto* policy = pool->find_policy<autoscaling_pool_policy>("autoscaling_pool_policy");
if (policy) {
    policy->start();   // Start monitoring
    // ...
    policy->stop();    // Stop monitoring
}

// Remove when no longer needed
pool->remove_policy("autoscaling_pool_policy");
```

#### Policy Behavior

The `autoscaling_pool_policy` **never rejects jobs** via `on_enqueue()` — it always returns `common::ok()`. Instead of rejecting work, it adjusts the worker count to handle the load. The `on_job_start()` and `on_job_complete()` hooks record metrics used by the autoscaler for decision-making.

#### Additional Methods

| Method | Description |
|--------|-------------|
| `start()` / `stop()` | Control the monitor thread |
| `is_active()` | Check if monitoring is running |
| `get_autoscaler()` | Access the underlying `shared_ptr<autoscaler>` |
| `get_stats()` | Get scaling statistics |
| `set_policy(config)` / `get_policy()` | Update/read configuration |
| `evaluate_now()` | Manual scaling evaluation |
| `scale_to(n)` | Manual scaling to target count |
| `is_enabled()` / `set_enabled(bool)` | Enable/disable the policy (stops autoscaler when disabled) |

### Composing Multiple Policies

Multiple policies can coexist on the same thread pool:

```cpp
// Autoscaling + circuit breaker
pool->add_policy(std::make_unique<autoscaling_pool_policy>(*pool, scaling_config));
pool->add_policy(std::make_unique<circuit_breaker_policy>(cb_config));

// Both policies are invoked for every job lifecycle event
```

---

## Metrics and Observability

### Metrics Sample (`scaling_metrics_sample`)

Each metrics sample captures a snapshot of the pool at a specific moment:

| Field | Type | Description |
|-------|------|-------------|
| `timestamp` | `steady_clock::time_point` | When the sample was collected |
| `worker_count` | `size_t` | Total workers in the pool |
| `active_workers` | `size_t` | Workers currently processing jobs |
| `queue_depth` | `size_t` | Jobs waiting in the queue |
| `utilization` | `double` (0.0–1.0) | `active_workers / worker_count` |
| `queue_depth_per_worker` | `double` | `queue_depth / worker_count` |
| `p95_latency_ms` | `double` | 95th percentile job latency |
| `jobs_completed` | `uint64_t` | Jobs completed since last sample |
| `jobs_submitted` | `uint64_t` | Jobs submitted since last sample |
| `throughput_per_second` | `double` | Computed job throughput rate |

### Scaling Decision (`scaling_decision`)

Returned by `evaluate_now()` and used internally:

| Field | Type | Description |
|-------|------|-------------|
| `direction` | `scaling_direction` | `none`, `up`, or `down` |
| `reason` | `scaling_reason` | What triggered the decision |
| `target_workers` | `size_t` | Desired worker count after scaling |
| `explanation` | `string` | Human-readable decision rationale |

```cpp
auto decision = scaler->evaluate_now();
if (decision.should_scale()) {
    std::cout << "Action: " << (decision.direction == scaling_direction::up ? "UP" : "DOWN")
              << " to " << decision.target_workers << " workers\n"
              << "Reason: " << decision.explanation << std::endl;
}
```

### Scaling Reasons (`scaling_reason`)

| Value | Trigger |
|-------|---------|
| `queue_depth` | Queue depth per worker exceeded threshold |
| `worker_utilization` | Worker utilization exceeded threshold |
| `latency` | P95 latency exceeded threshold |
| `manual` | Explicit API call (`scale_up()`, `scale_down()`, `scale_to()`) |
| `scheduled` | Scheduled scaling event |

### Autoscaling Statistics (`autoscaling_stats`)

Cumulative statistics since last `reset_stats()`:

| Field | Type | Description |
|-------|------|-------------|
| `scale_up_count` | `size_t` | Number of scale-up events |
| `scale_down_count` | `size_t` | Number of scale-down events |
| `decisions_evaluated` | `size_t` | Total decisions evaluated |
| `last_scale_up` | `steady_clock::time_point` | Timestamp of last scale-up |
| `last_scale_down` | `steady_clock::time_point` | Timestamp of last scale-down |
| `peak_workers` | `size_t` | Maximum worker count observed |
| `min_workers` | `size_t` | Minimum worker count observed |

### Monitoring Example

```cpp
#include <kcenon/thread/scaling/autoscaler.h>
#include <kcenon/thread/scaling/autoscaling_policy.h>
#include <kcenon/thread/core/thread_pool.h>

// Setup with callback for real-time monitoring
autoscaling_policy policy;
policy.min_workers = 2;
policy.max_workers = 16;
policy.scaling_mode = autoscaling_policy::mode::automatic;

policy.scaling_callback = [](scaling_direction dir, scaling_reason reason,
                             std::size_t from, std::size_t to) {
    // Log to your monitoring system
    log_scaling_event(dir, reason, from, to);
};

auto scaler = std::make_shared<autoscaler>(pool, policy);
scaler->start();

// Periodic metrics export
auto metrics = scaler->get_current_metrics();
auto stats = scaler->get_stats();
auto history = scaler->get_metrics_history(60);  // Last 60 samples
```

---

## Configuration Examples

### Basic Automatic Scaling

```cpp
#include <kcenon/thread/scaling/autoscaler.h>
#include <kcenon/thread/scaling/autoscaling_policy.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/thread_worker.h>
#include <kcenon/thread/core/job_builder.h>

// 1. Create and start the pool
auto pool = std::make_shared<thread_pool>("WorkerPool");

for (int i = 0; i < 4; ++i) {
    pool->enqueue(std::make_unique<thread_worker>(true));
}
pool->start();

// 2. Configure autoscaling policy
autoscaling_policy policy;
policy.min_workers = 2;
policy.max_workers = 16;
policy.scale_up.utilization_threshold = 0.8;
policy.scale_up.queue_depth_threshold = 50.0;
policy.scale_down.utilization_threshold = 0.3;
policy.scale_down.queue_depth_threshold = 5.0;
policy.scaling_mode = autoscaling_policy::mode::automatic;

// 3. Create and start the autoscaler
auto scaler = std::make_shared<autoscaler>(*pool, policy);
scaler->start();

// 4. Submit jobs (workers scale automatically)
for (int i = 0; i < 1000; ++i) {
    auto j = job_builder()
        .name("task_" + std::to_string(i))
        .work([]() -> common::VoidResult {
            // Simulate work
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return common::ok();
        })
        .build();
    pool->enqueue(std::move(j));
}

// 5. Cleanup
scaler->stop();
pool->stop();
```

### Pool Policy Integration

```cpp
#include <kcenon/thread/pool_policies/autoscaling_pool_policy.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/thread_worker.h>

auto pool = std::make_shared<thread_pool>("AutoscalePool");

for (int i = 0; i < 2; ++i) {
    pool->enqueue(std::make_unique<thread_worker>(true));
}

// Add autoscaling as a pool policy
autoscaling_policy config;
config.min_workers = 1;
config.max_workers = 8;
config.scaling_mode = autoscaling_policy::mode::automatic;

pool->add_policy(std::make_unique<autoscaling_pool_policy>(*pool, config));
pool->start();

// Find the policy later to start monitoring
auto* as_policy = pool->find_policy<autoscaling_pool_policy>("autoscaling_pool_policy");
if (as_policy) {
    as_policy->start();
}

// ... submit jobs ...

// Cleanup
if (as_policy) {
    as_policy->stop();
}
pool->stop();
```

### Manual Scaling with Evaluation

```cpp
autoscaling_policy policy;
policy.min_workers = 2;
policy.max_workers = 16;
policy.scaling_mode = autoscaling_policy::mode::manual;

auto scaler = std::make_shared<autoscaler>(*pool, policy);

// Evaluate current state without acting
auto decision = scaler->evaluate_now();
if (decision.should_scale()) {
    std::cout << "Recommended: " << decision.explanation << std::endl;

    // Operator decides whether to apply
    auto result = scaler->scale_to(decision.target_workers);
    if (result.is_err()) {
        std::cerr << "Scaling failed" << std::endl;
    }
}

// Direct manual scaling
scaler->scale_up();    // Add scale_up_increment workers
scaler->scale_down();  // Remove scale_down_increment workers
scaler->scale_to(8);   // Scale to exactly 8 workers (clamped to [min, max])
```

### Burst Workload Configuration

For workloads with sudden spikes, use multiplicative scaling with aggressive thresholds:

```cpp
autoscaling_policy burst_policy;
burst_policy.min_workers = 2;
burst_policy.max_workers = 32;
burst_policy.scaling_mode = autoscaling_policy::mode::automatic;

// Aggressive scale-up
burst_policy.scale_up.utilization_threshold = 0.6;
burst_policy.scale_up.queue_depth_threshold = 20.0;
burst_policy.scale_up.latency_threshold_ms = 30.0;
burst_policy.use_multiplicative_scaling = true;
burst_policy.scale_up_factor = 2.0;  // Double workers on each scale-up

// Conservative scale-down
burst_policy.scale_down.utilization_threshold = 0.1;
burst_policy.scale_down.queue_depth_threshold = 2.0;
burst_policy.scale_down.idle_duration = std::chrono::seconds{120};

// Short cooldowns for fast response
burst_policy.scale_up_cooldown = std::chrono::seconds{10};
burst_policy.scale_down_cooldown = std::chrono::seconds{120};

// Fast sampling
burst_policy.sample_interval = std::chrono::milliseconds{500};
burst_policy.samples_for_decision = 3;
```

### Shared Autoscaler Across Components

```cpp
// Create a shared autoscaler
auto shared_scaler = std::make_shared<autoscaler>(*pool, policy);
shared_scaler->start();

// Multiple components can reference the same autoscaler
auto policy1 = std::make_unique<autoscaling_pool_policy>(shared_scaler);
auto policy2 = std::make_unique<autoscaling_pool_policy>(shared_scaler);

// Access from any policy
auto scaler = policy1->get_autoscaler();
auto metrics = scaler->get_current_metrics();
```

---

## Best Practices

### 1. Choosing Thresholds

| Workload Type | Scale-Up Utilization | Scale-Down Utilization | Queue Depth |
|---------------|:---:|:---:|:---:|
| **Latency-sensitive** (web servers) | 0.6 | 0.2 | 10–20 |
| **Throughput-oriented** (batch processing) | 0.85 | 0.3 | 50–100 |
| **Mixed** (general purpose) | 0.8 (default) | 0.3 (default) | 100 (default) |
| **Burst** (event-driven) | 0.5 | 0.1 | 5–10 |

> **Rule of thumb**: Always maintain a gap of at least 0.3 between scale-up and scale-down utilization thresholds to avoid oscillation.

### 2. Cooldown Period Recommendations

| Environment | Scale-Up Cooldown | Scale-Down Cooldown |
|-------------|:-:|:-:|
| **Development / Testing** | 5–10s | 10–15s |
| **Staging** | 15–30s | 30–60s |
| **Production** | 30s (default) | 60–120s |

### 3. Sample Aggregation

- Use `samples_for_decision >= 3` to avoid reacting to transient spikes
- With `sample_interval = 1000ms` and `samples_for_decision = 5`, a scaling decision requires at least 5 seconds of data
- Reduce both for faster response (at the cost of potential oscillation)

### 4. Multiplicative vs Additive Scaling

| Strategy | When to Use |
|----------|-------------|
| **Additive** (default) | Steady, predictable workloads. Gradual scaling provides more control. |
| **Multiplicative** | Burst workloads where demand can increase 10x in seconds. `scale_up_factor = 1.5–2.0` is typical. |

### 5. Production Monitoring

- Always set a `scaling_callback` to log events to your monitoring system
- Periodically export `get_stats()` (scale counts, peak/min workers)
- Use `get_metrics_history()` to correlate scaling events with workload patterns
- Monitor the gap between `scale_up_count` and `scale_down_count` — a large gap indicates the pool is consistently growing (check if `max_workers` is appropriate)

### 6. Testing Autoscaling

- Use `manual` mode in unit tests for deterministic control
- Set short cooldowns (1s) and sample intervals (100ms) in tests
- Verify `get_stats()` counts match expected scaling events
- Test with `scale_to()` to validate worker count clamping to `[min, max]`

---

## Anti-Patterns

### 1. Overlapping Utilization Thresholds

```cpp
// WRONG: scale_down >= scale_up causes oscillation
policy.scale_up.utilization_threshold = 0.5;
policy.scale_down.utilization_threshold = 0.5;  // is_valid() returns false
```

The `is_valid()` method catches this: `scale_down.utilization_threshold` must be **strictly less than** `scale_up.utilization_threshold`.

### 2. Zero Min Workers

```cpp
// WRONG: pool cannot function with 0 workers
policy.min_workers = 0;  // is_valid() returns false
```

### 3. Too Aggressive Sampling

```cpp
// WRONG: Reacts to single-sample noise
policy.sample_interval = std::chrono::milliseconds{50};
policy.samples_for_decision = 1;
```

This makes the autoscaler react to every transient spike. Use at least `samples_for_decision = 3` in production.

### 4. Forgetting to Start the Autoscaler

```cpp
// WRONG: autoscaler created but never started
auto scaler = std::make_shared<autoscaler>(*pool, policy);
// scaler->start();  // Missing! No automatic scaling happens
```

When using `autoscaling_pool_policy`, remember that adding the policy to the pool does **not** start the monitor thread automatically:

```cpp
pool->add_policy(std::make_unique<autoscaling_pool_policy>(*pool, config));

// Must explicitly start after pool is running
auto* as_policy = pool->find_policy<autoscaling_pool_policy>("autoscaling_pool_policy");
as_policy->start();
```

### 5. Ignoring `is_valid()` Return Value

```cpp
// WRONG: Using an invalid policy
autoscaling_policy policy;
policy.min_workers = 10;
policy.max_workers = 5;  // max < min

// Always validate before use
if (!policy.is_valid()) {
    // Handle configuration error
}
```

### 6. Mixing Disabled Mode with Automatic Expectations

```cpp
// WRONG: Mode is disabled but expecting automatic scaling
autoscaling_policy policy;
// scaling_mode defaults to mode::disabled
auto scaler = std::make_shared<autoscaler>(*pool, policy);
scaler->start();  // Monitor thread runs but mode is disabled — no scaling decisions
```

---

## Troubleshooting

### Autoscaler Not Scaling

1. **Check the mode**: Ensure `scaling_mode` is `automatic` (default is `disabled`)
2. **Verify `start()` was called**: The monitor thread must be running
3. **Check cooldown**: Scaling may be blocked by a recent event; use `get_stats()` to check `last_scale_up` / `last_scale_down`
4. **Verify sample count**: The autoscaler needs `samples_for_decision` samples before making its first decision
5. **Check bounds**: Verify current worker count is not already at `min_workers` or `max_workers`

### Excessive Oscillation (Scaling Up Then Down Repeatedly)

1. **Increase cooldowns**: Set `scale_down_cooldown` to at least 2x `scale_up_cooldown`
2. **Widen the threshold gap**: Ensure at least 0.3 difference between scale-up and scale-down utilization thresholds
3. **Increase `samples_for_decision`**: More samples smooth out transient load changes
4. **Check for bursty workload**: Consider multiplicative scaling for rapid initial response

### Scale-Down Blocking

Scale-down may appear to hang if workers are busy. By design, `remove_workers()` waits for workers to become idle before removing them. This is graceful behavior, not a bug.

### Memory Concerns

Metrics history is stored in a `std::deque` and can grow over time. Call `get_metrics_history(count)` periodically to read and the autoscaler internally manages the buffer size relative to the sample window.

---

## Related Documentation

- [Architecture Overview](ARCHITECTURE.md) — System structure and design principles
- [Policy Queue Guide](POLICY_QUEUE_GUIDE.md) — Queue-level policy composition
- [API Reference](API_REFERENCE.md) — Complete API documentation
- [Features](FEATURES.md) — Feature overview including autoscaling
- [Benchmarks](BENCHMARKS.md) — Performance measurements
