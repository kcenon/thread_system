---
doc_id: "THR-ADR-003"
doc_title: "ADR-003: Queue Architecture Selection"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Accepted"
project: "thread_system"
category: "ADR"
---

# ADR-003: Queue Architecture Selection

> **SSOT**: This document is the single source of truth for **ADR-003: Queue Architecture Selection**.

| Field | Value |
|-------|-------|
| Status | Accepted |
| Date | 2025-03-01 |
| Decision Makers | kcenon ecosystem maintainers |

## Context

thread_system's thread pool requires a job queue to buffer submitted tasks before
worker threads consume them. The queue is the critical path for throughput and
latency — every job passes through it.

Requirements:
1. **High throughput** — Support millions of job submissions per second.
2. **Low latency** — Sub-microsecond enqueue/dequeue for latency-sensitive workloads.
3. **Thread safety** — Multiple producers (submitters) and consumers (workers).
4. **Bounded mode** — Optional backpressure when queue is full.
5. **Simplicity** — Default queue should work well without tuning.

Three queue strategies were evaluated:
- Mutex-based FIFO queue
- Lock-free queue (Michael-Scott algorithm)
- Adaptive queue that switches strategies at runtime

## Decision

**Provide two public queue types with an adaptive default:**

1. **`adaptive_job_queue`** (recommended default) — Monitors contention at runtime
   and automatically switches between mutex-based and lock-free modes. Uses a
   contention counter: when CAS failures exceed a threshold, switches to lock-free;
   when contention drops, reverts to mutex mode.

2. **`job_queue`** — Simple mutex-based FIFO with optional bounded size. Suitable
   for low-contention scenarios or when deterministic behavior is preferred.

The lock-free queue (`lockfree_job_queue`) is internal — used by `adaptive_job_queue`
but not directly exposed to users.

```cpp
// Default: adaptive (recommended)
auto pool = thread_pool_builder()
    .with_workers(8)
    .build();  // uses adaptive_job_queue internally

// Explicit: mutex-based with bounded size
auto pool = thread_pool_builder()
    .with_workers(8)
    .with_queue<job_queue>(/*max_size=*/10000)
    .build();
```

## Alternatives Considered

### Lock-Free Only

- **Pros**: Lowest latency under high contention, no mutex overhead.
- **Cons**: Higher memory usage (hazard pointers), complex debugging, worse
  performance than mutex under low contention due to CAS retry overhead.

### Mutex-Based Only

- **Pros**: Simple, predictable, easy to debug, works well under low contention.
- **Cons**: Performance degrades under high contention due to thread parking/waking.

### User-Selectable at Compile Time

- **Pros**: No runtime overhead from strategy switching.
- **Cons**: Requires users to understand their contention profile upfront, which
  most users cannot predict accurately.

## Consequences

### Positive

- **Zero-configuration performance**: `adaptive_job_queue` delivers good performance
  across a wide range of workloads without user tuning.
- **Escape hatch**: Power users can select `job_queue` explicitly when they need
  deterministic mutex behavior (e.g., for reproducible benchmarks).
- **Lock-free internals**: The Michael-Scott lock-free queue with hazard pointer
  memory reclamation is available when contention warrants it.

### Negative

- **Runtime overhead**: The adaptive queue incurs a small overhead (~2-5 ns per
  operation) for contention monitoring and mode switching.
- **Complexity**: Two queue implementations plus an adaptive wrapper increase
  maintenance burden compared to a single implementation.
- **Debugging difficulty**: Lock-free code paths are harder to debug with
  traditional tools (GDB, Valgrind) due to non-blocking memory reclamation.
