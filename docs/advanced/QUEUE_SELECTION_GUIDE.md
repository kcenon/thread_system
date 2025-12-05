# Queue Selection Guide

> **Language:** **English** | [한국어](QUEUE_SELECTION_GUIDE_KO.md)

This guide helps you choose the right queue implementation for your use case.

---

## Quick Decision Tree

```
Do you need EXACT size() or empty()?
├─ YES → Use job_queue (standard)
│
└─ NO → Do you need MAXIMUM throughput?
        ├─ YES → Use lockfree_job_queue
        │
        └─ NO/UNSURE → Do you need automatic optimization?
                       ├─ YES → Use adaptive_job_queue
                       │
                       └─ NO → Use job_queue (safe default)
```

---

## Queue Implementations

### job_queue (Standard)

The default mutex-based FIFO queue. Best for most use cases.

**Characteristics:**
- Exact `size()` and `empty()` operations
- Supports batch enqueue/dequeue
- Supports blocking wait with `dequeue()`
- Thread-safe with mutex protection
- Throughput: ~300K ops/sec

**Use When:**
- You need accurate monitoring metrics
- You need backpressure based on queue size
- You need batch operations
- Moderate throughput is sufficient
- You're unsure which queue to use

**Example:**
```cpp
auto queue = std::make_shared<job_queue>();
queue->enqueue(std::make_unique<my_job>());
auto exact_size = queue->size();  // Always exact
```

---

### lockfree_job_queue (High Performance)

Lock-free MPMC queue using Michael-Scott algorithm with hazard pointers.

**Characteristics:**
- 4x faster than mutex-based (~1.2M ops/sec)
- Approximate `size()` (may drift under contention)
- Non-atomic `empty()` (state may change after call)
- Wait-free enqueue, lock-free dequeue
- No blocking wait support

**Use When:**
- Maximum throughput required (>500K ops/sec)
- Many concurrent producers/consumers
- You don't need exact size/empty checks
- Using `try_dequeue()` loop pattern

**Example:**
```cpp
auto queue = std::make_unique<lockfree_job_queue>();
queue->enqueue(std::make_unique<my_job>());

// Use try_dequeue pattern (non-blocking)
while (running) {
    auto result = queue->try_dequeue();
    if (result.has_value()) {
        auto& job = result.value();
        job->do_work();
    }
}
```

---

### adaptive_job_queue (Flexible)

Adaptive queue that automatically switches between mutex and lock-free modes.

**Characteristics:**
- Wraps both job_queue and lockfree_job_queue
- Automatic or manual mode switching
- RAII guard for temporary accuracy mode
- Statistics tracking for mode usage
- Policy-based behavior (accuracy, performance, balanced, manual)

**Use When:**
- You want automatic optimization
- Workload varies between accuracy-critical and throughput-critical phases
- You need temporary accuracy sometimes
- You're unsure which implementation to choose

**Example:**
```cpp
auto queue = std::make_unique<adaptive_job_queue>(
    adaptive_job_queue::policy::balanced);

// Normal usage
queue->enqueue(std::make_unique<my_job>());

// Temporarily require accuracy for backpressure check
{
    auto guard = queue->require_accuracy();
    if (queue->size() > 1000) {
        apply_backpressure();
    }
}
// After scope, may revert to lock-free mode

// Check statistics
auto stats = queue->get_stats();
std::cout << "Mode switches: " << stats.mode_switches << "\n";
```

---

## Comparison Table

| Feature | job_queue | lockfree_job_queue | adaptive_job_queue |
|---------|-----------|--------------------|--------------------|
| **Throughput** | ~300K/s | ~1.2M/s | Mode-dependent |
| **size() accuracy** | Exact | Approximate | Mode-dependent |
| **empty() atomic** | Yes | No | Mode-dependent |
| **Batch operations** | Yes | No | Mode-dependent |
| **Blocking wait** | Yes | No (spin) | Mode-dependent |
| **Lock-free** | No | Yes | Mode-dependent |
| **Memory overhead** | Low | ~256B/thread | Moderate |
| **Contention scaling** | Moderate | Excellent | Auto-tuned |

---

## Performance Characteristics

### Throughput by Contention Level

| Contention | job_queue | lockfree_job_queue | Winner |
|------------|-----------|--------------------|----|
| Low (1-2 threads) | 96 ns/op | 320 ns/op | job_queue |
| Medium (4 threads) | 150 ns/op | 320 ns/op | job_queue |
| High (8+ threads) | 291 ns/op | 71 ns/op | lockfree (4x) |

### When Lock-Free Excels

- 8+ concurrent producers/consumers
- High contention scenarios
- Latency-sensitive applications
- Real-time systems

### When Mutex Excels

- Low contention scenarios
- Accuracy-critical operations
- Batch processing
- Simpler debugging

---

## Queue Factory

Use `queue_factory` for convenient queue creation.

> **See also:** [`examples/queue_factory_sample/`](../../examples/queue_factory_sample/) for a complete working example.

### Direct Creation

```cpp
#include <kcenon/thread/queue/queue_factory.h>

// Create specific queue types
auto standard = queue_factory::create_standard_queue();
auto lockfree = queue_factory::create_lockfree_queue();
auto adaptive = queue_factory::create_adaptive_queue();
```

### Requirements-Based Creation

```cpp
queue_factory::requirements reqs;
reqs.need_exact_size = true;
reqs.need_batch_operations = true;
auto queue = queue_factory::create_for_requirements(reqs);
// Returns job_queue
```

### Environment-Optimized Creation

```cpp
// Automatically selects based on:
// - CPU architecture (x86 vs ARM)
// - Number of CPU cores
// - Memory model strength
auto optimal = queue_factory::create_optimal();
```

---

## Compile-Time Selection

For zero-runtime-overhead queue selection:

```cpp
#include <kcenon/thread/queue/queue_factory.h>

// Type aliases
accurate_queue_t accurate;   // job_queue
fast_queue_t fast;           // lockfree_job_queue
balanced_queue_t balanced;   // adaptive_job_queue

// Template-based selection
queue_t<true, false> q1;     // Need exact size → job_queue
queue_t<false, true> q2;     // Prefer lock-free → lockfree_job_queue
queue_t<false, false> q3;    // Balanced → adaptive_job_queue
```

---

## Adaptive Queue Policies

### accuracy_first

Always uses mutex mode. Use when accuracy is paramount.

```cpp
auto queue = std::make_unique<adaptive_job_queue>(
    adaptive_job_queue::policy::accuracy_first);
// Always exact size() and atomic empty()
```

### performance_first

Always uses lock-free mode. Use when throughput is paramount.

```cpp
auto queue = std::make_unique<adaptive_job_queue>(
    adaptive_job_queue::policy::performance_first);
// Maximum throughput, approximate size()
```

### balanced (Default)

Automatically switches based on workload analysis.

```cpp
auto queue = std::make_unique<adaptive_job_queue>(
    adaptive_job_queue::policy::balanced);
// Auto-optimizes based on usage patterns
```

### manual

User controls mode switching explicitly.

```cpp
auto queue = std::make_unique<adaptive_job_queue>(
    adaptive_job_queue::policy::manual);

queue->switch_mode(adaptive_job_queue::mode::lock_free);
// ... high-throughput phase ...

queue->switch_mode(adaptive_job_queue::mode::mutex);
// ... accuracy-critical phase ...
```

---

## Capability Introspection

Check queue capabilities at runtime:

```cpp
#include <kcenon/thread/interfaces/queue_capabilities_interface.h>

void process_queue(scheduler_interface* queue) {
    // Check if queue supports capability queries
    if (auto* cap = dynamic_cast<queue_capabilities_interface*>(queue)) {
        auto caps = cap->get_capabilities();

        if (caps.exact_size) {
            // Safe to use size() for decisions
            if (queue->size() > threshold) {
                apply_backpressure();
            }
        }

        if (caps.lock_free) {
            // Adjust polling strategy
            use_spin_wait();
        }

        if (caps.supports_batch) {
            // Use batch operations for efficiency
            queue->enqueue_batch(jobs);
        }
    }
}
```

---

## Best Practices

### 1. Start with job_queue

When in doubt, use `job_queue`. It's reliable, predictable, and sufficient for most workloads.

### 2. Profile Before Optimizing

Don't switch to lock-free prematurely. Profile your actual workload to confirm contention is a bottleneck.

### 3. Use Capability Checking for Generic Code

When writing libraries or frameworks that accept any queue type:

```cpp
template<typename Queue>
void process(Queue& queue) {
    if constexpr (requires { queue.has_exact_size(); }) {
        if (queue.has_exact_size()) {
            // Use size-based logic
        }
    }
}
```

### 4. Consider adaptive_job_queue for Variable Workloads

If your application has phases with different characteristics (initialization, steady-state, shutdown), `adaptive_job_queue` can optimize automatically.

### 5. Use accuracy_guard for Critical Sections

When you temporarily need exact size in an otherwise high-throughput path:

```cpp
auto queue = std::make_unique<adaptive_job_queue>();

// Normal high-throughput operation
queue->enqueue(job);

// Critical section needing exact count
{
    auto guard = queue->require_accuracy();
    if (queue->size() >= max_size) {
        reject_new_work();
    }
}
// Back to high-throughput mode
```

---

## Troubleshooting

### Q: My lock-free queue shows inconsistent size

**Expected behavior.** Lock-free queues provide approximate size due to concurrent modifications. Use `job_queue` if exact size is required.

### Q: Performance didn't improve with lock-free queue

Check your contention level. Lock-free queues shine under high contention (8+ threads). Under low contention, mutex-based queues may actually be faster.

### Q: adaptive_job_queue keeps switching modes

This might indicate a workload with varied characteristics. Consider using `manual` policy with explicit mode control, or adjust your workload pattern.

---

*Last Updated: 2025-12-05*
