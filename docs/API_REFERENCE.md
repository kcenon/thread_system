# thread_system API Reference

> **Version**: 0.2.0
> **Last Updated**: 2025-11-21
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

**Description**: High-performance task-based thread pool (4.5x performance improvement)

**Key Features**:
- Work-stealing algorithm
- Future/Promise pattern
- Priority-based task execution
- Dynamic worker scaling

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
| Throughput | 1.2M ops/sec | Work-stealing |
| Latency (p50) | 0.8 μs | Task scheduling |
| Scalability | Near-linear to 16 cores | 95%+ efficiency |

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

## Lock-Free Queues

### mpmc_queue

**Header**: `#include <kcenon/thread/queues/mpmc_queue.h>`

**Description**: Multi-Producer Multi-Consumer queue (5.2x performance improvement)

**Algorithm**: Lock-free ring buffer

**Performance**: 2.1M ops/sec

#### Usage Example

```cpp
#include <kcenon/thread/queues/mpmc_queue.h>

using namespace kcenon::thread;

// Create (capacity: 1024)
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
        process(value);
    }
});
```

#### Core Methods

```cpp
bool enqueue(const T& item);        // Add to queue
bool dequeue(T& item);              // Remove from queue
bool try_enqueue(const T& item);    // Non-blocking add
bool try_dequeue(T& item);          // Non-blocking remove
size_t size() const;                // Current size
bool empty() const;                 // Check if empty
```

---

### spsc_queue

**Header**: `#include <kcenon/thread/queues/spsc_queue.h>`

**Description**: Single-Producer Single-Consumer queue

**Algorithm**: Lock-free circular buffer

**Performance**: 3.5M ops/sec

#### Usage Example

```cpp
#include <kcenon/thread/queues/spsc_queue.h>

using namespace kcenon::thread;

// Create (capacity: 1024)
spsc_queue<int> queue(1024);

// Producer thread (single only!)
std::thread producer([&queue]() {
    for (int i = 0; i < 1000; ++i) {
        queue.push(i);
    }
});

// Consumer thread (single only!)
std::thread consumer([&queue]() {
    int value;
    while (queue.pop(value)) {
        process(value);
    }
});
```

#### Core Methods

```cpp
bool push(const T& item);       // Add to queue
bool pop(T& item);              // Remove from queue
size_t size() const;            // Current size
bool empty() const;             // Check if empty
size_t capacity() const;        // Capacity
```

---

### adaptive_queue

**Header**: `#include <kcenon/thread/queues/adaptive_queue.h>`

**Description**: Load-based automatic resizing queue

**Algorithm**: Dynamic resizing queue

**Performance**: 1.5M ops/sec

#### Usage Example

```cpp
#include <kcenon/thread/queues/adaptive_queue.h>

using namespace kcenon::thread;

// Create (initial capacity: 128)
adaptive_queue<int> queue(128);

// Configure
queue.set_high_watermark(0.8);   // Expand at 80% usage
queue.set_low_watermark(0.2);    // Shrink at 20% usage

// Use (automatic resizing)
for (int i = 0; i < 10000; ++i) {
    queue.enqueue(i);
    // Queue automatically resizes based on load
}
```

#### Core Methods

```cpp
void enqueue(const T& item);             // Add to queue (auto-expand)
bool dequeue(T& item);                   // Remove from queue (auto-shrink)
void set_high_watermark(double ratio);   // Set expansion threshold (0.0-1.0)
void set_low_watermark(double ratio);    // Set shrink threshold (0.0-1.0)
size_t capacity() const;                 // Current capacity
size_t size() const;                     // Current size
```

---

## Synchronization Primitives

### hazard_pointer

**Header**: `#include <kcenon/thread/sync/hazard_pointer.h>`

**Description**: Safe memory reclamation for lock-free structures

**Key Features**:
- ABA problem mitigation
- Automatic garbage collection
- Thread-safe memory reclamation

#### Usage Example

```cpp
#include <kcenon/thread/sync/hazard_pointer.h>

using namespace kcenon::thread;

// Create hazard pointer domain
hazard_pointer_domain<Node> hp_domain;

// Acquire hazard pointer
auto hp = hp_domain.acquire();

// Protect pointer
Node* node = load_node();
hp.protect(node);

// Use node safely
process(node);

// Release (automatic on destruction)
```

---

### spinlock

**Header**: `#include <kcenon/thread/sync/spinlock.h>`

**Description**: Low-latency spinlock

#### Usage Example

```cpp
#include <kcenon/thread/sync/spinlock.h>

using namespace kcenon::thread;

spinlock lock;

// Use with RAII
{
    std::lock_guard<spinlock> guard(lock);
    // Critical section
}
```

---

### rw_lock

**Header**: `#include <kcenon/thread/sync/rw_lock.h>`

**Description**: Reader-writer lock (optimized for read-heavy workloads)

#### Usage Example

```cpp
#include <kcenon/thread/sync/rw_lock.h>

using namespace kcenon::thread;

rw_lock lock;

// Reader
{
    std::shared_lock<rw_lock> read_guard(lock);
    // Read operations
}

// Writer
{
    std::unique_lock<rw_lock> write_guard(lock);
    // Write operations
}
```

---

## Performance Comparison

### Queue Performance Comparison

| Queue Type | Throughput | Latency | Use Case |
|-----------|------------|---------|----------|
| **spsc_queue** | 3.5M ops/sec | 0.29 μs | Pipeline, single producer/consumer |
| **mpmc_queue** | 2.1M ops/sec | 0.48 μs | High-throughput task queue |
| **adaptive_queue** | 1.5M ops/sec | 0.67 μs | Variable load systems |

### Thread Pool Performance Comparison

| Pool Type | Throughput | Latency (p50) | Type Safety |
|----------|------------|---------------|-------------|
| **thread_pool** | 1.2M ops/sec | 0.8 μs | Runtime |
| **typed_thread_pool** | 980K ops/sec | 1.0 μs | Compile-time |

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

### From v1.x to v2.0

**Changes**:
- Work-stealing algorithm implementation
- Lock-free queue optimization (5.2x)
- Added typed thread pool
- Added adaptive queue
- Hazard pointer implementation

**Migration Example**:
```cpp
// v1.x
thread_pool pool(4);
auto future = pool.submit_task(task);

// v2.0
thread_pool pool(4);
auto future = pool.enqueue(task);
```

---

## Notes

### Thread Safety

- **thread_pool**: Thread-safe (all methods)
- **typed_thread_pool**: Thread-safe (all methods)
- **mpmc_queue**: Thread-safe (multiple producers/consumers)
- **spsc_queue**: Thread-safe (single producer, single consumer only)
- **adaptive_queue**: Thread-safe (multiple producers/consumers)

### Recommendations

- **General task processing**: Use `thread_pool` (recommended)
- **Type safety required**: Use `typed_thread_pool`
- **High-throughput queue**: Use `mpmc_queue`
- **Pipeline**: Use `spsc_queue`
- **Variable load**: Use `adaptive_queue`

---

**Created**: 2025-11-21
**Version**: 0.2.0
**Author**: kcenon@naver.com
