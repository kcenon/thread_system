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

- [x] **Task 2.2**: Basic implementation (Week 3-4) ✅ **COMPLETED**
  - **Status**: Core implementation complete with full test coverage
  - **Implementation**: hazard_pointer and hazard_pointer_domain<T> classes
  - **Features**:
    - Thread-local hazard arrays (4 slots per thread)
    - Slot ownership tracking with SLOT_OWNED_MARKER
    - Automatic reclamation (threshold: 64 objects)
    - Statistics tracking (retired/reclaimed/scan count)
  - **Testing**: 13 unit tests, all passing (100%)
  - **Files**:
    - include/kcenon/thread/core/hazard_pointer.h
    - src/core/hazard_pointer.cpp
    - unittest/thread_base_test/hazard_pointer_test.cpp
  - **Commit**: e1972ed43 "Implement Hazard Pointer for safe lock-free queue"

- [x] **Task 2.3**: Lock-free queue integration (Week 5-6) ✅ **COMPLETED**
  - **Status**: Lock-free MPMC queue with Hazard Pointers implemented
  - **Algorithm**: Michael-Scott queue with HP-based memory reclamation
  - **Features**:
    - Protect-then-verify pattern for safe concurrent access
    - CAS-before-read to prevent data races
    - Automatic HP reclamation (threshold: 64 objects)
    - No TLS node pool (eliminates P0 bug)
  - **Tests**: 10/10 passing (100% success rate)
    - BasicEnqueueDequeue, DequeueEmpty, NullJobRejection
    - MultipleOperations, ConcurrentEnqueue, ConcurrentDequeue
    - ConcurrentMPMC, HazardPointerReclamation
    - DestructionWithPendingJobs, StressTest
  - **Files**:
    - include/kcenon/thread/lockfree/lockfree_job_queue.h
    - src/lockfree/lockfree_job_queue.cpp
    - unittest/thread_base_test/lockfree_job_queue_test.cpp
  - **Commit**: 5728126ea "Implement lock-free MPMC queue with Hazard Pointers"

- [x] **Task 2.4**: Testing & benchmarking (Week 7-8) ✅
  - ✅ Thread safety tests (ThreadSanitizer)
    - Fixed 2 critical data races:
      1. Non-atomic `bool active` in thread_hazard_list
      2. Unsafe `reclaim_all()` during thread cleanup
    - All 13 HazardPointerTest.* pass cleanly
    - All 10 LockFreeJobQueueTest.* pass cleanly
  - ✅ Full test suite verification
    - All 89 tests pass without regressions
  - ✅ Performance benchmark (vs mutex-based)
    - Lock-free: 71 μs/operation
    - Mutex-based: 291 μs/operation
    - **4x performance improvement**
  - ⚠️ Memory leak verification (Valgrind) - Deferred
    - macOS Valgrind support limited
    - Will verify on Linux in Sprint 3
  - **Commit**: 01bab9a5e "Fix ThreadSanitizer data races in hazard pointer implementation"

**Resources**: 1 developer (Senior, concurrency expertise required)
**Risk Level**: High (complex concurrency algorithm)

---

### Sprint 3: Error System Unification (Week 6-8)
**Goal**: Integrate error handling with common_system

- [x] **Task 3.1**: result<T> wrapper implementation ✅ **COMPLETED** (2025-11-09)
  - **Status**: Implemented Phase 1 integration with common::Result<T>
  - **Implementation**: Modified result<T> to use common::Result<T> internally when THREAD_HAS_COMMON_RESULT is defined
  - **API Compatibility**: Fully maintained - all 89 tests pass without modification
  - **Performance**: Zero overhead when common_system not available, minimal conversion overhead when available
  - **Files Modified**: include/kcenon/thread/core/error_handling.h
  - **Commit**: 8eb40047 "Unify error handling with common_system (Phase 1)"

- [x] **Task 3.2**: Deprecation warnings ✅ **COMPLETED** (2025-11-09)
  - **Status**: Added deprecation notice and comprehensive migration guide
  - **Documentation**: docs/ERROR_SYSTEM_MIGRATION_GUIDE.md
  - **Content**:
    - Phase 1/2/3 migration plan
    - API mapping table (thread::result<T> → common::Result<T>)
    - Step-by-step migration instructions
    - Error code mapping
    - Performance considerations
  - **Commit**: 8eb40047

- [x] **Task 3.3**: Log level enum unification plan ✅ **COMPLETED** (2025-11-09)
  - **Status**: Created comprehensive RFC for log level standardization
  - **Documentation**: docs/LOG_LEVEL_UNIFICATION_PLAN.md
  - **Content**:
    - Problem analysis (descending vs ascending order)
    - Proposed solution (adopt standard ascending order)
    - 3-phase migration plan
    - Breaking changes documentation
    - Testing strategy
    - Implementation checklist
  - **Decision**: Awaiting approval for next minor version
  - **Commit**: 8eb40047

**Resources**: 1 developer (Mid-level)
**Risk Level**: Medium (API changes)
**Status**: ✅ **SPRINT 3 COMPLETED** (2025-11-09)

---

### Sprint 4-5: Documentation & Cleanup (Week 9-12)
**Goal**: Improve code quality, address technical debt

- [x] **Task 4.1**: Thread safety documentation ✅ **COMPLETED** (2025-11-09)
  - **Status**: Enhanced service_registry.h with comprehensive thread safety documentation
  - **Changes**:
    - Added `@thread_safety` tags to class and all public methods
    - Documented synchronization behavior (shared vs exclusive locks)
    - Added usage examples and warnings
    - Improved error handling with try-catch for any_cast
  - **Commit**: d87c30e "Improve code documentation and build system maintainability"

- [x] **Task 4.2**: CMake improvements ✅ **COMPLETED** (2025-11-09)
  - **Status**: Replaced GLOB with explicit file lists in core/CMakeLists.txt
  - **Changes**:
    - Converted GLOB_RECURSE to explicit set() for headers (26 files)
    - Converted GLOB_RECURSE to explicit set() for sources (14 files)
    - Added comments explaining rationale
    - CMake will now properly detect when files are added/removed
  - **Build**: Successfully tested and verified
  - **Commit**: d87c30e

- [x] **Task 4.3**: Deprecated interface cleanup ✅ **ALREADY COMPLETED**
  - **Status**: executor_interface and logger_interface already properly deprecated
  - **Files**:
    - include/kcenon/thread/interfaces/executor_interface.h
    - include/kcenon/thread/interfaces/logger_interface.h
  - **Features**:
    - [[deprecated]] attributes applied
    - Comprehensive migration guides included
    - Removal planned for v2.0
    - Timeline documented (minimum 6-month deprecation period)

**Resources**: 1 developer (Junior)
**Risk Level**: Low
**Status**: ✅ **SPRINT 4-5 COMPLETED** (2025-11-09)

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
