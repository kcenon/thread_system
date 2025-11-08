# thread_system Improvement Plan

**Date**: 2025-11-08
**Status**: ⚠️ URGENT - P0 Bug Exists

> ⚠️ **TEMPORARY DOCUMENT**: This improvement plan will be deleted once all action items are completed and changes are integrated into the main documentation.

---

## 📋 Executive Summary

The thread_system is a well-designed C++20 multithreading framework, but it contains **production-blocking bugs (lock-free queue TLS issue)**, **dual error systems**, and **platform-specific hardcoded paths** that require immediate attention.

**Overall Assessment**: B- (Functional but needs hardening)
- Architecture: B
- Concurrency Safety: C+ (P0 bug exists)
- Reusability: B-
- Performance: Unknown (benchmarking needed)

**⚠️ CRITICAL**: Lock-free queue is **PROHIBITED for production use**

---

## 🔴 P0 - Production Blocker Issues

### 1. Lock-Free Queue TLS Destructor Bug

**Problem**:
```cpp
// src/impl/typed_pool/typed_lockfree_job_queue.h:99-117
// ⚠️ DO NOT USE IN PRODUCTION ⚠️
// Thread-local storage destructors may run AFTER node pool is freed
// → segmentation fault
```

**Root Cause**:
- Lock-free node pool uses thread-local caches
- Test fixture destructor runs before TLS destructors
- TLS destructor attempts to return nodes to freed pool → use-after-free

**Impact**:
- **CRITICAL**: Intermittent segfaults in production
- Difficult to reproduce (timing-dependent)

**Solution**:

**Option A: Hazard Pointers (Recommended)**
```cpp
// include/kcenon/thread/core/hazard_pointer.h (new file)
namespace kcenon::thread {
    template<typename T>
    class hazard_pointer_domain {
    public:
        class hazard_pointer {
            void protect(T* ptr);
            void reset();
        };

        void retire(T* ptr);
        void reclaim();
    };
}

// Usage example
class lock_free_queue {
    T* pop() {
        hazard_pointer hp;
        while (true) {
            auto* node = head.load();
            hp.protect(node);  // Protect from reclamation
            if (head.load() != node) continue;
            // ... CAS ...
        }
    }
};
```

**Estimated Effort**: 3 weeks (Senior Developer)

**Option B: Reference Counting (Alternative)**
```cpp
// Add atomic reference count to nodes
struct node {
    std::atomic<int> ref_count{1};
    T data;
    std::atomic<node*> next;
};

void release_node(node* n) {
    if (n->ref_count.fetch_sub(1) == 1) {
        delete n;  // Last reference
    }
}
```

**Estimated Effort**: 1 week (performance degradation concern)

**Option C: Mutex-Based Queue (Temporary Measure)**
```cmake
# CMakeLists.txt
option(THREAD_USE_LOCKFREE_QUEUE "Use lock-free queue (UNSAFE)" OFF)
```

**Immediate Action**: Sprint 1 (Week 1)
**Complete Fix**: Sprint 2-4 (Week 2-8, Hazard Pointers implementation)

---

### 2. Worker Queue Replacement Race Condition

**Problem**:
```cpp
// include/kcenon/thread/core/thread_worker.h:101
auto set_job_queue(std::shared_ptr<job_queue> job_queue) -> void;
// ❌ No synchronization during job_queue_ replacement
```

**Scenario**:
1. Worker thread calls `job_queue_->pop()`
2. Main thread calls `set_job_queue()`
3. Worker accesses freed queue → crash

**Solution**:
```cpp
// thread_worker.h
class thread_worker : public thread_base {
    void set_job_queue(std::shared_ptr<job_queue> new_queue) {
        std::unique_lock lock(queue_mutex_);
        queue_being_replaced_ = true;

        // Wait for current job completion
        queue_cv_.wait(lock, [this] {
            return !currently_processing_job_;
        });

        job_queue_ = std::move(new_queue);
        queue_being_replaced_ = false;
        queue_cv_.notify_all();
    }

private:
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::atomic<bool> currently_processing_job_{false};
    bool queue_being_replaced_{false};
};
```

**Milestone**: Sprint 1 (Week 1-2)

---

### 3. Cancellation Token Callback Race

**Problem**:
```cpp
// include/kcenon/thread/core/cancellation_token.h:190-202
void register_callback(std::function<void()> callback) {
    // Window exists between check and add
    if (is_cancelled()) {
        callback();  // Execute immediately
        return;
    }
    // ⚠️ Can be cancelled here → callback missed
    callbacks_.push_back(std::move(callback));
}
```

**Solution**:
```cpp
void register_callback(std::function<void()> callback) {
    std::unique_lock lock(mutex_);

    if (cancelled_.load(std::memory_order_acquire)) {
        lock.unlock();
        callback();
        return;
    }

    callbacks_.push_back(std::move(callback));
    // Double-check inside lock
    if (cancelled_.load(std::memory_order_acquire)) {
        auto cb = std::move(callbacks_.back());
        callbacks_.pop_back();
        lock.unlock();
        cb();
    }
}
```

**Milestone**: Sprint 1 (Week 2)

---

## 🟡 High Priority Issues

### 4. Dual Error System

**Problem**:
```cpp
// include/kcenon/thread/core/error_handling.h:603-711
namespace thread {
    template<typename T> class result;  // Local
}

#ifdef THREAD_HAS_COMMON_EXECUTOR
    // Conversion overhead to common::Result<T>
#endif
```

**Impact**:
- Conversion overhead (on every operation)
- Code duplication
- Confusing API

**Solution**:

**Phase 1 (Sprint 2)**: Optimize conversion
```cpp
template<typename T>
class result {
    #ifdef THREAD_HAS_COMMON_EXECUTOR
        // Use common::Result<T> internally
        common::Result<T> impl_;
    #else
        std::variant<T, error_info> impl_;
    #endif

    // Provide unified API
    bool is_ok() const { return impl_.is_ok(); }
    // ...
};
```

**Phase 2**: Full integration
```cpp
// Remove error_handling.h
#include <kcenon/common/patterns/result.h>
namespace thread {
    using common::Result;
    using common::VoidResult;
}
```

**Milestone**:
- Sprint 2 (Phase 1)
- Next major version (Phase 2)

---

### 5. Log Level Enumeration Mismatch

**Problem**:
```cpp
// thread_system
enum class log_level {
    critical = 0,  // High → Low
    error = 1,
    // ...
    trace = 5
};

// logger_system / common_system
enum class log_level {
    trace = 0,     // Low → High
    debug = 1,
    // ...
    fatal = 5
};
```

**Impact**:
- Adapter requires conversion logic
- Easy to use wrong level
- Debugging confusion

**Solution**:

**Option A: Unify ordering (Breaking Change)**
```cpp
// Change thread_system to match common_system
enum class log_level {
    trace = 0,
    debug = 1,
    info = 2,
    warn = 3,
    error = 4,
    critical = 5
};
```

**Option B: Strong Type Wrapper**
```cpp
struct thread_log_level {
    enum value { trace, debug, info, warn, error, critical };

    // Explicit conversion only
    explicit constexpr operator common::log_level() const {
        return static_cast<common::log_level>(6 - value_);
    }
};
```

**Recommended**: Option A (apply in next major version)
**Milestone**: Sprint 3 (design), next major version (implementation)

---

### 6. Hardcoded Development Paths

**Problem**:
```cmake
# core/CMakeLists.txt:37-39
"/Users/dongcheolshin/Sources/common_system/include"  # ❌ macOS developer path
"/home/${USER}/Sources/common_system/include"         # ❌ Linux guess
```

**Solution**:
```cmake
# Prefer environment variable
set(COMMON_SYSTEM_HINTS
    $ENV{COMMON_SYSTEM_ROOT}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common_system
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common_system
)

find_path(COMMON_SYSTEM_INCLUDE_DIR
    NAMES kcenon/common/patterns/result.h
    HINTS ${COMMON_SYSTEM_HINTS}
    PATH_SUFFIXES include
)

if(NOT COMMON_SYSTEM_INCLUDE_DIR)
    message(FATAL_ERROR
        "common_system not found. Set COMMON_SYSTEM_ROOT environment variable")
endif()
```

**Milestone**: Sprint 1 (Week 1)

---

## 🟢 Medium Priority Issues

### 7. Service Registry Thread Safety Documentation Missing

**Problem**:
```cpp
// include/kcenon/thread/core/service_registry.h
class service_registry {
    // std::any casting is not atomic
    template<typename T>
    static std::shared_ptr<T> get_service(const std::string& name) {
        std::shared_lock lock(mutex_);
        auto it = services_.find(name);
        if (it != services_.end()) {
            return std::any_cast<std::shared_ptr<T>>(it->second);  // ⚠️
        }
        return nullptr;
    }
};
```

**Solution**:
```cpp
// Enhance documentation
/**
 * @brief Retrieve a registered service by name
 * @thread_safety Thread-safe for concurrent reads and writes
 * @note Service must be registered as std::shared_ptr<T>
 * @warning any_cast failure throws std::bad_any_cast
 */
template<typename T>
static std::shared_ptr<T> get_service(const std::string& name) {
    std::shared_lock lock(mutex_);
    auto it = services_.find(name);
    if (it == services_.end()) {
        return nullptr;
    }

    try {
        return std::any_cast<std::shared_ptr<T>>(it->second);
    } catch (const std::bad_any_cast& e) {
        throw std::runtime_error(
            "Service '" + name + "' type mismatch: " + e.what()
        );
    }
}
```

**Milestone**: Sprint 4 (documentation)

---

### 8. GLOB Pattern Usage (CMake Anti-pattern)

**Problem**:
```cmake
# core/CMakeLists.txt:8
file(GLOB_RECURSE CORE_HEADERS ...)
# Files added/removed don't trigger reconfigure
```

**Solution**:
```cmake
# Explicit file list
set(CORE_HEADERS
    include/kcenon/thread/core/thread_base.h
    include/kcenon/thread/core/thread_pool.h
    include/kcenon/thread/core/thread_worker.h
    # ... explicitly list all files
)

add_library(thread_core INTERFACE ${CORE_HEADERS})
```

**Milestone**: Sprint 5 (refactoring)

---

## 📊 Implementation Roadmap

### Sprint 1: Critical Fixes & Quick Wins (Week 1-2)
**Goal**: Block P0 bugs + immediately fixable issues

- [x] **Task 1.1**: Disable lock-free queue (default to mutex-based) ✅ **COMPLETED**
  - Add warning to CMakeLists.txt
  - Default `THREAD_ENABLE_LOCKFREE_QUEUE=OFF`
  - Document risks
  - **Status**: Already implemented in CMakeLists.txt:39-51

- [x] **Task 1.2**: Add worker queue replacement synchronization ✅ **COMPLETED**
  - Mutex + condition variable (already implemented in thread_worker.cpp:137-151)
  - Unit test (concurrent queue replacement) - created queue_replacement_test.cpp
  - **Status**: Implementation verified, synchronization uses queue_mutex_, queue_cv_, and queue_being_replaced_ flag

- [x] **Task 1.3**: Fix cancellation token race ✅ **COMPLETED**
  - Lock-based synchronization (already implemented in cancellation_token.h:190-202)
  - Unit test (concurrent registration) - created cancellation_token_race_test.cpp
  - **Status**: Implementation verified, uses callback_mutex to prevent race between register_callback() and cancel()

- [x] **Task 1.4**: Remove hardcoded paths ✅ **COMPLETED**
  - CMake `find_path` usage
  - Environment variable support
  - **Status**: Already implemented in `core/CMakeLists.txt:36-43` and `cmake/ThreadSystemDependencies.cmake:31-34`

**Resources**: 2 developers (1 Senior + 1 Mid)
**Risk Level**: Low (conservative fixes)

---

### Sprint 2-4: Hazard Pointers Implementation (Week 2-8)
**Goal**: Enable safe lock-free queue usage

- [x] **Task 2.1**: Hazard Pointer design (Week 2) ✅ **COMPLETED**
  - **Status**: Design document created (docs/HAZARD_POINTER_DESIGN.md)
  - **Algorithm**: Selected Maged Michael's Hazard Pointers
  - **API**: Designed hazard_pointer and hazard_pointer_domain<T>
  - **Performance Goals**: Defined latency/throughput targets
  - **Commit**: 2a813d6f7 "Add Hazard Pointer design document"

- [ ] **Task 2.2**: Basic implementation (Week 3-4)
  - `hazard_pointer_domain<T>` class
  - Thread-local hazard pointer list
  - Retire list management

- [ ] **Task 2.3**: Lock-free queue integration (Week 5-6)
  - Apply HP to `pop()` operation
  - Verify `push()` operation
  - Memory reclamation logic

- [ ] **Task 2.4**: Testing & benchmarking (Week 7-8)
  - Thread safety tests (ThreadSanitizer)
  - Performance benchmark (vs mutex-based)
  - Memory leak verification (Valgrind)

**Resources**: 1 developer (Senior, concurrency expertise required)
**Risk Level**: High (complex concurrency algorithm)

---

### Sprint 3: Error System Unification (Week 6-8)
**Goal**: Integrate error handling with common_system

- [ ] **Task 3.1**: result<T> wrapper implementation
  - Use `common::Result<T>` internally
  - Maintain API compatibility
  - Measure performance (conversion overhead)

- [ ] **Task 3.2**: Deprecation warnings
  - Mark `thread::result<T>` deprecated
  - Write migration guide

- [ ] **Task 3.3**: Log level enum unification plan
  - Write breaking change RFC
  - Plan for next major version

**Resources**: 1 developer (Mid-level)
**Risk Level**: Medium (API changes)

---

### Sprint 4-5: Documentation & Cleanup (Week 9-12)
**Goal**: Improve code quality, address technical debt

- [ ] **Task 4.1**: Thread safety documentation
  - Add `@thread_safety` tags to all public methods
  - Document synchronization contracts
  - Provide example code

- [ ] **Task 4.2**: CMake improvements
  - Remove GLOB
  - Explicit file lists
  - Clean up build options

- [ ] **Task 4.3**: Deprecated interface cleanup
  - Finalize `executor_interface`, `logger_interface` status
  - Plan for next major version removal

**Resources**: 1 developer (Junior)
**Risk Level**: Low

---

## 🔬 Testing Strategy

### P0 Bug Verification
```cpp
// tests/lockfree_queue_stress_test.cpp
TEST(LockFreeQueue, TLSDestructorSafety) {
    // Repeat 1000 times: create queue → spawn threads → destruct
    for (int i = 0; i < 1000; ++i) {
        auto queue = std::make_unique<lockfree_job_queue>();
        std::vector<std::thread> threads;

        for (int t = 0; t < 10; ++t) {
            threads.emplace_back([&] {
                queue->push(/* job */);
            });
        }

        for (auto& t : threads) t.join();
        // No crash on queue destruction
    }
}
```

### Concurrency Tests
```cpp
// Run with ThreadSanitizer
TEST(ThreadWorker, ConcurrentQueueReplacement) {
    thread_worker worker;
    std::atomic<bool> stop{false};

    // Producer thread
    std::thread producer([&] {
        while (!stop) {
            worker.set_job_queue(make_new_queue());
        }
    });

    // Worker thread
    worker.start();
    std::this_thread::sleep_for(5s);
    stop = true;

    producer.join();
    worker.stop();
    // No data races
}
```

### Performance Benchmarks
```cpp
// benchmarks/queue_performance.cpp
static void BM_MutexQueue(benchmark::State& state) {
    for (auto _ : state) {
        // Benchmark logic
    }
}
BENCHMARK(BM_MutexQueue)->Threads(8);

static void BM_LockFreeQueueWithHP(benchmark::State& state) {
    for (auto _ : state) {
        // Benchmark logic
    }
}
BENCHMARK(BM_LockFreeQueueWithHP)->Threads(8);
```

---

## 📈 Success Metrics

1. **Crash Rate**: 0 segfaults related to lock-free queue
2. **Thread Safety**: 0 ThreadSanitizer warnings
3. **Performance**:
   - Hazard Pointer queue 30%+ faster than mutex-based
   - Or maintain mutex-based (safety priority)
4. **API Stability**: Breaking changes in major versions only
5. **Documentation**: All synchronization contracts documented

---

## 🚧 Risk Mitigation

### Lock-Free Algorithm Complexity
- **Risk**: Hazard Pointer implementation failure
- **Mitigation**:
  - Reference proven libraries (libcds, Folly)
  - Incremental implementation (basic HP → queue integration)
  - Fallback plan (maintain mutex-based)

### Breaking Changes
- **Risk**: Existing user code breaks
- **Mitigation**:
  - Adhere to semantic versioning
  - 6-month deprecation period
  - Provide automatic migration scripts

### Performance Degradation
- **Risk**: Hazard Pointer overhead
- **Mitigation**:
  - Set benchmark baselines
  - Keep configurable build options

---

## 📚 Reference Documents

1. **Known Issues**: `/Users/raphaelshin/Sources/thread_system/KNOWN_ISSUES.md`
2. **Architecture**: `/Users/raphaelshin/Sources/thread_system/docs/ARCHITECTURE.md`
3. **Hazard Pointers Paper**: Maged M. Michael, "Hazard Pointers: Safe Memory Reclamation for Lock-Free Objects" (2004)

---

## ✅ Acceptance Criteria

### Sprint 1 Completion:
- [ ] Lock-free queue disabled by default
- [ ] Worker queue race fixed + tests passing
- [ ] Cancellation token race fixed + tests passing
- [ ] CMake path issues resolved
- [ ] ThreadSanitizer clean run

### Sprint 2-4 Completion:
- [ ] Hazard Pointer implementation complete
- [ ] Lock-free queue integration complete
- [ ] Performance benchmark goals achieved
- [ ] Memory safety verified (Valgrind)

---

**Next Review**: In 1 week (before Sprint 1 starts)
**Responsibility**: Senior Developer (Concurrency Expert)
**Status**: ⚠️ URGENT - P0 Bug Exists
