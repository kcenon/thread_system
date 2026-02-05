# Hazard Pointer Design Document

**Version**: 0.1.0.0
**Date**: 2025-11-08
**Status**: Design Phase
**Author**: Development Team

---

## 1. Overview

### 1.1 Purpose

This document describes the design and implementation strategy for Hazard Pointers in the thread_system, which will enable safe lock-free queue operations by solving the ABA problem and preventing use-after-free issues.

### 1.2 Background

The current lock-free queue implementation has a critical P0 bug where thread-local storage destructors run after the node pool is freed, causing segmentation faults. Hazard Pointers provide a safe memory reclamation mechanism for lock-free data structures without garbage collection.

### 1.3 Goals

- **Primary**: Enable safe lock-free queue usage in production
- **Performance**: Maintain competitive performance with mutex-based alternatives
- **Safety**: Zero use-after-free errors, verified by ThreadSanitizer
- **Simplicity**: Provide a clean API that's easy to use correctly

---

## 2. Algorithm Selection

### 2.1 Chosen Algorithm: Maged Michael's Hazard Pointers

**Reference**: Maged M. Michael, "Hazard Pointers: Safe Memory Reclamation for Lock-Free Objects" (IEEE TPDS, 2004)

**Rationale**:
- Well-tested and proven in production systems
- Bounded memory overhead
- No dependency on garbage collection
- Compatible with C++ RAII patterns
- Simpler than alternatives (RCU, Epoch-Based Reclamation)

### 2.2 Core Concepts

1. **Hazard Pointer**: A single-writer, multi-reader pointer that protects an object from reclamation
2. **Thread-Local Hazard List**: Each thread maintains a small array of hazard pointers
3. **Retire List**: Objects pending deletion are added to a retire list
4. **Scan and Reclaim**: Periodically scan all hazard pointers and reclaim safe objects

### 2.3 Comparison with Alternatives

| Approach | Memory Overhead | Complexity | Performance | GC Required |
|----------|----------------|------------|-------------|-------------|
| **Hazard Pointers** | O(T*H) bounded | Medium | Good | No |
| Reference Counting | Per-object atomic | Low | Slower | No |
| Epoch-Based Reclamation | O(T*E) | High | Best | No |
| Manual Management | Minimal | Very High | Best | No |

Where:
- T = number of threads
- H = hazard pointers per thread (typically 2-4)
- E = epochs (typically 3)

---

## 3. API Design

### 3.1 Core Classes

#### 3.1.1 `hazard_pointer`

```cpp
namespace kcenon::thread {

/// Single hazard pointer that protects one object
class hazard_pointer {
public:
    /// Default constructor
    hazard_pointer();

    /// Move constructor
    hazard_pointer(hazard_pointer&& other) noexcept;

    /// Destructor - automatically releases protection
    ~hazard_pointer();

    /// Non-copyable
    hazard_pointer(const hazard_pointer&) = delete;
    hazard_pointer& operator=(const hazard_pointer&) = delete;

    /// Protect a pointer from reclamation
    /// @param ptr Pointer to protect
    /// @note Thread-safe, can be called concurrently
    template<typename T>
    void protect(T* ptr) noexcept;

    /// Release protection
    /// @note After reset(), the previously protected pointer may be reclaimed
    void reset() noexcept;

    /// Check if currently protecting a pointer
    bool is_protected() const noexcept;

    /// Get the protected pointer (may be null)
    void* get_protected() const noexcept;

private:
    std::atomic<void*>* slot_;  // Pointer to thread-local hazard slot
};

} // namespace kcenon::thread
```

#### 3.1.2 `hazard_pointer_domain<T>`

```cpp
namespace kcenon::thread {

/// Domain managing hazard pointers and retirement for a specific type
/// @tparam T Type of objects protected by this domain
template<typename T>
class hazard_pointer_domain {
public:
    /// Get the global domain instance for type T
    static hazard_pointer_domain& global();

    /// Acquire a hazard pointer for this domain
    /// @return Hazard pointer that will automatically release on destruction
    hazard_pointer acquire() noexcept;

    /// Retire an object for later reclamation
    /// @param ptr Pointer to object to retire
    /// @note Object will be deleted when no hazard pointers protect it
    void retire(T* ptr) noexcept;

    /// Force reclamation scan (optional, for testing)
    /// @return Number of objects reclaimed
    size_t reclaim() noexcept;

    /// Get statistics
    struct stats {
        size_t hazard_pointers_allocated;
        size_t objects_retired;
        size_t objects_reclaimed;
        size_t scan_count;
    };
    stats get_stats() const noexcept;

private:
    hazard_pointer_domain();
    ~hazard_pointer_domain();

    class impl;
    std::unique_ptr<impl> impl_;
};

} // namespace kcenon::thread
```

### 3.2 Usage Examples

#### 3.2.1 Basic Protection Pattern

```cpp
template<typename T>
T* lock_free_stack<T>::pop() {
    auto hp = hazard_pointer_domain<node>::global().acquire();

    while (true) {
        node* old_head = head_.load(std::memory_order_acquire);
        if (!old_head) {
            return nullptr;  // Stack is empty
        }

        // Protect the head node
        hp.protect(old_head);

        // Verify head hasn't changed (ABA protection)
        if (head_.load(std::memory_order_acquire) != old_head) {
            continue;  // Retry
        }

        // Now old_head is safely protected
        node* new_head = old_head->next.load(std::memory_order_relaxed);

        if (head_.compare_exchange_weak(old_head, new_head,
                                        std::memory_order_release,
                                        std::memory_order_relaxed)) {
            T* result = &old_head->data;

            // Retire the node (will be deleted when safe)
            hazard_pointer_domain<node>::global().retire(old_head);

            return result;
        }
    }
}
```

#### 3.2.2 Multiple Hazard Pointers

```cpp
// Protecting multiple nodes in a linked list
auto hp1 = hazard_pointer_domain<node>::global().acquire();
auto hp2 = hazard_pointer_domain<node>::global().acquire();

node* curr = head_.load();
hp1.protect(curr);

while (curr) {
    node* next = curr->next.load();
    hp2.protect(next);

    // Both curr and next are protected
    process(curr);

    // Move forward
    hp1.protect(next);
    curr = next;
}
```

### 3.3 RAII Integration

The design leverages RAII for automatic cleanup:

```cpp
{
    auto hp = hazard_pointer_domain<node>::global().acquire();
    hp.protect(some_pointer);

    // Use the pointer...

} // hp destructor automatically releases protection
```

---

## 4. Implementation Strategy

### 4.1 Data Structures

#### 4.1.1 Thread-Local Hazard Array

```cpp
struct thread_hazard_list {
    static constexpr size_t MAX_HAZARDS = 4;

    std::atomic<void*> hazards[MAX_HAZARDS];
    thread_hazard_list* next;  // Linked list of all thread lists

    thread_hazard_list() {
        for (auto& h : hazards) {
            h.store(nullptr, std::memory_order_relaxed);
        }
    }
};
```

#### 4.1.2 Retire List

```cpp
struct retire_list {
    struct retire_node {
        void* ptr;
        std::function<void(void*)> deleter;
        retire_node* next;
    };

    retire_node* head = nullptr;
    size_t count = 0;

    static constexpr size_t THRESHOLD = 2 * MAX_HAZARDS * MAX_THREADS;
};
```

### 4.2 Key Algorithms

#### 4.2.1 Protection Algorithm

```
protect(ptr):
    1. Find an available hazard slot in thread-local array
    2. Store ptr in the slot using memory_order_release
    3. Memory fence (ensures visibility to other threads)
    4. Return handle to the slot
```

#### 4.2.2 Retirement Algorithm

```
retire(ptr):
    1. Add ptr to thread-local retire list
    2. If retire list size >= THRESHOLD:
        a. Scan all threads' hazard pointers
        b. Build a set of protected pointers
        c. For each retired pointer:
            - If NOT in protected set: delete it
            - If in protected set: keep in retire list
```

#### 4.2.3 Scan and Reclaim Algorithm

```
scan_and_reclaim():
    1. plist = {}  // Protected list
    2. For each thread's hazard array:
        For each hazard pointer:
            If ptr != null:
                Add ptr to plist

    3. For each retired object:
        If object NOT in plist:
            Delete object
            Remove from retire list
```

### 4.3 Memory Ordering

| Operation | Memory Order | Rationale |
|-----------|--------------|-----------|
| `protect(ptr)` | `release` | Ensure ptr write is visible to scanners |
| `reset()` | `release` | Ensure release is visible |
| Retire list read | `acquire` | See all retired pointers |
| Hazard scan read | `acquire` | See all protected pointers |

---

## 5. Performance Goals

### 5.1 Latency Targets

| Operation | Target | Measurement Method |
|-----------|--------|-------------------|
| `acquire()` | < 20 ns | Google Benchmark |
| `protect()` | < 10 ns | Single atomic store |
| `reset()` | < 10 ns | Single atomic store |
| `retire()` | < 50 ns (no scan) | Add to list |
| `reclaim()` | < 1 μs (100 objects) | Periodic scan |

### 5.2 Throughput Targets

| Workload | Target | Baseline (Mutex) |
|----------|--------|------------------|
| Single-threaded push/pop | > 10M ops/sec | 5M ops/sec |
| 4-thread concurrent | > 8M ops/sec | 2M ops/sec |
| 8-thread concurrent | > 10M ops/sec | 1M ops/sec |

### 5.3 Memory Overhead

| Component | Memory Usage |
|-----------|--------------|
| Per-thread hazard array | 32 bytes (4 pointers * 8 bytes) |
| Global thread list | O(T) * 32 bytes |
| Per-thread retire list | O(R) * 24 bytes (ptr + deleter + next) |
| **Total per thread** | ~256 bytes |

Where:
- T = number of threads
- R = retired objects (typically < 100)

---

## 6. Testing Strategy

### 6.1 Unit Tests

```cpp
// Test basic protection
TEST(HazardPointer, BasicProtection) {
    auto hp = hazard_pointer_domain<int>::global().acquire();
    int* ptr = new int(42);

    hp.protect(ptr);
    EXPECT_EQ(hp.get_protected(), ptr);

    hp.reset();
    EXPECT_EQ(hp.get_protected(), nullptr);
}

// Test retirement and reclamation
TEST(HazardPointer, RetireAndReclaim) {
    int* ptr = new int(42);
    hazard_pointer_domain<int>::global().retire(ptr);

    // Force reclamation
    auto reclaimed = hazard_pointer_domain<int>::global().reclaim();
    EXPECT_GE(reclaimed, 1);
}

// Test protection prevents reclamation
TEST(HazardPointer, ProtectionPreventsReclaim) {
    auto hp = hazard_pointer_domain<int>::global().acquire();
    int* ptr = new int(42);

    hp.protect(ptr);
    hazard_pointer_domain<int>::global().retire(ptr);

    auto reclaimed = hazard_pointer_domain<int>::global().reclaim();
    EXPECT_EQ(reclaimed, 0);  // Should not reclaim protected pointer

    hp.reset();
    reclaimed = hazard_pointer_domain<int>::global().reclaim();
    EXPECT_GE(reclaimed, 1);  // Now it can be reclaimed
}
```

### 6.2 Concurrency Tests

```cpp
TEST(HazardPointer, ConcurrentRetirement) {
    std::vector<std::thread> threads;

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([i] {
            for (int j = 0; j < 1000; ++j) {
                int* ptr = new int(i * 1000 + j);
                hazard_pointer_domain<int>::global().retire(ptr);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // All objects should eventually be reclaimed
    auto stats = hazard_pointer_domain<int>::global().get_stats();
    EXPECT_EQ(stats.objects_retired, stats.objects_reclaimed);
}
```

### 6.3 Integration Tests

```cpp
TEST(LockFreeQueue, WithHazardPointers) {
    lockfree_job_queue queue;
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    // Start producers and consumers
    for (int i = 0; i < 4; ++i) {
        producers.emplace_back([&] {
            for (int j = 0; j < 10000; ++j) {
                queue.push(std::make_unique<test_job>());
            }
        });

        consumers.emplace_back([&] {
            for (int j = 0; j < 10000; ++j) {
                auto job = queue.pop();
                if (job) {
                    job->execute();
                }
            }
        });
    }

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();

    // No crashes, no leaks
}
```

### 6.4 Sanitizer Verification

```bash
# ThreadSanitizer (detect data races)
cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread" ..
ctest --output-on-failure

# AddressSanitizer (detect memory errors)
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address" ..
ctest --output-on-failure

# MemorySanitizer (detect uninitialized reads)
cmake -DCMAKE_CXX_FLAGS="-fsanitize=memory" ..
ctest --output-on-failure
```

---

## 7. Implementation Phases

### Phase 1: Basic Implementation (Week 3-4)

**Deliverables**:
- `hazard_pointer` class
- `hazard_pointer_domain<T>` class
- Thread-local hazard array management
- Basic retire list

**Tests**:
- Unit tests for protection/retirement
- Single-threaded correctness tests

### Phase 2: Lock-Free Queue Integration (Week 5-6)

**Deliverables**:
- Integrate HP into `typed_lockfree_job_queue`
- Update `pop()` operation with protection
- Remove TLS node pool (root cause of P0 bug)

**Tests**:
- Queue-specific integration tests
- Multi-threaded stress tests

### Phase 3: Optimization & Tuning (Week 7-8)

**Deliverables**:
- Performance benchmarks
- Tuning parameters (THRESHOLD, MAX_HAZARDS)
- Memory leak verification

**Tests**:
- Performance regression tests
- Long-running stability tests (24+ hours)
- Memory profiling (Valgrind, Heaptrack)

---

## 8. Risks and Mitigation

### 8.1 Performance Risk

**Risk**: Hazard Pointer overhead may exceed mutex-based performance

**Mitigation**:
- Establish baseline benchmarks first
- Set "good enough" criteria (e.g., within 20% of mutex)
- Keep mutex-based option available
- Profile hot paths and optimize

### 8.2 Complexity Risk

**Risk**: Implementation bugs in lock-free code are hard to debug

**Mitigation**:
- Extensive testing with sanitizers
- Reference implementation study (Folly, libcds)
- Code review by concurrency expert
- Incremental development with tests at each step

### 8.3 ABA Problem

**Risk**: ABA problem may still occur despite hazard pointers

**Mitigation**:
- Use double-checked pattern (protect → verify)
- Consider tagged pointers if needed
- Extensive stress testing

---

## 9. References

### 9.1 Papers

1. Maged M. Michael, "Hazard Pointers: Safe Memory Reclamation for Lock-Free Objects", IEEE TPDS, 2004
2. Maurice Herlihy & Nir Shavit, "The Art of Multiprocessor Programming", 2nd Edition
3. Anthony Williams, "C++ Concurrency in Action", 2nd Edition

### 9.2 Implementations

1. **Folly** (Facebook): `folly::hazptr`
   - https://github.com/facebook/folly/blob/main/folly/synchronization/Hazptr.h
2. **libcds**: Concurrent Data Structures library
   - https://github.com/khizmax/libcds
3. **ConcurrencyKit**: Lock-free primitives
   - https://github.com/concurrencykit/ck

### 9.3 Resources

1. CppCon 2014: "Lock-Free Programming (or, Juggling Razor Blades)" - Herb Sutter
2. CppCon 2017: "A Foundation for Lock-free Data Structures" - Paul E. McKenney
3. ISO C++ Concurrency Study Group (SG1) Papers on Hazard Pointers

---

## 10. Acceptance Criteria

### 10.1 Functional Requirements

- [ ] Hazard pointer protects object from reclamation
- [ ] Retired objects are eventually reclaimed
- [ ] Multiple threads can use hazard pointers concurrently
- [ ] No memory leaks (verified by Valgrind)
- [ ] No data races (verified by ThreadSanitizer)

### 10.2 Performance Requirements

- [ ] Single-threaded: > 5M ops/sec
- [ ] Multi-threaded (4 threads): > 4M ops/sec
- [ ] Memory overhead: < 1KB per thread
- [ ] Reclamation latency: < 1μs for 100 objects

### 10.3 Code Quality Requirements

- [ ] Full API documentation
- [ ] All public APIs have usage examples
- [ ] 90%+ code coverage
- [ ] All tests pass on Linux, macOS, Windows
- [ ] Sanitizer-clean (Thread, Address, Memory)

---

**Document Version History**:
- v1.0 (2025-11-08): Initial design document

**Next Steps**:
1. Review this design with team
2. Begin Phase 1 implementation
3. Create tracking issues for each phase
