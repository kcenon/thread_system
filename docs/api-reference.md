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
6. [Quick Reference](#quick-reference)

## Overview

The Thread System framework provides a comprehensive set of classes for building multi-threaded applications. The core components include:

- **Core Classes** (`thread_module` namespace)
  - `thread_base` - Abstract base class for all workers
  - `job` - Unit of work representation
  - `job_queue` - Thread-safe job queue
  - `result<T>` - Error handling type
  - `callback_job` - Job implementation with lambda support

- **Thread Pool** (`thread_pool_module` namespace)
  - `thread_pool` - Fixed-size thread pool
  - `thread_worker` - Worker thread implementation

- **Typed Thread Pool** (`typed_thread_pool_module` namespace)
  - `typed_thread_pool_t<T>` - Template-based type pool
  - `typed_thread_worker_t<T>` - Type-aware worker
  - `typed_job_t<T>` - Job with type information
  - `job_types` - Default type enumeration

- **Logger** (`log_module` namespace)
  - Free functions for logging
  - `log_types` - Logging level enumeration
  - Multiple output targets (console, file, callback)

- **Utilities** (`utility_module` namespace)
  - `formatter` - String formatting utilities
  - `convert_string` - String conversion helpers

## Core Module (thread_module)

The core module provides the fundamental building blocks for all threading operations.

### thread_base Class

The `thread_base` class is the abstract base class for all worker threads in the system.

#### Class Definition

```cpp
namespace thread_module {
    class thread_base {
    public:
        // Constructors
        explicit thread_base(const std::string& thread_title = "thread_base");
        virtual ~thread_base();
        
        // Delete copy/move operations
        thread_base(const thread_base&) = delete;
        thread_base& operator=(const thread_base&) = delete;
        thread_base(thread_base&&) = delete;
        thread_base& operator=(thread_base&&) = delete;
        
        // Lifecycle methods
        auto start() -> result_void;
        auto stop() -> result_void;
        [[nodiscard]] auto is_running() const -> bool;
        
        // Configuration
        auto set_wake_interval(const std::optional<std::chrono::milliseconds>& wake_interval) -> void;
        [[nodiscard]] auto get_wake_interval() const -> std::optional<std::chrono::milliseconds>;
        [[nodiscard]] auto get_thread_title() const -> std::string;
        
        // String representation
        [[nodiscard]] virtual auto to_string() const -> std::string;
        
    protected:
        // Override these in derived classes
        [[nodiscard]] virtual auto should_continue_work() const -> bool { return false; }
        virtual auto before_start() -> result_void { return {}; }
        virtual auto do_work() -> result_void { return {}; }
        virtual auto after_stop() -> result_void { return {}; }
    };
}
```

#### Usage Example

```cpp
class MyWorker : public thread_module::thread_base {
public:
    MyWorker() : thread_base("MyWorker") {}
    
protected:
    auto do_work() -> result_void override {
        // Your work logic here
        log_module::write_information("Worker is processing...");
        return {};
    }
    
    auto should_continue_work() const -> bool override {
        // Return true if there's more work to do
        return !work_queue_.empty();
    }
};

// Usage
MyWorker worker;
worker.set_wake_interval(std::chrono::milliseconds(100));
auto result = worker.start();
if (result.has_error()) {
    log_module::write_error("Failed to start worker: {}", result.error().message());
}
```

### job Class

The `job` class represents a unit of work to be executed by a thread pool.

#### Class Definition

```cpp
namespace thread_module {
    class job {
    public:
        // Constructors
        job() = default;
        virtual ~job() = default;
        
        // Delete copy operations
        job(const job&) = delete;
        job& operator=(const job&) = delete;
        
        // Allow move operations
        job(job&&) = default;
        job& operator=(job&&) = default;
        
        // Execution
        [[nodiscard]] virtual auto operator()() -> result_void = 0;
        
        // Type information
        [[nodiscard]] virtual auto to_string() const -> std::string { return "job"; }
    };
    
    // Callback job implementation
    class callback_job : public job {
    public:
        using callback_function = std::function<result_void()>;
        
        explicit callback_job(callback_function callback);
        
        [[nodiscard]] auto operator()() -> result_void override;
        [[nodiscard]] auto to_string() const -> std::string override;
        
    private:
        callback_function callback_;
    };
}
```

#### Usage Example

```cpp
// Create a callback job
auto job1 = std::make_unique<callback_job>(
    []() -> result_void {
        // Do some work
        log_module::write_information("Job executing");
        return {}; // Success
    }
);

// Create a job that may fail
auto job2 = std::make_unique<callback_job>(
    []() -> result_void {
        if (some_condition_fails) {
            return error_info(error_type::runtime_error, "Job failed");
        }
        return {};
    }
);

// Jobs are typically executed by the thread pool
// Manual execution:
auto result = (*job1)();
if (result.has_error()) {
    log_module::write_error("Job failed: {}", result.error().message());
}
```

### job_queue Class

Thread-safe queue for managing jobs in the thread pool.

#### Class Definition

```cpp
namespace thread_module {
    class job_queue : public std::enable_shared_from_this<job_queue> {
    public:
        // Constructors
        job_queue();
        virtual ~job_queue();
        
        // Queue operations
        [[nodiscard]] virtual auto enqueue(std::unique_ptr<job>&& job) -> std::optional<std::string>;
        [[nodiscard]] virtual auto enqueue(std::vector<std::unique_ptr<job>>&& jobs) -> std::optional<std::string>;
        [[nodiscard]] virtual auto dequeue() -> std::tuple<std::unique_ptr<job>, bool>;
        [[nodiscard]] virtual auto dequeue(const std::chrono::milliseconds& timeout) -> std::tuple<std::unique_ptr<job>, bool>;
        
        // Queue state
        [[nodiscard]] virtual auto size() const -> size_t;
        [[nodiscard]] virtual auto empty() const -> bool;
        virtual auto clear() -> void;
        virtual auto notify_one() -> void;
        virtual auto notify_all() -> void;
        
        // String representation
        [[nodiscard]] virtual auto to_string() const -> std::string;
    };
}
```

### result<T> Template

The `result<T>` template provides a type-safe way to return either a value or an error.

#### Definition

```cpp
namespace thread_module {
    template<typename T>
    class result {
    public:
        // Constructors
        result(); // Success with default value
        result(T value); // Success with value
        result(error_info error); // Error
        
        // Accessors
        [[nodiscard]] bool has_value() const;
        [[nodiscard]] bool has_error() const;
        [[nodiscard]] operator bool() const { return !has_error(); }
        
        [[nodiscard]] T& value();
        [[nodiscard]] const T& value() const;
        [[nodiscard]] const error_info& error() const;
        
        // Comparison
        bool operator==(const result& other) const;
        bool operator!=(const result& other) const;
    };
    
    // Specialization for void
    template<>
    class result<void> {
    public:
        result(); // Success
        result(error_info error); // Error
        
        [[nodiscard]] bool has_error() const;
        [[nodiscard]] operator bool() const { return !has_error(); }
        [[nodiscard]] const error_info& error() const;
    };
    
    // Type alias for common case
    using result_void = result<void>;
}
```

## Thread Pool Module (thread_pool_module)

The thread pool module provides a standard thread pool implementation.

### thread_pool Class

Manages a pool of worker threads that process jobs from a shared queue.

#### Class Definition

```cpp
namespace thread_pool_module {
    class thread_pool : public std::enable_shared_from_this<thread_pool> {
    public:
        // Constructor
        thread_pool(const std::string& thread_title = "thread_pool");
        virtual ~thread_pool();
        
        // Lifecycle
        [[nodiscard]] auto get_ptr() -> std::shared_ptr<thread_pool>;
        auto start() -> std::optional<std::string>;
        auto stop(const bool& immediately_stop = false) -> void;
        
        // Job management
        [[nodiscard]] auto get_job_queue() -> std::shared_ptr<job_queue>;
        auto enqueue(std::unique_ptr<job>&& job) -> std::optional<std::string>;
        auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> std::optional<std::string>;
        
        // Worker management
        auto enqueue(std::unique_ptr<thread_worker>&& worker) -> std::optional<std::string>;
        auto enqueue_batch(std::vector<std::unique_ptr<thread_worker>>&& workers) -> std::optional<std::string>;
        
        // String representation
        [[nodiscard]] auto to_string() const -> std::string;
    };
}
```

#### Usage Example

```cpp
// Create and configure thread pool
auto pool = std::make_shared<thread_pool>("MyPool");

// Add workers
std::vector<std::unique_ptr<thread_worker>> workers;
for (int i = 0; i < 4; ++i) {
    workers.push_back(std::make_unique<thread_worker>());
}
pool->enqueue_batch(std::move(workers));

// Start the pool
if (auto error = pool->start()) {
    log_module::write_error("Failed to start pool: {}", *error);
    return;
}

// Submit jobs
for (int i = 0; i < 10; ++i) {
    pool->enqueue(std::make_unique<callback_job>(
        [i]() -> result_void {
            log_module::write_information("Processing job {}", i);
            return {};
        }
    ));
}

// Stop when done
pool->stop();
```

### thread_worker Class

Worker thread that processes jobs from a shared queue.

#### Class Definition

```cpp
namespace thread_pool_module {
    class thread_worker : public thread_base {
    public:
        // Constructor
        explicit thread_worker(std::shared_ptr<job_queue> job_queue = nullptr);
        
        // Queue management
        auto set_job_queue(std::shared_ptr<job_queue> job_queue) -> void;
        [[nodiscard]] auto get_job_queue() const -> std::shared_ptr<job_queue>;
        
    protected:
        // Override from thread_base
        [[nodiscard]] auto should_continue_work() const -> bool override;
        auto do_work() -> result_void override;
    };
}
```

## Typed Thread Pool Module (typed_thread_pool_module)

The typed thread pool module provides priority-based job scheduling.

### typed_thread_pool_t Template

Thread pool that processes jobs based on their type/priority.

#### Class Definition

```cpp
namespace typed_thread_pool_module {
    template <typename job_type = job_types>
    class typed_thread_pool_t : public std::enable_shared_from_this<typed_thread_pool_t<job_type>> {
    public:
        // Constructor
        typed_thread_pool_t(const std::string& thread_title = "typed_thread_pool");
        virtual ~typed_thread_pool_t();
        
        // Lifecycle
        [[nodiscard]] auto get_ptr() -> std::shared_ptr<typed_thread_pool_t<job_type>>;
        auto start() -> result_void;
        auto stop(bool clear_queue = false) -> result_void;
        
        // Job management
        [[nodiscard]] auto get_job_queue() -> std::shared_ptr<typed_job_queue_t<job_type>>;
        auto enqueue(std::unique_ptr<typed_job_t<job_type>>&& job) -> result_void;
        auto enqueue_batch(std::vector<std::unique_ptr<typed_job_t<job_type>>>&& jobs) -> result_void;
        
        // Worker management
        auto enqueue(std::unique_ptr<typed_thread_worker_t<job_type>>&& worker) -> result_void;
        auto enqueue_batch(std::vector<std::unique_ptr<typed_thread_worker_t<job_type>>>&& workers) -> result_void;
        
        // String representation
        [[nodiscard]] auto to_string() const -> std::string;
    };
    
    // Type alias for default job_types
    using typed_thread_pool = typed_thread_pool_t<job_types>;
}
```

#### Usage Example

```cpp
// Create typed thread pool
auto pool = std::make_shared<typed_thread_pool>();

// Add workers with different type responsibilities
pool->enqueue(std::make_unique<typed_thread_worker_t<job_types>>(
    std::initializer_list<job_types>{job_types::High}
));
pool->enqueue(std::make_unique<typed_thread_worker_t<job_types>>(
    std::initializer_list<job_types>{job_types::Normal, job_types::Low}
));

// Start the pool
pool->start();

// Submit jobs with types
pool->enqueue(std::make_unique<callback_typed_job<job_types>>(
    job_types::High,
    []() -> result_void {
        log_module::write_information("High priority job");
        return {};
    }
));

pool->enqueue(std::make_unique<callback_typed_job<job_types>>(
    job_types::Low,
    []() -> result_void {
        log_module::write_information("Low priority job");
        return {};
    }
));
```

### job_types Enumeration

Default job type levels for the typed thread pool.

```cpp
namespace typed_thread_pool_module {
    enum class job_types : uint8_t {
        Undefined = 0,
        RealTime = 1,
        High = 2,
        Normal = 3,
        Low = 4,
        Idle = 5
    };
}
```

## Logger Module (log_module)

The logger module provides thread-safe asynchronous logging.

### Logging Functions

All logging functions are free functions in the `log_module` namespace.

```cpp
namespace log_module {
    // Configuration
    auto set_title(const std::string& title) -> void;
    auto set_max_lines(uint32_t max_lines) -> void;
    auto set_use_backup(bool use_backup) -> void;
    auto set_wake_interval(std::chrono::milliseconds interval) -> void;
    
    // Target configuration
    auto console_target(const log_types& type) -> void;
    auto file_target(const log_types& type) -> void;
    auto callback_target(const log_types& type) -> void;
    auto message_callback(const std::function<void(const log_types&, const std::string&, const std::string&)>& callback) -> void;
    
    // Lifecycle
    auto start() -> std::optional<std::string>;
    auto stop() -> void;
    
    // Logging functions (variadic templates)
    template <typename... Args>
    auto write_exception(const char* formats, const Args&... args) -> void;
    
    template <typename... Args>
    auto write_error(const char* formats, const Args&... args) -> void;
    
    template <typename... Args>
    auto write_information(const char* formats, const Args&... args) -> void;
    
    template <typename... Args>
    auto write_debug(const char* formats, const Args&... args) -> void;
    
    template <typename... Args>
    auto write_sequence(const char* formats, const Args&... args) -> void;
    
    template <typename... Args>
    auto write_parameter(const char* formats, const Args&... args) -> void;
}
```

#### Usage Example

```cpp
// Configure logger
log_module::set_title("MyApplication");
log_module::console_target(log_types::Information | log_types::Error);
log_module::file_target(log_types::All);

// Start logger
if (auto error = log_module::start()) {
    std::cerr << "Failed to start logger: " << *error << std::endl;
    return;
}

// Log messages
log_module::write_information("Application started");
log_module::write_debug("Debug value: {}", debug_value);
log_module::write_error("Error occurred: {}", error_message);

// Stop logger
log_module::stop();
```

### log_types Enumeration

Log severity levels with bitwise operations support.

```cpp
namespace log_module {
    enum class log_types : uint16_t {
        None = 0,
        Exception = 1 << 0,
        Error = 1 << 1,
        Information = 1 << 2,
        Debug = 1 << 3,
        Sequence = 1 << 4,
        Parameter = 1 << 5,
        All = Exception | Error | Information | Debug | Sequence | Parameter
    };
    
    // Bitwise operators
    constexpr log_types operator|(log_types lhs, log_types rhs);
    constexpr log_types operator&(log_types lhs, log_types rhs);
    constexpr log_types operator^(log_types lhs, log_types rhs);
    constexpr log_types operator~(log_types lhs);
}
```

## Quick Reference

### Creating a Basic Thread Pool

```cpp
// Include headers
#include "thread_pool/core/thread_pool.h"
#include "thread_base/jobs/callback_job.h"

// Create pool
auto pool = std::make_shared<thread_pool_module::thread_pool>();

// Add workers
std::vector<std::unique_ptr<thread_pool_module::thread_worker>> workers;
for (int i = 0; i < 4; ++i) {
    workers.push_back(std::make_unique<thread_pool_module::thread_worker>());
}
pool->enqueue_batch(std::move(workers));

// Start pool
pool->start();

// Submit job
pool->enqueue(std::make_unique<thread_module::callback_job>(
    []() -> thread_module::result_void {
        // Work here
        return {};
    }
));

// Stop pool
pool->stop();
```

### Creating a Typed Thread Pool

```cpp
// Include headers
#include "typed_thread_pool/pool/typed_thread_pool.h"
#include "typed_thread_pool/jobs/callback_typed_job.h"

// Create pool
auto pool = std::make_shared<typed_thread_pool_module::typed_thread_pool>();

// Add specialized workers
pool->enqueue(std::make_unique<typed_thread_pool_module::typed_thread_worker_t<job_types>>(
    std::initializer_list<job_types>{job_types::High}
));

// Start and use
pool->start();
pool->enqueue(std::make_unique<typed_thread_pool_module::callback_typed_job<job_types>>(
    job_types::High,
    []() -> thread_module::result_void { return {}; }
));
```

### Setting Up Logging

```cpp
#include "logger/core/logger.h"

// Configure
log_module::set_title("MyApp");
log_module::console_target(log_module::log_types::All);

// Start
log_module::start();

// Log
log_module::write_information("Hello, {}", "World");

// Stop
log_module::stop();
```