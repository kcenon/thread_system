# Migration Guide - Thread System

> **Language:** **English** | [한국어](MIGRATION.kr.md)

## Overview

This guide helps you migrate to thread_system from previous threading solutions, including std::thread, OpenMP, TBB, or older versions of thread_system.

**Migration Paths Available:**
1. From std::thread to thread_system
2. From OpenMP/TBB to thread_system
3. From older thread_system versions to latest version
4. From custom threading implementations to thread_system

**Estimated Time:** 2-6 hours for typical applications
**Breaking Changes:** Yes (API, namespace, build configuration for version upgrades)

---

## Table of Contents

- [Quick Migration Checklist](#quick-migration-checklist)
- [Migration Path 1: From std::thread](#migration-path-1-from-stdthread)
- [Migration Path 2: From OpenMP/TBB](#migration-path-2-from-openmptbb)
- [Migration Path 3: From Older thread_system](#migration-path-3-from-older-thread_system)
- [Migration Path 4: From Custom Threading](#migration-path-4-from-custom-threading)
- [Common Migration Issues](#common-migration-issues)
- [Testing Your Migration](#testing-your-migration)
- [Rollback Plan](#rollback-plan)

---

## Quick Migration Checklist

- [ ] Update CMakeLists.txt dependencies
- [ ] Replace header includes
- [ ] Update namespace references
- [ ] Migrate thread creation code
- [ ] Update synchronization primitives
- [ ] Rebuild and test
- [ ] Update integration points
- [ ] Performance benchmark comparison

---

## Migration Path 1: From std::thread

### Background

Standard library threading requires manual thread management, synchronization, and workload distribution. thread_system provides automatic thread pooling, adaptive queuing, and comprehensive features.

### Step 1: Update CMakeLists.txt

**Before** (std::thread):
```cmake
cmake_minimum_required(VERSION 3.16)
project(YourApplication)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(your_app main.cpp)
target_link_libraries(your_app PRIVATE Threads::Threads)
```

**After** (thread_system):
```cmake
cmake_minimum_required(VERSION 3.16)
project(YourApplication)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find thread_system
find_package(thread_system CONFIG REQUIRED)

add_executable(your_app main.cpp)

target_link_libraries(your_app PRIVATE
    kcenon::thread_system
    Threads::Threads  # Still needed for std::thread support
)
```

### Step 2: Update Header Includes

**Before**:
```cpp
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
```

**After**:
```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/callback_job.h>
#include <kcenon/thread/core/sync_primitives.h>
```

### Step 3: Migrate Thread Creation

**Before** (std::thread):
```cpp
#include <thread>
#include <vector>
#include <mutex>
#include <queue>
#include <condition_variable>

class WorkQueue {
    std::queue<std::function<void()>> tasks;
    std::mutex mutex;
    std::condition_variable cv;
    std::vector<std::thread> workers;
    bool stop{false};

public:
    WorkQueue(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        cv.wait(lock, [this]() { return stop || !tasks.empty(); });

                        if (stop && tasks.empty()) return;

                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    void enqueue(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            tasks.push(std::move(task));
        }
        cv.notify_one();
    }

    ~WorkQueue() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            stop = true;
        }
        cv.notify_all();
        for (auto& worker : workers) {
            worker.join();
        }
    }
};
```

**After** (thread_system):
```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/callback_job.h>

using namespace kcenon::thread;

// Create thread pool
auto pool = std::make_shared<thread_pool>("worker_pool");

// Add workers
std::vector<std::unique_ptr<thread_worker>> workers;
for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
    workers.push_back(std::make_unique<thread_worker>());
}
pool->enqueue_batch(std::move(workers));

// Start processing
pool->start();

// Submit tasks (convenience API)
pool->submit_task([]() {
    // Your task code here
});

// Or use job-based API for advanced features
pool->enqueue(std::make_unique<callback_job>([]() -> result_void {
    // Your job code here
    return {};
}));

// Graceful shutdown (waits for current tasks)
pool->shutdown();
```

**Benefits**:
- **No manual thread management**: Automatic lifecycle handling
- **Adaptive queuing**: Automatic optimization for contention
- **Error handling**: Result<T> pattern for robust error management
- **Performance monitoring**: Built-in metrics collection
- **100+ lines reduced to ~20 lines**

### Step 4: Migrate Synchronization

**Before** (std::mutex):
```cpp
std::mutex mutex;
std::condition_variable cv;
std::atomic<bool> ready{false};

// Lock with timeout
std::unique_lock<std::mutex> lock(mutex, std::defer_lock);
if (!lock.try_lock_for(std::chrono::milliseconds(100))) {
    // Timeout handling
}

// Condition variable with predicate
cv.wait(lock, []() { return ready.load(); });
```

**After** (thread_system):
```cpp
#include <kcenon/thread/core/sync_primitives.h>
#include <kcenon/thread/core/cancellation_token.h>

// Lock with timeout (RAII)
scoped_lock_guard lock(mutex, std::chrono::milliseconds(100));
if (!lock.owns_lock()) {
    // Timeout handling
}

// Enhanced condition variable
condition_variable_wrapper cv;
cv.wait(lock, []() { return condition_met; });

// Cooperative cancellation
auto token = cancellation_token::create();

// In worker thread
while (!token.is_cancelled()) {
    // Do work
    token.throw_if_cancelled();  // Throws if cancelled
}

// From another thread
token.cancel();  // Request cancellation
```

**Benefits**:
- **RAII with timeout**: Automatic lock release
- **Predicate-based CV**: Cleaner condition handling
- **Cooperative cancellation**: Graceful task termination
- **Thread-safe by design**: Built-in safety guarantees

### Step 5: Performance Comparison

Run benchmarks to verify performance improvements:

```cpp
// Benchmark std::thread approach
auto start = std::chrono::high_resolution_clock::now();
{
    WorkQueue queue(8);
    for (int i = 0; i < 100000; ++i) {
        queue.enqueue([]() { /* work */ });
    }
}
auto end = std::chrono::high_resolution_clock::now();
auto std_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

// Benchmark thread_system
start = std::chrono::high_resolution_clock::now();
{
    auto pool = std::make_shared<thread_pool>("benchmark");
    std::vector<std::unique_ptr<thread_worker>> workers;
    for (size_t i = 0; i < 8; ++i) {
        workers.push_back(std::make_unique<thread_worker>());
    }
    pool->enqueue_batch(std::move(workers));
    pool->start();

    for (int i = 0; i < 100000; ++i) {
        pool->submit_task([]() { /* work */ });
    }

    while (pool->get_pending_task_count() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    pool->shutdown();
}
end = std::chrono::high_resolution_clock::now();
auto thread_system_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

std::cout << "std::thread: " << std_duration.count() << " ms\n";
std::cout << "thread_system: " << thread_system_duration.count() << " ms\n";
std::cout << "Improvement: " << (1.0 - double(thread_system_duration.count()) / std_duration.count()) * 100 << "%\n";
```

**Expected Results**:
- **Throughput improvement**: 2-3x faster for typical workloads
- **Latency reduction**: Sub-microsecond job scheduling (77ns)
- **Scalability**: Near-linear scaling up to hardware concurrency

---

## Migration Path 2: From OpenMP/TBB

### Overview

Migrating from OpenMP or Intel TBB to thread_system provides better C++ integration, adaptive performance, and comprehensive features.

### From OpenMP

**Before** (OpenMP):
```cpp
#include <omp.h>

void process_data(const std::vector<int>& data) {
    #pragma omp parallel for num_threads(8)
    for (size_t i = 0; i < data.size(); ++i) {
        // Process data[i]
        expensive_operation(data[i]);
    }
}
```

**After** (thread_system):
```cpp
#include <kcenon/thread/core/thread_pool.h>

void process_data(const std::vector<int>& data) {
    auto pool = std::make_shared<thread_pool>("data_processor");

    // Add workers
    std::vector<std::unique_ptr<thread_worker>> workers;
    for (size_t i = 0; i < 8; ++i) {
        workers.push_back(std::make_unique<thread_worker>());
    }
    pool->enqueue_batch(std::move(workers));
    pool->start();

    // Submit jobs
    for (const auto& item : data) {
        pool->submit_task([item]() {
            expensive_operation(item);
        });
    }

    // Wait for completion
    while (pool->get_pending_task_count() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    pool->shutdown();
}
```

**Benefits over OpenMP**:
- **Better C++ integration**: Type-safe, no compiler pragmas
- **Adaptive performance**: Automatic queue optimization
- **Fine-grained control**: Per-job cancellation, priority
- **Production features**: Error handling, monitoring, logging

### From Intel TBB

**Before** (TBB):
```cpp
#include <tbb/task_arena.h>
#include <tbb/parallel_for.h>

tbb::task_arena arena(8);  // 8 threads

arena.execute([&]() {
    tbb::parallel_for(tbb::blocked_range<size_t>(0, data.size()),
        [&](const tbb::blocked_range<size_t>& r) {
            for (size_t i = r.begin(); i < r.end(); ++i) {
                expensive_operation(data[i]);
            }
        });
});
```

**After** (thread_system with typed pool for priority):
```cpp
#include <kcenon/thread/core/typed_thread_pool.h>

using namespace kcenon::thread;

// Create typed pool for priority-based processing
auto pool = std::make_shared<typed_thread_pool_t<job_types>>("priority_processor");

// Add workers with responsibility lists
std::vector<std::unique_ptr<typed_thread_worker_t<job_types>>> workers;
for (size_t i = 0; i < 8; ++i) {
    auto worker = std::make_unique<typed_thread_worker_t<job_types>>();
    pool->add_worker(std::move(worker),
                     {job_types::RealTime, job_types::Batch, job_types::Background});
}
pool->start();

// Submit jobs with priorities
for (const auto& item : data) {
    auto job = std::make_unique<callback_typed_job<job_types>>(
        job_types::Batch,  // Priority
        [item]() -> result_void {
            expensive_operation(item);
            return {};
        }
    );
    pool->enqueue(std::move(job));
}

pool->shutdown();
```

**Benefits over TBB**:
- **Simpler API**: No blocked_range abstractions
- **Type-based priorities**: RealTime, Batch, Background
- **Better error handling**: Result<T> pattern throughout
- **Adaptive queues**: Automatic contention optimization
- **6.9% faster** in real-world benchmarks

---

## Migration Path 3: From Older thread_system

### Version Compatibility

| Version | Release Date | API Changes | Migration Effort |
|---------|-------------|-------------|------------------|
| 1.0.0 (Current) | 2025-10-22 | Result<T>, interfaces | Medium |
| 0.9.x | 2025-07-22 | Memory optimization | Low |
| 0.8.x | 2025-06-30 | Lock-free removal | Medium |
| 0.7.x | 2025-06-29 | Lock-free addition | Low |

### From 0.9.x to 1.0.0

**Major Changes**:
1. Result<T> pattern for error handling
2. Interface-based design (executor_interface, scheduler_interface)
3. Enhanced synchronization primitives
4. Service container for dependency injection

**Step 1: Update Error Handling**

**Before** (0.9.x):
```cpp
// APIs returned bool or void
bool success = pool->start();
if (!success) {
    // Handle error
}

pool->enqueue(std::make_unique<callback_job>([]() {
    // Job code
}));
```

**After** (1.0.0):
```cpp
// APIs return result_void or result<T>
auto result = pool->start();
if (!result.has_value()) {
    const auto& error = result.error();
    std::cerr << "Start failed: " << error.message
              << " (code: " << static_cast<int>(error.code) << ")\n";
    return;
}

auto enqueue_result = pool->enqueue(std::make_unique<callback_job>([]() -> result_void {
    // Job code
    return {};  // Return result_void
}));
```

**Step 2: Update Dependency Injection**

**Before** (0.9.x - manual injection):
```cpp
// Manual logger injection
auto logger = std::make_shared<my_logger>();
pool->set_logger(logger);

// Manual monitoring injection
auto monitoring = std::make_shared<my_monitoring>();
pool->set_monitoring(monitoring);
```

**After** (1.0.0 - service container):
```cpp
#include <kcenon/thread/interfaces/service_container.h>

// Register services globally
thread_module::service_container::global()
    .register_singleton<thread_module::logger_interface>(my_logger);

monitoring_interface::service_container::global()
    .register_singleton<monitoring_interface::monitoring_interface>(my_monitoring);

// Pool automatically uses registered services via thread_context
auto pool = std::make_shared<thread_pool>("monitored_pool");
pool->start();  // Logger and monitoring automatically connected
```

**Step 3: Update Synchronization Primitives**

**Before** (0.9.x - basic primitives):
```cpp
std::mutex mutex;
std::unique_lock<std::mutex> lock(mutex);
```

**After** (1.0.0 - enhanced primitives):
```cpp
#include <kcenon/thread/core/sync_primitives.h>

std::mutex mutex;
scoped_lock_guard lock(mutex, std::chrono::milliseconds(100));
if (!lock.owns_lock()) {
    // Timeout handling
}
```

### From 0.8.x to 1.0.0

Includes all changes from 0.9.x to 1.0.0, plus:

**Major Change**: Lock-free implementations removed for simplification

**Before** (0.8.x):
```cpp
#include <kcenon/thread/core/lockfree_thread_pool.h>

// Lock-free pool
auto pool = std::make_shared<lockfree_thread_pool>("lockfree_pool");
```

**After** (1.0.0):
```cpp
#include <kcenon/thread/core/thread_pool.h>

// Adaptive pool (includes lock-free mode automatically)
auto pool = std::make_shared<thread_pool>("adaptive_pool");

// Adaptive queues automatically use lock-free when beneficial
// No manual lock-free pool needed
```

**Benefits**:
- **Simpler API**: Single pool type, automatic optimization
- **Better performance**: Adaptive queues provide 7.7x improvement when needed
- **Easier migration**: Drop-in replacement, no code changes

---

## Migration Path 4: From Custom Threading

### Common Custom Threading Patterns

#### Pattern 1: Producer-Consumer Queue

**Before** (custom):
```cpp
template<typename T>
class ProducerConsumerQueue {
    std::queue<T> queue;
    std::mutex mutex;
    std::condition_variable cv;
    bool done{false};

public:
    void push(T item) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push(std::move(item));
        }
        cv.notify_one();
    }

    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this]() { return !queue.empty() || done; });

        if (queue.empty()) return std::nullopt;

        T item = std::move(queue.front());
        queue.pop();
        return item;
    }

    void finish() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            done = true;
        }
        cv.notify_all();
    }
};
```

**After** (thread_system):
```cpp
#include <kcenon/thread/core/thread_pool.h>

// Create thread pool with adaptive queue
auto pool = std::make_shared<thread_pool>("producer_consumer");

// Add workers
std::vector<std::unique_ptr<thread_worker>> workers;
for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
    workers.push_back(std::make_unique<thread_worker>());
}
pool->enqueue_batch(std::move(workers));
pool->start();

// Producer: Submit jobs
for (const auto& item : items) {
    pool->submit_task([item]() {
        process(item);
    });
}

// Consumer: Workers automatically process jobs
// Adaptive queue optimizes based on contention

// Finish: Graceful shutdown
pool->shutdown();
```

**Benefits**:
- **Automatic optimization**: Adaptive queue selects best strategy
- **No manual CV management**: Built-in synchronization
- **Better performance**: 7.7x faster under high contention
- **Comprehensive**: Error handling, monitoring, logging

#### Pattern 2: Thread Pool with Priority

**Before** (custom):
```cpp
class PriorityThreadPool {
    std::priority_queue<Task> high_priority;
    std::priority_queue<Task> low_priority;
    std::mutex mutex;
    std::condition_variable cv;
    std::vector<std::thread> workers;
    bool stop{false};

    Task get_task() {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this]() {
            return stop || !high_priority.empty() || !low_priority.empty();
        });

        if (!high_priority.empty()) {
            auto task = high_priority.top();
            high_priority.pop();
            return task;
        }

        if (!low_priority.empty()) {
            auto task = low_priority.top();
            low_priority.pop();
            return task;
        }

        return Task{};  // Empty task
    }

public:
    void submit_high_priority(Task task) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            high_priority.push(std::move(task));
        }
        cv.notify_one();
    }

    void submit_low_priority(Task task) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            low_priority.push(std::move(task));
        }
        cv.notify_one();
    }
};
```

**After** (thread_system):
```cpp
#include <kcenon/thread/core/typed_thread_pool.h>

using namespace kcenon::thread;

// Create typed pool with priorities
auto pool = std::make_shared<typed_thread_pool_t<job_types>>("priority_pool");

// Add workers with priority handling
std::vector<std::unique_ptr<typed_thread_worker_t<job_types>>> workers;
for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
    auto worker = std::make_unique<typed_thread_worker_t<job_types>>();
    pool->add_worker(std::move(worker),
                     {job_types::RealTime, job_types::Batch, job_types::Background});
}
pool->start();

// Submit high-priority jobs
auto high_priority_job = std::make_unique<callback_typed_job<job_types>>(
    job_types::RealTime,
    [/* capture */]() -> result_void {
        // High-priority work
        return {};
    }
);
pool->enqueue(std::move(high_priority_job));

// Submit low-priority jobs
auto low_priority_job = std::make_unique<callback_typed_job<job_types>>(
    job_types::Background,
    [/* capture */]() -> result_void {
        // Low-priority work
        return {};
    }
);
pool->enqueue(std::move(low_priority_job));

pool->shutdown();
```

**Benefits**:
- **Type-based priorities**: RealTime > Batch > Background
- **Per-type queues**: Separate adaptive queue for each priority
- **Worker specialization**: Responsibility lists define handling
- **99%+ type accuracy**: Maintained under all conditions
- **6.9% faster** than standard pool

---

## Common Migration Issues

### Issue 1: Result<T> Pattern Unfamiliar

**Problem**: Don't know how to handle Result<T> from APIs

**Solution**: Use .has_value() and .error()
```cpp
auto result = pool->start();

if (result.has_value()) {
    // Success
    std::cout << "Pool started successfully\n";
} else {
    // Error
    const auto& error = result.error();
    std::cerr << "Error: " << error.message
              << " (code: " << static_cast<int>(error.code) << ")\n";
}

// Or use monadic operations
auto result = pool->start()
    .and_then([&pool]() {
        return pool->enqueue(create_job());
    })
    .or_else([](const auto& error) {
        log_error(error);
        return retry_operation();
    });
```

### Issue 2: CMake Cache Issues

**Problem**: CMake still finds old thread_system

**Solution**: Clear CMake cache
```bash
rm -rf build
mkdir build && cd build
cmake ..
```

### Issue 3: Header Not Found

**Problem**:
```
fatal error: kcenon/thread/core/thread_pool.h: No such file or directory
```

**Solution**: Ensure thread_system is properly installed
```bash
# Build and install thread_system
cd /path/to/thread_system
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make
sudo make install
```

Or use FetchContent in CMakeLists.txt:
```cmake
include(FetchContent)
FetchContent_Declare(
    thread_system
    GIT_REPOSITORY https://github.com/kcenon/thread_system.git
    GIT_TAG main
)
FetchContent_MakeAvailable(thread_system)
```

### Issue 4: Performance Regression

**Problem**: Slower than previous custom implementation

**Solution**: Enable optimizations
```bash
# Ensure Release build
cmake .. -DCMAKE_BUILD_TYPE=Release

# Enable link-time optimization
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
```

And configure for optimal performance:
```cpp
// Use adaptive queues (default)
auto pool = std::make_shared<thread_pool>("optimized_pool");

// Add workers equal to hardware concurrency
std::vector<std::unique_ptr<thread_worker>> workers;
for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
    workers.push_back(std::make_unique<thread_worker>());
}
pool->enqueue_batch(std::move(workers));
```

### Issue 5: Namespace Conflicts

**Problem**: Both old and new thread code in same file

**Solution**: Use explicit namespaces during transition
```cpp
// Temporary migration phase
namespace old_threading {
    // Your old threading code
}

namespace new_threading = kcenon::thread;

// Gradually migrate to new_threading
auto pool = new_threading::thread_pool("migration_pool");
```

---

## Testing Your Migration

### 1. Compile Test

```bash
mkdir build && cd build
cmake ..
make
```

Expected: Clean build with no errors

### 2. Unit Tests

```bash
# Run your existing tests
ctest

# Or manually
./build/tests/your_tests
```

Expected: All tests pass

### 3. Integration Tests

Create a verification test:

```cpp
// migration_verify.cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/callback_job.h>
#include <iostream>
#include <cassert>

using namespace kcenon::thread;

int main() {
    std::cout << "Thread System Migration Verification\n";
    std::cout << "====================================\n\n";

    // Test 1: Pool creation and startup
    std::cout << "Test 1: Pool creation and startup..." << std::flush;
    auto pool = std::make_shared<thread_pool>("verify_pool");

    std::vector<std::unique_ptr<thread_worker>> workers;
    for (size_t i = 0; i < 4; ++i) {
        workers.push_back(std::make_unique<thread_worker>());
    }
    pool->enqueue_batch(std::move(workers));

    auto result = pool->start();
    assert(result.has_value());
    std::cout << " PASSED\n";

    // Test 2: Job submission
    std::cout << "Test 2: Job submission..." << std::flush;
    std::atomic<int> counter{0};

    for (int i = 0; i < 100; ++i) {
        pool->submit_task([&counter]() {
            counter.fetch_add(1);
        });
    }

    // Wait for completion
    while (pool->get_pending_task_count() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    assert(counter.load() == 100);
    std::cout << " PASSED\n";

    // Test 3: Cleanup
    std::cout << "Test 3: Cleanup..." << std::flush;
    pool->shutdown();
    std::cout << " PASSED\n";

    std::cout << "\n====================================\n";
    std::cout << "All migration verification tests PASSED!\n";
    return 0;
}
```

Compile and run:
```bash
g++ -std=c++20 migration_verify.cpp \
    -I/path/to/thread_system/include \
    -L/path/to/thread_system/build \
    -lthread_system -lpthread \
    -o migration_verify

./migration_verify
```

### 4. Performance Benchmark

```cpp
// migration_benchmark.cpp
#include <kcenon/thread/core/thread_pool.h>
#include <chrono>
#include <iostream>

using namespace kcenon::thread;

int main() {
    auto pool = std::make_shared<thread_pool>("benchmark");

    std::vector<std::unique_ptr<thread_worker>> workers;
    for (size_t i = 0; i < 8; ++i) {
        workers.push_back(std::make_unique<thread_worker>());
    }
    pool->enqueue_batch(std::move(workers));
    pool->start();

    const int NUM_JOBS = 100000;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_JOBS; ++i) {
        pool->submit_task([]() {
            // Simulate work
            volatile int sum = 0;
            for (int j = 0; j < 100; ++j) {
                sum += j;
            }
        });
    }

    while (pool->get_pending_task_count() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    double throughput = (NUM_JOBS * 1000.0) / duration.count();

    std::cout << "Throughput: " << throughput << " jobs/s\n";
    std::cout << "Expected: > 500,000 jobs/s\n";
    std::cout << "Result: " << (throughput > 500000 ? "PASSED" : "NEEDS INVESTIGATION") << "\n";

    pool->shutdown();
    return 0;
}
```

---

## Rollback Plan

If migration fails, you can rollback:

### Option 1: Revert Changes

```bash
# Revert git changes
git checkout -- .

# Or restore from backup
cp -r backup_before_migration/* .

# Rebuild with old version
mkdir build && cd build
cmake ..
make
```

### Option 2: Gradual Migration

Keep both old and new code during transition:

```cmake
# CMakeLists.txt
option(USE_THREAD_SYSTEM "Use new thread_system" ON)

if(USE_THREAD_SYSTEM)
    find_package(thread_system REQUIRED)
    target_link_libraries(your_app PRIVATE kcenon::thread_system)
    target_compile_definitions(your_app PRIVATE USE_THREAD_SYSTEM)
else()
    # Old threading code
    find_package(Threads REQUIRED)
    target_link_libraries(your_app PRIVATE Threads::Threads)
endif()
```

```cpp
// Gradual migration in code
#ifdef USE_THREAD_SYSTEM
    #include <kcenon/thread/core/thread_pool.h>
    using namespace kcenon::thread;
    auto pool = std::make_shared<thread_pool>("migration_pool");
#else
    #include "old_threading.h"
    auto pool = OldThreadPool(8);
#endif
```

---

## Migration Assistance

### Automated Migration Script

We provide a migration script to automate common changes:

```bash
# Download migration script
curl -O https://raw.githubusercontent.com/kcenon/thread_system/main/scripts/migrate_to_thread_system.sh
chmod +x migrate_to_thread_system.sh

# Run migration on your source directory
./migrate_to_thread_system.sh /path/to/your/source

# Review changes
git diff

# Rebuild
mkdir build && cd build
cmake ..
make
```

The script performs:
- Header include updates
- Namespace replacements
- CMakeLists.txt modifications
- Basic API pattern updates

**Note**: Manual review is still required for complex logic.

---

## Support

Need help with migration?

- **Migration Issues**: [GitHub Issues](https://github.com/kcenon/thread_system/issues)
- **Migration Questions**: [GitHub Discussions](https://github.com/kcenon/thread_system/discussions)
- **Email Support**: kcenon@naver.com

---

## Next Steps After Migration

1. **Read Documentation**:
   - [README.md](README.md) - Feature overview
   - [ARCHITECTURE.md](ARCHITECTURE.md) - System design
   - [examples/](examples/) - Code examples

2. **Optimize Configuration**:
   - Review worker count
   - Configure adaptive queues
   - Set up monitoring integration

3. **Enable Advanced Features**:
   - Type-based priorities
   - Cooperative cancellation
   - Performance monitoring

---

**Last Updated:** 2025-10-22
**Migration Script Version:** 1.0.0
