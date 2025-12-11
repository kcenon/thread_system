# Thread System Features

**Version**: 0.2.0
**Last Updated**: 2025-11-15
**Language**: [English] | [한국어](FEATURES_KO.md)

---

## Overview

This document provides comprehensive coverage of all thread_system features, including detailed descriptions, use cases, and implementation details.

---

## Table of Contents

1. [Core Threading Features](#core-threading-features)
2. [Queue Implementations](#queue-implementations)
3. [Thread Pool Features](#thread-pool-features)
4. [Typed Thread Pool](#typed-thread-pool)
5. [Adaptive Components](#adaptive-components)
6. [Synchronization Primitives](#synchronization-primitives)
7. [Service Infrastructure](#service-infrastructure)
8. [Advanced Features](#advanced-features)

---

## Core Threading Features

### thread_base Class

The foundational abstract class for all thread operations in the system.

#### Key Features

- **Dual Thread Support**: Supports both `std::jthread` (C++20) and `std::thread` through conditional compilation
- **Lifecycle Management**: Built-in start/stop lifecycle with customizable hooks
- **Thread Monitoring**: Condition monitoring and state management
- **Custom Naming**: Enhanced debugging with meaningful thread identification

#### API Overview

```cpp
class thread_base {
public:
    virtual auto start() -> result_void;
    virtual auto stop(bool immediately = false) -> result_void;
    virtual auto is_running() const -> bool;

protected:
    virtual auto on_initialize() -> result_void;
    virtual auto on_execute() -> result_void;
    virtual auto on_destroy() -> result_void;
};
```

#### Use Cases

- Base class for worker threads
- Custom thread implementations
- Thread pool workers
- Background service threads

---

## Queue Implementations

### 1. Standard Job Queue

Thread-safe FIFO queue for basic job management.

```cpp
class job_queue {
public:
    auto enqueue(std::unique_ptr<job>&& job) -> result_void;
    auto dequeue() -> result<std::unique_ptr<job>>;
    auto size() const -> std::size_t;
    auto empty() const -> bool;
};
```

**Features**:
- Mutex-based thread safety
- FIFO ordering guarantee
- Blocking dequeue operations
- Unlimited capacity

**Best For**:
- General-purpose job queuing
- Low to medium contention scenarios
- Simple use cases requiring reliability

---

### 2. Bounded Job Queue

Production-ready queue with backpressure support and capacity limits.

```cpp
class bounded_job_queue {
public:
    bounded_job_queue(size_t max_size);

    auto enqueue(std::unique_ptr<job>&& job,
                 std::optional<std::chrono::milliseconds> timeout = std::nullopt)
        -> result_void;
    auto dequeue() -> result<std::unique_ptr<job>>;
    auto get_metrics() const -> queue_metrics;
};

struct queue_metrics {
    size_t total_enqueued;
    size_t total_dequeued;
    size_t total_rejected;
    size_t timeout_count;
    size_t peak_size;
    size_t current_size;
};
```

**Features**:
- Maximum queue size enforcement
- Backpressure signaling when near capacity
- Timeout support for enqueue operations
- Comprehensive metrics tracking
- Memory exhaustion prevention

**Best For**:
- Production systems with bounded resources
- Systems requiring backpressure handling
- High-reliability applications
- Resource-constrained environments

**Metrics**:
- Total jobs enqueued/dequeued/rejected
- Timeout tracking
- Peak queue size monitoring
- Current queue depth

---

### 3. Lock-Free Job Queue

High-performance lock-free MPMC queue with hazard pointer memory reclamation.

```cpp
class lockfree_job_queue {
public:
    auto enqueue(std::unique_ptr<job>&& job) -> result_void;
    auto dequeue() -> result<std::unique_ptr<job>>;
    auto size() const -> std::size_t;
    auto empty() const -> bool;
};
```

**Features**:
- Lock-free multi-producer, multi-consumer (MPMC) design
- Hazard pointer-based memory reclamation
- CAS (Compare-And-Swap) operations
- Node pooling for efficiency
- **4x faster** than mutex-based queues (71 μs vs 291 μs)

**Best For**:
- High-contention scenarios (8+ threads)
- Maximum throughput requirements
- Low-latency critical paths
- Systems where lock overhead is significant

**Performance**:
| Threads | Throughput | Latency |
|---------|-----------|---------|
| 1-2     | ~96 ns    | Low contention |
| 4       | ~142 ns   | Medium load |
| 8+      | ~320 ns   | High contention (still 37% faster) |

---

### 4. Adaptive Job Queue

Intelligent queue that automatically switches between mutex and lock-free strategies.

```cpp
class adaptive_job_queue {
public:
    auto enqueue(std::unique_ptr<job>&& job) -> result_void;
    auto dequeue() -> result<std::unique_ptr<job>>;
    auto get_statistics() -> queue_statistics;
};

struct queue_statistics {
    size_t total_enqueued;
    size_t total_dequeued;
    size_t current_size;
    queue_mode current_mode;  // mutex or lockfree
};
```

**Features**:
- **Automatic strategy selection** based on contention
- Seamless switching between mutex and lock-free modes
- Performance monitoring and adaptation
- Zero configuration required

**Adaptive Strategy**:
- **Low contention** (1-2 threads): Uses mutex-based queue
- **Medium contention** (4 threads): Evaluates and adapts
- **High contention** (8+ threads): Switches to lock-free mode

**Best For**:
- Variable workload patterns
- Systems where contention changes over time
- Deployments where optimal performance is critical
- Applications requiring "set and forget" optimization

**Performance Benefits**:
| Scenario | Improvement | Strategy |
|----------|-------------|----------|
| Low load | Baseline | Mutex |
| Variable | +8.2% | Adaptive |
| High contention | +37% | Lock-free |

---

## Thread Pool Features

### Standard Thread Pool

Multi-worker thread pool with adaptive queue support.

```cpp
class thread_pool {
public:
    thread_pool(const std::string& name = "ThreadPool");

    auto start() -> result_void;
    auto stop(bool immediately = false) -> result_void;

    // Job submission
    auto enqueue(std::unique_ptr<job>&& job) -> result_void;
    auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> result_void;
    bool submit_task(std::function<void()> task);  // Convenience API

    // Worker management
    auto add_worker(std::unique_ptr<thread_worker>&& worker) -> result_void;
    auto add_workers(size_t count) -> result_void;

    // Monitoring
    auto get_thread_count() const -> size_t;
    auto get_pending_task_count() const -> size_t;
    auto get_idle_worker_count() const -> size_t;

    // Shutdown
    bool shutdown_pool(bool immediately = false);
};
```

**Key Features**:

1. **Dynamic Worker Management**
   - Add/remove workers at runtime
   - Automatic worker lifecycle management
   - Worker statistics tracking

2. **Adaptive Queue Architecture**
   - Automatic optimization based on load
   - Dual-mode support (mutex/lock-free)
   - Batch processing capabilities

3. **Dual API Design**
   - Result-based API for detailed error handling
   - Convenience API (`submit_task`, `shutdown_pool`) for simplicity

4. **Comprehensive Monitoring**
   - Worker count tracking
   - Pending task monitoring
   - Idle worker detection
   - Performance metrics

**Performance**:
- **Throughput**: 1.16M jobs/second (10 workers, production workload)
- **Latency**: ~77 ns per job scheduling
- **Memory**: <1 MB baseline overhead
- **Scalability**: 96% efficiency up to 8 workers

**Use Cases**:
- General-purpose concurrent task execution
- Web server request handling
- Background job processing
- Parallel algorithm execution

---

### Thread Worker

Individual worker thread with batch processing support.

```cpp
class thread_worker : public thread_base {
public:
    struct worker_statistics {
        uint64_t jobs_processed;
        uint64_t total_processing_time_ns;
        uint64_t batch_operations;
        uint64_t avg_processing_time_ns;
    };

    auto set_batch_processing(bool enabled, size_t batch_size = 32) -> void;
    auto get_statistics() const -> worker_statistics;
};
```

**Features**:
- Batch job processing for improved throughput
- Per-worker statistics tracking
- Customizable batch sizes
- Automatic job dequeuing

**Batch Processing**:
- Reduces queue lock contention
- Improves cache locality
- Configurable batch size (default: 32)
- Automatic fallback to single-job mode

---

## Typed Thread Pool

Priority-based thread pool with type-aware job scheduling.

### Overview

```cpp
template<typename T>
class typed_thread_pool_t {
public:
    auto start() -> result_void;
    auto stop(bool clear_queue = false) -> result_void;

    auto enqueue(std::unique_ptr<typed_job_t<T>>&& job) -> result_void;
    auto enqueue_batch(std::vector<std::unique_ptr<typed_job_t<T>>>&& jobs)
        -> result_void;

    auto get_statistics() const -> typed_pool_statistics_t<T>;
};
```

### Job Types

Default priority levels:

```cpp
enum class job_types {
    RealTime,    // Highest priority
    Batch,       // Medium priority
    Background   // Lowest priority
};
```

**Custom Types**: Use your own enums or types for domain-specific prioritization.

### Features

1. **Per-Type Adaptive Queues**
   - Each job type has its own adaptive queue
   - Independent optimization per priority level
   - Automatic queue lifecycle management

2. **Priority-Based Routing**
   - RealTime > Batch > Background ordering
   - Workers pull from highest priority first
   - FIFO guarantee within same type

3. **Type-Aware Workers**
   - Configurable type responsibility lists
   - Workers can handle multiple types
   - Dynamic type adaptation

4. **Advanced Statistics**
   - Per-type performance metrics
   - Queue depth monitoring
   - Processing time tracking

### Performance

**Typed Pool Comparison**:
| Configuration | Throughput | vs Basic Pool | Type Accuracy |
|--------------|------------|---------------|---------------|
| Single Type  | 525K/s     | -3%           | 100%          |
| 3 Types      | 495K/s     | -9%           | 99.6%         |
| Real Workload| **1.24M/s**| **+6.9%**     | 100%          |

**With Adaptive Queues**:
| Scenario | Performance | Type Accuracy | Notes |
|----------|-------------|---------------|-------|
| Low contention | 1.24M/s | 100% | Mutex strategy |
| High contention | Dynamic | 99%+ | Lock-free engaged |
| Mixed workload | Optimized | 99.5% | Auto-switching |

### Use Cases

- Real-time systems with priority requirements
- Game engines (rendering vs physics vs AI)
- Media processing (encoding vs decoding vs I/O)
- Financial systems (trading vs reporting vs analytics)

---

## Adaptive Components

### Adaptive Typed Job Queue

Per-type adaptive MPMC queue with automatic optimization.

```cpp
template<typename T>
class adaptive_typed_job_queue_t {
public:
    auto enqueue(std::unique_ptr<typed_job_t<T>>&& job) -> result_void;
    auto dequeue() -> result<std::unique_ptr<job>>;
    auto dequeue(const T& type) -> result<std::unique_ptr<typed_job_t<T>>>;

    auto get_typed_statistics() const -> typed_queue_statistics_t<T>;
};

template<typename T>
struct typed_queue_statistics_t {
    std::map<T, size_t> jobs_per_type;
    size_t total_jobs;
    queue_mode current_mode;
};
```

**Features**:
- Type-specific queue management
- Automatic mode switching per type
- Comprehensive per-type metrics
- Priority ordering preservation

---

### Hazard Pointers

Safe memory reclamation for lock-free data structures.

```cpp
class hazard_pointer {
public:
    static auto protect(const void* ptr) -> hazard_pointer_guard;
    static auto retire(void* ptr, std::function<void(void*)> deleter) -> void;
    static auto scan() -> void;
};
```

**Features**:
- Safe memory reclamation in concurrent environments
- Protection against use-after-free
- Automatic garbage collection
- Zero false positives

**How It Works**:
1. Threads mark pointers they're using as "hazardous"
2. Deleted nodes are added to retire list
3. Periodic scan checks for safe deletion
4. Only unreferenced nodes are freed

**Performance**:
- Minimal overhead (few atomic operations)
- Scales well with thread count
- Memory efficient (small metadata per thread)

---

### Node Pool

Memory pool for efficient node allocation in adaptive queues.

```cpp
class node_pool {
public:
    auto allocate() -> node*;
    auto deallocate(node* ptr) -> void;
    auto get_stats() const -> pool_statistics;
};
```

**Features**:
- Pre-allocated node cache
- Reduced allocation overhead
- Improved cache locality
- Statistics tracking

**Benefits**:
- Faster allocation/deallocation
- Reduced memory fragmentation
- Better performance under high load

---

## Synchronization Primitives

Enhanced synchronization wrappers with modern C++ features.

### Scoped Lock Guard

RAII lock with timeout support.

```cpp
class scoped_lock_guard {
public:
    scoped_lock_guard(std::mutex& mtx,
                     std::optional<std::chrono::milliseconds> timeout = std::nullopt);

    auto is_locked() const -> bool;
};
```

**Usage**:
```cpp
std::mutex mtx;
{
    scoped_lock_guard lock(mtx, std::chrono::milliseconds(100));
    if (lock.is_locked()) {
        // Critical section
    } else {
        // Timeout handling
    }
}  // Automatic unlock
```

---

### Condition Variable Wrapper

Enhanced condition variable with predicate support.

```cpp
class condition_variable_wrapper {
public:
    auto wait(std::unique_lock<std::mutex>& lock,
             std::function<bool()> predicate) -> void;

    auto wait_for(std::unique_lock<std::mutex>& lock,
                 std::chrono::milliseconds timeout,
                 std::function<bool()> predicate) -> bool;

    auto notify_one() -> void;
    auto notify_all() -> void;
};
```

---

### Shared Mutex Wrapper

Reader-writer lock implementation.

```cpp
class shared_mutex_wrapper {
public:
    auto lock_shared() -> void;
    auto unlock_shared() -> void;
    auto lock() -> void;
    auto unlock() -> void;
};
```

**Use Cases**:
- Multiple readers, single writer scenarios
- Configuration data access
- Cache implementations

---

## Service Infrastructure

### Service Registry

Lightweight dependency injection container.

```cpp
class service_registry {
public:
    template<typename T>
    auto register_service(std::shared_ptr<T> service) -> void;

    template<typename T>
    auto get_service() -> std::shared_ptr<T>;

    template<typename T>
    auto has_service() const -> bool;
};
```

**Features**:
- Type-safe service registration
- Thread-safe access with shared_mutex
- Automatic lifetime management via shared_ptr
- Global singleton pattern

**Usage**:
```cpp
// Register services
auto logger = std::make_shared<ConsoleLogger>();
service_registry::instance().register_service(logger);

// Retrieve services
auto logger = service_registry::instance().get_service<ILogger>();
```

---

### Cancellation Token

Enhanced cooperative cancellation mechanism.

```cpp
class cancellation_token {
public:
    auto cancel() -> void;
    auto is_cancelled() const -> bool;

    auto register_callback(std::function<void()> callback) -> void;
    auto create_linked_token() -> std::shared_ptr<cancellation_token>;
};
```

**Features**:
- Linked token creation for hierarchical cancellation
- Thread-safe callback registration
- Automatic propagation of cancellation signals
- Non-blocking cancellation checks

**Usage**:
```cpp
auto token = std::make_shared<cancellation_token>();

// In worker thread
pool->submit_task([token]() {
    for (int i = 0; i < 1000000; ++i) {
        if (token->is_cancelled()) {
            return;  // Exit early
        }
        // Do work
    }
});

// From control thread
token->cancel();  // Request cancellation
```

---

## Advanced Features

### Worker Policy System

Fine-grained control over worker behavior.

```cpp
struct worker_policy {
    scheduling_policy scheduling;  // FIFO, LIFO, Priority, Work-stealing
    idle_strategy idle_behavior;   // Timeout, Yield, Sleep
    size_t max_batch_size;
    std::optional<int> cpu_affinity;

    static auto default_policy() -> worker_policy;
    static auto high_performance() -> worker_policy;
    static auto low_latency() -> worker_policy;
    static auto power_efficient() -> worker_policy;
};
```

**Predefined Policies**:

1. **Default**: Balanced performance and efficiency
2. **High Performance**: Minimal latency, maximum throughput
3. **Low Latency**: Fastest response time
4. **Power Efficient**: Lower CPU usage

**Custom Policies**:
```cpp
worker_policy custom;
custom.scheduling = scheduling_policy::priority;
custom.idle_strategy = idle_strategy::yield;
custom.max_batch_size = 64;
custom.cpu_affinity = 2;  // Pin to CPU core 2
```

---

### Future-Based Task Extensions

Async result handling with futures.

```cpp
template<typename T>
class task {
public:
    auto get() -> T;
    auto wait() -> void;
    auto is_ready() const -> bool;
};

// Usage
auto future = pool->submit_with_result<int>([]() {
    return 42;
});

int result = future.get();  // Blocks until ready
```

---

### Exception Safety

Strong exception safety guarantees throughout the framework.

**Guarantees**:
- No resource leaks in exception paths
- Consistent state after exceptions
- RAII-based automatic cleanup
- Exception-safe queue operations

**Error Handling**:
```cpp
// Result<T> pattern for explicit error handling
auto result = pool->start();
if (result.has_error()) {
    const auto& error = result.get_error();
    std::cerr << "Error: " << error.message()
              << " (code: " << static_cast<int>(error.code()) << ")\n";
}
```

---

## Integration Features

### Logger Integration (Optional)

Seamless integration with separate logger project using common_system's ILogger interface.

```cpp
#include <kcenon/common/interfaces/logger_interface.h>
#include <kcenon/common/interfaces/global_logger_registry.h>

// Implement the ILogger interface from common_system
class my_logger : public kcenon::common::interfaces::ILogger {
    // Implement interface methods
};

// Register with GlobalLoggerRegistry for thread_system integration
auto logger = std::make_shared<my_logger>();
kcenon::common::interfaces::GlobalLoggerRegistry::instance().set_default_logger(logger);

// thread_context will automatically use the registered logger
kcenon::thread::thread_context context;
context.log(kcenon::common::interfaces::log_level::info, "Ready");
```

---

### Monitoring Integration (Optional)

Real-time metrics collection integration.

```cpp
#include <kcenon/thread/interfaces/monitoring_interface.h>

class my_monitor : public monitoring_interface {
    // Implement interface
};

// Use with thread pool
pool->set_monitor(std::make_shared<my_monitor>());
pool->report_metrics();  // Automatic reporting
```

---

## Summary

The thread_system provides a comprehensive, production-ready threading framework with:

- **Multiple queue implementations** for different scenarios
- **Adaptive components** that automatically optimize
- **Type-based scheduling** for priority workloads
- **Rich synchronization primitives** for complex scenarios
- **Service infrastructure** for clean architecture
- **Optional integrations** for logging and monitoring

Choose the right features for your use case:
- **Simple tasks**: Standard thread pool with adaptive queue
- **Priority workloads**: Typed thread pool
- **Bounded resources**: Bounded job queue
- **Maximum performance**: Lock-free queue or adaptive mode
- **Variable load**: Adaptive queue (automatic optimization)

---

**See Also**:
- [Performance Benchmarks](BENCHMARKS.md)
- [Architecture Guide](advanced/ARCHITECTURE.md)
- [API Reference](guides/API_REFERENCE.md)
- [User Guide](guides/USER_GUIDE.md)

---

**Last Updated**: 2025-11-15
**Maintained by**: kcenon@naver.com
