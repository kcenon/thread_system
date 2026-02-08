# Policy Queue Combinations Guide

> **Language:** **English** | [한국어](POLICY_QUEUE_GUIDE.kr.md)

## Table of Contents

1. [Overview](#overview)
2. [Policy System Architecture](#policy-system-architecture)
3. [SyncPolicy Options](#syncpolicy-options)
4. [BoundPolicy Options](#boundpolicy-options)
5. [OverflowPolicy Options](#overflowpolicy-options)
6. [Compatibility Matrix](#compatibility-matrix)
7. [Recommended Combinations](#recommended-combinations)
8. [Performance Characteristics](#performance-characteristics)
9. [Code Examples](#code-examples)
10. [Anti-Patterns](#anti-patterns)

---

## Overview

thread_system provides a **policy-based queue** template (`policy_queue`) that composes three orthogonal policy dimensions at compile time:

```
policy_queue<SyncPolicy, BoundPolicy, OverflowPolicy>
```

This design follows the **Policy-Based Design** pattern (Alexandrescu, *Modern C++ Design*), enabling zero-overhead abstractions through template specialization rather than virtual dispatch.

### Key Benefits

- **Zero runtime overhead**: Policies resolved at compile time — no vtable indirection
- **Type-safe combinations**: Invalid configurations produce compile-time errors
- **Orthogonal composition**: Each policy dimension is independent and interchangeable

### Source Files

| File | Purpose |
|------|---------|
| `include/kcenon/thread/policies/sync_policies.h` | Synchronization policies |
| `include/kcenon/thread/policies/bound_policies.h` | Capacity/bounding policies |
| `include/kcenon/thread/policies/overflow_policies.h` | Overflow handling policies |
| `include/kcenon/thread/policies/policy_queue.h` | Queue template + type aliases |
| `include/kcenon/thread/policies/policies.h` | Convenience header (includes all) |

---

## Policy System Architecture

### Template Composition

```cpp
template<
    typename SyncPolicy,
    typename BoundPolicy = policies::unbounded_policy,
    typename OverflowPolicy = policies::overflow_reject_policy
>
class policy_queue : public scheduler_interface,
                     public queue_capabilities_interface;
```

- **SyncPolicy** controls how concurrent access is synchronized (mutex, lock-free, adaptive)
- **BoundPolicy** controls whether the queue has a size limit (unbounded, fixed, dynamic)
- **OverflowPolicy** controls what happens when a bounded queue is full (reject, block, drop)

### Policy Tags

Each policy declares a tag type for compile-time identification:

```cpp
struct sync_policy_tag {};      // SyncPolicy types
struct bound_policy_tag {};     // BoundPolicy types
struct overflow_policy_tag {};  // OverflowPolicy types
```

### Runtime Capabilities

The `queue_capabilities` struct provides runtime-queryable metadata:

```cpp
struct queue_capabilities {
    bool exact_size;             // size() returns exact value
    bool atomic_empty_check;     // empty() is atomically consistent
    bool lock_free;              // Uses lock-free algorithms
    bool wait_free;              // Uses wait-free algorithms
    bool supports_batch;         // Batch enqueue/dequeue
    bool supports_blocking_wait; // Blocking dequeue with wait
    bool supports_stop;          // stop() method available
};
```

---

## SyncPolicy Options

### `mutex_sync_policy`

| Property | Value |
|----------|-------|
| **Mechanism** | `std::mutex` + `std::condition_variable` |
| **Exact Size** | Yes |
| **Atomic Empty Check** | Yes |
| **Lock-Free** | No |
| **Blocking Wait** | Yes |
| **Batch Support** | Yes |

**How It Works**: Wraps a `std::deque<std::unique_ptr<job>>` with a mutex. Enqueue/dequeue acquire the lock, and blocking dequeue uses a condition variable to wait for items.

**Best For**: General-purpose queues where correctness and blocking support are more important than extreme throughput under high contention.

### `lockfree_sync_policy`

| Property | Value |
|----------|-------|
| **Mechanism** | Michael-Scott lock-free linked list (CAS-based) |
| **Exact Size** | No (approximate) |
| **Atomic Empty Check** | No (approximate) |
| **Lock-Free** | Yes |
| **Blocking Wait** | No |
| **Batch Support** | No |

**How It Works**: Implements a linked-list queue using `compare_exchange_weak` for enqueue (tail CAS) and dequeue (head CAS). A dummy sentinel node separates head and tail to avoid contention. Uses an `approximate_size_` atomic counter.

**Best For**: High-throughput scenarios with many concurrent producers/consumers where contention on a mutex would be a bottleneck.

**Limitations**:
- `size()` and `empty()` are approximate — not suitable for precise capacity decisions
- No blocking dequeue — consumers must poll or use external notification
- No batch operations

### `adaptive_sync_policy`

| Property | Value |
|----------|-------|
| **Mechanism** | Runtime-switchable between mutex and lock-free |
| **Exact Size** | Depends on current mode |
| **Lock-Free** | Depends on current mode |
| **Blocking Wait** | Only in mutex mode |
| **Mode Switching** | `switch_mode(mode::mutex)` / `switch_mode(mode::lock_free)` |

**How It Works**: Internally holds both a `mutex_sync_policy` and a `lockfree_sync_policy`. Routes operations to the active policy based on an atomic mode flag. Mode switching does **not** migrate data between policies.

**Best For**: Systems that need to change queue behavior at runtime based on workload characteristics (e.g., switch to lock-free during high-contention bursts).

**Limitations**:
- Higher memory footprint (maintains two internal queues)
- Mode switching does not transfer queued items — caller must drain before switching

---

## BoundPolicy Options

### `unbounded_policy`

| Property | Value |
|----------|-------|
| **Bounded** | No |
| **Max Size** | `std::nullopt` (unlimited) |
| **Memory** | Grows with demand (limited by system memory) |
| **`is_full()`** | Always returns `false` |

**When to Use**: When the consumer is faster than the producer on average, or when queue growth is naturally bounded by the application logic.

### `bounded_policy`

| Property | Value |
|----------|-------|
| **Bounded** | Yes |
| **Max Size** | Fixed at construction (mutable via `set_max_size()`) |
| **Memory** | Bounded by `max_size * sizeof(job_ptr)` |
| **`is_full(n)`** | Returns `true` when `n >= max_size` |

**When to Use**: When memory must be bounded or when backpressure/overflow behavior is needed.

```cpp
bounded_policy bound(1024);  // Max 1024 items
bound.set_max_size(2048);    // Resize at runtime
```

### `dynamic_bounded_policy`

| Property | Value |
|----------|-------|
| **Bounded** | Configurable at runtime |
| **Max Size** | Optional — can switch between bounded and unbounded |
| **`set_unbounded()`** | Remove size limit |
| **`set_max_size(n)`** | Set new limit |

**When to Use**: When the capacity limit needs to change dynamically based on system conditions (e.g., autoscaling, memory pressure).

```cpp
dynamic_bounded_policy bound(1000);
bound.set_unbounded();    // Remove limit during burst
bound.set_max_size(500);  // Tighten limit under memory pressure
```

---

## OverflowPolicy Options

Overflow policies are only invoked when a bounded queue is full. With `unbounded_policy`, the overflow policy is never triggered.

### `overflow_reject_policy`

| Property | Value |
|----------|-------|
| **Behavior** | Returns error immediately |
| **Data Loss** | New item is dropped (caller receives error) |
| **Blocks** | No |
| **Return** | `VoidResult` with error code `-120` |

**Use Case**: Fast-fail load shedding where the caller handles overflow explicitly.

### `overflow_block_policy`

| Property | Value |
|----------|-------|
| **Behavior** | Waits until space is available |
| **Data Loss** | None |
| **Blocks** | Yes (indefinitely) |
| **Requires** | `supports_blocking_wait = true` (mutex sync only) |

**Use Case**: Producer-consumer with backpressure — producers slow down when the consumer cannot keep up.

> **Warning**: Incompatible with `lockfree_sync_policy` (no condition variable for blocking).

### `overflow_drop_oldest_policy`

| Property | Value |
|----------|-------|
| **Behavior** | Dequeues and discards the oldest item, then enqueues the new item |
| **Data Loss** | Oldest item is lost |
| **Blocks** | No |
| **Ring Buffer** | Effectively creates a circular buffer |

**Use Case**: Real-time telemetry, metrics, or sensor data where the latest value is always more relevant than older values.

### `overflow_drop_newest_policy`

| Property | Value |
|----------|-------|
| **Behavior** | Silently drops the new item |
| **Data Loss** | New item is lost (but returns `ok()`) |
| **Blocks** | No |
| **Return** | `VoidResult` success (silent drop) |

**Use Case**: Burst absorption where existing queued work is more valuable than new incoming work. The caller does **not** receive an error, making this suitable for fire-and-forget patterns.

> **Note**: Unlike `overflow_reject_policy`, this returns success. Use `overflow_reject_policy` if the caller needs to know the item was dropped.

### `overflow_timeout_policy`

| Property | Value |
|----------|-------|
| **Behavior** | Blocks for a configurable duration, then fails |
| **Data Loss** | New item is dropped if timeout expires |
| **Blocks** | Yes (with timeout) |
| **Default Timeout** | 1 second |
| **Requires** | `supports_blocking_wait = true` (mutex sync only) |

**Use Case**: Interactive systems or SLA-bound services where indefinite blocking is unacceptable but brief waits are tolerable.

```cpp
overflow_timeout_policy policy(std::chrono::milliseconds(500));
policy.set_timeout(std::chrono::seconds(2));  // Adjustable at runtime
```

---

## Compatibility Matrix

### Full Compatibility Table

| SyncPolicy | BoundPolicy | OverflowPolicy | Compatible | Notes |
|------------|-------------|----------------|:----------:|-------|
| `mutex_sync` | `unbounded` | `reject` (default) | Yes | Standard queue — overflow never triggered |
| `mutex_sync` | `bounded` | `reject` | Yes | Fast-fail bounded queue |
| `mutex_sync` | `bounded` | `block` | Yes | Backpressure bounded queue |
| `mutex_sync` | `bounded` | `drop_oldest` | Yes | Ring buffer behavior |
| `mutex_sync` | `bounded` | `drop_newest` | Yes | Silent drop on overflow |
| `mutex_sync` | `bounded` | `timeout` | Yes | Bounded wait queue |
| `mutex_sync` | `dynamic_bounded` | any | Yes | Runtime-adjustable bounds |
| `lockfree_sync` | `unbounded` | `reject` (default) | Yes | High-throughput lock-free queue |
| `lockfree_sync` | `bounded` | `reject` | Yes | Lock-free with fast-fail overflow |
| `lockfree_sync` | `bounded` | `drop_oldest` | Yes | Lock-free ring buffer |
| `lockfree_sync` | `bounded` | `drop_newest` | Yes | Lock-free with silent drop |
| `lockfree_sync` | `bounded` | **`block`** | **No** | Lock-free has no blocking wait |
| `lockfree_sync` | `bounded` | **`timeout`** | **No** | Lock-free has no blocking wait |
| `adaptive_sync` | any | any | Varies | Depends on current runtime mode |

### Quick Decision: Blocking Policies

Blocking overflow policies (`overflow_block_policy`, `overflow_timeout_policy`) require `supports_blocking_wait = true`:

| SyncPolicy | `supports_blocking_wait` |
|------------|:------------------------:|
| `mutex_sync_policy` | Yes |
| `lockfree_sync_policy` | No |
| `adaptive_sync_policy` | Only in mutex mode |

---

## Recommended Combinations

### 1. General-Purpose Work Queue

```cpp
using work_queue = policy_queue<
    policies::mutex_sync_policy,
    policies::unbounded_policy,
    policies::overflow_reject_policy
>;
// Pre-defined alias: standard_queue
```

| Aspect | Detail |
|--------|--------|
| **Use Case** | Task scheduling, thread pools, background workers |
| **Trade-off** | Simple, reliable; no memory bound |
| **Size accuracy** | Exact |
| **Blocking dequeue** | Yes |

### 2. Bounded Backpressure Queue

```cpp
using backpressure_queue = policy_queue<
    policies::mutex_sync_policy,
    policies::bounded_policy,
    policies::overflow_block_policy
>;
// Pre-defined alias: bounded_blocking_queue<MaxSize>
```

| Aspect | Detail |
|--------|--------|
| **Use Case** | Producer-consumer with flow control, pipeline stages |
| **Trade-off** | No data loss; producers may stall |
| **Memory** | Bounded by max size |
| **Data loss** | None |

### 3. High-Throughput Lock-Free Queue

```cpp
using fast_queue = policy_queue<
    policies::lockfree_sync_policy,
    policies::unbounded_policy,
    policies::overflow_reject_policy
>;
// Pre-defined alias: policy_lockfree_queue
```

| Aspect | Detail |
|--------|--------|
| **Use Case** | Low-latency messaging, inter-thread communication under high contention |
| **Trade-off** | Best throughput; approximate size, no blocking dequeue |
| **Algorithm** | Michael-Scott CAS-based linked list |
| **Size accuracy** | Approximate |

### 4. Telemetry Ring Buffer

```cpp
using telemetry_queue = policy_queue<
    policies::mutex_sync_policy,
    policies::bounded_policy,
    policies::overflow_drop_oldest_policy
>;
// Pre-defined alias: ring_buffer_queue<MaxSize>
```

| Aspect | Detail |
|--------|--------|
| **Use Case** | Metrics, sensor data, log tailing — latest data is most valuable |
| **Trade-off** | Bounded memory; stale data is discarded |
| **Data loss** | Oldest items dropped when full |
| **Memory** | Bounded by max size |

### 5. Fire-and-Forget Event Bus

```cpp
using event_queue = policy_queue<
    policies::lockfree_sync_policy,
    policies::bounded_policy,
    policies::overflow_drop_newest_policy
>;
```

| Aspect | Detail |
|--------|--------|
| **Use Case** | Best-effort event delivery, non-critical notifications |
| **Trade-off** | Lock-free + bounded; excess events silently dropped |
| **Data loss** | New items dropped when full (caller unaware) |
| **Blocking** | Never blocks |

### 6. SLA-Bound Service Queue

```cpp
using sla_queue = policy_queue<
    policies::mutex_sync_policy,
    policies::bounded_policy,
    policies::overflow_timeout_policy
>;
```

| Aspect | Detail |
|--------|--------|
| **Use Case** | Request handling with response time guarantees |
| **Trade-off** | Bounded wait prevents indefinite stall; may drop after timeout |
| **Timeout** | Configurable (default 1 second) |
| **Data loss** | Only after timeout expiry |

### 7. Adaptive Runtime Queue

```cpp
using adaptive_queue = policy_queue<
    policies::adaptive_sync_policy,
    policies::dynamic_bounded_policy,
    policies::overflow_reject_policy
>;
```

| Aspect | Detail |
|--------|--------|
| **Use Case** | Systems with varying workload patterns, dynamic scaling |
| **Trade-off** | Maximum flexibility; higher memory cost (two internal queues) |
| **Mode switch** | `queue.sync_policy().switch_mode(mode::lock_free)` |
| **Bound switch** | `queue.bound_policy().set_unbounded()` |

---

## Performance Characteristics

### Throughput Comparison

| Configuration | Enqueue | Dequeue | Under Contention |
|--------------|---------|---------|------------------|
| mutex + unbounded | O(1) | O(1) | Degrades with thread count (lock contention) |
| lockfree + unbounded | O(1) amortized | O(1) amortized | Scales better (CAS retry, no lock) |
| adaptive (mutex mode) | O(1) | O(1) | Same as mutex + atomic mode check overhead |
| adaptive (lockfree mode) | O(1) amortized | O(1) amortized | Same as lockfree + atomic mode check overhead |

### Latency Profiles

| Configuration | P50 | P99 | P999 | Notes |
|--------------|-----|-----|------|-------|
| mutex + unbounded | Low | Moderate | Spikes possible | Lock contention causes tail latency |
| lockfree + unbounded | Very low | Low | Low | Predictable due to no locking |
| mutex + bounded + block | Low | High | Very high | Blocking adds unpredictable delay |
| mutex + bounded + reject | Low | Low | Low | Immediate fail — no blocking |

### Memory Usage

| BoundPolicy | Memory Behavior |
|-------------|----------------|
| `unbounded_policy` | Grows proportionally to queue depth; no upper bound |
| `bounded_policy` | Capped at `max_size * sizeof(unique_ptr<job>)` + overhead |
| `dynamic_bounded_policy` | Same as bounded when limit set; unbounded otherwise |

| SyncPolicy | Per-Queue Overhead |
|------------|-------------------|
| `mutex_sync_policy` | `std::mutex` + `std::condition_variable` + `std::deque` |
| `lockfree_sync_policy` | Linked-list nodes (1 allocation per enqueue) + sentinel |
| `adaptive_sync_policy` | Both mutex and lock-free policy instances |

### Thread Scaling

| SyncPolicy | 1 Thread | 4 Threads | 16 Threads | 64+ Threads |
|------------|----------|-----------|------------|-------------|
| `mutex_sync` | Baseline | Good | Contention degrades | Significant contention |
| `lockfree_sync` | Slight overhead | Good | Better scaling | Best scaling |
| `adaptive_sync` | Mode-dependent | Mode-dependent | Switch to lock-free | Switch to lock-free |

---

## Code Examples

### Basic Usage

```cpp
#include <kcenon/thread/policies/policies.h>

using namespace kcenon::thread;
using namespace kcenon::thread::policies;

// Create a standard unbounded queue
standard_queue queue;

auto job = std::make_unique<callback_job>(
    []() -> std::optional<std::string> {
        // Do work...
        return std::nullopt;
    });

auto result = queue.enqueue(std::move(job));
if (result.is_err()) {
    // Handle error
}

// Blocking dequeue (waits for item)
auto next = queue.dequeue();
if (next.is_ok()) {
    next.unwrap()->execute();
}
```

### Bounded Queue with Backpressure

```cpp
// Create bounded queue with max 1000 items
policy_queue<mutex_sync_policy, bounded_policy, overflow_block_policy>
    queue(bounded_policy(1000));

// Producer (blocks when queue is full)
auto result = queue.enqueue(std::make_unique<callback_job>(work));

// Check capacity
std::size_t remaining = queue.remaining_capacity();
bool at_limit = queue.is_full();
```

### Lock-Free High-Throughput Queue

```cpp
// Lock-free queue for inter-thread messaging
policy_lockfree_queue queue;

// Producer thread
queue.enqueue(std::make_unique<callback_job>(fast_work));

// Consumer thread (non-blocking — must poll)
while (running) {
    auto result = queue.try_dequeue();
    if (result.is_ok()) {
        result.unwrap()->execute();
    } else {
        // No items available — yield or do other work
        std::this_thread::yield();
    }
}
```

### Ring Buffer for Telemetry

```cpp
// Keep only the latest 100 telemetry samples
policy_queue<mutex_sync_policy, bounded_policy, overflow_drop_oldest_policy>
    telemetry(bounded_policy(100));

// Always succeeds — oldest item dropped if full
telemetry.enqueue(std::make_unique<callback_job>(collect_metric));
```

### Dynamic Bounds with Adaptive Sync

```cpp
policy_queue<adaptive_sync_policy, dynamic_bounded_policy, overflow_reject_policy>
    adaptive_queue;

// Start in mutex mode with bounded capacity
adaptive_queue.bound_policy().set_max_size(500);

// Switch to lock-free during high-contention period
adaptive_queue.sync_policy().switch_mode(
    adaptive_sync_policy::mode::lock_free);

// Remove bounds during burst
adaptive_queue.bound_policy().set_unbounded();

// Restore limits after burst
adaptive_queue.bound_policy().set_max_size(1000);
adaptive_queue.sync_policy().switch_mode(
    adaptive_sync_policy::mode::mutex);
```

### Using the Scheduler Interface

```cpp
// policy_queue implements scheduler_interface
scheduler_interface& scheduler = queue;

// Schedule via interface (delegates to enqueue)
scheduler.schedule(std::make_unique<callback_job>(work));

// Get next job via interface (delegates to dequeue)
auto next = scheduler.get_next_job();
```

### Querying Capabilities at Runtime

```cpp
auto caps = queue.get_capabilities();

if (caps.exact_size) {
    auto count = queue.size();  // Safe for precise decisions
}

if (caps.supports_blocking_wait) {
    auto item = queue.dequeue();  // Will block until available
} else {
    auto item = queue.try_dequeue();  // Non-blocking only
}

if (caps.lock_free) {
    // Can expect better performance under high contention
}
```

---

## Anti-Patterns

### 1. Lock-Free + Blocking Overflow

```cpp
// DO NOT: lockfree_sync_policy does not support blocking wait
policy_queue<lockfree_sync_policy, bounded_policy, overflow_block_policy> queue;   // Bad
policy_queue<lockfree_sync_policy, bounded_policy, overflow_timeout_policy> queue;  // Bad
```

**Why**: `lockfree_sync_policy` has `supports_blocking_wait = false`. Blocking overflow policies require condition variable support that only `mutex_sync_policy` provides.

**Fix**: Use `overflow_reject_policy` or `overflow_drop_*` policies with lock-free sync.

### 2. Unbounded Queue in Memory-Constrained Environments

```cpp
// CAUTION: No upper bound on memory usage
policy_queue<mutex_sync_policy, unbounded_policy> queue;  // Risk in constrained env
```

**Why**: If the producer outpaces the consumer for sustained periods, the queue grows without limit, potentially exhausting system memory.

**Fix**: Use `bounded_policy` with an appropriate `max_size` and choose an overflow policy that matches your requirements.

### 3. Relying on Lock-Free `size()` for Precise Decisions

```cpp
policy_lockfree_queue queue;

// BAD: size() is approximate for lock-free queues
if (queue.size() == 0) {
    // May still have items — race condition
}
```

**Why**: `lockfree_sync_policy::size()` uses `std::memory_order_relaxed` and can be stale. Use `empty()` for a better (though still approximate) check, or use `try_dequeue()` and check the result.

**Fix**: Check `queue.get_capabilities().exact_size` before relying on `size()`.

### 4. Switching Adaptive Mode Without Draining

```cpp
adaptive_sync_policy sync;
// Items enqueued in mutex mode...
sync.switch_mode(adaptive_sync_policy::mode::lock_free);
// Items in mutex queue are now inaccessible!
```

**Why**: `adaptive_sync_policy::switch_mode()` does **not** migrate data between internal queues. Items in the previous mode's queue remain there but are no longer accessible through the policy's operations.

**Fix**: Drain all items before switching modes:

```cpp
while (!queue.empty()) {
    auto item = queue.try_dequeue();
    // Process or re-enqueue after switch
}
queue.sync_policy().switch_mode(target_mode);
```

### 5. Using `overflow_drop_newest_policy` When Caller Needs Feedback

```cpp
// BAD: Caller cannot distinguish success from silent drop
policy_queue<mutex_sync_policy, bounded_policy, overflow_drop_newest_policy> queue;
auto result = queue.enqueue(make_job());
// result.is_ok() == true even if item was dropped!
```

**Why**: `overflow_drop_newest_policy` returns `ok()` on drop. The caller has no way to know the item was discarded.

**Fix**: Use `overflow_reject_policy` if the caller needs to know about overflow.

### 6. Bounded Queue Without Considering Overflow Policy

```cpp
// DEFAULT: overflow_reject_policy — may surprise callers
policy_queue<mutex_sync_policy, bounded_policy> queue(bounded_policy(10));
// After 10 enqueues, the 11th silently fails if you don't check the result
```

**Why**: The default overflow policy is `overflow_reject_policy`. Callers must always check the `VoidResult` from `enqueue()`, or items will be silently lost.

**Fix**: Always check enqueue results, or choose an overflow policy that matches your error-handling strategy.

---

## Pre-Defined Type Aliases

For convenience, `policy_queue.h` provides these ready-to-use aliases:

| Alias | SyncPolicy | BoundPolicy | OverflowPolicy |
|-------|------------|-------------|----------------|
| `standard_queue` | `mutex_sync` | `unbounded` | `reject` |
| `policy_lockfree_queue` | `lockfree_sync` | `unbounded` | `reject` |
| `bounded_blocking_queue<N>` | `mutex_sync` | `bounded` | `block` |
| `bounded_rejecting_queue<N>` | `mutex_sync` | `bounded` | `reject` |
| `ring_buffer_queue<N>` | `mutex_sync` | `bounded` | `drop_oldest` |

> **Note**: `N` is a template parameter for documentation purposes. The actual `bounded_policy` max size is set at construction time, not via the template parameter.

---

## Further Reading

- [Architecture Overview](ARCHITECTURE.md) — System-wide design and components
- [API Reference](API_REFERENCE.md) — Complete API documentation
- [Benchmarks](BENCHMARKS.md) — Performance measurement results
- [Queue Backward Compatibility](QUEUE_BACKWARD_COMPATIBILITY.md) — Migration from legacy queues
