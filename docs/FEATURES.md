# Thread System Features

**Version**: 0.3.0.0
**Last Updated**: 2026-02-08
**Language**: [English] | [한국어](FEATURES.kr.md)

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
9. [DAG Scheduler](#dag-scheduler)
10. [NUMA-Aware Work Stealing](#numa-aware-work-stealing)
11. [Autoscaling](#autoscaling)
12. [Diagnostics](#diagnostics)

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

### 2. Job Queue with Bounded Size

Thread-safe queue with optional capacity limits via `max_size` parameter.

```cpp
class job_queue {
public:
    // Create unbounded queue (default)
    job_queue();

    // Create bounded queue with max capacity
    explicit job_queue(std::optional<std::size_t> max_size);

    auto enqueue(std::unique_ptr<job>&& job) -> common::VoidResult;
    auto dequeue() -> common::Result<std::unique_ptr<job>>;
    auto is_bounded() const -> bool;
    auto get_max_size() const -> std::optional<std::size_t>;
    auto is_full() const -> bool;
};
```

**Features**:
- Optional maximum queue size enforcement
- Backpressure support via `backpressure_job_queue`
- Thread-safe operations with mutex protection
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

### Choosing Between Thread Pools

The thread_system provides two thread pool implementations optimized for different use cases:

| Feature | `thread_pool` | `typed_thread_pool_t<T>` |
|---------|--------------|--------------------------|
| Scheduling | FIFO (First In, First Out) | Priority-based |
| Job Priority | Not supported | Template parameter (compile-time) |
| Worker Specialization | All workers handle all jobs | Workers can be assigned specific priorities |
| Work Stealing | Supported | Not supported |
| Metrics Collection | Built-in `ThreadPoolMetrics` | Basic statistics |
| Health Checks | Supported | Not supported |
| Best For | General task execution | Priority-critical workloads |

**When to use `thread_pool`:**
- General-purpose concurrent task execution
- When all tasks have equal priority
- When work-stealing load balancing is needed
- When detailed metrics and health monitoring are required

**When to use `typed_thread_pool_t<T>`:**
- Real-time systems with strict priority requirements
- When tasks must be processed in priority order
- When workers should specialize in handling specific priority levels
- When compile-time type safety for priorities is desired

---

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

### Work-Stealing Scheduler

Optional work-stealing scheduler for improved load balancing in uneven workload scenarios.

```cpp
// Enable work-stealing for a thread pool
worker_policy policy;
policy.enable_work_stealing = true;
policy.victim_selection = steal_policy::adaptive;
pool->set_worker_policy(policy);

// Or enable at runtime
pool->enable_work_stealing(true);
```

**Architecture**:

```
Worker 1 (Owner)         Worker 2 (Thief)
    ↓ push/pop              ↓ steal
    ┌─────────────────┐     ┌─────────────────┐
    │  Local Deque    │←────│  Local Deque    │
    │  [J4][J3][J2]   │     │  [empty]        │
    │   ↑LIFO     FIFO↑     │                 │
    └─────────────────┘     └─────────────────┘
           ↑                        ↑
           └────────────────────────┘
                  Global Queue
```

**Key Features**:

1. **Chase-Lev Work-Stealing Deque**
   - Lock-free LIFO for owner (cache locality)
   - Lock-free FIFO stealing (fairness)
   - Dynamic resizing support

2. **Steal Policy Options**
   - `random`: Random victim selection (good for uniform loads)
   - `round_robin`: Sequential victim selection (deterministic)
   - `adaptive`: Steal from busiest worker (best for uneven loads)

3. **Tunable Parameters**
   - `max_steal_attempts`: Cap on consecutive steal failures
   - `steal_backoff`: Exponential backoff between attempts

**Compile-Time Configuration**:
```cmake
cmake -DTHREAD_ENABLE_WORK_STEALING=ON ..
```

**Performance Benefits**:
| Scenario | Without Stealing | With Stealing | Improvement |
|----------|-----------------|---------------|-------------|
| Uniform load | 1.16M jobs/s | 1.16M jobs/s | No regression |
| Uneven (90/10) | ~800K jobs/s | >1.0M jobs/s | +25% |
| Producer-consumer | ~900K jobs/s | >1.1M jobs/s | +22% |

**Use Cases**:
- Parallel algorithms with uneven work distribution
- Fork-join parallelism
- Task graphs with variable task sizes
- Load balancing in heterogeneous workloads

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

### Circuit Breaker Pattern

Prevent cascading failures with circuit breaker protection.

```cpp
#include <kcenon/thread/resilience/circuit_breaker.h>

// Configure circuit breaker
circuit_breaker_config config;
config.failure_threshold = 5;          // Open after 5 consecutive failures
config.failure_rate_threshold = 0.5;   // Or 50% failure rate
config.minimum_requests = 10;          // Minimum requests before rate check
config.open_duration = std::chrono::seconds{30};
config.half_open_max_requests = 3;     // Test requests in half-open
config.half_open_success_threshold = 2; // Successes to close

// Enable in thread pool
pool->enable_circuit_breaker(config);

// Submit protected jobs
auto result = pool->enqueue_protected(std::move(job));
if (result.is_err()) {
    // Circuit may be open
    auto code = thread::get_error_code(result.error());
    if (code == thread::error_code::circuit_open) {
        // Handle circuit open - retry later or fallback
    }
}

// Check health before submitting
if (pool->is_accepting_work()) {
    pool->enqueue_protected(std::move(job));
}

// Manual circuit control
auto cb = pool->get_circuit_breaker();
cb->trip();   // Force open
cb->reset();  // Force close

// State change monitoring
config.state_change_callback = [](circuit_state old_state, circuit_state new_state) {
    log("Circuit transitioned from {} to {}", to_string(old_state), to_string(new_state));
};
```

**State Machine**:
- **Closed**: Normal operation, requests allowed
- **Open**: Failure threshold exceeded, requests blocked
- **Half-Open**: Testing recovery, limited requests allowed

**Features**:
- Sliding window failure rate calculation
- Configurable thresholds and timeouts
- RAII guard for automatic success/failure recording
- State change callbacks for monitoring
- Thread-safe operations

---

## DAG Scheduler

Directed Acyclic Graph (DAG) based job scheduling with dependency management.

### Overview

The DAG scheduler enables complex job orchestration where tasks have interdependencies. Jobs execute automatically when their dependencies are satisfied, with support for parallel execution of independent tasks.

```cpp
#include <kcenon/thread/dag/dag_job.h>
#include <kcenon/thread/dag/dag_job_builder.h>
#include <kcenon/thread/dag/dag_scheduler.h>
```

### dag_job_builder (Fluent Builder)

```cpp
auto job = dag_job_builder("process_data")
    .depends_on(fetch_id)
    .work([]() -> common::VoidResult {
        // Execute task
        return common::ok();
    })
    .on_failure([]() -> common::VoidResult {
        // Fallback logic
        return common::ok();
    })
    .build();
```

### dag_scheduler

```cpp
class dag_scheduler {
public:
    dag_scheduler(std::shared_ptr<thread_pool> pool, dag_config config = {});

    // Job management
    auto add_job(std::unique_ptr<dag_job> job) -> job_id;
    auto add_job(dag_job_builder&& builder) -> job_id;
    auto add_dependency(job_id dependent, job_id dependency) -> common::VoidResult;

    // Execution
    auto execute_all() -> std::future<common::VoidResult>;
    auto execute(job_id target) -> std::future<common::VoidResult>;
    auto cancel_all() -> void;
    auto wait() -> common::VoidResult;

    // Query
    auto get_execution_order() -> std::vector<job_id>;
    auto has_cycles() -> bool;
    template<typename T> auto get_result(job_id id) -> const T&;

    // Visualization
    auto to_dot() -> std::string;   // Graphviz DOT format
    auto to_json() -> std::string;  // JSON export
    auto get_stats() -> dag_stats;
};
```

### Features

- **Dependency Resolution**: Automatic topological ordering and dependency tracking
- **Parallel Execution**: Independent jobs execute concurrently on the thread pool
- **Cycle Detection**: Validates DAG structure before execution
- **Typed Results**: Pass results between jobs via `std::any` with type-safe retrieval
- **Failure Policies**: Configurable behavior on job failure
  - `fail_fast`: Cancel all dependents immediately
  - `continue_others`: Continue unrelated jobs, skip dependents
  - `retry`: Retry with configurable delay and max attempts
  - `fallback`: Execute fallback function on failure
- **Visualization**: Export DAG as DOT (Graphviz) or JSON for debugging
- **Statistics**: Execution metrics including critical path time and parallelism efficiency

### Configuration

```cpp
dag_config config;
config.failure_policy = dag_failure_policy::continue_others;
config.max_retries = 3;
config.retry_delay = std::chrono::milliseconds(1000);
config.detect_cycles = true;
config.execute_in_parallel = true;
config.state_callback = [](job_id id, dag_job_state old_s, dag_job_state new_s) {
    // Monitor state transitions
};
```

### Job States

| State | Description |
|-------|-------------|
| `pending` | Waiting for dependencies |
| `ready` | Dependencies satisfied |
| `running` | Currently executing |
| `completed` | Successfully completed |
| `failed` | Execution failed |
| `cancelled` | Cancelled by user or failure policy |
| `skipped` | Skipped due to dependency failure |

### Use Cases

- ETL pipelines with dependent stages
- Build systems with compilation dependencies
- Workflow orchestration
- Data processing graphs with fan-in/fan-out patterns

---

## NUMA-Aware Work Stealing

Enhanced work-stealing scheduler with NUMA topology awareness for optimal memory locality.

### Overview

On NUMA (Non-Uniform Memory Access) systems, cross-node memory access incurs significant latency penalties. The NUMA-aware work stealer minimizes these penalties by preferring to steal work from workers on the same NUMA node.

```cpp
#include <kcenon/thread/stealing/numa_topology.h>
#include <kcenon/thread/stealing/numa_work_stealer.h>
#include <kcenon/thread/stealing/enhanced_work_stealing_config.h>
```

### NUMA Topology Detection

```cpp
auto topology = numa_topology::detect();

// Query topology
auto node_count = topology.node_count();
auto cpu_count = topology.cpu_count();
auto is_numa = topology.is_numa_available();

// Check locality
auto node = topology.get_node_for_cpu(cpu_id);
auto distance = topology.get_distance(node1, node2);
auto same = topology.is_same_node(cpu1, cpu2);
```

**Platform Support**:
- **Linux**: Full support via `/sys/devices/system/node`
- **macOS/Windows**: Fallback to single-node topology (no degradation)

### Steal Policies

| Policy | Description | Best For |
|--------|-------------|----------|
| `random` | Random victim selection | Baseline, uniform loads |
| `round_robin` | Sequential rotation | Deterministic behavior |
| `adaptive` | Queue-size based selection | Uneven workloads |
| `numa_aware` | Prefer same NUMA node | NUMA systems |
| `locality_aware` | Historical cooperation tracking | Repeated patterns |
| `hierarchical` | NUMA node first, then random | Large NUMA systems |

### Configuration

```cpp
// Pre-built configurations
auto config = enhanced_work_stealing_config::numa_optimized();
auto config = enhanced_work_stealing_config::locality_optimized();
auto config = enhanced_work_stealing_config::batch_optimized();
auto config = enhanced_work_stealing_config::hierarchical_numa();

// Custom configuration
enhanced_work_stealing_config config;
config.enabled = true;
config.policy = enhanced_steal_policy::numa_aware;
config.numa_aware = true;
config.numa_penalty_factor = 2.0;      // Cross-node cost multiplier
config.prefer_same_node = true;
config.max_steal_batch = 4;
config.adaptive_batch_size = true;
config.collect_statistics = true;
```

### Statistics

```cpp
auto stats = stealer.get_stats_snapshot();
auto success_rate = stats.steal_success_rate();    // 0.0 - 1.0
auto cross_ratio = stats.cross_node_ratio();       // Locality indicator
auto avg_batch = stats.avg_batch_size();
auto avg_time = stats.avg_steal_time_ns();
```

### Features

- **Automatic Topology Detection**: Discovers NUMA layout at initialization
- **Locality-Optimized Stealing**: Minimizes cross-node memory access
- **Adaptive Batch Sizing**: Dynamic batch size based on queue depth
- **Exponential Backoff**: Configurable backoff strategy on steal failures
- **Detailed Statistics**: Atomic counters for monitoring steal efficiency
- **Graceful Fallback**: Non-NUMA systems use standard work stealing without overhead

---

## Autoscaling

Dynamic thread pool sizing that automatically adjusts worker count based on workload metrics.

### Overview

The autoscaler monitors thread pool metrics and makes scaling decisions to maintain optimal throughput while minimizing resource usage.

### API Overview

```cpp
#include <kcenon/thread/scaling/autoscaler.h>
#include <kcenon/thread/scaling/autoscaling_policy.h>

autoscaling_policy policy;
policy.min_workers = 2;
policy.max_workers = 16;
policy.scaling_mode = autoscaling_policy::mode::automatic;

autoscaler scaler(*pool, policy);
scaler.start();

// Manual overrides
auto decision = scaler.evaluate_now();
scaler.scale_to(8);
scaler.scale_up();
scaler.scale_down();

// Metrics
auto metrics = scaler.get_current_metrics();
auto history = scaler.get_metrics_history(60);
auto stats = scaler.get_stats();

scaler.stop();
```

### Scaling Policies

**Scale-Up Triggers** (ANY condition triggers scale-up):
- Queue depth per worker exceeds threshold (default: 100)
- Worker utilization above threshold (default: 80%)
- P95 latency above threshold (default: 50ms)
- Pending jobs above threshold (default: 1000)

**Scale-Down Triggers** (ALL conditions required):
- Worker utilization below threshold (default: 30%)
- Queue depth per worker below threshold (default: 10)
- Idle duration exceeded (default: 60s)

### Configuration

```cpp
autoscaling_policy policy;
policy.min_workers = 2;
policy.max_workers = std::thread::hardware_concurrency();
policy.scale_up.utilization_threshold = 0.8;
policy.scale_down.utilization_threshold = 0.3;
policy.scale_up_cooldown = std::chrono::seconds{30};
policy.scale_down_cooldown = std::chrono::seconds{60};
policy.sample_interval = std::chrono::milliseconds{1000};
policy.samples_for_decision = 5;
policy.scaling_callback = [](scaling_direction dir, scaling_reason reason,
                             std::size_t old_count, std::size_t new_count) {
    // Monitor scaling events
};
```

### Scaling Modes

| Mode | Description |
|------|-------------|
| `disabled` | No automatic scaling |
| `manual` | Only explicit API calls trigger scaling |
| `automatic` | Fully automatic based on metrics |

### Features

- **Asymmetric Scaling**: Fast scale-up (responsive), slow scale-down (conservative)
- **Cooldown Periods**: Prevents scaling oscillation
- **Multi-Sample Decisions**: Aggregates metrics over configurable windows
- **Manual Override**: Direct scale-to, scale-up, scale-down commands
- **Metrics History**: Access historical metrics for analysis
- **Multiplicative Scaling**: Optional multiplicative factor for rapid scaling

### Use Cases

- Cloud-native services with variable traffic
- Batch processing with fluctuating job volumes
- Microservices with auto-scaling requirements
- Cost optimization through dynamic resource allocation

---

## Diagnostics

Comprehensive thread pool health monitoring, bottleneck detection, and event tracing.

### Overview

The diagnostics system provides non-intrusive observability into thread pool behavior, including health checks, bottleneck analysis, and execution event tracing.

```cpp
#include <kcenon/thread/diagnostics/thread_pool_diagnostics.h>

diagnostics_config config;
config.enable_tracing = true;
config.recent_jobs_capacity = 1000;

thread_pool_diagnostics diag(*pool, config);
```

### Health Checks

```cpp
auto health = diag.health_check();

// HTTP-compatible health endpoint
int status_code = health.http_status_code();  // 200 or 503
std::string json = health.to_json();

// Component-level health
for (const auto& component : health.components) {
    // name, state, message, details
}

// Quick check
bool ok = diag.is_healthy();
```

**Health States**:

| State | HTTP Code | Description |
|-------|-----------|-------------|
| `healthy` | 200 | Fully operational |
| `degraded` | 200 | Operational with reduced capacity |
| `unhealthy` | 503 | Not operational |
| `unknown` | 503 | State cannot be determined |

### Bottleneck Detection

```cpp
auto report = diag.detect_bottlenecks();

if (report.has_bottleneck) {
    // type, description, severity (0-3)
    for (const auto& rec : report.recommendations) {
        // Actionable recommendations
    }
}
```

**Bottleneck Types**:

| Type | Description |
|------|-------------|
| `queue_full` | Queue at capacity |
| `slow_consumer` | Workers cannot keep up with producers |
| `worker_starvation` | Not enough workers for load |
| `lock_contention` | High mutex wait times |
| `uneven_distribution` | Work not evenly distributed |
| `memory_pressure` | Excessive memory allocations |

### Thread Dump

```cpp
auto threads = diag.dump_thread_states();
std::string formatted = diag.format_thread_dump();
// Outputs worker state, current job, utilization per worker
```

### Event Tracing

```cpp
// Enable tracing
diag.enable_tracing(true, 1000);

// Custom event listener
class MyListener : public execution_event_listener {
public:
    void on_event(const job_execution_event& event) override {
        // event.type: enqueued, started, completed, failed, etc.
        // event.to_json() for structured output
    }
};

diag.add_event_listener(std::make_shared<MyListener>());

// Query recent events
auto events = diag.get_recent_events(100);
```

### Export Formats

```cpp
std::string json = diag.to_json();           // JSON for dashboards
std::string text = diag.to_string();          // Human-readable
std::string prom = diag.to_prometheus();      // Prometheus metrics
```

**Prometheus Metrics**:
- `thread_pool_health_status` (gauge)
- `thread_pool_jobs_total` (counter)
- `thread_pool_success_rate` (gauge)
- `thread_pool_latency_avg_ms` (gauge)
- `thread_pool_workers_active` / `_idle` (gauge)
- `thread_pool_queue_depth` / `_saturation` (gauge)

### Features

- **Non-Intrusive**: Minimal overhead when not actively queried
- **Thread-Safe**: All methods callable from any thread
- **Kubernetes Integration**: HTTP-compatible health checks for liveness/readiness probes
- **Prometheus Compatible**: Export metrics in Prometheus format
- **Bottleneck Analysis**: Automatic detection with severity levels and recommendations
- **Event Tracing**: Fine-grained execution event tracking with listener pattern
- **Thread Dump**: Per-worker state snapshots for debugging

### Use Cases

- Kubernetes liveness and readiness probes
- Performance dashboards and alerting
- Production debugging and root cause analysis
- SLA monitoring and capacity planning

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

The thread_system provides a comprehensive threading framework with:

- **Multiple queue implementations** for different scenarios
- **Adaptive components** that automatically optimize
- **Type-based scheduling** for priority workloads
- **DAG-based orchestration** for complex dependency graphs
- **NUMA-aware scheduling** for optimal memory locality
- **Dynamic autoscaling** for workload-responsive pool sizing
- **Comprehensive diagnostics** for health monitoring and bottleneck detection
- **Rich synchronization primitives** for complex scenarios
- **Service infrastructure** for clean architecture
- **Optional integrations** for logging and monitoring

Choose the right features for your use case:
- **Simple tasks**: Standard thread pool with adaptive queue
- **Priority workloads**: Typed thread pool
- **Bounded resources**: Bounded job queue
- **Maximum performance**: Lock-free queue or adaptive mode
- **Variable load**: Adaptive queue (automatic optimization)
- **Complex workflows**: DAG scheduler for dependency management
- **Multi-socket servers**: NUMA-aware work stealing
- **Cloud services**: Autoscaling with diagnostics

---

**See Also**:
- [Performance Benchmarks](BENCHMARKS.md)
- [Architecture Guide](advanced/ARCHITECTURE.md)
- [API Reference](guides/API_REFERENCE.md)
- [User Guide](guides/USER_GUIDE.md)

---

**Last Updated**: 2026-02-08
**Maintained by**: kcenon@naver.com
