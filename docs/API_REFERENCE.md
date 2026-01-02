# thread_system API Reference

> **Version**: 3.0.0
> **Last Updated**: 2025-12-19
> **Language**: [English]

## Table of Contents

1. [Namespace](#namespace)
2. [thread_pool (Recommended)](#thread_pool-recommended)
3. [typed_thread_pool](#typed_thread_pool)
4. [Lock-Free Queues](#lock-free-queues)
5. [Synchronization Primitives](#synchronization-primitives)

---

## Namespace

### `kcenon::thread`

All public APIs of thread_system are contained in this namespace

**Included Items**:
- `thread_pool` - Task-based thread pool
- `typed_thread_pool` - Type-safe thread pool
- `mpmc_queue` - Multi-Producer Multi-Consumer queue
- `spsc_queue` - Single-Producer Single-Consumer queue
- `adaptive_queue` - Adaptive queue
- Synchronization primitives

---

## thread_pool (Recommended)

### Overview

**Header**: `#include <kcenon/thread/thread_pool.h>`

**Description**: High-performance task-based thread pool

**Key Features**:
- Future/Promise pattern
- Priority-based task execution (optional)
- Dynamic worker configuration
- Common system IExecutor integration (when available)

> **Note**: Work-stealing is planned but not yet implemented. See `config.h` for configuration options.

### Creation and Usage

```cpp
#include <kcenon/thread/thread_pool.h>

using namespace kcenon::thread;

// Create with 4 worker threads
thread_pool pool(4);

// Enqueue task (with result)
auto future = pool.enqueue([]() {
    return 42;
});

// Wait for result
int result = future.get();  // 42

// Fire-and-forget task
pool.enqueue([]() {
    std::cout << "Background task" << std::endl;
});
```

### Core Methods

#### `enqueue()`

```cpp
template<typename F, typename... Args>
auto enqueue(F&& func, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>;
```

**Description**: Enqueue task and return Future

**Parameters**:
- `func`: Function or lambda to execute
- `args...`: Function arguments

**Return Value**: `std::future<Result>` - Task result

**Example**:
```cpp
// Function without parameters
auto future1 = pool.enqueue([]() { return 42; });

// Function with parameters
auto future2 = pool.enqueue([](int x, int y) { return x + y; }, 10, 20);

// Member function
struct Calculator {
    int add(int x, int y) { return x + y; }
};
Calculator calc;
auto future3 = pool.enqueue(&Calculator::add, &calc, 10, 20);
```

#### `thread_count()`

```cpp
size_t thread_count() const noexcept;
```

**Description**: Return number of worker threads

**Example**:
```cpp
thread_pool pool(4);
std::cout << "Workers: " << pool.thread_count() << std::endl;  // 4
```

### Performance Characteristics

| Metric | Value | Notes |
|--------|-------|-------|
| Throughput | ~1M ops/sec | Varies by workload |
| Latency (p50) | ~1 μs | Task scheduling |
| Scalability | Near-linear to 16 cores | Depends on task granularity |

> **Note**: Actual performance depends on workload characteristics, hardware, and configuration.

---

## typed_thread_pool

### Overview

**Header**: `#include <kcenon/thread/typed_thread_pool.h>`

**Description**: Type-safe thread pool (3.8x performance improvement)

**Key Features**:
- Compile-time type safety
- Automatic type deduction
- Custom process function
- Adaptive queue management

### Creation and Usage

```cpp
#include <kcenon/thread/typed_thread_pool.h>

using namespace kcenon::thread;

// Thread pool for processing int type tasks
typed_thread_pool<int> pool(4);

// Set process function
pool.set_process_function([](int value) {
    std::cout << "Processing: " << value << std::endl;
    return value * 2;
});

// Enqueue type-safe tasks
pool.enqueue(10);
pool.enqueue(20);
pool.enqueue(30);

// Start
pool.start();
```

### Core Methods

#### `enqueue()`

```cpp
template<typename T>
void enqueue(T&& item);
```

**Description**: Enqueue type-safe task

**Parameters**:
- `item`: Item to process (type T)

**Example**:
```cpp
typed_thread_pool<std::string> pool(4);

pool.set_process_function([](const std::string& msg) {
    std::cout << "Message: " << msg << std::endl;
});

pool.enqueue("Hello");
pool.enqueue("World");
```

#### `set_process_function()`

```cpp
void set_process_function(std::function<void(T)> func);
```

**Description**: Set task processing function

**Parameters**:
- `func`: Processing function (takes T, returns void)

**Example**:
```cpp
typed_thread_pool<Data> pool(4);

pool.set_process_function([](const Data& data) {
    // Process data
    process_data(data);
});
```

#### `start()` / `stop()`

```cpp
void start();
void stop();
```

**Description**: Start/stop thread pool

**Example**:
```cpp
typed_thread_pool<int> pool(4);
pool.set_process_function(process_int);

pool.start();   // Start processing tasks
// ... use pool ...
pool.stop();    // Stop (complete in-progress tasks first)
```

### Performance Characteristics

| Metric | Value | Notes |
|--------|-------|-------|
| Throughput | 980K ops/sec | Type-safe |
| Latency (p50) | 1.0 μs | Queue overhead |
| Memory overhead | +10% vs generic | Type metadata |

---

## Queue Factory

### queue_factory

**Header**: `#include <kcenon/thread/queue/queue_factory.h>`

**Description**: Factory utility for creating queue instances based on requirements

**Key Features**:
- Convenience methods for standard, lock-free, and adaptive queues
- Requirements-based queue selection
- Environment-optimized auto-selection
- Compile-time queue type selectors

#### Usage Examples

```cpp
#include <kcenon/thread/queue/queue_factory.h>

using namespace kcenon::thread;

// Convenience factory methods
auto standard = queue_factory::create_standard_queue();   // job_queue
auto lockfree = queue_factory::create_lockfree_queue();   // lockfree_job_queue
auto adaptive = queue_factory::create_adaptive_queue();   // adaptive_job_queue

// Requirements-based creation
queue_factory::requirements reqs;
reqs.need_exact_size = true;    // Need accurate size()
reqs.need_blocking_wait = true; // Need blocking dequeue
auto queue = queue_factory::create_for_requirements(reqs);
// Returns job_queue because accuracy was requested

// Environment-optimized creation
auto optimal = queue_factory::create_optimal();
// Considers CPU architecture and core count

// Compile-time type selection
accurate_queue_t accurate;   // job_queue
fast_queue_t fast;           // lockfree_job_queue
balanced_queue_t balanced;   // adaptive_job_queue

// Template-based selection
queue_t<true, false> must_be_accurate;   // job_queue
queue_t<false, true> prefer_fast;        // lockfree_job_queue
```

#### Core Methods

```cpp
// Convenience factory methods
static auto create_standard_queue() -> std::shared_ptr<job_queue>;
static auto create_lockfree_queue() -> std::unique_ptr<lockfree_job_queue>;
static auto create_adaptive_queue(policy p = policy::balanced)
    -> std::unique_ptr<adaptive_job_queue>;

// Requirements-based selection
static auto create_for_requirements(const requirements& reqs)
    -> std::unique_ptr<scheduler_interface>;

// Environment-optimized selection
static auto create_optimal() -> std::unique_ptr<scheduler_interface>;
```

#### requirements Struct

```cpp
struct requirements {
    bool need_exact_size = false;       // Require exact size()
    bool need_atomic_empty = false;     // Require atomic empty()
    bool prefer_lock_free = false;      // Prefer lock-free if possible
    bool need_batch_operations = false; // Require batch enqueue/dequeue
    bool need_blocking_wait = false;    // Require blocking dequeue
};
```

#### Type Aliases

| Alias | Resolves To | Use Case |
|-------|-------------|----------|
| `accurate_queue_t` | `job_queue` | Accuracy-first |
| `fast_queue_t` | `lockfree_job_queue` | Performance-first |
| `balanced_queue_t` | `adaptive_job_queue` | Balanced/adaptive |

---

## Concurrent Queues

### concurrent_queue

**Header**: `#include <kcenon/thread/concurrent/concurrent_queue.h>`

**Namespace**: `kcenon::thread::detail`

**Description**: Thread-safe MPMC queue with blocking wait support (internal implementation)

**Algorithm**: Fine-grained locking with separate head/tail mutexes

> **Note**: This is an internal implementation in the `detail::` namespace.
> For public API, use `adaptive_job_queue` or `job_queue` instead.
> The old `lockfree_queue<T>` alias is deprecated due to misleading naming
> (uses fine-grained locking, not lock-free algorithms).

**Key Features**:
- Thread-safe concurrent access from multiple producers and consumers
- Blocking wait with timeout support
- Works with any move-constructible type
- Low contention between enqueue and dequeue operations

#### Usage Example

```cpp
#include <kcenon/thread/concurrent/concurrent_queue.h>

using namespace kcenon::thread::detail;

concurrent_queue<std::string> queue;

// Producer thread
std::thread producer([&queue]() {
    queue.enqueue("message");
});

// Consumer thread (non-blocking)
std::thread consumer([&queue]() {
    if (auto value = queue.try_dequeue()) {
        process(*value);
    }
});

// Consumer thread (blocking with timeout)
if (auto value = queue.wait_dequeue(std::chrono::milliseconds{100})) {
    process(*value);
}
```

#### Core Methods

```cpp
void enqueue(T value);                              // Add to queue
std::optional<T> try_dequeue();                     // Non-blocking remove
std::optional<T> wait_dequeue(duration timeout);    // Blocking remove with timeout
size_t size() const;                                // Current size (approximate)
bool empty() const;                                 // Check if empty
void shutdown();                                    // Signal shutdown
```

---

### adaptive_job_queue

**Header**: `#include <kcenon/thread/queue/adaptive_job_queue.h>`

**Description**: Adaptive queue that switches between mutex and lock-free modes

**Algorithm**: Dynamic mode switching based on policy

**Key Features**:
- Wraps both mutex-based and lock-free queue implementations
- Multiple selection policies (accuracy, performance, balanced, manual)
- RAII guard for temporary accuracy mode
- Thread-safe mode switching with data migration

#### Usage Example

```cpp
#include <kcenon/thread/queue/adaptive_job_queue.h>

using namespace kcenon::thread;

// Create adaptive queue (defaults to balanced policy)
auto queue = std::make_unique<adaptive_job_queue>();

// Use like any other queue
queue->enqueue(std::make_unique<my_job>());
auto job = queue->dequeue();

// Temporarily require accuracy
{
    auto guard = queue->require_accuracy();
    size_t exact = queue->size();  // Now guaranteed exact
}

// Create with specific policy
auto perf_queue = std::make_unique<adaptive_job_queue>(
    adaptive_job_queue::policy::performance_first
);
```

#### Policies

| Policy | Description |
|--------|-------------|
| `accuracy_first` | Always use mutex mode |
| `performance_first` | Always use lock-free mode |
| `balanced` | Auto-switch based on usage |
| `manual` | User controls mode |

#### Core Methods

```cpp
auto enqueue(std::unique_ptr<job>&& j) -> common::VoidResult;
auto dequeue() -> std::unique_ptr<job>;
size_t size() const;
bool empty() const;
auto require_accuracy() -> accuracy_guard;  // RAII guard for exact size
void set_mode(mode m);                      // Switch mode (manual policy only)
mode current_mode() const;                  // Get current mode
```

---

## Synchronization Primitives

### safe_hazard_pointer

**Header**: `#include <kcenon/thread/core/safe_hazard_pointer.h>`

**Description**: Thread-safe hazard pointer implementation for lock-free data structures

**Key Features**:
- Explicit memory ordering for ARM and weak memory model architectures
- Per-thread hazard pointer records with automatic registration
- Deferred reclamation with configurable threshold
- Thread-safe with proper synchronization

> **Note**: The legacy `hazard_pointer.h` is deprecated due to memory ordering issues (TICKET-002).
> Use `safe_hazard_pointer.h` or `atomic_shared_ptr.h` instead.

#### Usage Example

```cpp
#include <kcenon/thread/core/safe_hazard_pointer.h>

using namespace kcenon::thread;

// Create hazard pointer domain
safe_hazard_pointer_domain<Node> hp_domain;

// Acquire a record
auto* record = hp_domain.acquire();

// Protect a pointer (slot 0 by default)
Node* node = shared_head.load();
record->protect(node);

// Use node safely - other threads won't reclaim it
process(node);

// Clear protection when done
record->clear();

// Retire a node for deferred reclamation
hp_domain.retire(old_node);
```

---

### scoped_lock_guard

**Header**: `#include <kcenon/thread/core/sync_primitives.h>`

**Description**: RAII-based scoped lock guard with optional timeout support

**Key Features**:
- Automatic lock acquisition and release
- Optional timeout for timed mutexes
- Exception-safe lock management

#### Usage Example

```cpp
#include <kcenon/thread/core/sync_primitives.h>

using namespace kcenon::thread::sync;

std::mutex mtx;

// Basic usage
{
    scoped_lock_guard<std::mutex> guard(mtx);
    // Critical section
}

// With timeout (for timed_mutex)
std::timed_mutex timed_mtx;
{
    scoped_lock_guard<std::timed_mutex> guard(
        timed_mtx, std::chrono::milliseconds{100}
    );
    if (guard.owns_lock()) {
        // Lock acquired
    }
}
```

---

### atomic_shared_ptr

**Header**: `#include <kcenon/thread/core/atomic_shared_ptr.h>`

**Description**: Thread-safe atomic operations on shared_ptr (alternative to hazard pointers)

**Key Features**:
- Lock-free atomic load/store/exchange for shared_ptr
- Simpler API than hazard pointers
- Compatible with standard shared_ptr

#### Usage Example

```cpp
#include <kcenon/thread/core/atomic_shared_ptr.h>

using namespace kcenon::thread;

atomic_shared_ptr<Node> shared_node;

// Atomic store
shared_node.store(std::make_shared<Node>());

// Atomic load
auto node = shared_node.load();

// Atomic exchange
auto old = shared_node.exchange(std::make_shared<Node>());
```

---

## Performance Comparison

### Queue Performance Comparison

| Queue Type | Description | Use Case |
|-----------|-------------|----------|
| **job_queue** | Mutex-based, accurate size | When exact size matters |
| **lockfree_job_queue** | Lock-free MPMC | High-throughput task queue |
| **concurrent_queue<T>** | Thread-safe with blocking wait | General MPMC use cases |
| **adaptive_job_queue** | Auto-switching between modes | Variable load systems |

> **Note**: Actual performance depends on workload characteristics and hardware.

### Thread Pool Comparison

| Pool Type | Type Safety | Use Case |
|----------|-------------|----------|
| **thread_pool** | Runtime | General-purpose task execution |
| **typed_thread_pool_t** | Compile-time | Priority-based job scheduling |

---

## Usage Examples

### Basic Usage

```cpp
#include <kcenon/thread/thread_pool.h>

using namespace kcenon::thread;

int main() {
    // Create thread pool with 4 worker threads
    thread_pool pool(4);

    // Enqueue task
    auto future = pool.enqueue([]() {
        return 42;
    });

    // Wait for result
    int result = future.get();
    std::cout << "Result: " << result << std::endl;

    return 0;
}
```

### Type-Safe Thread Pool

```cpp
#include <kcenon/thread/typed_thread_pool.h>

using namespace kcenon::thread;

int main() {
    // Thread pool for processing int type tasks
    typed_thread_pool<int> pool(4);

    // Enqueue type-safe tasks
    pool.enqueue(10);
    pool.enqueue(20);
    pool.enqueue(30);

    // Register task processing function
    pool.set_process_function([](int value) {
        std::cout << "Processing: " << value << std::endl;
    });

    pool.start();

    // Wait for processing
    std::this_thread::sleep_for(std::chrono::seconds(1));

    pool.stop();

    return 0;
}
```

### Lock-Free Queue

```cpp
#include <kcenon/thread/queues/mpmc_queue.h>

using namespace kcenon::thread;

int main() {
    mpmc_queue<int> queue(1024);

    // Producer thread
    std::thread producer([&queue]() {
        for (int i = 0; i < 1000; ++i) {
            queue.enqueue(i);
        }
    });

    // Consumer thread
    std::thread consumer([&queue]() {
        int value;
        while (queue.dequeue(value)) {
            std::cout << "Consumed: " << value << std::endl;
        }
    });

    producer.join();
    consumer.join();

    return 0;
}
```

---

## Migration Guide

### From v2.x to v3.0

**Changes**:
- Migrated to `common_system` Result/Error types
- Removed legacy `thread::result<T>` and `thread::error` types
- Deprecated `hazard_pointer.h` (use `safe_hazard_pointer.h`)
- Added stable umbrella headers (`<kcenon/thread/thread_pool.h>`)
- Removed `shared_interfaces.h` - use `common_system` interfaces

**Migration Example**:
```cpp
// v2.x - legacy includes
#include <kcenon/thread/core/thread_pool.h>

// v3.0 - stable umbrella headers
#include <kcenon/thread/thread_pool.h>

// v2.x - thread-specific Result
thread::result<int> result = ...;

// v3.0 - common system Result
common::Result<int> result = ...;
```

---

## Notes

### Thread Safety

- **thread_pool**: Thread-safe (all methods)
- **typed_thread_pool_t**: Thread-safe (all methods)
- **concurrent_queue<T>**: Thread-safe (multiple producers/consumers)
- **lockfree_job_queue**: Thread-safe (multiple producers/consumers)
- **job_queue**: Thread-safe (mutex-based)
- **adaptive_job_queue**: Thread-safe (multiple producers/consumers)

### Recommendations

- **General task processing**: Use `thread_pool` (recommended)
- **Priority-based scheduling**: Use `typed_thread_pool_t`
- **High-throughput queue**: Use `lockfree_job_queue`
- **Generic type queue**: Use `concurrent_queue<T>`
- **Variable load**: Use `adaptive_job_queue`
- **Safe memory reclamation**: Use `safe_hazard_pointer` or `atomic_shared_ptr`

---

**Created**: 2025-11-21
**Updated**: 2025-12-19
**Version**: 3.0.0
**Author**: kcenon@naver.com
