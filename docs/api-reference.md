# Thread System API Reference

Complete API documentation for the Thread System framework.

## Table of Contents

1. [Overview](#overview)
2. [Core Module](#core-module-thread_module)
   - [thread_base Class](#thread_base-class)
   - [job Class](#job-class)
   - [job_queue Class](#job_queue-class)
   - [result Template](#resultt-template)
3. [Thread Pool Module](#thread-pool-module-thread_pool_module)
   - [thread_pool Class](#thread_pool-class)
   - [thread_worker Class](#thread_worker-class)
4. [Typed Thread Pool Module](#typed-thread-pool-module-typed_thread_pool_module)
   - [typed_thread_pool_t Template](#typed_thread_pool_t-template)
   - [typed_thread_worker_t Template](#typed_thread_worker_t-template)
   - [job_types Enumeration](#job_types-enumeration)
5. [Logger Module](#logger-module-log_module)
   - [Logging Functions](#logging-functions)
   - [log_types Enumeration](#log_types-enumeration)
6. [Monitoring Module](#monitoring-module-monitoring_module)
   - [metrics_collector Class](#metrics_collector-class)
   - [Performance Statistics](#performance-statistics)
7. [Utilities Module](#utilities-module-utility_module)
   - [formatter_macros](#formatter-macros)
   - [convert_string](#convert_string)
8. [Quick Reference](#quick-reference)

## Overview

The Thread System framework provides a comprehensive set of classes for building multi-threaded applications with adaptive performance optimization.

### Core Components

- **Core Classes** (`thread_module` namespace)
  - `thread_base` - Abstract base class for all workers
  - `job` - Unit of work representation
  - `job_queue` - Thread-safe job queue
  - `result<T>` - Error handling type
  - `callback_job` - Job implementation with lambda support

### Thread Pool Components

- **Standard Thread Pool** (`thread_pool_module` namespace)
  - `thread_pool` - Thread pool with adaptive queue optimization
  - `thread_worker` - Worker thread implementation

### Typed Thread Pool Components

- **Type-based Priority Pool** (`typed_thread_pool_module` namespace)
  - `typed_thread_pool_t<T>` - Template-based priority pool
  - `typed_thread_worker_t<T>` - Priority-aware worker
  - `typed_job_t<T>` - Job with type information
  - `job_types` - Default priority enumeration (RealTime, Batch, Background)

### Advanced Features

- **Adaptive Queue System**: Automatic switching between mutex and lock-free strategies
- **Hazard Pointers**: Safe memory reclamation for lock-free structures
- **Node Pools**: Memory pools for reduced allocation overhead
- **Real-time Monitoring**: Performance metrics collection and analysis

## Core Module (thread_module)

### thread_base Class

Abstract base class for all worker threads in the system.

```cpp
class thread_base {
public:
    thread_base(const std::string& thread_title = "thread_base");
    virtual ~thread_base();
    
    // Thread control
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

### job Class

Base class for all work units.

```cpp
class job {
public:
    job(const std::string& name = "job");
    job(const std::vector<uint8_t>& data, const std::string& name = "data_job");
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

### job_queue Class

Thread-safe queue for job management.

```cpp
class job_queue : public std::enable_shared_from_this<job_queue> {
public:
    job_queue();
    virtual ~job_queue();
    
    // Queue operations
    [[nodiscard]] virtual auto enqueue(std::unique_ptr<job>&& value) -> result_void;
    [[nodiscard]] virtual auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> result_void;
    [[nodiscard]] virtual auto dequeue() -> result<std::unique_ptr<job>>;
    [[nodiscard]] virtual auto dequeue_batch() -> std::deque<std::unique_ptr<job>>;
    
    // State management
    virtual auto clear() -> void;
    auto stop_waiting_dequeue() -> void;
    
    // Status
    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> std::size_t;
    [[nodiscard]] auto is_stopped() const -> bool;
    
    // Configuration
    auto set_notify(bool notify) -> void;
};
```

### result<T> Template

Error handling type based on `std::expected` pattern.

```cpp
template<typename T>
using result = std::expected<T, error>;

using result_void = result<void>;

struct error {
    error_code code;
    std::string message;
};

enum class error_code {
    success = 0,
    invalid_argument,
    resource_allocation_failed,
    operation_canceled,
    thread_start_failure,
    job_execution_failed,
    queue_full,
    queue_empty
};
```

## Thread Pool Module (thread_pool_module)

### thread_pool Class

Main thread pool implementation with adaptive queue optimization.

```cpp
class thread_pool : public std::enable_shared_from_this<thread_pool> {
public:
    thread_pool(const std::string& thread_title = "thread_pool");
    virtual ~thread_pool();
    
    // Pool control
    auto start() -> std::optional<std::string>;
    auto stop(const bool& immediately_stop = false) -> void;
    
    // Job management
    auto enqueue(std::unique_ptr<job>&& job) -> std::optional<std::string>;
    auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> std::optional<std::string>;
    
    // Worker management
    auto enqueue(std::unique_ptr<thread_worker>&& worker) -> std::optional<std::string>;
    auto enqueue_batch(std::vector<std::unique_ptr<thread_worker>>&& workers) -> std::optional<std::string>;
    
    // Access
    auto get_job_queue() -> std::shared_ptr<job_queue>;
    auto get_workers() const -> const std::vector<std::unique_ptr<thread_worker>>&;
};
```

### thread_worker Class

Worker thread implementation with adaptive capabilities.

```cpp
class thread_worker : public thread_base {
public:
    struct worker_statistics {
        uint64_t jobs_processed;
        uint64_t total_processing_time_ns;
        uint64_t batch_operations;
        uint64_t avg_processing_time_ns;
    };
    
    thread_worker(const std::string& name = "thread_worker");
    
    // Configuration
    auto set_batch_processing(bool enabled, size_t batch_size = 32) -> void;
    auto get_statistics() const -> worker_statistics;
    
    // Queue association
    auto set_job_queue(const std::shared_ptr<job_queue>& queue) -> void;
    
protected:
    auto do_work() -> result_void override;
};
```

## Typed Thread Pool Module (typed_thread_pool_module)

### typed_thread_pool_t Template

Priority-based thread pool with per-type queues.

```cpp
template<typename T>
class typed_thread_pool_t : public std::enable_shared_from_this<typed_thread_pool_t<T>> {
public:
    typed_thread_pool_t(const std::string& name = "typed_thread_pool");
    
    // Pool control
    auto start() -> result_void;
    auto stop(bool clear_queue = false) -> result_void;
    
    // Job management
    auto enqueue(std::unique_ptr<typed_job_t<T>>&& job) -> result_void;
    auto enqueue_batch(std::vector<std::unique_ptr<typed_job_t<T>>>&& jobs) -> result_void;
    
    // Worker management
    auto add_worker(std::unique_ptr<typed_thread_worker_t<T>>&& worker) -> result_void;
    auto add_workers(std::vector<std::unique_ptr<typed_thread_worker_t<T>>>&& workers) -> result_void;
    
    // Queue strategy (adaptive mode)
    auto set_queue_strategy(queue_strategy strategy) -> void;
};
```

### typed_thread_worker_t Template

Worker with type-based job handling.

```cpp
template<typename T>
class typed_thread_worker_t : public thread_base {
public:
    typed_thread_worker_t(const std::string& name = "typed_worker");
    
    // Type responsibilities
    auto set_responsibilities(const std::vector<T>& types) -> void;
    auto get_responsibilities() const -> std::vector<T>;
    
    // Queue association
    auto set_job_queue(const std::shared_ptr<typed_job_queue_t<T>>& queue) -> void;
};
```

### job_types Enumeration

Default priority levels for typed pools.

```cpp
enum class job_types : uint8_t {
    Background = 0,  // Lowest priority
    Batch = 1,       // Medium priority
    RealTime = 2     // Highest priority
};
```

## Logger Module (log_module)

### Logging Functions

Free functions for asynchronous logging.

```cpp
namespace log_module {
    // Configuration
    auto set_title(const std::string& title) -> void;
    auto console_target(log_types types) -> void;
    auto file_target(log_types types, const std::string& filename = "") -> void;
    auto callback_target(log_types types) -> void;
    auto message_callback(log_callback callback) -> void;
    
    // Lifecycle
    auto start() -> std::optional<std::string>;
    auto stop() -> void;
    
    // Logging functions
    template<typename... Args>
    auto write_information(const char* format, const Args&... args) -> void;
    
    template<typename... Args>
    auto write_error(const char* format, const Args&... args) -> void;
    
    template<typename... Args>
    auto write_debug(const char* format, const Args&... args) -> void;
    
    template<typename... Args>
    auto write_sequence(const char* format, const Args&... args) -> void;
    
    template<typename... Args>
    auto write_parameter(const char* format, const Args&... args) -> void;
}
```

### log_types Enumeration

Bitwise-enabled log levels.

```cpp
enum class log_types : uint8_t {
    None = 0,
    Exception = 1 << 0,
    Error = 1 << 1,
    Information = 1 << 2,
    Debug = 1 << 3,
    Sequence = 1 << 4,
    Parameter = 1 << 5,
    All = Exception | Error | Information | Debug | Sequence | Parameter
};
```

## Monitoring Module (monitoring_module)

### metrics_collector Class

Real-time performance metrics collection.

```cpp
class metrics_collector {
public:
    // Lifecycle
    auto start() -> result_void;
    auto stop() -> void;
    
    // Metrics registration
    auto register_system_metrics(std::shared_ptr<system_metrics> metrics) -> void;
    auto register_thread_pool_metrics(std::shared_ptr<thread_pool_metrics> metrics) -> void;
    
    // Data access
    auto get_current_snapshot() const -> metrics_snapshot;
    auto get_history(std::chrono::seconds duration) const -> std::vector<metrics_snapshot>;
};

// Global functions
namespace metrics {
    auto start_global_monitoring(monitoring_config config = {}) -> result_void;
    auto stop_global_monitoring() -> void;
    auto get_current_metrics() -> metrics_snapshot;
}
```

### Performance Statistics

Structures for performance monitoring.

```cpp
struct queue_statistics {
    std::atomic<uint64_t> enqueue_count{0};
    std::atomic<uint64_t> dequeue_count{0};
    std::atomic<uint64_t> enqueue_failures{0};
    std::atomic<uint64_t> dequeue_failures{0};
    std::atomic<uint64_t> max_size{0};
    std::atomic<double> avg_wait_time_ns{0.0};
};

struct system_metrics {
    std::atomic<uint64_t> memory_usage_bytes{0};
    std::atomic<double> cpu_usage_percent{0.0};
    std::atomic<uint32_t> active_threads{0};
};

struct thread_pool_metrics {
    std::atomic<uint64_t> jobs_completed{0};
    std::atomic<uint64_t> jobs_failed{0};
    std::atomic<double> avg_processing_time_ns{0.0};
    std::atomic<uint32_t> active_workers{0};
    std::atomic<uint32_t> queue_depth{0};
};
```

## Utilities Module (utility_module)

### formatter_macros

Macros for reducing formatter code duplication.

```cpp
#include "utilities/core/formatter_macros.h"

// Generate formatter specializations for custom types
DECLARE_FORMATTER(my_namespace::my_class)
```

### convert_string

String conversion utilities.

```cpp
namespace convert_string {
    auto to_string(const std::wstring& str) -> std::string;
    auto to_wstring(const std::string& str) -> std::wstring;
    auto to_utf8(const std::wstring& str) -> std::string;
    auto from_utf8(const std::string& str) -> std::wstring;
}
```

## Quick Reference

### Creating a Basic Thread Pool

```cpp
// Create and configure pool
auto pool = std::make_shared<thread_pool>("MyPool");

// Add workers
for (int i = 0; i < 4; ++i) {
    pool->enqueue(std::make_unique<thread_worker>());
}

// Start pool
pool->start();

// Submit jobs
pool->enqueue(std::make_unique<callback_job>([]() -> result_void {
    // Do work
    return {};
}));

// Stop pool
pool->stop();
```

### Using Typed Thread Pool

```cpp
// Create typed pool with priorities
auto typed_pool = std::make_shared<typed_thread_pool_t<job_types>>();

// Add specialized workers
auto realtime_worker = std::make_unique<typed_thread_worker_t<job_types>>();
realtime_worker->set_responsibilities({job_types::RealTime});
typed_pool->add_worker(std::move(realtime_worker));

// Start pool
typed_pool->start();

// Submit priority job
auto priority_job = std::make_unique<callback_typed_job<job_types>>(
    job_types::RealTime,
    []() -> result_void {
        // High priority work
        return {};
    }
);
typed_pool->enqueue(std::move(priority_job));
```

### Logging Example

```cpp
// Configure logger
log_module::set_title("MyApp");
log_module::console_target(log_module::log_types::All);
log_module::file_target(log_module::log_types::Error | log_module::log_types::Information);

// Start logger
log_module::start();

// Log messages
log_module::write_information("Application started");
log_module::write_error("Failed to connect: {}", error_message);
log_module::write_debug("Processing {} items", item_count);

// Stop logger
log_module::stop();
```

### Performance Monitoring

```cpp
// Start global monitoring
monitoring_module::metrics::start_global_monitoring();

// Get current metrics
auto snapshot = monitoring_module::metrics::get_current_metrics();
std::cout << "CPU Usage: " << snapshot.system.cpu_usage_percent << "%\n";
std::cout << "Jobs Completed: " << snapshot.thread_pool.jobs_completed << "\n";

// Stop monitoring
monitoring_module::metrics::stop_global_monitoring();
```