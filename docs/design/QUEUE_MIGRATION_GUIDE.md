# Queue Migration Guide

**Version**: 0.1.0.0
**Issue**: #439 (Phase 1.3), #448 (Phase 1.3.1)
**Status**: Active
**Last Updated**: 2026-01-11

---

## Overview

This guide provides step-by-step instructions for migrating from legacy queue implementations to the new policy-based `policy_queue` template. The new design offers:

- **Compile-time configuration**: Zero runtime polymorphism overhead
- **Type safety**: Policy combinations validated at compile time
- **Flexibility**: Mix and match policies without creating new classes
- **Consistency**: Single queue class supports all scenarios

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Migration Mapping](#migration-mapping)
3. [Detailed Migration Examples](#detailed-migration-examples)
4. [Policy Reference](#policy-reference)
5. [Common Patterns](#common-patterns)
6. [Troubleshooting](#troubleshooting)

---

## Quick Start

### Include the New Headers

```cpp
// Old way - multiple headers
#include <kcenon/thread/core/job_queue.h>
#include <kcenon/thread/core/backpressure_job_queue.h>
#include <kcenon/thread/lockfree/lockfree_job_queue.h>

// New way - single header with all policies
#include <kcenon/thread/policies/policies.h>
```

### Basic Queue Creation

```cpp
// Old way
auto queue = std::make_shared<job_queue>();

// New way
using namespace kcenon::thread;
auto queue = std::make_unique<standard_queue>();  // Same behavior as job_queue
```

---

## Migration Mapping

### Queue Type Correspondence

| Legacy Queue | New Policy Queue | Type Alias |
|-------------|------------------|------------|
| `job_queue` | `policy_queue<mutex_sync_policy, unbounded_policy>` | `standard_queue` |
| `bounded_job_queue(N)` | `policy_queue<mutex_sync_policy, bounded_policy>` | `bounded_rejecting_queue<N>` |
| `backpressure_job_queue` | `policy_queue<mutex_sync_policy, bounded_policy, overflow_block_policy>` | `bounded_blocking_queue<N>` |
| `lockfree_job_queue` | `policy_queue<lockfree_sync_policy, unbounded_policy>` | `policy_lockfree_queue` |
| `adaptive_job_queue` | `policy_queue<adaptive_sync_policy, unbounded_policy>` | (custom) |
| `typed_job_queue_t<T>` | `standard_queue` with `enqueue<T>()` | N/A |

### Feature Comparison

| Feature | Legacy Queues | Policy Queue |
|---------|---------------|--------------|
| Thread-safe | ✓ | ✓ |
| Type-safe enqueue | Limited | `enqueue<T>()` template |
| Blocking dequeue | job_queue only | `mutex_sync_policy` |
| Lock-free ops | lockfree_job_queue only | `lockfree_sync_policy` |
| Bounded capacity | backpressure_job_queue | `bounded_policy` |
| Overflow handling | backpressure_job_queue | Any `overflow_*_policy` |
| Runtime mode switch | adaptive_job_queue | `adaptive_sync_policy` |

---

## Detailed Migration Examples

### 1. Migrating from `job_queue`

**Before (Legacy):**
```cpp
#include <kcenon/thread/core/job_queue.h>

auto queue = std::make_shared<job_queue>();
queue->enqueue(std::make_unique<my_job>());
auto result = queue->dequeue();  // Blocking
```

**After (Policy-based):**
```cpp
#include <kcenon/thread/policies/policies.h>

using namespace kcenon::thread;

// Using type alias (recommended)
auto queue = std::make_unique<standard_queue>();

// Or explicit policy specification
auto queue = std::make_unique<policy_queue<
    policies::mutex_sync_policy,
    policies::unbounded_policy,
    policies::overflow_reject_policy
>>();

queue->enqueue(std::make_unique<my_job>());
auto result = queue->dequeue();  // Blocking (same behavior)
```

**Key Changes:**
- Use `std::make_unique` instead of `std::make_shared` (recommended but not required)
- Same API: `enqueue()`, `dequeue()`, `try_dequeue()`, `empty()`, `size()`

---

### 2. Migrating from Bounded Queue with Backpressure

**Before (Legacy):**
```cpp
#include <kcenon/thread/core/backpressure_job_queue.h>

backpressure_config config;
config.max_queue_size = 1000;
config.overflow_policy = overflow_policy::block;

auto queue = std::make_shared<backpressure_job_queue>(1000, config);
```

**After (Policy-based):**
```cpp
#include <kcenon/thread/policies/policies.h>

using namespace kcenon::thread;
using namespace kcenon::thread::policies;

// Bounded queue that blocks on overflow
auto queue = std::make_unique<policy_queue<
    mutex_sync_policy,
    bounded_policy,
    overflow_block_policy
>>(bounded_policy(1000));  // Pass bound policy with size

// Alternative: Use type alias with runtime size
bounded_policy bound(1000);
auto queue = std::make_unique<policy_queue<
    mutex_sync_policy,
    bounded_policy,
    overflow_block_policy
>>(std::move(bound));
```

**For Drop-Oldest Behavior (Ring Buffer):**
```cpp
// Ring buffer: drops oldest when full
auto queue = std::make_unique<policy_queue<
    mutex_sync_policy,
    bounded_policy,
    overflow_drop_oldest_policy
>>(bounded_policy(1000));
```

---

### 3. Migrating from `lockfree_job_queue`

**Before (Legacy):**
```cpp
#include <kcenon/thread/lockfree/lockfree_job_queue.h>

auto queue = std::make_shared<detail::lockfree_job_queue>();
queue->enqueue(std::make_unique<my_job>());
auto result = queue->dequeue();  // Non-blocking, returns error if empty
```

**After (Policy-based):**
```cpp
#include <kcenon/thread/policies/policies.h>

using namespace kcenon::thread;

auto queue = std::make_unique<policy_lockfree_queue>();

queue->enqueue(std::make_unique<my_job>());
auto result = queue->dequeue();  // Non-blocking (same behavior)
```

**Important Notes:**
- Lock-free queue does NOT support blocking dequeue
- `size()` returns approximate value
- `empty()` check is approximate (may have false negatives under contention)

---

### 4. Migrating from `adaptive_job_queue`

**Before (Legacy):**
```cpp
#include <kcenon/thread/queue/adaptive_job_queue.h>

auto queue = std::make_shared<adaptive_job_queue>();
queue->switch_mode(queue_mode::lock_free);  // Explicit mode switch

// Temporary accuracy mode
{
    auto guard = queue->accuracy_guard();
    auto size = queue->size();  // Exact size in mutex mode
}
```

**After (Policy-based):**
```cpp
#include <kcenon/thread/policies/policies.h>

using namespace kcenon::thread;
using namespace kcenon::thread::policies;

auto queue = std::make_unique<policy_queue<
    adaptive_sync_policy,
    unbounded_policy,
    overflow_reject_policy
>>();

// Mode switching through sync policy access
queue->sync_policy().switch_mode(adaptive_sync_policy::mode::lock_free);

// Check current mode
auto current = queue->sync_policy().current_mode();
```

---

### 5. Migrating from `typed_job_queue_t<T>`

**Before (Legacy, Deprecated):**
```cpp
#include <kcenon/thread/impl/typed_pool/typed_job_queue.h>

using my_types = job_types<job_a, job_b, job_c>;
auto queue = std::make_shared<typed_job_queue_t<my_types>>();

queue->enqueue<job_a>(std::make_unique<job_a>());
```

**After (Policy-based):**
```cpp
#include <kcenon/thread/policies/policies.h>

using namespace kcenon::thread;

auto queue = std::make_unique<standard_queue>();

// Type-safe enqueue works with any job subclass
queue->enqueue(std::make_unique<job_a>());  // Type deduced
queue->enqueue(std::make_unique<job_b>());

// Explicit type specification (optional)
queue->enqueue<job_a>(std::make_unique<job_a>());
```

---

## Policy Reference

### Sync Policies

| Policy | Description | Capabilities |
|--------|-------------|--------------|
| `mutex_sync_policy` | Mutex + condition variable | Blocking wait, exact size, batch ops |
| `lockfree_sync_policy` | Michael-Scott lock-free queue | No blocking, approximate size |
| `adaptive_sync_policy` | Mode switching between mutex/lockfree | Runtime configurable |

### Bound Policies

| Policy | Description | Use Case |
|--------|-------------|----------|
| `unbounded_policy` | No size limit | Default, memory is the only limit |
| `bounded_policy` | Fixed maximum size | Backpressure, resource control |
| `dynamic_bounded_policy` | Runtime-adjustable limit | Elastic capacity |

### Overflow Policies

| Policy | Description | Behavior on Full |
|--------|-------------|-----------------|
| `overflow_reject_policy` | Reject new item | Returns error, item dropped |
| `overflow_block_policy` | Block until space | Waits for dequeue |
| `overflow_drop_oldest_policy` | Remove oldest | Ring buffer behavior |
| `overflow_drop_newest_policy` | Drop silently | Returns success, item dropped |
| `overflow_timeout_policy` | Block with timeout | Waits up to specified duration |

---

## Common Patterns

### Pattern 1: Producer-Consumer with Bounded Buffer

```cpp
using namespace kcenon::thread;
using namespace kcenon::thread::policies;

// Bounded buffer that blocks producers when full
using bounded_buffer = policy_queue<
    mutex_sync_policy,
    bounded_policy,
    overflow_block_policy
>;

auto buffer = std::make_unique<bounded_buffer>(bounded_policy(100));

// Producer thread
void producer() {
    auto job = std::make_unique<my_job>();
    auto result = buffer->enqueue(std::move(job));  // Blocks if full
    if (result.is_err()) {
        // Handle error (e.g., queue stopped)
    }
}

// Consumer thread
void consumer() {
    auto result = buffer->dequeue();  // Blocks if empty
    if (result.is_ok()) {
        result.value()->execute();
    }
}
```

### Pattern 2: High-Throughput Lock-Free Pipeline

```cpp
using namespace kcenon::thread;

auto stage1 = std::make_unique<policy_lockfree_queue>();
auto stage2 = std::make_unique<policy_lockfree_queue>();

// Pipeline processing without locks
void pipeline_worker() {
    while (!stage1->is_stopped()) {
        auto result = stage1->try_dequeue();
        if (result.is_ok()) {
            process(result.value());
            stage2->enqueue(std::move(result.value()));
        }
    }
}
```

### Pattern 3: Metrics Collection with Ring Buffer

```cpp
using namespace kcenon::thread;
using namespace kcenon::thread::policies;

// Keep only the last 1000 metrics, drop oldest when full
using metrics_buffer = policy_queue<
    mutex_sync_policy,
    bounded_policy,
    overflow_drop_oldest_policy
>;

auto metrics = std::make_unique<metrics_buffer>(bounded_policy(1000));

void record_metric(std::unique_ptr<metric_job> m) {
    // Always succeeds - drops oldest if necessary
    metrics->enqueue(std::move(m));
}
```

---

## Troubleshooting

### Compilation Error: "No matching constructor"

**Problem:** Bounded policy requires size parameter.

```cpp
// Error: bounded_policy needs size
auto queue = std::make_unique<bounded_blocking_queue<1000>>();
```

**Solution:** Pass bound policy with size to constructor.

```cpp
auto queue = std::make_unique<policy_queue<
    mutex_sync_policy,
    bounded_policy,
    overflow_block_policy
>>(bounded_policy(1000));
```

### Blocking Dequeue with Lock-Free Policy

**Problem:** Lock-free policy doesn't support blocking.

```cpp
// This dequeue() returns immediately with error if empty
policy_lockfree_queue queue;
auto result = queue.dequeue();  // Non-blocking!
```

**Solution:** Use polling or switch to mutex policy.

```cpp
// Option 1: Polling with try_dequeue
while (true) {
    auto result = queue.try_dequeue();
    if (result.is_ok()) {
        process(result.value());
        break;
    }
    std::this_thread::yield();
}

// Option 2: Use adaptive policy with mode switching
auto queue = policy_queue<adaptive_sync_policy, ...>();
queue.sync_policy().switch_mode(adaptive_sync_policy::mode::mutex);
auto result = queue.dequeue();  // Now blocks
```

### Size/Empty Check Accuracy

**Problem:** Lock-free queue size is approximate.

```cpp
policy_lockfree_queue queue;
if (queue.empty()) {  // May be inaccurate under contention!
    // ...
}
```

**Solution:** Check capabilities or use mutex policy for exact size.

```cpp
auto caps = queue.get_capabilities();
if (caps.exact_size) {
    auto size = queue.size();  // Exact
} else {
    // size() is approximate, use try_dequeue() instead
}
```

---

## Version History

- **1.0.0** (2026-01-11): Initial migration guide for Phase 1.3

---

## References

- [QUEUE_POLICY_DESIGN.md](./QUEUE_POLICY_DESIGN.md) - Policy-based design specification
- Issue #438: Phase 1.2 Implementation
- Issue #439: Phase 1.3 Migration
