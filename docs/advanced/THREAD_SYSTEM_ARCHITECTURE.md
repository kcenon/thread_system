# Architecture - Thread System

> **Language:** **English** | [한국어](ARCHITECTURE_KO.md)

## Overview

The Thread System is a production-ready C++20 high-performance threading framework designed with modular, interface-based architecture for concurrent applications. Built with zero external dependencies (except standard library), it provides intuitive abstractions and robust implementations that democratize concurrent programming.

**Version:** 1.0.0
**Last Updated:** 2025-10-22

---

## Table of Contents

- [Design Philosophy](#design-philosophy)
- [Layered Architecture](#layered-architecture)
- [Core Components](#core-components)
- [Integration Architecture](#integration-architecture)
- [Threading Model](#threading-model)
- [Memory Management](#memory-management)
- [Performance Characteristics](#performance-characteristics)
- [Design Patterns](#design-patterns)
- [Build Configuration](#build-configuration)

---

## Design Philosophy

### Core Principles

1. **Zero-Overhead Abstractions**: Modern C++ design with minimal runtime cost
   - Template metaprogramming for compile-time optimization
   - Move semantics and perfect forwarding throughout
   - Inline functions for hot paths

2. **Interface-Driven Design**: Clean separation via abstract interfaces
   - logger_interface, monitoring_interface for ecosystem integration
   - executor_interface, scheduler_interface for component abstraction
   - Zero circular dependencies through interface-only dependencies

3. **Robust and Reliable**: Production-ready implementation
   - Thread-safe mutex-based job queue
   - Dynamic contention handling
   - Workload-aware scheduling policies

4. **Performance First**: Optimized for production workloads
   - 1.16M+ jobs/second real-world throughput
   - Sub-microsecond latency (77ns job scheduling)
   - Linear scaling up to hardware concurrency

5. **Type Safety**: Comprehensive compile-time guarantees
   - Result<T> pattern for error handling
   - Concepts for template constraints (C++20)
   - Strong type system prevents misuse

---

## Layered Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                      Application Layer                       │
│  (User code using thread pools via builder or DI adapter)   │
└───────────────────────────┬──────────────────────────────────┘
                            │
┌───────────────────────────┴──────────────────────────────────┐
│                    Public API Layer                          │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────────────┐  │
│  │thread_pool  │  │typed_thread_ │  │ service_registry │  │
│  │  (main)     │  │  pool (type) │  │     (DI)         │  │
│  └─────────────┘  └──────────────┘  └───────────────────┘  │
└───────────────────────────┬──────────────────────────────────┘
                            │
┌───────────────────────────┴──────────────────────────────────┐
│                     Core Layer                               │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────────┐   │
│  │ thread_base  │  │  job_queue   │  │ cancellation_   │   │
│  │ (lifecycle)  │  │  (adaptive)  │  │ token (coop)    │   │
│  └──────────────┘  └──────────────┘  └─────────────────┘   │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────────┐   │
│  │     job      │  │ error_codes  │  │ sync_           │   │
│  │  (work unit) │  │ (-100~-199)  │  │ primitives      │   │
│  └──────────────┘  └──────────────┘  └─────────────────┘   │
└───────────────────────────┬──────────────────────────────────┘
                            │
┌───────────────────────────┴──────────────────────────────────┐
│                  Interface Layer                             │
│  ┌─────────────────────┐  ┌─────────────────────────────┐   │
│  │ executor_interface  │  │ scheduler_interface         │   │
│  │ (pool abstraction)  │  │ (queue abstraction)         │   │
│  └─────────────────────┘  └─────────────────────────────┘   │
│  ┌─────────────────────┐  ┌─────────────────────────────┐   │
│  │ logger_interface    │  │ monitoring_interface        │   │
│  │ (logging)           │  │ (metrics)                   │   │
│  └─────────────────────┘  └─────────────────────────────┘   │
│  ┌─────────────────────┐  ┌─────────────────────────────┐   │
│  │ monitorable_        │  │ service_container           │   │
│  │ interface (metrics) │  │ (dependency injection)      │   │
│  └─────────────────────┘  └─────────────────────────────┘   │
└───────────────────────────┬──────────────────────────────────┘
                            │
┌───────────────────────────┴──────────────────────────────────┐
│               Implementation Layer                           │
│  ┌──────────────────┐  ┌──────────────────┐  ┌────────────┐ │
│  │ Thread Pools:    │  │ Job Queues:      │  │ Workers:   │ │
│  │ • thread_pool    │  │ • job_queue      │  │ • thread_  │ │
│  │   (standard)     │  │   (mutex-based)  │  │   worker   │ │
│  │ • typed_thread_  │  │                  │  │ • typed_   │ │
│  │   pool (priority)│  │                  │  │   worker   │ │
│  │                  │  │                  │  │            │ │
│  │                  │  │                  │  │            │ │
│  └──────────────────┘  └──────────────────┘  └────────────┘ │
└───────────────────────────┬──────────────────────────────────┘
                            │
┌───────────────────────────┴──────────────────────────────────┐
│                 Advanced Features Layer                      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │ hazard_      │  │ node_pool    │  │ bounded_job_     │  │
│  │ pointer      │  │ (memory)     │  │ queue (limit)    │  │
│  └──────────────┘  └──────────────┘  └──────────────────┘  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │ worker_      │  │ future_      │  │ event_bus        │  │
│  │ policy       │  │ extensions   │  │ (events)         │  │
│  └──────────────┘  └──────────────┘  └──────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### Layer Responsibilities

1. **Application Layer**: User-facing code using thread system
2. **Public API Layer**: Main entry points (pools, builders, adapters)
3. **Core Layer**: Business logic (threading, scheduling, synchronization)
4. **Interface Layer**: Abstract contracts for all components
5. **Implementation Layer**: Concrete pool/queue/worker implementations
6. **Advanced Features Layer**: Optional high-level features

---

## Core Components

### 1. thread_base (`core/base/include/thread_base.h`)

**Purpose**: Abstract base class for all worker threads

**Key Methods**:
```cpp
class thread_base {
public:
    // Lifecycle
    auto start() -> result_void;
    auto stop() -> result_void;

    // Configuration
    auto set_wake_interval(const std::optional<std::chrono::milliseconds>& interval) -> void;
    auto get_wake_interval() const -> std::optional<std::chrono::milliseconds>;

    // Status
    auto is_running() const -> bool;
    auto get_thread_title() const -> std::string;

protected:
    // Override in derived classes
    virtual auto before_start() -> result_void { return {}; }
    virtual auto do_work() -> result_void = 0;
    virtual auto after_stop() -> result_void { return {}; }
    virtual auto should_continue_work() const -> bool { return false; }
};
```

**Features**:
- C++20 std::jthread support with fallback to std::thread
- Customizable lifecycle hooks (before_start, after_stop)
- Wake interval for periodic tasks
- Thread naming for debugging
- Cooperative cancellation via stop_token

---

### 2. job (`core/jobs/include/job.h`)

**Purpose**: Abstract base class for units of work

**Key Methods**:
```cpp
class job {
public:
    job(const std::string& name = "job");
    virtual ~job();

    // Core functionality
    virtual auto do_work() -> result_void;

    // Cancellation support
    virtual auto set_cancellation_token(const cancellation_token& token) -> void;
    virtual auto get_cancellation_token() const -> cancellation_token;

    // Queue association
    virtual auto set_job_queue(const std::shared_ptr<job_queue>& queue) -> void;
    virtual auto get_job_queue() const -> std::shared_ptr<job_queue>;

    // Metadata
    auto get_name() const -> std::string;
    virtual auto to_string() const -> std::string;
};
```

**Derived Classes**:
- `callback_job`: Lambda-based job implementation
- `typed_job_t<T>`: Job with priority/type information
- `cancellable_job`: Job with enhanced cancellation support

---

### 3. job_queue (`core/jobs/include/job_queue.h`)

**Purpose**: Thread-safe FIFO queue for job management

**Key Methods**:
```cpp
class job_queue : public scheduler_interface {
public:
    // Queue operations
    [[nodiscard]] auto enqueue(std::unique_ptr<job>&& value) -> result_void;
    [[nodiscard]] auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> result_void;
    [[nodiscard]] auto dequeue() -> result<std::unique_ptr<job>>;
    [[nodiscard]] auto dequeue_batch() -> std::deque<std::unique_ptr<job>>;

    // State management
    auto clear() -> void;
    auto stop() -> void;

    // Status
    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> std::size_t;
    [[nodiscard]] auto is_stopped() const -> bool;
};
```

**Features**:
- Mutex-based thread-safe operations
- Batch operations for improved throughput
- Blocking dequeue with condition variable
- Graceful shutdown support

---

### 4. Thread Pool (`implementations/thread_pool/`)

**Purpose**: Standard thread pool with mutex-based job queue

```cpp
class thread_pool : public executor_interface {
public:
    thread_pool(const std::string& pool_name = "thread_pool");

    // Lifecycle
    auto start() -> result_void;
    auto stop(bool immediately = false) -> result_void;
    auto shutdown() -> result_void;

    // Job submission
    auto execute(std::unique_ptr<job>&& job) -> result_void;  // executor_interface
    auto enqueue(std::unique_ptr<job>&& job) -> result_void;
    auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> result_void;

    // Convenience wrapper
    bool submit_task(std::function<void()> task);

    // Worker management
    auto add_worker(std::unique_ptr<thread_worker>&& worker) -> result_void;
    auto get_thread_count() const -> size_t;
    auto get_idle_worker_count() const -> size_t;

    // Status
    auto get_pending_task_count() const -> size_t;
    auto is_running() const -> bool;
};
```

**Features**:
- Adaptive job queue for automatic optimization
- Dynamic worker addition/removal
- Graceful and immediate shutdown modes
- Metrics reporting via monitoring_interface
- Thread-safe operations throughout

**Performance**:
- **Real-world throughput**: 1.16M jobs/second (10 workers)
- **Job scheduling latency**: 77 nanoseconds average
- **Linear scaling**: 96% efficiency up to 8 workers
- **Memory baseline**: < 1MB with mutex-based queues

---

### 6. Typed Thread Pool (`implementations/typed_thread_pool/`)

**Purpose**: Priority-based thread pool with type-aware scheduling

```cpp
template<typename T = job_types>
class typed_thread_pool_t : public executor_interface {
public:
    typed_thread_pool_t(const std::string& pool_name = "typed_pool");

    // Type-aware job submission
    auto enqueue(std::unique_ptr<typed_job_t<T>>&& job) -> result_void;
    auto enqueue_batch(std::vector<std::unique_ptr<typed_job_t<T>>>&& jobs) -> result_void;

    // Per-type queue management
    auto get_queue_size(const T& type) const -> size_t;
    auto get_type_statistics(const T& type) const -> typed_queue_statistics_t<T>;

    // Worker configuration
    auto add_worker(std::unique_ptr<typed_thread_worker_t<T>>&& worker,
                    const std::vector<T>& responsibility_list) -> result_void;
};

// Default priority levels
enum class job_types { RealTime, Batch, Background };

// Type-aware job
template<typename T>
class typed_job_t : public job {
public:
    typed_job_t(const T& type, const std::string& name = "typed_job");

    auto get_type() const -> T;
    auto set_type(const T& type) -> void;
};
```

**Scheduling Strategy**:
1. **Priority ordering**: RealTime > Batch > Background
2. **Per-type queues**: Separate job queue for each type
3. **Worker specialization**: Responsibility lists define what each worker handles
4. **FIFO within type**: Strict ordering within same priority level

**Performance**:
- **Real-world throughput**: 1.24M jobs/second (6 workers, 3 types)
- **Type accuracy**: 99%+ under all conditions
- **6.9% faster** than standard pool with fewer workers
- **Adaptive queues**: Automatic per-type optimization

---

### 7. Synchronization Primitives (`core/sync/`)

**Purpose**: Enhanced synchronization wrappers with timeout support

#### scoped_lock_guard

```cpp
template<typename Mutex>
class scoped_lock_guard {
public:
    scoped_lock_guard(Mutex& mutex, std::chrono::milliseconds timeout = {});

    [[nodiscard]] bool owns_lock() const noexcept;
    explicit operator bool() const noexcept;
};
```

#### condition_variable_wrapper

```cpp
class condition_variable_wrapper {
public:
    template<typename Predicate>
    void wait(std::unique_lock<std::mutex>& lock, Predicate pred);

    template<typename Rep, typename Period, typename Predicate>
    bool wait_for(std::unique_lock<std::mutex>& lock,
                  const std::chrono::duration<Rep, Period>& rel_time,
                  Predicate pred);

    void notify_one() noexcept;
    void notify_all() noexcept;
};
```

#### cancellation_token

```cpp
class cancellation_token {
public:
    static cancellation_token create();
    static cancellation_token create_linked(std::initializer_list<cancellation_token> tokens);

    void cancel();
    [[nodiscard]] bool is_cancelled() const;
    void throw_if_cancelled() const;
    void register_callback(std::function<void()> callback);
};
```

**Features**:
- Timeout support for lock acquisition
- Predicate-based condition variables
- Hierarchical cancellation with linked tokens
- Callback registration for cancellation events
- Thread-safe weak pointer usage

---

## Integration Architecture

### Logger System Integration

**Purpose**: Optional logging for thread operations

```cpp
// Interface definition
namespace thread_module {
    class logger_interface {
    public:
        virtual void log(log_level level, std::string_view message) = 0;
        virtual void flush() = 0;
    };
}

// Adapter for logger_system
class logger_adapter : public thread_module::logger_interface {
    std::shared_ptr<kcenon::logger::logger> logger_;
public:
    void log(log_level level, std::string_view message) override {
        logger_->log(convert_level(level), message);
    }
};

// Usage with dependency injection
auto logger = kcenon::logger::logger_builder().build().value();
auto adapter = std::make_shared<logger_adapter>(logger);

thread_module::service_container::global()
    .register_singleton<thread_module::logger_interface>(adapter);
```

**Benefits**:
- Reuse existing loggers
- Consistent logging across application
- Optional dependency (thread_system works without logger)

---

### Monitoring System Integration

**Purpose**: Real-time performance metrics collection

```cpp
// Interface definition
namespace monitoring_interface {
    struct thread_pool_metrics {
        std::string pool_name;
        uint32_t pool_instance_id;
        uint64_t jobs_submitted{0};
        uint64_t jobs_completed{0};
        uint32_t worker_count{0};
        uint32_t idle_workers{0};
        uint64_t queue_size{0};
    };

    class monitoring_interface {
    public:
        virtual void update_thread_pool_metrics(
            const std::string& pool_name,
            uint32_t pool_instance_id,
            const thread_pool_metrics& metrics) = 0;
    };
}

// Usage with dependency injection
auto monitoring = std::make_shared<kcenon::monitoring::monitoring>();

monitoring_interface::service_container::global()
    .register_singleton<monitoring_interface::monitoring_interface>(monitoring);

// Thread pool automatically reports metrics
auto pool = std::make_shared<thread_pool>("monitored_pool");
pool->start();  // Metrics reported via context
```

**Logged Metrics**:
- Thread pool: workers, idle count, queue size, job counts
- Worker metrics: jobs processed, processing time, batch operations
- Queue metrics: enqueue/dequeue latency, retry counts, strategy changes
- System metrics: CPU usage, memory usage, thread count

---

### Common System Integration

**Purpose**: Standardized error handling via Result<T>

```cpp
// All thread_system APIs return Result<T>
auto result = pool->start();
if (!result.has_value()) {
    const auto& error = result.error();
    std::cerr << "Pool start failed: " << error.message
              << " (code: " << static_cast<int>(error.code) << ")\n";
    return;
}

// Monadic error handling
auto result = pool->start()
    .and_then([&pool]() {
        return pool->enqueue(create_job());
    })
    .or_else([](const auto& error) {
        log_error(error);
        return retry_operation();
    });
```

**Error Code Range**: -100 to -199 (allocated in common_system)
- System integration: -100 to -109
- Pool lifecycle: -110 to -119
- Job submission: -120 to -129
- Worker management: -130 to -139

---

## Threading Model

### Worker Thread Lifecycle

```
┌─────────────────────────────────────┐
│         Application                 │
│                                     │
│  pool->start()                      │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│       Thread Pool                   │
│                                     │
│  For each worker:                   │
│  1. before_start()                  │
│  2. Start worker thread             │
│  3. Worker enters event loop        │
└─────────────┬───────────────────────┘
              │
      ┌───────┴─────────┬─────────────┐
      │                 │             │
┌─────▼─────┐   ┌───────▼──────┐  ┌──▼──────┐
│  Worker 1 │   │   Worker 2   │  │Worker N │
│           │   │              │  │         │
│ Event Loop│   │  Event Loop  │  │Event Loop│
│ ┌───────┐ │   │  ┌───────┐  │  │┌───────┐│
│ │Dequeue│ │   │  │Dequeue│  │  ││Dequeue││
│ │Job    │ │   │  │Job    │  │  ││Job    ││
│ └───┬───┘ │   │  └───┬───┘  │  │└───┬───┘│
│     │     │   │      │      │  │    │    │
│ ┌───▼────┐│   │  ┌───▼────┐ │  │┌───▼────┐│
│ │Execute ││   │  │Execute │ │  ││Execute ││
│ │Job     ││   │  │Job     │ │  ││Job     ││
│ └────────┘│   │  └────────┘ │  │└────────┘│
└───────────┘   └─────────────┘  └──────────┘
```

### Job Queue Flow

```
Application Thread 1 ─────┐
Application Thread 2 ─────┤
Application Thread N ─────┼──> enqueue() ──> Adaptive Queue
                          │                      │
                          │     ┌────────────────┴─────────────┐
                          │     │   Strategy Selection         │
                          │     │   - Low: Mutex (96ns)        │
                          │     │   - High: Lock-free (320ns)  │
                          │     └──────────────┬───────────────┘
                          │                    │
                          └────────────────────┼────────────────┐
                                              │                │
                                      ┌───────▼──────┐  ┌─────▼──────┐
                                      │ Worker 1     │  │ Worker N   │
                                      │ dequeue()    │  │ dequeue()  │
                                      └──────────────┘  └────────────┘
```

### Adaptive Queue Strategy Selection

```
Startup:
  ├─> Initialize with MUTEX strategy (minimal overhead)
  │
Runtime Monitoring (every 100 operations):
  ├─> Collect metrics:
  │   ├─> Enqueue latency (P50, P99)
  │   ├─> Dequeue latency (P50, P99)
  │   ├─> Contention ratio (failed lock attempts / total)
  │   └─> Active thread count
  │
Strategy Decision:
  ├─> If contention_ratio > 0.3 AND latency > 200ns:
  │   └─> Switch to LOCKFREE (up to 7.7x faster enqueue)
  │
  ├─> If contention_ratio < 0.1 AND latency < 100ns:
  │   └─> Switch to MUTEX (lower overhead)
  │
  └─> Else:
      └─> Keep current strategy
```

---

## Memory Management

### Smart Pointer Usage (Grade A - 100% RAII)

```cpp
// All dynamic allocations use smart pointers
std::shared_ptr<thread_pool> pool;           // Pool lifetime
std::unique_ptr<thread_worker> worker;       // Worker ownership
std::unique_ptr<job> job_instance;           // Job ownership
std::shared_ptr<logger_interface> logger;    // Service lifetime
```

**Benefits**:
- Zero manual memory management
- Exception-safe resource cleanup
- Clear ownership semantics
- No memory leaks (validated by AddressSanitizer)

### Hazard Pointer for Lock-Free Structures

```cpp
template<typename T>
class hazard_pointer {
public:
    // Protect a pointer from reclamation
    void protect(const T* ptr);

    // Retire a pointer for later reclamation
    void retire(T* ptr);

    // Clear protection
    void clear();

    // Reclaim retired pointers
    static void reclaim_all();
};
```

**Purpose**: Safe memory reclamation in lock-free MPMC queues

**Mechanism**:
1. Reader publishes pointer to hazard list before dereferencing
2. Writer checks hazard list before reclaiming memory
3. Safe reclamation only when no readers hold pointer
4. Prevents ABA problem and use-after-free

### Node Pool for Efficient Allocation

```cpp
template<typename T>
class node_pool {
public:
    node_pool(size_t initial_chunks = 1, size_t chunk_size = 256);

    // Allocate node from pool
    T* allocate();

    // Return node to pool
    void deallocate(T* ptr);

    // Statistics
    struct pool_statistics {
        size_t total_allocated{0};
        size_t total_deallocated{0};
        size_t active_nodes{0};
        size_t pool_capacity{0};
    };
};
```

**Optimization**:
- Reduces allocation overhead by 93.75% (from 1024 to 256 initial nodes)
- Lazy initialization saves ~50% memory when queue unused
- Thread-local caching for lock-free operation
- Cache-line aligned nodes prevent false sharing

---

## Performance Characteristics

### Benchmarks

**Platform**: Apple M1 (8-core) @ 3.2GHz, 16GB RAM, macOS Sonoma

| Metric | Value | Comparison |
|--------|-------|------------|
| **Real-world throughput (standard)** | 1.16M jobs/s | Baseline (10 workers) |
| **Real-world throughput (typed)** | 1.24M jobs/s | +6.9% faster (6 workers) |
| **Peak throughput** | 13.0M jobs/s | Theoretical max (empty jobs) |
| **Job scheduling latency (P50)** | 77 ns | 15.7x lower than alternatives |
| **Job queue (mutex-based)** | 96 ns | Thread-safe operation |
| **Worker scaling efficiency (8 workers)** | 96% | Near-linear scaling |
| **Memory baseline** | < 1MB | With mutex-based queues |
| **Thread creation overhead** | 24.5 μs | Measured on M1 |

### Optimization Techniques

1. **Efficient Mutex Usage**: Well-optimized mutex-based synchronization
2. **Batch Processing**: Process multiple jobs per worker iteration
3. **Zero-Copy**: Move semantics for job transfer
4. **Cache-Line Alignment**: Prevent false sharing in concurrent data structures
7. **Node Pooling**: Reduce allocation overhead

---

## Design Patterns

### 1. Command Pattern
- Job encapsulation for flexible task execution
- Callback jobs support lambda-based commands
- Type-safe job parameter passing

### 2. Observer Pattern
- Monitoring interface for metrics collection
- Logger interface for event notification
- Service container for dependency observation

### 3. Strategy Pattern
- Adaptive queue strategy selection (mutex, lock-free, adaptive)
- Worker policy system (scheduling, idle behavior)
- Backoff strategies for contention handling

### 4. Template Method Pattern
- thread_base defines workflow: before_start() → do_work() → after_stop()
- Derived classes customize specific behaviors
- Consistent lifecycle management

### 5. Dependency Injection
- service_container for runtime service registration
- service_registry for lightweight header-only DI
- Interface-based integration (logger, monitoring)

### 6. Factory Pattern
- Builder pattern for thread pool construction
- Job factory for creating typed jobs
- Worker factory for specialized workers

### 7. Object Pool Pattern
- node_pool for memory allocation
- Worker pool for thread reuse
- Connection pooling in typed pools

---

## Build Configuration

### CMake Options

```cmake
# Core options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)
option(BUILD_SAMPLES "Build sample programs" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Integration options
option(BUILD_WITH_COMMON_SYSTEM "Integrate with common_system" ON)
option(BUILD_THREADSYSTEM_AS_SUBMODULE "Build as submodule" OFF)

# Testing options
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

# Documentation
option(BUILD_DOCUMENTATION "Build Doxygen documentation" OFF)
```

### Dependency Resolution

**Priority Order** (CMakeLists.txt):
1. Local path: `/Users/raphaelshin/Sources/common_system/include`
2. User home: `/home/$USER/Sources/common_system/include`
3. GitHub workspace: `$ENV{GITHUB_WORKSPACE}/common_system/include`
4. Sibling directory: `${CMAKE_CURRENT_SOURCE_DIR}/../common_system/include`
5. Parent directory: `${CMAKE_SOURCE_DIR}/../common_system/include`
6. FetchContent from GitHub (fallback)

### Compiler Requirements

- **C++ Standard**: C++20 required
- **Compilers**: GCC 10+, Clang 10+, MSVC 19.29+ (Visual Studio 2019+)
- **Platforms**: Windows, Linux, macOS
- **Architectures**: x86, x86_64, ARM, ARM64

---

## Future Enhancements

### Planned Features

1. **Work Stealing**: Cross-worker job stealing for better load balancing
2. **SIMD Operations**: Vectorized batch processing for job queues
3. **GPU Integration**: CUDA/OpenCL support for heterogeneous computing
4. **Distributed Thread Pools**: Multi-node thread pool coordination
5. **Advanced Scheduling**: Machine learning-based workload prediction

### Research Areas

1. **Quantum Threading**: Quantum-inspired scheduling algorithms
2. **Energy Efficiency**: Power-aware scheduling policies
3. **Real-time Guarantees**: Hard real-time scheduling support
4. **Formal Verification**: Mathematical proof of correctness

---

## Error Code Allocation

Thread System uses error codes in the range **-100 to -199**.

Common error codes:
- `-100` to `-109`: System integration errors
- `-110` to `-119`: Pool lifecycle errors
- `-120` to `-129`: Job submission errors
- `-130` to `-139`: Worker management errors
- `-140` to `-149`: Queue operation errors
- `-150` to `-159`: Synchronization errors

See `core/sync/include/error_handling.h` for complete error code listing.

---

## References

- [README.md](README.md) - Project overview and quick start
- [CHANGELOG.md](CHANGELOG.md) - Version history
- [MIGRATION.md](MIGRATION.md) - Migration from older versions
- [BASELINE.md](BASELINE.md) - Build requirements and baseline
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) - Ecosystem architecture
- [docs/API_REFERENCE.md](docs/API_REFERENCE.md) - Complete API documentation
- [docs/USER_GUIDE.md](docs/USER_GUIDE.md) - Build and usage guide

---

**Last Updated:** 2025-10-22
**Version:** 1.0.0
**Maintainer:** kcenon@naver.com
