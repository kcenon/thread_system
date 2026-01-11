# Queue Policy Interface Design

**Version**: 1.1.0
**Issue**: #437 (Phase 1.1), #438 (Phase 1.2)
**Status**: Implementation Complete
**Last Updated**: 2026-01-11

---

## Overview

This document defines the policy-based design for consolidating the thread_system's queue implementations from 10 variants to a unified template system. The design follows Kent Beck's Simple Design Principles: "No Duplication" and "Fewest Elements."

---

## Table of Contents

1. [Current State Analysis](#current-state-analysis)
2. [Policy Requirements Mapping](#policy-requirements-mapping)
3. [Policy Interface Hierarchy](#policy-interface-hierarchy)
4. [Template Class Structure](#template-class-structure)
5. [Migration Path](#migration-path)
6. [Consolidation Roadmap](#consolidation-roadmap)

---

## Current State Analysis

### Queue Variants Inventory

| # | Variant | Sync Method | Status | Location |
|---|---------|-------------|--------|----------|
| 1 | job_queue | Mutex + CV | **Active** | core/job_queue.h |
| 2 | bounded_job_queue | Mutex | **DEPRECATED** | (removed) |
| 3 | backpressure_job_queue | Mutex | **Active** | core/backpressure_job_queue.h |
| 4 | detail::lockfree_job_queue | Atomic | **Active** | lockfree/lockfree_job_queue.h |
| 5 | detail::concurrent_queue | Fine-grain | **Internal** | concurrent/concurrent_queue.h |
| 6 | adaptive_job_queue | Wrapper | **Active** | queue/adaptive_job_queue.h |
| 7 | typed_job_queue_t | Mutex | **REMOVED** | (removed in Phase 1.4.4) |
| 8 | typed_lockfree_job_queue_t | Lock-free | **REMOVED** | (removed in Phase 1.4.2) |
| 9 | adaptive_typed_job_queue_t | Wrapper | **REMOVED** | (removed in Phase 1.4.1) |
| 10 | aging_typed_job_queue_t | Mutex + Thread | **Active** | impl/typed_pool/aging_typed_job_queue.h |

### Common Functionality (>80% Shared)

All queue implementations share these core operations:
- `enqueue()` / `dequeue()` operations
- `empty()` / `size()` queries
- `clear()` / `stop()` lifecycle methods
- `scheduler_interface` inheritance
- `queue_capabilities_interface` inheritance

### Unique Features by Variant

| Feature | job_queue | backpressure | lockfree | concurrent | adaptive | aging |
|---------|-----------|--------------|----------|------------|----------|-------|
| Blocking wait | ✓ | ✓ | ✗ | ✓ | varies | ✓ |
| Exact size | ✓ | ✓ | ✗ | ✗ | varies | ✓ |
| Lock-free | ✗ | ✗ | ✓ | ✗ | varies | ✗ |
| Batch ops | ✓ | ✓ | ✗ | ✗ | ✓ | ✓ |
| Bounded | ✓ | ✓ | ✗ | ✗ | ✗ | ✓ |
| Backpressure | ✗ | ✓ | ✗ | ✗ | ✗ | ✗ |
| Rate limiting | ✗ | ✓ | ✗ | ✗ | ✗ | ✗ |
| Mode switching | ✗ | ✗ | ✗ | ✗ | ✓ | ✗ |
| Priority aging | ✗ | ✗ | ✗ | ✗ | ✗ | ✓ |

---

## Policy Requirements Mapping

### Synchronization Policies

| Requirement | Policy | Provides |
|-------------|--------|----------|
| Accuracy-first | `mutex_sync` | Exact size, blocking wait, batch ops |
| Performance-first | `lockfree_sync` | Lock-free ops, approximate size |
| Balanced | `adaptive_sync` | Auto-switching between modes |
| Low contention | `dual_mutex_sync` | Separate head/tail locks |

### Capacity Policies

| Requirement | Policy | Behavior |
|-------------|--------|----------|
| Unlimited | `unbounded` | No size limit |
| Fixed limit | `bounded<N>` | Compile-time capacity |
| Dynamic limit | `bounded_dynamic` | Runtime-configurable capacity |

### Overflow Policies (for bounded queues)

| Requirement | Policy | Behavior |
|-------------|--------|----------|
| Block producer | `block_on_full` | Wait until space available |
| Reject new | `drop_newest` | Discard incoming job |
| Remove old | `drop_oldest` | Remove oldest to make room |
| Custom | `callback_policy` | User-defined handler |
| Load-aware | `adaptive_overflow` | Adjust based on pressure |

### Backpressure Policies

| Requirement | Policy | Features |
|-------------|--------|----------|
| None | `no_backpressure` | Default behavior |
| Watermark-based | `watermark_pressure` | Three-level pressure (low/high/critical) |
| Rate-limited | `rate_limited` | Token bucket algorithm |

### Priority Policies

| Requirement | Policy | Behavior |
|-------------|--------|----------|
| FIFO | `fifo_order` | First-in, first-out |
| Priority | `priority_order` | Per-type priority queues |
| Aging | `aging_priority` | Boost priority over time |

---

## Policy Interface Hierarchy

### Base Interfaces

```cpp
namespace kcenon::thread::policy {

// Synchronization policy interface
struct sync_policy_base {
    // Implemented by: mutex_sync, lockfree_sync, adaptive_sync
};

// Capacity policy interface
struct capacity_policy_base {
    virtual auto max_capacity() const -> std::optional<std::size_t> = 0;
    virtual auto is_full(std::size_t current) const -> bool = 0;
};

// Overflow policy interface
struct overflow_policy_base {
    virtual auto on_full(/* context */) -> overflow_action = 0;
};

// Backpressure policy interface
struct backpressure_policy_base {
    virtual auto get_pressure_level() const -> pressure_level = 0;
    virtual auto should_accept() const -> bool = 0;
};

// Priority policy interface
struct priority_policy_base {
    virtual auto compare(const job&, const job&) const -> bool = 0;
};

}
```

### Concrete Policy Classes

```cpp
namespace kcenon::thread::policy {

// Synchronization policies
struct mutex_sync : sync_policy_base {
    // Uses std::mutex + std::condition_variable
    // Provides: exact_size=true, blocking_wait=true, batch_ops=true
};

struct lockfree_sync : sync_policy_base {
    // Uses atomic CAS operations with hazard pointers
    // Provides: lock_free=true, approximate_size=true
};

struct adaptive_sync : sync_policy_base {
    // Wraps mutex_sync and lockfree_sync
    // Provides: auto-switching based on usage patterns
};

// Capacity policies
struct unbounded : capacity_policy_base {
    auto max_capacity() const -> std::optional<std::size_t> override {
        return std::nullopt;
    }
    auto is_full(std::size_t) const -> bool override {
        return false;
    }
};

template<std::size_t N>
struct bounded : capacity_policy_base {
    auto max_capacity() const -> std::optional<std::size_t> override {
        return N;
    }
    auto is_full(std::size_t current) const -> bool override {
        return current >= N;
    }
};

// Overflow policies
struct block_on_full : overflow_policy_base {
    std::chrono::milliseconds timeout{std::chrono::hours(24)};
    auto on_full(/* context */) -> overflow_action override;
};

struct drop_newest : overflow_policy_base {
    auto on_full(/* context */) -> overflow_action override {
        return overflow_action::reject;
    }
};

struct drop_oldest : overflow_policy_base {
    auto on_full(/* context */) -> overflow_action override {
        return overflow_action::remove_oldest;
    }
};

// Backpressure policies
struct no_backpressure : backpressure_policy_base {
    auto get_pressure_level() const -> pressure_level override {
        return pressure_level::none;
    }
    auto should_accept() const -> bool override {
        return true;
    }
};

struct watermark_pressure : backpressure_policy_base {
    std::size_t low_watermark;
    std::size_t high_watermark;
    std::size_t critical_watermark;
    // Implements three-level pressure tracking
};

// Priority policies
struct fifo_order : priority_policy_base {
    auto compare(const job& a, const job& b) const -> bool override {
        return a.created_time() < b.created_time();
    }
};

struct aging_priority : priority_policy_base {
    aging_curve curve{aging_curve::linear};
    std::chrono::milliseconds aging_interval{1000};
    // Implements priority boost over wait time
};

}
```

---

## Template Class Structure

### Primary Queue Template

```cpp
namespace kcenon::thread {

template<
    typename SyncPolicy = policy::mutex_sync,
    typename CapacityPolicy = policy::unbounded,
    typename OverflowPolicy = policy::block_on_full,
    typename BackpressurePolicy = policy::no_backpressure,
    typename PriorityPolicy = policy::fifo_order
>
class queue : public scheduler_interface,
              public queue_capabilities_interface {
public:
    // Core operations (delegated to policies)
    [[nodiscard]] auto enqueue(std::unique_ptr<job>&& j) -> common::VoidResult;
    [[nodiscard]] auto dequeue() -> common::Result<std::unique_ptr<job>>;
    [[nodiscard]] auto try_dequeue() -> common::Result<std::unique_ptr<job>>;

    // Queries
    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> std::size_t;
    [[nodiscard]] auto is_full() const -> bool;

    // Lifecycle
    auto clear() -> void;
    auto stop() -> void;
    [[nodiscard]] auto is_stopped() const -> bool;

    // Capabilities (derived from policies)
    [[nodiscard]] auto get_capabilities() const -> queue_capabilities override;

    // Batch operations (if supported by SyncPolicy)
    [[nodiscard]] auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs)
        -> common::VoidResult;
    [[nodiscard]] auto dequeue_batch() -> std::deque<std::unique_ptr<job>>;
    [[nodiscard]] auto dequeue_batch_limited(std::size_t max_count)
        -> std::deque<std::unique_ptr<job>>;

    // Type-safe enqueue
    template<typename JobType>
    [[nodiscard]] auto enqueue(std::unique_ptr<JobType>&& j) -> common::VoidResult;

private:
    SyncPolicy sync_;
    CapacityPolicy capacity_;
    OverflowPolicy overflow_;
    BackpressurePolicy backpressure_;
    PriorityPolicy priority_;

    // Implementation details vary by SyncPolicy
};

}
```

### Type Aliases for Common Configurations

```cpp
namespace kcenon::thread {

// Standard FIFO queue (replaces job_queue)
using standard_queue = queue<
    policy::mutex_sync,
    policy::unbounded
>;

// Bounded queue (replaces bounded_job_queue)
template<std::size_t N>
using bounded_queue = queue<
    policy::mutex_sync,
    policy::bounded<N>,
    policy::block_on_full
>;

// Backpressure queue (replaces backpressure_job_queue)
using backpressure_queue = queue<
    policy::mutex_sync,
    policy::bounded_dynamic,
    policy::adaptive_overflow,
    policy::watermark_pressure
>;

// Lock-free queue (replaces lockfree_job_queue)
// Named policy_lockfree_queue to avoid collision with deprecated lockfree_queue<T>
using policy_lockfree_queue = queue<
    policy::lockfree_sync,
    policy::unbounded
>;

// Adaptive queue (replaces adaptive_job_queue)
using adaptive_queue = queue<
    policy::adaptive_sync,
    policy::unbounded
>;

// Aging priority queue (replaces aging_typed_job_queue_t)
using aging_queue = queue<
    policy::mutex_sync,
    policy::unbounded,
    policy::block_on_full,
    policy::no_backpressure,
    policy::aging_priority
>;

}
```

---

## Migration Path

### Phase 1.2: Core Implementation

1. Create policy base interfaces in `include/kcenon/thread/policy/`
2. Implement `mutex_sync` policy (wrapping job_queue logic)
3. Implement `lockfree_sync` policy (wrapping lockfree_job_queue)
4. Implement capacity and overflow policies
5. Create primary `queue` template class

### Phase 1.3: Migration

| Old Type | New Type | Migration Notes |
|----------|----------|-----------------|
| `job_queue` | `standard_queue` | Direct replacement |
| `bounded_job_queue` | `bounded_queue<N>` | Use template parameter |
| `backpressure_job_queue` | `backpressure_queue` | Configure watermarks |
| `lockfree_job_queue` | `policy_lockfree_queue` | Check for capability changes |
| `adaptive_job_queue` | `adaptive_queue` | Minimal changes |
| `typed_job_queue_t<T>` | `standard_queue` with `enqueue<T>()` | Use template enqueue |
| `typed_lockfree_job_queue_t<T>` | `policy_lockfree_queue` with `enqueue<T>()` | **REMOVED** in Phase 1.4.2 |
| `adaptive_typed_job_queue_t<T>` | `adaptive_queue` with `enqueue<T>()` | **REMOVED** in Phase 1.4.1 |
| `aging_typed_job_queue_t<T>` | `aging_queue` | Configure aging params |

### Phase 1.4: Cleanup (In Progress)

1. ~~Mark deprecated classes with `[[deprecated("Use queue<...> instead")]]`~~
2. Update all internal usages to new types
3. **DONE**: Remove `adaptive_typed_job_queue_t` (Phase 1.4.1)
4. **DONE**: Remove `typed_lockfree_job_queue_t` (Phase 1.4.2)
5. **DONE**: Migrate `aging_typed_job_queue_t` to policy_queue (Phase 1.4.3)
6. **DONE**: Remove `typed_job_queue_t` (Phase 1.4.4)
7. TODO: Update documentation and examples (Phase 1.4.5)

---

## Consolidation Roadmap

### Milestone 1: Foundation (Phase 1.1) ✓

- [x] Complete queue variants inventory
- [x] Identify common functionality (>80% shared)
- [x] Map unique features to policy requirements
- [x] Design policy interface hierarchy
- [x] Document consolidation roadmap

### Milestone 2: Implementation (Phase 1.2) ✓

- [x] Create policy header files
- [x] Implement sync policies (mutex, lockfree, adaptive)
- [x] Implement capacity policies (unbounded, bounded, dynamic_bounded)
- [x] Implement overflow policies (reject, block, drop_oldest, drop_newest, timeout)
- [x] Create queue template class (policy_queue)
- [x] Add unit tests for policies (42 tests passing)

### Milestone 3: Migration (Phase 1.3) (In Progress)

- [x] Create migration guide (see [QUEUE_MIGRATION_GUIDE.md](./QUEUE_MIGRATION_GUIDE.md))
- [ ] Update thread_pool to use new queue (#450)
- [ ] Migrate internal usages
- [ ] Performance regression testing
- [ ] Update examples

### Milestone 4: Cleanup (Phase 1.4) (In Progress)

- [x] Phase 1.4.1: Remove adaptive_typed_job_queue_t (#456)
- [x] Phase 1.4.2: Remove typed_lockfree_job_queue_t (#457)
- [x] Phase 1.4.3: Migrate aging_typed_job_queue_t to policy_queue (#458)
- [x] Phase 1.4.4: Remove typed_job_queue_t (#459)
- [ ] Phase 1.4.5: Update all documentation (#460)

---

## Acceptance Criteria

- [ ] Queue variants reduced from 10 to 1 template + type aliases
- [ ] All existing functionality preserved via policies
- [ ] Code duplication reduced by 60%+
- [ ] All existing tests pass with new implementation
- [ ] Performance parity or improvement
- [ ] No breaking changes to public API (deprecation only)

---

## References

- Kent Beck's Simple Design Principles
- C++ Core Guidelines (Policy-based design)
- Issue #434: Phase 1.0 Consolidate Queue Variants
