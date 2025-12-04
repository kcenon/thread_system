# Queue Backward Compatibility

> **Language:** **English** | [한국어](QUEUE_BACKWARD_COMPATIBILITY_KO.md)

## TL;DR: Your Code Still Works

**No migration is required.** All existing code continues to work exactly as before. The queue capability enhancements introduced in Phase 1-5 are purely additive and non-breaking.

---

## What Changed?

| Change | Impact on Existing Code |
|--------|------------------------|
| `job_queue` gains `queue_capabilities_interface` | None - additive inheritance |
| `lockfree_job_queue` gains `scheduler_interface` | None - additive inheritance |
| New `queue_capabilities` struct | None - new type |
| New `queue_capabilities_interface` | None - optional mixin |
| New `adaptive_job_queue` class | None - new class |
| New `queue_factory` utility | None - new utility |

### Compatibility Guarantees

| Aspect | Guarantee | Details |
|--------|-----------|---------|
| **Source Compatibility** | 100% | Existing code compiles without changes |
| **Binary Compatibility** | 100% | No ABI breaking changes |
| **API Compatibility** | 100% | All existing methods preserved |
| **Behavioral Compatibility** | 100% | Existing behavior unchanged |

---

## Existing Code Examples

All of these continue to work **exactly as before**:

### Direct Construction

```cpp
// These still work exactly as before:
auto queue = std::make_shared<job_queue>();
auto lockfree = std::make_unique<lockfree_job_queue>();
```

### Queue Operations

```cpp
// All existing methods - UNCHANGED
queue->enqueue(std::make_unique<my_job>());
auto job = queue->dequeue();
auto result = queue->try_dequeue();
queue->size();   // Still exact for job_queue
queue->empty();  // Still atomic for job_queue
queue->clear();
queue->stop();
```

### Scheduler Interface Usage

```cpp
// scheduler_interface usage - UNCHANGED
scheduler_interface* scheduler = queue.get();
scheduler->schedule(std::make_unique<my_job>());
auto next = scheduler->get_next_job();
```

### Thread Pool Integration

```cpp
// Thread pool usage - UNCHANGED
auto pool = std::make_shared<thread_pool>("MyPool");
pool->start();
pool->enqueue(std::make_unique<my_job>());
pool->stop();
```

---

## Optional New Features

You can **optionally** use these new features when you need them:

### Capability Checking

```cpp
#include <kcenon/thread/interfaces/queue_capabilities_interface.h>

// Check capabilities (optional)
if (auto* cap = dynamic_cast<queue_capabilities_interface*>(queue.get())) {
    if (cap->has_exact_size()) {
        // Safe to make decisions based on size()
        auto count = queue->size();
    }
    if (cap->is_lock_free()) {
        // Queue uses lock-free algorithms
    }
}
```

### Queue Factory

```cpp
#include <kcenon/thread/queue/queue_factory.h>

// Use factory (optional)
auto standard = queue_factory::create_standard_queue();
auto lockfree = queue_factory::create_lockfree_queue();
auto adaptive = queue_factory::create_adaptive_queue();

// Environment-optimized selection
auto optimal = queue_factory::create_optimal();

// Requirements-based creation
queue_factory::requirements reqs;
reqs.need_exact_size = true;
auto queue = queue_factory::create_for_requirements(reqs);
```

### Adaptive Queue

```cpp
#include <kcenon/thread/queue/adaptive_job_queue.h>

// Use adaptive queue (optional)
auto adaptive = std::make_unique<adaptive_job_queue>();

// Check current mode
if (adaptive->current_mode() == adaptive_job_queue::mode::mutex) {
    // Operating in mutex mode
}

// Temporarily require accuracy
{
    auto guard = adaptive->require_accuracy();
    // In this scope, size() is guaranteed exact
    auto exact_count = adaptive->size();
}
// After scope, may revert to lock-free mode
```

### Compile-Time Queue Selection

```cpp
#include <kcenon/thread/queue/queue_factory.h>

// Compile-time type selection
accurate_queue_t accurate;   // job_queue
fast_queue_t fast;           // lockfree_job_queue
balanced_queue_t balanced;   // adaptive_job_queue
```

---

## Lock-Free Queue Semantics

When using `lockfree_job_queue`, be aware of these inherent characteristics:

| Method | Behavior |
|--------|----------|
| `size()` | Returns **approximate** value (may drift under contention) |
| `empty()` | **Non-atomic** (state may change after call returns) |
| `enqueue()` | **Wait-free** - always completes in bounded steps |
| `dequeue()` | **Lock-free** - may retry under contention |

These are **fundamental characteristics** of lock-free data structures, not bugs. Use `job_queue` if you need exact size/empty checks.

---

## When to Consider Migration

You might want to adopt the new features if:

1. **You need capability introspection**
   - Use `queue_capabilities_interface` to check queue properties at runtime

2. **You want automatic optimization**
   - Use `adaptive_job_queue` for workload-aware mode switching

3. **You need flexible queue creation**
   - Use `queue_factory` for requirements-based queue selection

4. **You want compile-time queue selection**
   - Use `queue_t<>` template aliases for type-safe selection

---

## FAQ

### Q: Do I need to change my existing code?

**No.** All existing code continues to work without any modifications.

### Q: Will my code break if I update to the latest version?

**No.** All changes are additive. Existing APIs remain unchanged.

### Q: What if I'm using `lockfree_job_queue` directly?

It still works exactly as before. The only addition is that it now also implements `scheduler_interface`, which is purely additive.

### Q: What if I'm using `job_queue` through `scheduler_interface`?

It still works exactly as before. The queue now also implements `queue_capabilities_interface`, but this doesn't affect existing usage.

### Q: Can I mix old and new code?

**Yes.** New features are opt-in. Old code using direct queue construction works alongside new code using the factory.

---

## Version History

| Version | Changes |
|---------|---------|
| Phase 1 | Added `queue_capabilities` and `queue_capabilities_interface` |
| Phase 2 | Extended `job_queue` with capability interface |
| Phase 3 | Extended `lockfree_job_queue` with `scheduler_interface` |
| Phase 4 | Added `adaptive_job_queue` |
| Phase 5 | Added `queue_factory` and compile-time selection |
| Phase 6 | Documentation and migration guide |

---

*Last Updated: 2025-12-04*
