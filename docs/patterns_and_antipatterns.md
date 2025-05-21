# Thread System: Best Practices, Patterns, and Antipatterns

This guide outlines recommended approaches and patterns to use with Thread System, as well as antipatterns to avoid. Following these guidelines will help you write efficient, maintainable, and bug-free concurrent code.

## Best Practices

### 1. Thread Base Usage

#### ✅ DO:
- Derive from `thread_base` when implementing custom worker threads
- Override `before_start()`, `do_work()`, and `after_stop()` to customize behavior
- Implement proper cleanup in your `after_stop()` method
- Use the `set_wake_interval()` method for periodic operations
- Check `should_continue_work()` regularly in long-running operations

#### ❌ DON'T:
- Call `start()` from within constructors
- Directly manipulate the underlying thread
- Implement tight loops without condition checks
- Ignore return values from `start()` and `stop()`
- Use thread-unsafe operations inside `do_work()` without proper synchronization

### 2. Thread Pool Usage

#### ✅ DO:
- Use `thread_pool` for CPU-bound tasks
- Create an appropriate number of worker threads (typically core count or core count + 1)
- Batch-submit jobs when possible using `enqueue_batch()`
- Use callback jobs for most operations
- Properly handle errors returned from job execution

#### ❌ DON'T:
- Create too many thread pools (one per application is often sufficient)
- Create an excessive number of worker threads (can lead to context switching overhead)
- Use thread pools for I/O-bound tasks without careful consideration
- Submit individual jobs in a tight loop (use batch submission instead)
- Block worker threads with long-running synchronous operations

### 3. Priority Thread Pool Usage

#### ✅ DO:
- Use distinct priority levels for different types of tasks
- Create dedicated workers for critical priority levels
- Use lower priority for background or maintenance tasks
- Consider custom priority types for domain-specific scheduling
- Monitor queue sizes per priority to ensure balanced execution

#### ❌ DON'T:
- Assign high priority to all tasks (defeats the purpose)
- Create too many priority levels (3-5 levels are typically sufficient)
- Ignore priority inversion issues
- Use priorities inconsistently across the application
- Create priority workers without assigning appropriate jobs

### 4. Logging Usage

#### ✅ DO:
- Initialize the logger early in your application
- Use appropriate log levels for different types of messages
- Set appropriate log targets based on environment (development vs. production)
- Use structured logging where possible
- Configure reasonable wake intervals for log processing

#### ❌ DON'T:
- Log sensitive information
- Create excessive log volume in production
- Log in tight loops without level checks
- Use blocking log operations in performance-critical code paths
- Leave the logger running after your application is supposed to shut down

## Common Patterns

### 1. Worker Thread Pattern

```cpp
class MyWorker : public thread_module::thread_base {
protected:
    auto before_start() -> result_void override {
        // Initialize resources
        return {};
    }

    auto do_work() -> result_void override {
        // Perform work here
        return {};
    }

    auto should_continue_work() const -> bool override {
        // Logic to determine if more work is needed
        return !work_queue_.empty();
    }

    auto after_stop() -> result_void override {
        // Cleanup resources
        return {};
    }

private:
    // Worker-specific members
    std::queue<WorkItem> work_queue_;
};
```

### 2. Thread Pool Task Processing Pattern

```cpp
// Create a thread pool
auto [pool, error] = create_default(thread_counts_);
if (error.has_value()) {
    // Handle error
    return;
}

// Create a batch of jobs
std::vector<std::unique_ptr<thread_module::job>> jobs;
jobs.reserve(task_count);

for (auto i = 0; i < task_count; ++i) {
    jobs.push_back(std::make_unique<thread_module::callback_job>(
        [i]() -> std::optional<std::string> {
            // Process task
            return std::nullopt; // Success
        }
    ));
}

// Submit jobs as a batch for efficiency
error = pool->enqueue_batch(std::move(jobs));
if (error.has_value()) {
    // Handle error
    return;
}

// Start processing
error = pool->start();
if (error.has_value()) {
    // Handle error
    return;
}

// When done, stop the pool
pool->stop();
```

### 3. Priority-Based Job Execution Pattern

```cpp
// Create a priority thread pool with different priority workers
auto [pool, error] = create_priority_pool(
    high_priority_workers_,
    normal_priority_workers_,
    low_priority_workers_
);

// Creating jobs with different priorities
std::vector<std::unique_ptr<priority_thread_pool_module::priority_job>> jobs;
jobs.reserve(job_count);

// High priority critical tasks
jobs.push_back(std::make_unique<priority_thread_pool_module::callback_priority_job>(
    []() -> result_void {
        // Critical operation
        return {};
    },
    priority_thread_pool_module::job_priorities::High
));

// Normal priority regular tasks
jobs.push_back(std::make_unique<priority_thread_pool_module::callback_priority_job>(
    []() -> result_void {
        // Regular operation
        return {};
    },
    priority_thread_pool_module::job_priorities::Normal
));

// Low priority background tasks
jobs.push_back(std::make_unique<priority_thread_pool_module::callback_priority_job>(
    []() -> result_void {
        // Background operation
        return {};
    },
    priority_thread_pool_module::job_priorities::Low
));
```

### 4. Logger Initialization Pattern

```cpp
auto initialize_logger() -> std::optional<std::string> {
    // Set application name
    log_module::set_title("my_application");
    
    // Configure log targets
    log_module::file_target(log_module::log_types::Error | log_module::log_types::Warning);
    log_module::console_target(log_module::log_types::Information);
    
    // For production systems, use file logging with backup
    log_module::set_use_backup(true);
    log_module::set_max_lines(10000);
    
    // Set wake interval for non-critical logs
    log_module::set_wake_interval(std::chrono::milliseconds(100));
    
    // Start the logger
    return log_module::start();
}
```

## Antipatterns to Avoid

### 1. The Thread Explosion Antipattern

❌ **Problematic Approach**:
```cpp
// Creating a new thread for each small task
for (const auto& item : items) {
    auto thread = std::make_unique<thread_module::thread_base>();
    thread->start();
    // Process item in thread
}
```

✅ **Better Approach**:
```cpp
// Create a single thread pool
auto thread_pool = std::make_shared<thread_pool_module::thread_pool>();

// Submit all items as jobs
std::vector<std::unique_ptr<thread_module::job>> jobs;
for (const auto& item : items) {
    jobs.push_back(std::make_unique<thread_module::callback_job>(
        [item]() -> std::optional<std::string> {
            // Process item
            return std::nullopt;
        }
    ));
}

// Process all items with the thread pool
thread_pool->enqueue_batch(std::move(jobs));
thread_pool->start();
```

### 2. The Busy Waiting Antipattern

❌ **Problematic Approach**:
```cpp
auto do_work() -> result_void override {
    // Continuously check if work is available without yielding
    while (!work_available()) {
        // Tight loop consuming CPU
    }
    process_work();
    return {};
}
```

✅ **Better Approach**:
```cpp
auto do_work() -> result_void override {
    // Use condition variables and wake intervals
    if (!work_available()) {
        // Return and let the thread sleep until next wake interval
        return {};
    }
    process_work();
    return {};
}
```

### 3. The Priority Abuse Antipattern

❌ **Problematic Approach**:
```cpp
// Marking all jobs as high priority
for (auto i = 0; i < job_count; ++i) {
    jobs.push_back(std::make_unique<callback_priority_job>(
        [i]() -> result_void {
            // Regular task
            return {};
        },
        job_priorities::High // All jobs set to high priority
    ));
}
```

✅ **Better Approach**:
```cpp
// Assign appropriate priorities based on task importance
for (auto i = 0; i < job_count; ++i) {
    // Determine appropriate priority based on task characteristics
    auto priority = determine_appropriate_priority(i);
    
    jobs.push_back(std::make_unique<callback_priority_job>(
        [i]() -> result_void {
            // Regular task
            return {};
        },
        priority
    ));
}
```

### 4. The Blocking Thread Pool Antipattern

❌ **Problematic Approach**:
```cpp
// Submitting I/O-bound or blocking operations to thread pool
pool->enqueue(std::make_unique<thread_module::callback_job>(
    []() -> std::optional<std::string> {
        // Perform long-running I/O operation that blocks
        std::this_thread::sleep_for(std::chrono::seconds(10));
        return std::nullopt;
    }
));
```

✅ **Better Approach**:
```cpp
// Use asynchronous I/O or dedicated threads for blocking operations
// For I/O-bound operations, consider async I/O or a dedicated thread pool
auto io_thread = std::make_unique<thread_module::thread_base>("io_thread");
io_thread->start();

// Use the main thread pool for CPU-bound work only
pool->enqueue(std::make_unique<thread_module::callback_job>(
    []() -> std::optional<std::string> {
        // CPU-bound computation
        return std::nullopt;
    }
));
```

### 5. The Excessive Logging Antipattern

❌ **Problematic Approach**:
```cpp
for (int i = 0; i < 1000000; i++) {
    // Logging in a tight loop
    log_module::write_debug("Processing item: {}", i);
    // Process item
}
```

✅ **Better Approach**:
```cpp
// Log at appropriate intervals or significant events only
for (int i = 0; i < 1000000; i++) {
    // Only log at reasonable intervals
    if (i % 10000 == 0) {
        log_module::write_debug("Processing item: {}", i);
    }
    // Process item
}
// Log completion
log_module::write_information("Processed {} items", 1000000);
```

## Thread Synchronization Patterns

### 1. Producer-Consumer Pattern

```cpp
// Consumer thread worker
class ConsumerWorker : public thread_module::thread_base {
protected:
    auto do_work() -> result_void override {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        // Wait for data or stop signal
        data_condition_.wait(lock, [this]() {
            return !data_queue_.empty() || !should_continue_work();
        });
        
        // Check if should terminate
        if (!should_continue_work()) {
            return {};
        }
        
        // Process data
        auto data = std::move(data_queue_.front());
        data_queue_.pop();
        
        lock.unlock();
        process_data(data);
        
        return {};
    }
    
private:
    std::mutex queue_mutex_;
    std::condition_variable data_condition_;
    std::queue<Data> data_queue_;
    
    void process_data(const Data& data) {
        // Process the data
    }
};
```

### 2. Task Partitioning Pattern

```cpp
void process_large_dataset(const std::vector<Data>& dataset) {
    const size_t thread_count = std::thread::hardware_concurrency();
    const size_t chunk_size = (dataset.size() + thread_count - 1) / thread_count;
    
    // Create thread pool
    auto [pool, error] = create_default(thread_count);
    if (error.has_value()) {
        log_module::write_error("Failed to create thread pool: {}", error.value());
        return;
    }
    
    // Submit chunks as separate jobs
    std::vector<std::unique_ptr<thread_module::job>> jobs;
    for (size_t i = 0; i < thread_count; ++i) {
        size_t start_idx = i * chunk_size;
        size_t end_idx = std::min(start_idx + chunk_size, dataset.size());
        
        jobs.push_back(std::make_unique<thread_module::callback_job>(
            [&dataset, start_idx, end_idx]() -> std::optional<std::string> {
                for (size_t j = start_idx; j < end_idx; ++j) {
                    process_item(dataset[j]);
                }
                return std::nullopt;
            }
        ));
    }
    
    pool->enqueue_batch(std::move(jobs));
    pool->start();
    pool->stop(); // Wait for all jobs to complete
}
```

## Conclusion

Following these patterns and avoiding the antipatterns will help you use Thread System effectively. The core principles to remember are:

1. **Use the right tool for the job**: Choose the appropriate component based on your requirements
2. **Design for concurrency**: Think about thread safety from the start
3. **Avoid overengineering**: Use the simplest concurrency pattern that meets your needs
4. **Monitor and measure**: Always validate the performance benefits of your threading design
5. **Handle errors**: Always check return values and handle errors properly

By following these guidelines, you can create robust, efficient, and maintainable concurrent applications with Thread System.