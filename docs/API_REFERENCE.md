# thread_system API Reference

> **Version**: 3.1.0
> **Last Updated**: 2025-01-09
> **Language**: [English]

## Table of Contents

1. [Namespace](#namespace)
2. [thread_pool (Recommended)](#thread_pool-recommended)
3. [typed_thread_pool](#typed_thread_pool)
4. [Queue Factory](#queue-factory)
5. [Concurrent Queues](#concurrent-queues)
6. [Synchronization Primitives](#synchronization-primitives)
7. [Diagnostics API](#diagnostics-api)
8. [DAG Scheduler](#dag-scheduler)
9. [NUMA Topology](#numa-topology)

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

## Diagnostics API

### Overview

**Header**: `#include <kcenon/thread/diagnostics.h>`

**Description**: Comprehensive diagnostics API for thread pool monitoring, health checks, and event tracing.

**Key Features**:
- Thread dump functionality
- Job inspection and tracing
- Bottleneck detection
- Health check integration (Kubernetes, Prometheus compatible)
- Event tracing with listener support

### Quick Start

```cpp
#include <kcenon/thread/thread_pool.h>
#include <kcenon/thread/diagnostics.h>

using namespace kcenon::thread;
using namespace kcenon::thread::diagnostics;

auto pool = std::make_shared<thread_pool>("MyPool");
pool->start();

// Get diagnostics
auto& diag = pool->diagnostics();

// Health check
auto health = diag.health_check();
if (!health.is_operational()) {
    std::cerr << health.to_json() << std::endl;
}

// Thread dump
std::cout << diag.format_thread_dump() << std::endl;

// Bottleneck detection
auto report = diag.detect_bottlenecks();
if (report.has_bottleneck) {
    std::cerr << report.to_string() << std::endl;
}
```

### Health Check

```cpp
// Get comprehensive health status
auto health = pool->diagnostics().health_check();

// Check overall status
if (health.is_healthy()) {
    std::cout << "Pool is healthy" << std::endl;
}

// HTTP integration
return http_response(health.http_status_code(), health.to_json());

// Prometheus metrics
std::cout << health.to_prometheus("MyPool") << std::endl;
```

#### Health States

| State | HTTP Code | Description |
|-------|-----------|-------------|
| `healthy` | 200 | All components operational |
| `degraded` | 200 | Operational but with reduced capacity |
| `unhealthy` | 503 | Not operational |
| `unknown` | 503 | Status cannot be determined |

### Thread Dump

```cpp
// Get thread states
auto threads = pool->diagnostics().dump_thread_states();
for (const auto& t : threads) {
    std::cout << t.thread_name << ": " << worker_state_to_string(t.state)
              << " (utilization: " << t.utilization * 100 << "%)" << std::endl;
}

// Formatted dump
std::cout << pool->diagnostics().format_thread_dump() << std::endl;
```

### Job Inspection

```cpp
// Active jobs currently executing
auto active = pool->diagnostics().get_active_jobs();

// Pending jobs in queue
auto pending = pool->diagnostics().get_pending_jobs(100);

// Recent completed/failed jobs
auto recent = pool->diagnostics().get_recent_jobs(50);
```

### Bottleneck Detection

```cpp
auto report = pool->diagnostics().detect_bottlenecks();

if (report.has_bottleneck) {
    std::cout << "Type: " << bottleneck_type_to_string(report.type) << std::endl;
    std::cout << "Severity: " << report.severity_string() << std::endl;

    for (const auto& rec : report.recommendations) {
        std::cout << "  - " << rec << std::endl;
    }
}
```

#### Bottleneck Types

| Type | Description |
|------|-------------|
| `queue_full` | Queue at capacity |
| `slow_consumer` | Workers can't keep up |
| `worker_starvation` | Not enough workers |
| `lock_contention` | High mutex wait times |
| `uneven_distribution` | Work not evenly distributed |
| `memory_pressure` | Excessive memory usage |

### Event Tracing

```cpp
// Enable tracing
pool->diagnostics().enable_tracing(true, 1000);

// Add custom listener
class MyListener : public execution_event_listener {
public:
    void on_event(const job_execution_event& event) override {
        std::cout << event.to_json() << std::endl;
    }
};

auto listener = std::make_shared<MyListener>();
pool->diagnostics().add_event_listener(listener);

// Get recent events
auto events = pool->diagnostics().get_recent_events(100);
```

### Export Formats

```cpp
// JSON export
std::cout << pool->diagnostics().to_json() << std::endl;

// Human-readable string
std::cout << pool->diagnostics().to_string() << std::endl;

// Prometheus metrics
std::cout << pool->diagnostics().to_prometheus() << std::endl;
```

### Performance Characteristics

| Operation | Overhead | Notes |
|-----------|----------|-------|
| `health_check()` | ~15 μs | Full health evaluation |
| `is_healthy()` | ~50 ns | Quick check |
| `detect_bottlenecks()` | ~3 μs | Full analysis |
| `dump_thread_states()` | ~5 μs | O(n) worker count |
| Event recording (enabled) | ~250 ns | Per event |
| Event recording (disabled) | ~12 ns | Negligible |

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

## DAG Scheduler

### Overview

**Header**: `#include <kcenon/thread/dag/dag_scheduler.h>`

**Description**: DAG-based job scheduling with dependency management

The DAG Scheduler provides a way to define jobs with dependencies and execute them in the correct order. Jobs are executed in parallel when their dependencies are satisfied.

### dag_job

**Header**: `#include <kcenon/thread/dag/dag_job.h>`

A job with dependency support for DAG-based scheduling.

```cpp
#include <kcenon/thread/dag/dag_job.h>

using namespace kcenon::thread;

// Create a DAG job
auto job = std::make_unique<dag_job>("process_data");
job->set_work([]() -> common::VoidResult {
    // Do work
    return common::ok();
});
job->add_dependency(other_job_id);
```

#### Key Types

| Type | Description |
|------|-------------|
| `job_id` | `std::uint64_t` unique identifier |
| `INVALID_JOB_ID` | Constant for invalid job ID (0) |
| `dag_job_state` | Job state enum: `pending`, `ready`, `running`, `completed`, `failed`, `cancelled`, `skipped` |

### dag_job_builder

**Header**: `#include <kcenon/thread/dag/dag_job_builder.h>`

Fluent builder for creating dag_job instances with validation and reusability.

#### Basic Usage

```cpp
#include <kcenon/thread/dag/dag_job_builder.h>

using namespace kcenon::thread;

auto job = dag_job_builder("process_data")
    .depends_on(fetch_job_id)
    .work([]() -> common::VoidResult {
        process_data();
        return common::ok();
    })
    .on_failure([]() -> common::VoidResult {
        log_failure();
        return common::ok();
    })
    .build();
```

#### Methods

| Method | Description |
|--------|-------------|
| `work(callable)` | Sets the work function (returns `common::VoidResult`) |
| `work_with_result<T>(callable)` | Sets work function with result type T |
| `returns<T>()` | Specifies the expected result type |
| `depends_on(job_id)` | Adds a single dependency |
| `depends_on({ids...})` | Adds multiple dependencies |
| `on_failure(callable)` | Sets fallback function on failure |
| `is_valid()` | Validates builder configuration |
| `get_validation_error()` | Gets validation error message |
| `build()` | Builds the dag_job (returns nullptr if invalid) |
| `reset()` | Resets builder for reuse |

#### Validation

The builder validates configuration before building:
- A work function must be set (via `work()` or `work_with_result<T>()`)
- If invalid, `build()` returns `nullptr`

```cpp
dag_job_builder builder("job");
// No work function set
ASSERT_FALSE(builder.is_valid());
ASSERT_EQ(builder.build(), nullptr);
```

#### Reusability

After calling `build()`, the builder is automatically reset and can be reused:

```cpp
dag_job_builder builder("reusable");

auto job1 = builder.work(work_func1).depends_on(1).build();
// Builder is now reset

auto job2 = builder.work(work_func2).depends_on({2, 3}).build();
// Both jobs are valid with different dependencies
```

### dag_scheduler

**Header**: `#include <kcenon/thread/dag/dag_scheduler.h>`

Manages DAG-based job execution.

```cpp
#include <kcenon/thread/dag/dag_scheduler.h>

using namespace kcenon::thread;

// Create scheduler with a thread pool
dag_scheduler scheduler(pool);

// Add jobs
auto job_a = scheduler.add_job(
    dag_job_builder("job_a")
        .work([]() -> common::VoidResult { return common::ok(); })
        .build()
);

auto job_b = scheduler.add_job(
    dag_job_builder("job_b")
        .depends_on(job_a)
        .work([]() -> common::VoidResult { return common::ok(); })
        .build()
);

// Execute all jobs
auto future = scheduler.execute_all();
auto result = future.get();
```

---

## Work-Stealing Deque

### Overview

**Header**: `#include <kcenon/thread/lockfree/work_stealing_deque.h>`

The `work_stealing_deque` class implements a lock-free work-stealing deque based on the Chase-Lev algorithm. It provides efficient local operations for the owner thread and concurrent stealing for other threads.

### Key Features

- **Owner-side push/pop**: LIFO order for cache locality
- **Thief-side steal**: FIFO order for fairness
- **Batch stealing**: Efficiently steal multiple items at once
- **Lock-free operations**: Proper memory ordering with CAS operations
- **Dynamic resizing**: Automatic growth when full

### work_stealing_deque

```cpp
template<typename T>
class work_stealing_deque {
public:
    // Construction
    explicit work_stealing_deque(std::size_t log_initial_size = 5);

    // Owner operations (single-threaded)
    void push(T item);
    [[nodiscard]] std::optional<T> pop();

    // Thief operations (multi-threaded)
    [[nodiscard]] std::optional<T> steal();
    [[nodiscard]] std::vector<T> steal_batch(std::size_t max_count);

    // Query
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::size_t capacity() const noexcept;

    // Maintenance
    void cleanup_old_arrays();
};
```

### Thread Safety

| Method | Thread Safety |
|--------|---------------|
| `push()` | Owner thread only |
| `pop()` | Owner thread only |
| `steal()` | Any thief thread (concurrent safe) |
| `steal_batch()` | Any thief thread (concurrent safe) |
| `empty()`, `size()` | Any thread (approximate) |

### Usage Example

```cpp
#include <kcenon/thread/lockfree/work_stealing_deque.h>
#include <thread>
#include <vector>

using namespace kcenon::thread::lockfree;

int main() {
    work_stealing_deque<int*> deque;
    std::vector<int> values(100);

    // Owner pushes work
    for (int i = 0; i < 100; ++i) {
        values[i] = i;
        deque.push(&values[i]);
    }

    // Thieves steal work in batches
    std::vector<std::thread> thieves;
    for (int t = 0; t < 4; ++t) {
        thieves.emplace_back([&]() {
            while (!deque.empty()) {
                // Batch steal up to 4 items at once
                auto batch = deque.steal_batch(4);
                for (auto* item : batch) {
                    // Process stolen item
                }
            }
        });
    }

    // Owner can also pop locally (LIFO)
    while (auto item = deque.pop()) {
        // Process local item
    }

    for (auto& t : thieves) {
        t.join();
    }

    return 0;
}
```

### Batch Stealing

The `steal_batch()` method allows efficient stealing of multiple items:

```cpp
// Steal up to 4 items atomically
auto batch = deque.steal_batch(4);

// Returns:
// - Empty vector if deque is empty or contention
// - Up to max_count items in FIFO order
// - May return fewer items than requested
```

**Benefits of batch stealing**:
- Reduced contention overhead (one CAS instead of multiple)
- Better cache efficiency for transferring work
- Improved throughput under high contention

---

## NUMA Topology

### Overview

**Header**: `#include <kcenon/thread/stealing/numa_topology.h>`

The `numa_topology` class provides information about the system's NUMA (Non-Uniform Memory Access) topology, enabling NUMA-aware task scheduling for improved performance on multi-socket systems.

### Platform Support

| Platform | Support Level | Detection Method |
|----------|--------------|------------------|
| Linux | Full | /sys/devices/system/node |
| macOS | Fallback | Single-node topology |
| Windows | Fallback | Single-node topology |

### numa_node

```cpp
struct numa_node {
    int node_id;                    // NUMA node identifier
    std::vector<int> cpu_ids;       // CPUs belonging to this node
    std::size_t memory_size_bytes;  // Total memory on this node
};
```

### numa_topology

```cpp
class numa_topology {
public:
    // Detection
    [[nodiscard]] static auto detect() -> numa_topology;

    // Node queries
    [[nodiscard]] auto get_node_for_cpu(int cpu_id) const -> int;
    [[nodiscard]] auto get_distance(int node1, int node2) const -> int;
    [[nodiscard]] auto is_same_node(int cpu1, int cpu2) const -> bool;
    [[nodiscard]] auto is_numa_available() const -> bool;

    // Statistics
    [[nodiscard]] auto node_count() const -> std::size_t;
    [[nodiscard]] auto cpu_count() const -> std::size_t;
    [[nodiscard]] auto get_nodes() const -> const std::vector<numa_node>&;
    [[nodiscard]] auto get_cpus_for_node(int node_id) const -> std::vector<int>;
};
```

### Usage Example

```cpp
#include <kcenon/thread/stealing/numa_topology.h>
#include <iostream>

using namespace kcenon::thread;

int main() {
    // Detect system NUMA topology
    auto topology = numa_topology::detect();

    std::cout << "NUMA nodes: " << topology.node_count() << "\n";
    std::cout << "Total CPUs: " << topology.cpu_count() << "\n";

    if (topology.is_numa_available()) {
        // System has multiple NUMA nodes
        for (const auto& node : topology.get_nodes()) {
            std::cout << "Node " << node.node_id << ": "
                      << node.cpu_ids.size() << " CPUs\n";
        }

        // Check CPU locality
        int cpu1 = 0, cpu2 = 4;
        if (topology.is_same_node(cpu1, cpu2)) {
            std::cout << "CPU " << cpu1 << " and " << cpu2
                      << " are on the same NUMA node\n";
        }

        // Get inter-node distance
        int dist = topology.get_distance(0, 1);
        std::cout << "Distance between node 0 and 1: " << dist << "\n";
    } else {
        std::cout << "Single NUMA node (UMA system)\n";
    }

    return 0;
}
```

### Distance Values

The `get_distance()` method returns a relative measure of communication cost:

| Distance | Meaning |
|----------|---------|
| 10 | Local (same node) |
| 20-40 | Adjacent nodes |
| 50+ | Remote nodes |
| -1 | Invalid node ID |

---

## Notes

### Thread Safety

- **thread_pool**: Thread-safe (all methods)
- **typed_thread_pool_t**: Thread-safe (all methods)
- **concurrent_queue<T>**: Thread-safe (multiple producers/consumers)
- **lockfree_job_queue**: Thread-safe (multiple producers/consumers)
- **job_queue**: Thread-safe (mutex-based)
- **adaptive_job_queue**: Thread-safe (multiple producers/consumers)
- **numa_topology**: Thread-safe after construction (immutable)

### Recommendations

- **General task processing**: Use `thread_pool` (recommended)
- **Priority-based scheduling**: Use `typed_thread_pool_t`
- **High-throughput queue**: Use `lockfree_job_queue`
- **Generic type queue**: Use `concurrent_queue<T>`
- **Variable load**: Use `adaptive_job_queue`
- **Safe memory reclamation**: Use `safe_hazard_pointer` or `atomic_shared_ptr`

---

**Created**: 2025-11-21
**Updated**: 2025-01-09
**Version**: 3.1.0
**Author**: kcenon@naver.com
