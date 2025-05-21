# Concurrency Troubleshooting Guide

This guide addresses common concurrency issues and their solutions when working with Thread System. Concurrent programming introduces unique challenges that are often difficult to diagnose and resolve. This document will help you identify, understand, and fix these issues.

## Common Concurrency Issues

### 1. Race Conditions

#### Symptoms
- Inconsistent or unexpected results
- Program behavior varies between runs
- Results depend on timing or system load
- Intermittent crashes or data corruption

#### Example
```cpp
// Thread 1
counter_++;

// Thread 2
counter_++;
```

#### How Thread System Helps
Thread System encourages proper encapsulation of shared state. By using job-based designs, you can isolate shared state and access it in a controlled manner.

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

#### Example
```cpp
// Thread 1
std::lock_guard<std::mutex> lock1(mutex_a_);
std::lock_guard<std::mutex> lock2(mutex_b_);

// Thread 2
std::lock_guard<std::mutex> lock1(mutex_b_);
std::lock_guard<std::mutex> lock2(mutex_a_);
```

#### How Thread System Helps
Thread System encourages structured concurrency patterns that reduce the likelihood of deadlocks. Using the thread pool and job-based approach minimizes direct mutex handling.

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

### 3. Livelocks

#### Symptoms
- Program appears active (threads are running) but makes no progress
- CPU usage is high, but work is not completing
- Threads keep yielding to each other without accomplishing tasks

#### Example
```cpp
// Thread 1
while (true) {
    if (resource_available()) {
        if (!try_acquire_resource()) {
            std::this_thread::yield(); // Give up CPU to Thread 2
            continue;
        }
        break;
    }
}

// Thread 2 (similar code)
while (true) {
    if (resource_available()) {
        if (!try_acquire_resource()) {
            std::this_thread::yield(); // Give up CPU to Thread 1
            continue;
        }
        break;
    }
}
```

#### How Thread System Helps
Thread System's built-in scheduling in thread pools helps prevent livelocks by ensuring fair allocation of CPU time to jobs.

#### Solution Approaches
1. **Use randomized backoff:**
   ```cpp
   // Add randomized delay before retrying
   if (!try_acquire_resource()) {
       std::random_device rd;
       std::mt19937 gen(rd());
       std::uniform_int_distribution<> dist(1, 100);
       std::this_thread::sleep_for(std::chrono::milliseconds(dist(gen)));
       continue;
   }
   ```

2. **Implement priority-based resolution:**
   ```cpp
   // Use priority thread pool to handle resource contention
   auto job = std::make_unique<callback_priority_job>(
       [this]() -> result_void {
           acquire_resource();
           use_resource();
           release_resource();
           return {};
       },
       determine_priority() // Function that assigns appropriate priority
   );
   ```

3. **Use explicit lock ordering based on thread ID:**
   ```cpp
   if (my_thread_id < other_thread_id) {
       acquire_resource_a_then_b();
   } else {
       acquire_resource_b_then_a();
   }
   ```

### 4. Priority Inversion

#### Symptoms
- High-priority tasks experience unexpected delays
- System responsiveness is inconsistent
- Critical jobs take longer than lower-priority ones

#### Example
```cpp
// Low-priority thread acquires a shared resource
low_priority_thread->start();  // Acquires mutex

// High-priority thread tries to get the same resource
high_priority_thread->start(); // Blocks waiting for mutex

// Medium-priority thread runs instead of the blocked high-priority thread
medium_priority_thread->start(); // Preempts the low-priority thread
```

#### How Thread System Helps
Priority thread pools in Thread System provide a foundation for handling priorities correctly, but you still need to be aware of resource sharing across priority levels.

#### Solution Approaches
1. **Use priority inheritance mutexes:**
   ```cpp
   // Note: std::mutex doesn't support priority inheritance directly
   // Consider using OS-specific primitives when available
   #ifdef POSIX_PLATFORM
   pthread_mutexattr_t attr;
   pthread_mutexattr_init(&attr);
   pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
   
   pthread_mutex_t mutex;
   pthread_mutex_init(&mutex, &attr);
   #endif
   ```

2. **Minimize resource sharing across priority boundaries:**
   ```cpp
   // Design your system to minimize cases where high and low priority
   // threads need to share resources. Use separate resources when possible.
   auto high_priority_resources = std::make_unique<ResourcePool>("high");
   auto low_priority_resources = std::make_unique<ResourcePool>("low");
   ```

3. **Use priority ceilings:**
   ```cpp
   // When a low-priority thread acquires a critical resource,
   // temporarily boost its priority
   auto original_priority = get_current_priority();
   set_current_priority(high_priority);
   
   // Critical section with shared resource
   {
       std::lock_guard<std::mutex> lock(shared_mutex_);
       // Work with shared resource
   }
   
   // Restore original priority
   set_current_priority(original_priority);
   ```

### 5. Thread Starvation

#### Symptoms
- Certain tasks never complete or experience extreme delays
- Some threads never get CPU time
- System seems to focus on a subset of available work

#### Example
```cpp
// A thread pool with continuous high-priority jobs
while (true) {
    pool->enqueue(std::make_unique<callback_priority_job>(
        []() -> result_void {
            // High-priority work that keeps coming
            return {};
        },
        job_priorities::High
    ));
}

// Low-priority jobs never get executed
pool->enqueue(std::make_unique<callback_priority_job>(
    []() -> result_void {
        // This job may never run
        return {};
    },
    job_priorities::Low
));
```

#### How Thread System Helps
Thread System's priority thread pool allows you to assign workers to specific priority levels, ensuring that even low-priority tasks get some CPU time.

#### Solution Approaches
1. **Dedicate workers to each priority level:**
   ```cpp
   // Create workers specifically for low-priority tasks
   auto low_worker = std::make_unique<priority_thread_worker>(
       std::vector<job_priorities>{job_priorities::Low},
       "low_priority_worker"
   );
   
   priority_pool->enqueue(std::move(low_worker));
   ```

2. **Implement aging for low-priority tasks:**
   ```cpp
   class AgingJob : public priority_job {
   public:
       AgingJob(std::function<result_void()> func)
           : priority_job(job_priorities::Low), func_(func), 
             creation_time_(std::chrono::steady_clock::now()) {}
       
       auto get_priority() const -> job_priorities override {
           auto age = std::chrono::steady_clock::now() - creation_time_;
           if (age > std::chrono::minutes(5)) {
               return job_priorities::High;
           } else if (age > std::chrono::minutes(1)) {
               return job_priorities::Normal;
           }
           return job_priorities::Low;
       }
       
   private:
       std::function<result_void()> func_;
       std::chrono::steady_clock::time_point creation_time_;
   };
   ```

3. **Use fairness policies in schedulers:**
   ```cpp
   // Implement a custom job queue that ensures fairness
   class FairJobQueue : public priority_job_queue {
   public:
       auto dequeue() -> std::unique_ptr<priority_job> override {
           fairness_counter_++;
           
           // Every 10 dequeues, force a low-priority job if available
           if (fairness_counter_ % 10 == 0) {
               auto job = get_low_priority_job();
               if (job) {
                   return job;
               }
           }
           
           // Otherwise use normal priority logic
           return priority_job_queue::dequeue();
       }
       
   private:
       uint64_t fairness_counter_ = 0;
   };
   ```

### 6. False Sharing

#### Symptoms
- Unexpectedly poor performance in multi-threaded code
- Performance degrades as more cores are used
- CPU cache profiling shows high cache coherence traffic

#### Example
```cpp
// Thread 1 updates counter1_
// Thread 2 updates counter2_
// If these variables are on the same cache line, they will cause false sharing

struct ThreadData {
    int counter1_; // Updated by Thread 1
    int counter2_; // Updated by Thread 2
};
```

#### How Thread System Helps
Thread System itself doesn't directly address false sharing, but it provides a context where you can implement proper solutions.

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

3. **Group data by thread access patterns:**
   ```cpp
   struct ThreadOneData {
       // All data accessed by Thread 1
   };
   
   struct ThreadTwoData {
       // All data accessed by Thread 2
   };
   ```

### 7. Memory Visibility Issues

#### Symptoms
- Threads don't "see" updates made by other threads
- Stale data is used in calculations
- Non-atomic operations on shared variables cause corruption

#### Example
```cpp
// Thread 1
done_ = true;

// Thread 2
while (!done_) {
    // This might run forever on some architectures/compilers
    // if done_ is not volatile or atomic
}
```

#### How Thread System Helps
Thread System encourages proper synchronization through its job-based architecture, reducing the need to manually handle memory visibility.

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

3. **Leverage Thread System's job-based approach:**
   ```cpp
   // Instead of manually managing flags between threads,
   // design with jobs that have clear dependencies
   
   auto first_job = std::make_unique<thread_module::callback_job>(
       [this]() -> std::optional<std::string> {
           // Do first part of work
           return std::nullopt;
       }
   );
   
   auto second_job = std::make_unique<thread_module::callback_job>(
       [this]() -> std::optional<std::string> {
           // Do second part of work
           return std::nullopt;
       }
   );
   
   // Proper ordering is ensured by the thread pool
   thread_pool->enqueue(std::move(first_job));
   thread_pool->enqueue(std::move(second_job));
   ```

## Debugging Concurrent Code

### Using Logs Effectively

Thread System's logging module is a powerful tool for debugging concurrent code. Here are some strategies:

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

### Using Debuggers

When using debuggers with Thread System, keep these tips in mind:

1. **Set breakpoints in key thread functions:**
   - `thread_base::start()`
   - `thread_base::do_work()`
   - `job::execute()`
   - `priority_job_queue::dequeue()`

2. **Use conditional breakpoints for specific threads:**
   ```
   // Example GDB conditional breakpoint
   break thread_pool.cpp:125 if thread_id == 3
   ```

3. **Examine thread stacks:**
   Most debuggers have commands to view and switch between thread stacks:
   ```
   // In GDB
   info threads
   thread 2
   bt
   ```

4. **Look for lock contention:**
   Watch for threads blocked on mutex acquisition.

### Using Thread Sanitizers

Modern compilers come with thread sanitizers that can detect many concurrency issues:

1. **Enable ThreadSanitizer in your build:**
   ```bash
   # For GCC/Clang
   g++ -fsanitize=thread -g mycode.cpp
   
   # For MSVC
   # Use /fsanitize=address in recent versions
   ```

2. **Interpret sanitizer output:**
   Thread sanitizers will report issues like:
   - Data races
   - Deadlocks
   - Double-locking
   - Use-after-free in concurrent contexts

### Common Thread System Debugging Steps

When troubleshooting issues specific to Thread System, try these steps:

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

3. **Inspect worker states:**
   ```cpp
   // Print worker information
   for (const auto& worker : workers_) {
       log_module::write_debug("Worker {}: {}", 
                              worker->get_thread_title(),
                              worker->is_running() ? "Running" : "Stopped");
   }
   ```

4. **Validate priority queue behavior:**
   ```cpp
   // Add instrumentation to track job priorities
   log_module::write_debug("Job priority queue state: High={}, Normal={}, Low={}",
                          high_priority_count_,
                          normal_priority_count_,
                          low_priority_count_);
   ```

## Advanced Concurrency Patterns for Thread System

### Event-Based Communication

Use events to communicate between threads without direct coupling:

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

### Work Stealing Pattern

Implement work stealing between thread pool workers to balance load:

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

### Read-Write Lock Pattern

Implement a read-write lock for improved concurrency in read-heavy workloads:

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

// Usage with Thread System
class ReadWriteProtectedResource {
public:
    auto read_operation() -> void {
        read_write_lock_.read_lock();
        // Read operations
        read_write_lock_.read_unlock();
    }
    
    auto write_operation() -> void {
        read_write_lock_.write_lock();
        // Write operations
        read_write_lock_.write_unlock();
    }
    
    // Thread System integration
    auto submit_read_job(thread_pool_module::thread_pool& pool) -> void {
        pool.enqueue(std::make_unique<thread_module::callback_job>(
            [this]() -> std::optional<std::string> {
                read_operation();
                return std::nullopt;
            }
        ));
    }
    
    auto submit_write_job(thread_pool_module::thread_pool& pool) -> void {
        pool.enqueue(std::make_unique<thread_module::callback_job>(
            [this]() -> std::optional<std::string> {
                write_operation();
                return std::nullopt;
            }
        ));
    }
    
private:
    ReadWriteLock read_write_lock_;
    // Protected resource
};
```

## Conclusion

Concurrency issues can be challenging, but Thread System provides a structured approach to manage them. By understanding common concurrency problems and their solutions, leveraging Thread System's components properly, and following best practices, you can build robust concurrent applications.

Remember these key principles:

1. **Isolate shared state**: Minimize sharing mutable data between threads
2. **Use appropriate synchronization**: Choose the right tool for each concurrency challenge
3. **Leverage Thread System abstractions**: Let the library handle complex threading details
4. **Test thoroughly**: Concurrency bugs can be hard to reproduce, so write tests that stress your system
5. **Debug methodically**: Use logs, debuggers, and thread sanitizers to identify issues
6. **Design for concurrency**: Consider thread safety from the beginning of your design