# Thread System API Reference

Complete API documentation for the Thread System framework.

## Table of Contents

1. [Overview](#overview)
2. [Core Classes](#core-classes)
   - [thread_base Class](#thread_base-class)
   - [job Class](#job-class)
   - [job_queue Class](#job_queue-class)
   - [result Template](#resultt-template)
3. [Thread Pool](#thread-pool)
   - [thread_pool Class](#thread_pool-class)
   - [thread_worker Class](#thread_worker-class)
   - [Usage Patterns](#usage-patterns)
4. [API Conventions](#api-conventions)
5. [Quick Reference](#quick-reference)
6. [See Also](#see-also)

## Overview

The Thread System framework provides a comprehensive set of classes for building multi-threaded applications. The core components include:

- **Core Classes** - Base classes and fundamental types
  - `thread_base` - Abstract base class for all workers
  - `job` - Unit of work representation
  - `job_queue` - Thread-safe job queue
  - `result<T>` - Error handling type

- **Thread Pool** - High-performance thread pool implementation
  - `thread_pool` - Fixed-size thread pool
  - `thread_worker` - Worker thread implementation

- **Priority Thread Pool** - Priority-based scheduling
  - `priority_thread_pool<T>` - Template-based priority pool
  - `priority_thread_worker<T>` - Priority-aware worker

- **Logger** - Asynchronous logging system
  - `log_module` - Global logging interface

- **Utilities** - Helper functions and types

## Core Classes

The core classes form the foundation of the Thread System framework. These classes provide the basic building blocks for all threading operations.

### thread_base Class

The `thread_base` class is the abstract base class for all worker threads in the system.

#### Class Definition

```cpp
namespace thread_module {
    class thread_base {
    public:
        // Constructors
        thread_base();
        explicit thread_base(const std::wstring& name);
        
        // Lifecycle methods
        auto start() -> result_void;
        auto stop() -> result_void;
        auto is_running() const -> bool;
        
        // Configuration
        void set_wake_interval(const std::chrono::milliseconds& interval);
        auto get_name() const -> std::wstring;
        
    protected:
        // Override these in derived classes
        virtual auto before_start() -> result_void;
        virtual auto do_work() -> result_void = 0;
        virtual auto after_stop() -> result_void;
        
    private:
        class impl;
        std::unique_ptr<impl> pimpl;
    };
}
```

#### Usage Example

```cpp
class MyWorker : public thread_module::thread_base {
public:
    MyWorker() : thread_base(L"MyWorker") {}
    
protected:
    auto do_work() -> result_void override {
        // Your work logic here
        log_module::info(L"Worker is processing...");
        return {};
    }
};

// Usage
MyWorker worker;
auto result = worker.start();
if (!result) {
    log_module::error(L"Failed to start worker: {}", result.error());
}
```

#### Key Features

- **RAII Design**: Automatic cleanup on destruction
- **State Management**: Thread-safe state transitions
- **Error Handling**: Comprehensive error reporting via `result_void`
- **Wake Intervals**: Support for periodic wake-ups

### job Class

The `job` class represents a unit of work to be executed by a thread pool.

#### Class Definition

```cpp
namespace thread_pool_module {
    class job {
    public:
        using work_function = std::function<std::optional<std::string>()>;
        
        // Constructors
        job() = default;
        explicit job(work_function work);
        explicit job(std::function<void()> work);
        
        // Execution
        auto execute() -> std::optional<std::string>;
        
        // Properties
        auto is_valid() const -> bool;
        void reset();
        
    private:
        work_function work_;
    };
}
```

#### Usage Example

```cpp
// Create a job that returns a result
job job1([]{
    // Do some work
    return std::optional<std::string>{}; // Success
});

// Create a job from a void function
job job2([]{ 
    std::cout << "Hello from job!" << std::endl;
});

// Execute the job
auto error = job1.execute();
if (error) {
    log_module::error(L"Job failed: {}", *error);
}
```

### job_queue Class

Thread-safe queue for managing jobs in the thread pool.

#### Class Definition

```cpp
namespace thread_pool_module {
    class job_queue {
    public:
        // Constructor
        job_queue() = default;
        
        // Queue operations
        void push(job&& j);
        auto pop() -> std::optional<job>;
        auto try_pop() -> std::optional<job>;
        auto wait_and_pop() -> std::optional<job>;
        
        // Queue state
        auto size() const -> size_t;
        auto empty() const -> bool;
        void clear();
        void stop();
        
    private:
        mutable std::mutex mutex_;
        std::condition_variable cv_;
        std::queue<job> jobs_;
        bool stopped_ = false;
    };
}
```

#### Thread Safety

All methods in `job_queue` are thread-safe and can be called concurrently from multiple threads.

### result<T> Template

The `result<T>` template provides a type-safe way to return either a value or an error.

#### Definition

```cpp
namespace utilities {
    template<typename T>
    class result {
    public:
        // Constructors
        result(T value);
        result(std::string error);
        
        // Accessors
        bool has_value() const;
        bool has_error() const;
        operator bool() const { return has_value(); }
        
        T& value();
        const T& value() const;
        const std::string& error() const;
        
        // Monadic operations
        template<typename F>
        auto map(F&& f) -> result<decltype(f(std::declval<T>()))>;
        
        template<typename F>
        auto flat_map(F&& f) -> decltype(f(std::declval<T>()));
    };
    
    // Specialization for void
    using result_void = result<void>;
}
```

#### Usage Example

```cpp
auto divide(int a, int b) -> result<int> {
    if (b == 0) {
        return {"Division by zero"};
    }
    return {a / b};
}

auto result = divide(10, 2);
if (result) {
    std::cout << "Result: " << result.value() << std::endl;
} else {
    std::cout << "Error: " << result.error() << std::endl;
}
```

## Thread Pool

The thread pool module provides a high-performance, fixed-size thread pool implementation for executing jobs concurrently.

### thread_pool Class

#### Class Definition

```cpp
namespace thread_pool_module {
    class thread_pool {
    public:
        // Factory function
        friend auto create_default(size_t worker_count) 
            -> std::pair<std::shared_ptr<thread_pool>, std::optional<std::string>>;
        
        // Destructor
        ~thread_pool();
        
        // Job submission
        void add_job(job&& job);
        void add_job(std::function<void()> work);
        void add_job(std::function<std::optional<std::string>()> work);
        
        // Lifecycle
        auto start() -> result_void;
        auto stop() -> result_void;
        auto is_running() const -> bool;
        
        // Pool information
        auto size() const -> size_t;
        auto queue_size() const -> size_t;
        auto active_workers() const -> size_t;
        
    private:
        explicit thread_pool(size_t worker_count);
        class impl;
        std::unique_ptr<impl> pimpl;
    };
}
```

#### Creating a Thread Pool

```cpp
// Create a thread pool with 4 workers
auto [pool, error] = thread_pool_module::create_default(4);
if (error) {
    log_module::error(L"Failed to create thread pool: {}", *error);
    return;
}

// Start the pool
auto result = pool->start();
if (!result) {
    log_module::error(L"Failed to start thread pool: {}", result.error());
    return;
}
```

#### Submitting Jobs

```cpp
// Submit a simple job
pool->add_job([] {
    std::cout << "Hello from thread pool!" << std::endl;
});

// Submit a job that can fail
pool->add_job([]() -> std::optional<std::string> {
    if (rand() % 2 == 0) {
        return "Random failure";
    }
    // Success
    return std::nullopt;
});

// Submit many jobs
for (int i = 0; i < 100; ++i) {
    pool->add_job([i] {
        // Process item i
        process_item(i);
    });
}
```

### thread_worker Class

The `thread_worker` class is the internal worker thread used by the thread pool.

#### Class Definition

```cpp
namespace thread_pool_module {
    class thread_worker : public thread_module::thread_base {
    public:
        // Constructor
        thread_worker(std::shared_ptr<job_queue> queue, 
                     const std::wstring& name);
        
        // Statistics
        auto jobs_processed() const -> size_t;
        auto errors_count() const -> size_t;
        auto is_busy() const -> bool;
        
    protected:
        // Thread work function
        auto do_work() -> result_void override;
        
    private:
        std::shared_ptr<job_queue> job_queue_;
        std::atomic<size_t> jobs_processed_{0};
        std::atomic<size_t> errors_{0};
        std::atomic<bool> busy_{false};
    };
}
```

### Usage Patterns

#### Basic Usage

```cpp
#include "thread_pool.h"
#include "logger.h"

int main() {
    // Initialize logging
    log_module::start();
    
    // Create and start thread pool
    auto [pool, error] = thread_pool_module::create_default(
        std::thread::hardware_concurrency()
    );
    
    if (error) {
        log_module::error(L"Failed to create pool: {}", *error);
        return 1;
    }
    
    pool->start();
    
    // Submit work
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 10; ++i) {
        auto future = std::async(std::launch::async, [&pool, i] {
            std::promise<int> promise;
            auto future = promise.get_future();
            
            pool->add_job([p = std::move(promise), i]() mutable {
                // Simulate work
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                p.set_value(i * i);
            });
            
            return future.get();
        });
        
        futures.push_back(std::move(future));
    }
    
    // Collect results
    for (auto& f : futures) {
        std::cout << "Result: " << f.get() << std::endl;
    }
    
    // Cleanup
    pool->stop();
    log_module::stop();
    
    return 0;
}
```

#### Producer-Consumer Pattern

```cpp
class DataProcessor {
public:
    DataProcessor(size_t worker_count) {
        auto [p, error] = thread_pool_module::create_default(worker_count);
        if (!error) {
            pool_ = p;
            pool_->start();
        }
    }
    
    void process_data(std::vector<Data> data) {
        for (auto& item : data) {
            pool_->add_job([this, item = std::move(item)]() mutable {
                process_single_item(std::move(item));
            });
        }
    }
    
private:
    void process_single_item(Data item) {
        // Process the data
        auto result = transform(item);
        
        // Store result
        std::lock_guard<std::mutex> lock(results_mutex_);
        results_.push_back(result);
    }
    
    std::shared_ptr<thread_pool> pool_;
    std::vector<Result> results_;
    std::mutex results_mutex_;
};
```

#### Batch Processing

```cpp
template<typename T, typename F>
void parallel_for_each(std::vector<T>& items, F func, size_t batch_size = 100) {
    auto [pool, error] = thread_pool_module::create_default(
        std::thread::hardware_concurrency()
    );
    
    if (error) {
        // Fall back to sequential processing
        std::for_each(items.begin(), items.end(), func);
        return;
    }
    
    pool->start();
    
    // Process in batches
    for (size_t i = 0; i < items.size(); i += batch_size) {
        auto start = items.begin() + i;
        auto end = (i + batch_size < items.size()) 
                   ? items.begin() + i + batch_size 
                   : items.end();
        
        pool->add_job([start, end, func] {
            std::for_each(start, end, func);
        });
    }
    
    pool->stop(); // Wait for all jobs to complete
}
```

### Performance Considerations

1. **Pool Size**: Use `std::thread::hardware_concurrency()` as a starting point
2. **Job Granularity**: Avoid submitting very small jobs; batch them instead
3. **Memory Allocation**: Minimize allocations inside jobs
4. **Synchronization**: Use lock-free data structures where possible

### Error Handling

```cpp
// Job with error handling
pool->add_job([]() -> std::optional<std::string> {
    try {
        risky_operation();
        return std::nullopt; // Success
    } catch (const std::exception& e) {
        return std::string("Exception: ") + e.what();
    }
});
```

### Thread Safety

- All public methods of `thread_pool` are thread-safe
- Jobs are executed in FIFO order (per worker)
- No guarantees on execution order across workers

## API Conventions

1. **Namespace Organization**:
   - `thread_module` - Core threading functionality
   - `thread_pool_module` - Thread pool implementations
   - `priority_thread_pool_module` - Priority scheduling
   - `log_module` - Logging functionality
   - `utilities` - Utility functions

2. **Error Handling**:
   - Functions that can fail return `result<T>` or `result_void`
   - Jobs return `std::optional<std::string>` for errors

3. **Memory Management**:
   - RAII everywhere
   - Smart pointers for shared ownership
   - No manual memory management required

4. **Thread Safety**:
   - All public APIs are thread-safe unless noted
   - Internal synchronization is handled automatically

## Quick Reference

### Creating a Thread Pool

```cpp
auto [pool, error] = thread_pool_module::create_default(4);
```

### Submitting Jobs

```cpp
pool->add_job([] { /* work */ });
```

### Priority Scheduling

```cpp
priority_pool->add_job(task, priority::high);
```

### Logging

```cpp
log_module::info(L"Message: {}", value);
```

## Best Practices

1. **Always check results**: Use the `result<T>` type to handle errors properly
2. **RAII everywhere**: Let destructors handle cleanup
3. **Prefer composition**: Build complex behaviors by composing simple classes
4. **Thread safety**: Use the provided thread-safe classes; don't roll your own

## See Also

- [Priority Thread Pool API](./priority-thread-pool.md)
- [Logger API](./logger.md)
- [Utilities](./utilities.md)
- [Examples](../examples/README.md) - Practical usage examples
- [Tutorials](../tutorials/README.md) - Step-by-step guides
- [FAQ](../FAQ.md) - Common questions and answers