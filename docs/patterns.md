# Thread System: Patterns, Best Practices, and Troubleshooting Guide

This comprehensive guide covers patterns, best practices, antipatterns to avoid, and solutions to common concurrency issues when working with Thread System. Following these guidelines will help you write efficient, maintainable, and bug-free concurrent applications.

## Table of Contents

1. [Best Practices](#best-practices)
2. [Common Patterns](#common-patterns)
3. [Antipatterns to Avoid](#antipatterns-to-avoid)
4. [Troubleshooting Common Issues](#troubleshooting-common-issues)
5. [Advanced Concurrency Patterns](#advanced-concurrency-patterns)
6. [Debugging Concurrent Code](#debugging-concurrent-code)
7. [Performance Optimization](#performance-optimization)

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

### 3. Type Thread Pool Usage

#### ✅ DO:
- Use distinct type levels for different types of tasks
- Create dedicated workers for critical type levels
- Use lower type for background or maintenance tasks
- Consider custom type types for domain-specific scheduling
- Monitor queue sizes per type to ensure balanced execution

#### ❌ DON'T:
- Assign high type to all tasks (defeats the purpose)
- Create too many type levels (3-5 levels are typically sufficient)
- Ignore type inversion issues
- Use types inconsistently across the application
- Create type workers without assigning appropriate jobs

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

### 3. Type-Based Job Execution Pattern

```cpp
// Create a type thread pool with different type workers
auto [pool, error] = create_type_pool(
    high_type_workers_,
    normal_type_workers_,
    low_type_workers_
);

// Creating jobs with different types
std::vector<std::unique_ptr<typed_thread_pool_module::typed_job>> jobs;
jobs.reserve(job_count);

// High type critical tasks
jobs.push_back(std::make_unique<typed_thread_pool_module::callback_typed_job>(
    []() -> result_void {
        // Critical operation
        return {};
    },
    typed_thread_pool_module::job_types::High
));

// Normal type regular tasks
jobs.push_back(std::make_unique<typed_thread_pool_module::callback_typed_job>(
    []() -> result_void {
        // Regular operation
        return {};
    },
    typed_thread_pool_module::job_types::Normal
));

// Low type background tasks
jobs.push_back(std::make_unique<typed_thread_pool_module::callback_typed_job>(
    []() -> result_void {
        // Background operation
        return {};
    },
    typed_thread_pool_module::job_types::Low
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

### 5. Producer-Consumer Pattern

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

### 6. Task Partitioning Pattern

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

### 3. The Type Abuse Antipattern

❌ **Problematic Approach**:
```cpp
// Marking all jobs as high type
for (auto i = 0; i < job_count; ++i) {
    jobs.push_back(std::make_unique<callback_typed_job>(
        [i]() -> result_void {
            // Regular task
            return {};
        },
        job_types::High // All jobs set to high type
    ));
}
```

✅ **Better Approach**:
```cpp
// Assign appropriate types based on task importance
for (auto i = 0; i < job_count; ++i) {
    // Determine appropriate type based on task characteristics
    auto type = determine_appropriate_type(i);
    
    jobs.push_back(std::make_unique<callback_typed_job>(
        [i]() -> result_void {
            // Regular task
            return {};
        },
        type
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

## Troubleshooting Common Issues

### 1. Race Conditions

#### Symptoms
- Inconsistent or unexpected results
- Program behavior varies between runs
- Results depend on timing or system load
- Intermittent crashes or data corruption

#### Solution Approaches
1. **Use mutex protection:**
   ```cpp
   std::mutex counter_mutex_;
   
   // In thread code
   std::lock_guard<std::mutex> lock(counter_mutex_);
   counter_++;
   ```

2. **Use atomic variables:**
   ```cpp
   std::atomic<int> counter_{0};
   
   // In thread code (no mutex needed)
   counter_++;
   ```

3. **Use job-based design:**
   ```cpp
   // Define a counter modification job
   auto increment_job = std::make_unique<thread_module::callback_job>(
       [this]() -> std::optional<std::string> {
           counter_++;
           return std::nullopt;
       }
   );
   
   // Submit to thread pool
   thread_pool->enqueue(std::move(increment_job));
   ```

### 2. Deadlocks

#### Symptoms
- Program freezes or hangs
- Multiple threads become unresponsive
- No CPU usage despite program appearing to run
- Deadlock detection tools report lock cycles

#### Solution Approaches
1. **Consistent lock ordering:**
   ```cpp
   // Always acquire locks in the same order
   std::lock_guard<std::mutex> lock1(mutex_a_); // Always first
   std::lock_guard<std::mutex> lock2(mutex_b_); // Always second
   ```

2. **Use std::lock for multiple locks:**
   ```cpp
   std::unique_lock<std::mutex> lock_a(mutex_a_, std::defer_lock);
   std::unique_lock<std::mutex> lock_b(mutex_b_, std::defer_lock);
   std::lock(lock_a, lock_b); // Atomic acquisition of both locks
   ```

3. **Avoid nested locks where possible:**
   ```cpp
   // Instead of nested locks, acquire all needed locks upfront
   {
       std::lock_guard<std::mutex> lock(mutex_);
       // Do all work requiring the lock here
   }
   // Then do work not requiring the lock
   ```

4. **Use lock timeouts to detect deadlocks:**
   ```cpp
   std::unique_lock<std::mutex> lock(mutex_, std::chrono::seconds(5));
   if (!lock) {
       log_module::write_error("Potential deadlock detected - could not acquire lock within timeout");
       // Implement recovery strategy
   }
   ```

### 3. Type Inversion

#### Symptoms
- High-type tasks experience unexpected delays
- System responsiveness is inconsistent
- Critical jobs take longer than lower-type ones

#### Solution Approaches
1. **Minimize resource sharing across type boundaries:**
   ```cpp
   // Design your system to minimize cases where high and low type
   // threads need to share resources. Use separate resources when possible.
   auto high_type_resources = std::make_unique<ResourcePool>("high");
   auto low_type_resources = std::make_unique<ResourcePool>("low");
   ```

2. **Use type ceilings:**
   ```cpp
   // When a low-type thread acquires a critical resource,
   // temporarily boost its type
   auto original_type = get_current_type();
   set_current_type(high_type);
   
   // Critical section with shared resource
   {
       std::lock_guard<std::mutex> lock(shared_mutex_);
       // Work with shared resource
   }
   
   // Restore original type
   set_current_type(original_type);
   ```

### 4. Thread Starvation

#### Symptoms
- Certain tasks never complete or experience extreme delays
- Some threads never get CPU time
- System seems to focus on a subset of available work

#### Solution Approaches
1. **Dedicate workers to each type level:**
   ```cpp
   // Create workers specifically for low-type tasks
   auto low_worker = std::make_unique<typed_thread_worker>(
       std::vector<job_types>{job_types::Low},
       "low_type_worker"
   );
   
   type_pool->enqueue(std::move(low_worker));
   ```

2. **Implement aging for low-type tasks:**
   ```cpp
   class AgingJob : public typed_job {
   public:
       AgingJob(std::function<result_void()> func)
           : typed_job(job_types::Low), func_(func), 
             creation_time_(std::chrono::steady_clock::now()) {}
       
       auto get_type() const -> job_types override {
           auto age = std::chrono::steady_clock::now() - creation_time_;
           if (age > std::chrono::minutes(5)) {
               return job_types::High;
           } else if (age > std::chrono::minutes(1)) {
               return job_types::Normal;
           }
           return job_types::Low;
       }
       
   private:
       std::function<result_void()> func_;
       std::chrono::steady_clock::time_point creation_time_;
   };
   ```

### 5. False Sharing

#### Symptoms
- Unexpectedly poor performance in multi-threaded code
- Performance degrades as more cores are used
- CPU cache profiling shows high cache coherence traffic

#### Solution Approaches
1. **Pad data structures to avoid false sharing:**
   ```cpp
   struct alignas(64) PaddedCounter {
       std::atomic<int> value{0};
       char padding[64 - sizeof(std::atomic<int>)];
   };
   
   PaddedCounter counter1_;
   PaddedCounter counter2_;
   ```

2. **Use thread-local storage for counters:**
   ```cpp
   thread_local int local_counter_ = 0;
   
   // Each thread updates its own counter
   auto job = std::make_unique<thread_module::callback_job>(
       []() -> std::optional<std::string> {
           local_counter_++;
           return std::nullopt;
       }
   );
   ```

### 6. Memory Visibility Issues

#### Symptoms
- Threads don't "see" updates made by other threads
- Stale data is used in calculations
- Non-atomic operations on shared variables cause corruption

#### Solution Approaches
1. **Use atomic variables for flags:**
   ```cpp
   std::atomic<bool> done_{false};
   
   // Thread 1
   done_.store(true, std::memory_order_release);
   
   // Thread 2
   while (!done_.load(std::memory_order_acquire)) {
       std::this_thread::yield();
   }
   ```

2. **Use proper synchronization primitives:**
   ```cpp
   std::mutex mutex_;
   bool done_ = false;
   std::condition_variable cv_;
   
   // Thread 1
   {
       std::lock_guard<std::mutex> lock(mutex_);
       done_ = true;
   }
   cv_.notify_all();
   
   // Thread 2
   {
       std::unique_lock<std::mutex> lock(mutex_);
       cv_.wait(lock, [this]() { return done_; });
   }
   ```

## Advanced Concurrency Patterns

### 1. Event-Based Communication

```cpp
class EventSystem {
public:
    using EventHandler = std::function<void(const Event&)>;
    
    void subscribe(EventType type, EventHandler handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_[type].push_back(handler);
    }
    
    void publish(const Event& event) {
        std::vector<EventHandler> handlers_to_call;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = handlers_.find(event.type);
            if (it != handlers_.end()) {
                handlers_to_call = it->second;
            }
        }
        
        // Submit each handler to the thread pool
        for (const auto& handler : handlers_to_call) {
            thread_pool_->enqueue(std::make_unique<thread_module::callback_job>(
                [handler, event]() -> std::optional<std::string> {
                    handler(event);
                    return std::nullopt;
                }
            ));
        }
    }
    
private:
    std::mutex mutex_;
    std::map<EventType, std::vector<EventHandler>> handlers_;
    std::shared_ptr<thread_pool_module::thread_pool> thread_pool_;
};
```

### 2. Work Stealing Pattern

```cpp
class WorkStealingPool {
public:
    WorkStealingPool(size_t worker_count) {
        // Create per-worker queues
        queues_.resize(worker_count);
        
        // Create workers
        for (size_t i = 0; i < worker_count; ++i) {
            workers_.push_back(std::make_unique<WorkStealingWorker>(
                i, queues_, *this
            ));
        }
    }
    
    auto enqueue(std::unique_ptr<thread_module::job> job, size_t preferred_worker) -> void {
        if (preferred_worker >= queues_.size()) {
            preferred_worker = 0;
        }
        
        queues_[preferred_worker].enqueue(std::move(job));
    }
    
private:
    std::vector<std::unique_ptr<WorkStealingWorker>> workers_;
    std::vector<JobQueue> queues_;
};

class WorkStealingWorker : public thread_module::thread_base {
protected:
    auto do_work() -> result_void override {
        // Try to get job from own queue
        auto job = queues_[worker_id_].dequeue();
        
        // If no job, try to steal from other queues
        if (!job) {
            for (size_t i = 0; i < queues_.size(); ++i) {
                if (i == worker_id_) continue;
                
                job = queues_[i].try_steal();
                if (job) break;
            }
        }
        
        // If we got a job, execute it
        if (job) {
            job->execute();
            return {};
        }
        
        // No job found, let the thread sleep
        return {};
    }
    
private:
    size_t worker_id_;
    std::vector<JobQueue>& queues_;
};
```

### 3. Read-Write Lock Pattern

```cpp
class ReadWriteLock {
public:
    void read_lock() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (write_count_ > 0 || write_waiting_ > 0) {
            read_cv_.wait(lock);
        }
        read_count_++;
    }
    
    void read_unlock() {
        std::unique_lock<std::mutex> lock(mutex_);
        read_count_--;
        if (read_count_ == 0 && write_waiting_ > 0) {
            write_cv_.notify_one();
        }
    }
    
    void write_lock() {
        std::unique_lock<std::mutex> lock(mutex_);
        write_waiting_++;
        while (read_count_ > 0 || write_count_ > 0) {
            write_cv_.wait(lock);
        }
        write_waiting_--;
        write_count_++;
    }
    
    void write_unlock() {
        std::unique_lock<std::mutex> lock(mutex_);
        write_count_--;
        if (write_waiting_ > 0) {
            write_cv_.notify_one();
        } else {
            read_cv_.notify_all();
        }
    }
    
private:
    std::mutex mutex_;
    std::condition_variable read_cv_;
    std::condition_variable write_cv_;
    int read_count_ = 0;
    int write_count_ = 0;
    int write_waiting_ = 0;
};
```

## Debugging Concurrent Code

### Using Logs Effectively

1. **Include thread IDs in logs:**
   ```cpp
   log_module::message_callback([](const log_module::log_types& type,
                                  const std::string& datetime,
                                  const std::string& message) {
       // Get current thread ID
       auto thread_id = std::this_thread::get_id();
       std::cout << "[" << datetime << "][Thread " << thread_id << "][" << type << "] " 
                 << message << std::endl;
   });
   ```

2. **Log state transitions:**
   ```cpp
   auto do_work() -> result_void override {
       log_module::write_debug("Worker state: entering critical section");
       {
           std::lock_guard<std::mutex> lock(mutex_);
           // Critical section
           log_module::write_debug("Worker state: in critical section, count={}", count_);
       }
       log_module::write_debug("Worker state: exited critical section");
       return {};
   }
   ```

3. **Use sequence numbers for ordering events:**
   ```cpp
   std::atomic<uint64_t> global_seq_{0};
   
   void log_event(const std::string& event) {
       uint64_t seq = global_seq_++;
       log_module::write_sequence("[SEQ:{}] {}", seq, event);
   }
   ```

### Using Thread Sanitizers

1. **Enable ThreadSanitizer in your build:**
   ```bash
   # For GCC/Clang
   g++ -fsanitize=thread -g mycode.cpp
   
   # For MSVC
   # Use /fsanitize=address in recent versions
   ```

2. **Common issues detected by thread sanitizers:**
   - Data races
   - Deadlocks
   - Double-locking
   - Use-after-free in concurrent contexts

### Common Thread System Debugging Steps

1. **Verify thread pool startup:**
   ```cpp
   auto error = pool->start();
   if (error.has_value()) {
       log_module::write_error("Thread pool failed to start: {}", 
                               error.value_or("unknown error"));
       // Handle error
   } else {
       log_module::write_information("Thread pool started successfully with {} workers", 
                                     pool->get_worker_count());
   }
   ```

2. **Check job execution:**
   ```cpp
   // Add debugging to your job
   auto job = std::make_unique<thread_module::callback_job>(
       [](void) -> std::optional<std::string> {
           log_module::write_debug("Job started");
           
           // Your job logic here
           
           log_module::write_debug("Job completed");
           return std::nullopt;
       }
   );
   ```

## Performance Optimization

### Thread Pool Sizing Guidelines

1. **For CPU-bound tasks:**
   ```cpp
   // Use hardware concurrency as a baseline
   auto thread_count = std::thread::hardware_concurrency();
   ```

2. **For I/O-bound tasks:**
   ```cpp
   // Consider using more threads than cores
   auto thread_count = std::thread::hardware_concurrency() * 2;
   ```

3. **For mixed workloads:**
   ```cpp
   // Create separate pools for different workload types
   auto cpu_pool = create_default(std::thread::hardware_concurrency());
   auto io_pool = create_default(std::thread::hardware_concurrency() * 2);
   ```

### Batch Job Submission

Always prefer batch submission over individual job enqueueing:

```cpp
// ❌ Inefficient
for (const auto& task : tasks) {
    pool->enqueue(create_job(task));
}

// ✅ Efficient
std::vector<std::unique_ptr<thread_module::job>> jobs;
for (const auto& task : tasks) {
    jobs.push_back(create_job(task));
}
pool->enqueue_batch(std::move(jobs));
```

### Wake Interval Optimization

Configure wake intervals based on your workload:

```cpp
// For high-frequency tasks
worker.set_wake_interval(std::chrono::milliseconds(10));

// For periodic maintenance
worker.set_wake_interval(std::chrono::seconds(1));

// For rare events
worker.set_wake_interval(std::chrono::minutes(1));
```

## Conclusion

Following these patterns and avoiding the antipatterns will help you use Thread System effectively. The core principles to remember are:

1. **Use the right tool for the job**: Choose the appropriate component based on your requirements
2. **Design for concurrency**: Think about thread safety from the start
3. **Avoid overengineering**: Use the simplest concurrency pattern that meets your needs
4. **Monitor and measure**: Always validate the performance benefits of your threading design
5. **Handle errors**: Always check return values and handle errors properly
6. **Debug methodically**: Use logs, debuggers, and thread sanitizers to identify issues

By following these guidelines, you can create robust, efficient, and maintainable concurrent applications with Thread System.