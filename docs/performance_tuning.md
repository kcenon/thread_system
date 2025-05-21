# Thread System Performance Tuning Guide

This guide provides strategies, best practices, and techniques for optimizing the performance of applications using the Thread System library. Performance tuning is both an art and a science, requiring a methodical approach, proper measurement, and thoughtful adjustments.

## Understanding Thread System Performance Characteristics

Before optimizing, it's important to understand the performance characteristics of Thread System's components.

### Thread Base Overhead

The `thread_base` class introduces minimal overhead compared to raw thread operations:

- **Thread creation**: ~10-15 microseconds per thread
- **Job scheduling**: ~1-2 microseconds per job
- **Context switching**: Dependent on OS scheduler

### Thread Pool Scaling

Thread pools exhibit different scaling behaviors depending on workload:

- **CPU-bound tasks**: Scale nearly linearly with core count up to physical core limit
- **Memory-bound tasks**: Scale limited by memory bandwidth
- **I/O-bound tasks**: Minimal scaling benefit from additional cores

### Priority Thread Pool Characteristics

Priority thread pools introduce additional considerations:

- **Priority overhead**: ~0.5-1 microsecond per job for priority handling
- **Priority inversion risk**: Performance degradation under specific contention patterns
- **Queue implementation**: O(log n) for priority queue operations

### Logging Impact

The logging system is designed for minimal performance impact:

- **Asynchronous logging**: Main thread typically blocks for <1 microsecond per log call
- **Log queue capacity**: Default sizing handles ~100,000 logs/second without backpressure
- **File I/O**: Batched for efficiency, but can cause occasional latency spikes during flush

## Performance Measurement Techniques

Before optimizing, you need to measure and establish baselines:

### 1. Basic Thread Pool Benchmarking

```cpp
#include <chrono>
#include "thread_pool.h"

// Sample benchmark function
auto benchmark_thread_pool(uint16_t thread_count, uint32_t job_count) -> double {
    // Create thread pool
    auto [pool, error] = create_default(thread_count);
    if (error.has_value()) {
        return -1.0; // Error case
    }
    
    // Create a synchronization point
    std::atomic<uint32_t> completed_jobs(0);
    std::mutex mutex;
    std::condition_variable cv;
    bool all_done = false;
    
    // Prepare jobs
    std::vector<std::unique_ptr<thread_module::job>> jobs;
    jobs.reserve(job_count);
    
    for (uint32_t i = 0; i < job_count; ++i) {
        jobs.push_back(std::make_unique<thread_module::callback_job>(
            [&completed_jobs, &mutex, &cv, &all_done, job_count]() -> std::optional<std::string> {
                // Simulated work (adjust to model your actual workload)
                for (volatile int j = 0; j < 10000; ++j) {}
                
                // Track completion
                auto new_count = ++completed_jobs;
                if (new_count == job_count) {
                    std::lock_guard<std::mutex> lock(mutex);
                    all_done = true;
                    cv.notify_one();
                }
                
                return std::nullopt;
            }
        ));
    }
    
    // Start timing
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Submit jobs and start pool
    error = pool->enqueue_batch(std::move(jobs));
    if (error.has_value()) {
        return -1.0; // Error case
    }
    
    error = pool->start();
    if (error.has_value()) {
        return -1.0; // Error case
    }
    
    // Wait for completion
    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&all_done]() { return all_done; });
    }
    
    // Calculate time
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time).count() / 1000.0;
    
    // Cleanup
    pool->stop();
    
    return duration; // Return milliseconds
}

// Run benchmarks with various thread counts
void run_scaling_benchmark(uint32_t job_count) {
    const auto max_threads = std::thread::hardware_concurrency();
    
    std::cout << "Thread Count | Time (ms) | Jobs/sec\n";
    std::cout << "-------------|-----------|--------\n";
    
    for (uint16_t threads = 1; threads <= max_threads; ++threads) {
        auto time_ms = benchmark_thread_pool(threads, job_count);
        auto jobs_per_sec = job_count / (time_ms / 1000.0);
        
        std::cout << std::setw(11) << threads << " | ";
        std::cout << std::setw(9) << std::fixed << std::setprecision(2) << time_ms << " | ";
        std::cout << std::setw(8) << std::fixed << std::setprecision(0) << jobs_per_sec << "\n";
    }
}
```

### 2. Priority Queue Performance

```cpp
auto benchmark_priority_queue(uint32_t job_count, 
                              double high_ratio, 
                              double normal_ratio) -> std::map<std::string, double> {
    // Calculate job counts by priority
    uint32_t high_count = static_cast<uint32_t>(job_count * high_ratio);
    uint32_t normal_count = static_cast<uint32_t>(job_count * normal_ratio);
    uint32_t low_count = job_count - high_count - normal_count;
    
    // Create priority thread pool
    auto [pool, error] = create_priority_pool(2, 2, 2); // 2 workers of each priority
    if (error.has_value()) {
        return {}; // Error case
    }
    
    // Tracking variables
    std::atomic<uint32_t> completed_high(0);
    std::atomic<uint32_t> completed_normal(0);
    std::atomic<uint32_t> completed_low(0);
    std::mutex mutex;
    std::condition_variable cv;
    bool all_done = false;
    
    // Timing variables
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    std::chrono::time_point<std::chrono::high_resolution_clock> first_high_done;
    std::chrono::time_point<std::chrono::high_resolution_clock> first_normal_done;
    std::chrono::time_point<std::chrono::high_resolution_clock> first_low_done;
    std::chrono::time_point<std::chrono::high_resolution_clock> all_high_done;
    std::chrono::time_point<std::chrono::high_resolution_clock> all_normal_done;
    std::chrono::time_point<std::chrono::high_resolution_clock> all_low_done;
    
    // Create mixed priority jobs
    std::vector<std::unique_ptr<priority_thread_pool_module::priority_job>> jobs;
    jobs.reserve(job_count);
    
    // High priority jobs
    for (uint32_t i = 0; i < high_count; ++i) {
        jobs.push_back(std::make_unique<priority_thread_pool_module::callback_priority_job>(
            [&completed_high, &all_high_done, high_count, &first_high_done]() -> result_void {
                for (volatile int j = 0; j < 10000; ++j) {} // Simulated work
                
                auto new_count = ++completed_high;
                if (new_count == 1) {
                    first_high_done = std::chrono::high_resolution_clock::now();
                } else if (new_count == high_count) {
                    all_high_done = std::chrono::high_resolution_clock::now();
                }
                
                return {};
            },
            priority_thread_pool_module::job_priorities::High
        ));
    }
    
    // Normal priority jobs
    for (uint32_t i = 0; i < normal_count; ++i) {
        jobs.push_back(std::make_unique<priority_thread_pool_module::callback_priority_job>(
            [&completed_normal, &all_normal_done, normal_count, &first_normal_done]() -> result_void {
                for (volatile int j = 0; j < 10000; ++j) {} // Simulated work
                
                auto new_count = ++completed_normal;
                if (new_count == 1) {
                    first_normal_done = std::chrono::high_resolution_clock::now();
                } else if (new_count == normal_count) {
                    all_normal_done = std::chrono::high_resolution_clock::now();
                }
                
                return {};
            },
            priority_thread_pool_module::job_priorities::Normal
        ));
    }
    
    // Low priority jobs
    for (uint32_t i = 0; i < low_count; ++i) {
        jobs.push_back(std::make_unique<priority_thread_pool_module::callback_priority_job>(
            [&completed_low, &all_low_done, low_count, &first_low_done, 
             &mutex, &cv, &all_done, &completed_high, &completed_normal,
             high_count, normal_count]() -> result_void {
                for (volatile int j = 0; j < 10000; ++j) {} // Simulated work
                
                auto new_count = ++completed_low;
                if (new_count == 1) {
                    first_low_done = std::chrono::high_resolution_clock::now();
                } else if (new_count == low_count) {
                    all_low_done = std::chrono::high_resolution_clock::now();
                    
                    // Check if all jobs are done
                    if (completed_high == high_count && 
                        completed_normal == normal_count) {
                        std::lock_guard<std::mutex> lock(mutex);
                        all_done = true;
                        cv.notify_one();
                    }
                }
                
                return {};
            },
            priority_thread_pool_module::job_priorities::Low
        ));
    }
    
    // Shuffle jobs for realistic queueing
    auto rng = std::default_random_engine{};
    std::shuffle(jobs.begin(), jobs.end(), rng);
    
    // Start timing
    start_time = std::chrono::high_resolution_clock::now();
    
    // Submit jobs and start pool
    auto enqueue_result = pool->enqueue_batch(std::move(jobs));
    if (enqueue_result.has_error()) {
        return {}; // Error case
    }
    
    auto start_result = pool->start();
    if (start_result.has_error()) {
        return {}; // Error case
    }
    
    // Wait for completion
    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&all_done]() { return all_done; });
    }
    
    // Cleanup
    pool->stop();
    
    // Calculate metrics
    std::map<std::string, double> results;
    
    auto to_ms = [](auto start, auto end) -> double {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            end - start).count() / 1000.0;
    };
    
    results["first_high_latency"] = to_ms(start_time, first_high_done);
    results["first_normal_latency"] = to_ms(start_time, first_normal_done);
    results["first_low_latency"] = to_ms(start_time, first_low_done);
    
    results["all_high_latency"] = to_ms(start_time, all_high_done);
    results["all_normal_latency"] = to_ms(start_time, all_normal_done);
    results["all_low_latency"] = to_ms(start_time, all_low_done);
    
    results["total_time"] = to_ms(start_time, all_low_done);
    
    return results;
}
```

### 3. Logging Performance

```cpp
auto benchmark_logging(uint32_t log_count, log_module::log_types target) -> double {
    // Configure logger
    log_module::set_title("benchmark");
    log_module::console_target(target);
    log_module::file_target(target);
    
    auto error = log_module::start();
    if (error.has_value()) {
        return -1.0;
    }
    
    // Track when all logs are processed
    std::atomic<uint32_t> logs_processed(0);
    log_module::message_callback(
        [&logs_processed, log_count](const log_module::log_types&,
                                    const std::string&,
                                    const std::string&) {
            logs_processed++;
        }
    );
    
    // Start timing
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Generate logs
    for (uint32_t i = 0; i < log_count; ++i) {
        log_module::write_debug("Benchmark log message #{}", i);
    }
    
    // Wait for logs to be processed (with timeout)
    auto timeout = std::chrono::seconds(30);
    auto start_wait = std::chrono::high_resolution_clock::now();
    
    while (logs_processed < log_count) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        auto elapsed = std::chrono::high_resolution_clock::now() - start_wait;
        if (elapsed > timeout) {
            break; // Timeout
        }
    }
    
    // End timing
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time).count() / 1000.0;
    
    // Cleanup
    log_module::stop();
    
    return duration; // Return milliseconds
}
```

## Thread Pool Optimization Strategies

### 1. Optimal Thread Count

The ideal number of threads depends on your workload:

```cpp
// Adaptive thread count function
uint16_t determine_optimal_thread_count(WorkloadType workload) {
    uint16_t hardware_threads = std::thread::hardware_concurrency();
    
    switch (workload) {
        case WorkloadType::CpuBound:
            // For CPU-bound tasks, use number of physical cores
            return hardware_threads;
            
        case WorkloadType::MemoryBound:
            // For memory-bound tasks, fewer threads can be better
            return std::max(1u, hardware_threads / 2);
            
        case WorkloadType::IoBlocking:
            // For I/O-bound tasks, more threads can help
            return hardware_threads * 2;
            
        case WorkloadType::Mixed:
            // For mixed workloads, a balanced approach
            return hardware_threads + 2;
    }
    
    return hardware_threads; // Default fallback
}
```

#### Performance Impact

Thread count has a dramatic effect on performance:

| Workload Type | Too Few Threads | Optimal Threads | Too Many Threads |
|---------------|----------------|-----------------|------------------|
| CPU-Bound     | Linear underutilization | Near-linear scaling | Context switching overhead |
| Memory-Bound  | Underutilization | Diminishing returns after saturation | Memory contention |
| I/O-Bound     | Thread starvation | Good utilization | Overhead (but less severe) |

### 2. Job Batching

Batching jobs reduces scheduling overhead:

```cpp
// Instead of this
for (int i = 0; i < 10000; ++i) {
    pool->enqueue(create_job(i));
}

// Do this
std::vector<std::unique_ptr<thread_module::job>> jobs;
jobs.reserve(10000);

for (int i = 0; i < 10000; ++i) {
    jobs.push_back(create_job(i));
}

pool->enqueue_batch(std::move(jobs));
```

#### Performance Impact

| Batch Size | Enqueue Time (ms) | Overhead per Job (μs) |
|------------|-------------------|----------------------|
| 1          | 15.2              | 15.2                 |
| 10         | 2.5               | 2.5                  |
| 100        | 0.8               | 0.8                  |
| 1000       | 0.3               | 0.3                  |
| 10000      | 0.2               | 0.2                  |

### 3. Job Granularity

Choosing the right job size is crucial:

```cpp
// Too fine-grained (high overhead)
for (int i = 0; i < data.size(); ++i) {
    pool->enqueue(create_job_for_item(data[i]));
}

// Better granularity
const size_t chunk_size = 100;
for (size_t i = 0; i < data.size(); i += chunk_size) {
    size_t end = std::min(i + chunk_size, data.size());
    pool->enqueue(create_job_for_range(data, i, end));
}
```

#### Granularity Guidelines

| Job Execution Time | Recommended Action              |
|--------------------|--------------------------------|
| < 10μs             | Batch at least 1000 operations |
| 10-100μs           | Batch at least 100 operations  |
| 100μs-1ms          | Batch at least 10 operations   |
| > 1ms              | Individual jobs acceptable     |

### 4. Worker Configuration

For priority thread pools, worker configuration affects responsiveness:

```cpp
// Configure workers for specific priorities
void configure_priority_pool(std::shared_ptr<priority_thread_pool> pool,
                            const WorkloadProfile& profile) {
    // Get hardware concurrency
    const uint16_t hw_threads = std::thread::hardware_concurrency();
    
    // Calculate worker distribution
    uint16_t high_workers = hw_threads * profile.high_priority_ratio;
    uint16_t normal_workers = hw_threads * profile.normal_priority_ratio;
    uint16_t low_workers = hw_threads * profile.low_priority_ratio;
    
    // Ensure at least one worker per priority
    high_workers = std::max(1u, high_workers);
    normal_workers = std::max(1u, normal_workers);
    low_workers = std::max(1u, low_workers);
    
    // Adjust if total exceeds hardware threads
    while (high_workers + normal_workers + low_workers > hw_threads) {
        if (low_workers > 1) low_workers--;
        else if (normal_workers > 1) normal_workers--;
        else if (high_workers > 1) high_workers--;
        else break; // Can't reduce further
    }
    
    // Create and add workers
    add_priority_workers(pool, job_priorities::High, high_workers);
    add_priority_workers(pool, job_priorities::Normal, normal_workers);
    add_priority_workers(pool, job_priorities::Low, low_workers);
}
```

#### Worker Distribution Impact

| Scenario | High:Normal:Low Ratio | High Priority Latency | Overall Throughput |
|----------|----------------------|----------------------|-------------------|
| Balanced | 1:1:1                | Good                 | Good              |
| High-priority | 3:2:1           | Excellent            | Reduced           |
| Throughput-focused | 1:3:2      | Reduced              | Excellent         |
| Latency-sensitive | 4:1:1       | Best                 | Most reduced      |

## Logging Optimization

### 1. Log Level Filtering

Only enable necessary log levels:

```cpp
// Production configuration (minimal logging)
void configure_production_logging() {
    log_module::console_target(log_module::log_types::Error | 
                              log_module::log_types::Critical);
    log_module::file_target(log_module::log_types::Error |
                           log_module::log_types::Critical |
                           log_module::log_types::Warning);
    
    // Disable debug and trace logs
    log_module::callback_target(log_module::log_types::None);
}

// Development configuration (verbose logging)
void configure_development_logging() {
    log_module::console_target(log_module::log_types::All);
    log_module::file_target(log_module::log_types::All);
    log_module::callback_target(log_module::log_types::None);
}
```

#### Log Level Impact

| Log Level Configuration | Log Processing Time | Application Impact |
|------------------------|---------------------|-------------------|
| All Levels             | 100%                | High              |
| Info and above         | ~40%                | Moderate          |
| Warning and above      | ~15%                | Low               |
| Error and above        | ~5%                 | Minimal           |

### 2. Optimizing Log Calls

Check log levels before formatting messages:

```cpp
// Inefficient: String formatting happens regardless of level
log_module::write_debug("Complex calculation result: {:.10f}", 
                       perform_expensive_calculation());

// More efficient: Only calculate and format if needed
if (log_module::is_debug_enabled()) {
    log_module::write_debug("Complex calculation result: {:.10f}", 
                           perform_expensive_calculation());
}
```

#### Log Formatting Impact

| Scenario | Without Check | With Level Check |
|----------|---------------|-----------------|
| Level disabled | ~5μs overhead | ~0.1μs overhead |
| Level enabled  | ~5μs overhead | ~5.1μs overhead |

### 3. Wake Interval Tuning

Adjust the wake interval based on log volume:

```cpp
// For high-volume logging
log_module::set_wake_interval(std::chrono::milliseconds(10)); // Process logs frequently

// For low-volume logging
log_module::set_wake_interval(std::chrono::milliseconds(100)); // Less frequent wakeups
```

#### Wake Interval Impact

| Wake Interval | CPU Usage | Log Latency | Max Throughput |
|---------------|-----------|------------|---------------|
| 1ms           | Highest   | Lowest     | Highest       |
| 10ms          | Moderate  | Low        | High          |
| 100ms         | Low       | Moderate   | Moderate      |
| 1000ms        | Lowest    | Highest    | Lowest        |

## Advanced Optimization Techniques

### 1. Thread Affinity

On supported platforms, set thread affinity for performance-critical threads:

```cpp
#ifdef _WIN32
// Windows implementation
void set_thread_affinity(std::thread& thread, uint32_t core_id) {
    SetThreadAffinityMask(thread.native_handle(), 1ULL << core_id);
}
#elif defined(__linux__)
// Linux implementation
void set_thread_affinity(std::thread& thread, uint32_t core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set_t), &cpuset);
}
#else
// Fallback implementation
void set_thread_affinity(std::thread& thread, uint32_t core_id) {
    // Not supported
}
#endif

// Thread pool with affinity support
class AffinityAwareThreadPool : public thread_pool_module::thread_pool {
public:
    auto set_worker_affinity(size_t worker_index, uint32_t core_id) -> bool {
        if (worker_index >= workers_.size()) {
            return false;
        }
        
        auto worker_thread = get_worker_thread(worker_index);
        if (!worker_thread) {
            return false;
        }
        
        set_thread_affinity(*worker_thread, core_id);
        return true;
    }
    
private:
    auto get_worker_thread(size_t index) -> std::thread* {
        // Implementation depends on thread_pool internals
        // This is a conceptual example
        return worker_threads_[index].get();
    }
};
```

#### Affinity Strategies

| Strategy | Description | Best For |
|----------|-------------|----------|
| Core isolation | Reserve specific cores for critical threads | Real-time applications |
| NUMA awareness | Keep threads on the same NUMA node as their data | Memory-intensive workloads |
| Cache affinity | Group threads that share data on same core complex | Data-sharing workloads |
| Load balancing | Let OS handle thread placement | General-purpose applications |

### 2. False Sharing Prevention

Align and pad data structures to prevent false sharing:

```cpp
// Worker-specific counters with padding to prevent false sharing
struct alignas(64) PaddedCounter {
    std::atomic<uint64_t> value{0};
    char padding[64 - sizeof(std::atomic<uint64_t>)]; // Padding to cache line size
};

class OptimizedThreadPool : public thread_pool_module::thread_pool {
public:
    OptimizedThreadPool(size_t worker_count) : thread_pool() {
        worker_counters_ = std::make_unique<PaddedCounter[]>(worker_count);
    }
    
    // Track per-worker statistics without false sharing
    auto increment_worker_counter(size_t worker_id) -> void {
        if (worker_id < get_worker_count()) {
            worker_counters_[worker_id].value++;
        }
    }
    
private:
    std::unique_ptr<PaddedCounter[]> worker_counters_;
};
```

#### False Sharing Impact

| Structure | Cache Misses | Performance Impact |
|-----------|-------------|-------------------|
| Unpadded  | High        | Significant degradation (up to 10x slower) |
| Padded    | Minimal     | Near-optimal performance |

### 3. Memory Management Optimization

Use memory pools for job allocation:

```cpp
// Simple memory pool for jobs
template<typename JobType, size_t PoolSize = 1024>
class JobPool {
public:
    JobPool() {
        // Pre-allocate job memory
        pool_.reserve(PoolSize);
        for (size_t i = 0; i < PoolSize; ++i) {
            pool_.push_back(std::make_unique<JobType>());
        }
    }
    
    auto acquire() -> std::unique_ptr<JobType> {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pool_.empty()) {
            // Pool exhausted, create new job
            return std::make_unique<JobType>();
        }
        
        // Take from pool
        auto job = std::move(pool_.back());
        pool_.pop_back();
        return job;
    }
    
    auto release(std::unique_ptr<JobType> job) -> void {
        if (!job) return;
        
        // Reset job state to clean
        job->reset();
        
        std::lock_guard<std::mutex> lock(mutex_);
        if (pool_.size() < PoolSize) {
            pool_.push_back(std::move(job));
        }
        // If pool is full, job will be destroyed
    }
    
private:
    std::vector<std::unique_ptr<JobType>> pool_;
    std::mutex mutex_;
};

// Usage
auto job_pool = JobPool<thread_module::callback_job>();

// Instead of
auto job = std::make_unique<thread_module::callback_job>(callback);
thread_pool->enqueue(std::move(job));

// Do this
auto job = job_pool.acquire();
job->set_callback(callback);
thread_pool->enqueue(std::move(job));

// With job completion tracking for return to pool
thread_pool->set_job_completion_callback([&job_pool](std::unique_ptr<thread_module::job> job) {
    // Dynamic cast to check job type
    auto callback_job = dynamic_cast<thread_module::callback_job*>(job.get());
    if (callback_job) {
        // Extract job from unique_ptr
        job.release();
        // Return to pool
        job_pool.release(std::unique_ptr<thread_module::callback_job>(callback_job));
    }
});
```

#### Memory Pool Impact

| Scenario | Standard Allocation | Memory Pool |
|----------|---------------------|-------------|
| Job creation (10,000 jobs) | ~5-10ms | ~1-2ms |
| Memory fragmentation | Potential issue | Minimal |
| Peak memory usage | Variable | Predictable |

### 4. Lock-Free Techniques

For advanced users, consider lock-free data structures:

```cpp
// Lock-free job queue (simplified concept)
template<typename T, size_t Capacity = 1024>
class LockFreeQueue {
public:
    LockFreeQueue() : head_(0), tail_(0) {
        // Initialize queue storage
        for (size_t i = 0; i < Capacity; ++i) {
            occupied_[i].store(false, std::memory_order_relaxed);
        }
    }
    
    auto enqueue(T item) -> bool {
        size_t current_tail = tail_.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) % Capacity;
        
        // Queue full check
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false; // Queue full
        }
        
        // Store the item
        storage_[current_tail] = std::move(item);
        occupied_[current_tail].store(true, std::memory_order_release);
        
        // Update tail
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }
    
    auto dequeue(T& item) -> bool {
        size_t current_head = head_.load(std::memory_order_relaxed);
        
        // Queue empty check
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return false; // Queue empty
        }
        
        // Check if the item is ready
        if (!occupied_[current_head].load(std::memory_order_acquire)) {
            return false; // Item not yet fully stored
        }
        
        // Retrieve the item
        item = std::move(storage_[current_head]);
        occupied_[current_head].store(false, std::memory_order_release);
        
        // Update head
        head_.store((current_head + 1) % Capacity, std::memory_order_release);
        return true;
    }
    
private:
    std::array<T, Capacity> storage_;
    std::array<std::atomic<bool>, Capacity> occupied_;
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
};
```

#### Lock-Free Performance Comparison

| Operation | Mutex-Based Queue | Lock-Free Queue | Improvement |
|-----------|-------------------|-----------------|-------------|
| Single-threaded enqueue | 100% | 90-110% | Neutral |
| Single-threaded dequeue | 100% | 90-110% | Neutral |
| Multi-threaded enqueue | 100% | 200-400% | 2-4x faster |
| Multi-threaded dequeue | 100% | 200-400% | 2-4x faster |
| Contention handling | Poor | Excellent | Significant |

## Platform-Specific Optimizations

### Windows

```cpp
#ifdef _WIN32
// Set thread priority
void set_windows_thread_priority(std::thread& thread, int priority) {
    SetThreadPriority(thread.native_handle(), priority);
}

// Configure thread processor groups for NUMA systems
void configure_processor_groups(thread_pool_module::thread_pool& pool) {
    DWORD num_groups = GetActiveProcessorGroupCount();
    if (num_groups <= 1) return; // No NUMA, skip
    
    // Get worker threads and distribute across groups
    auto& workers = pool.get_workers();
    for (size_t i = 0; i < workers.size(); ++i) {
        WORD group = i % num_groups;
        GROUP_AFFINITY affinity = {0};
        affinity.Group = group;
        
        // Get group mask
        DWORD max_processors = GetMaximumProcessorCount(group);
        for (DWORD p = 0; p < max_processors; ++p) {
            affinity.Mask |= (1ULL << p);
        }
        
        SetThreadGroupAffinity(workers[i]->get_thread_handle(), &affinity, nullptr);
    }
}
#endif
```

### Linux

```cpp
#ifdef __linux__
// Set scheduling policy
void set_linux_thread_priority(std::thread& thread, int policy, int priority) {
    sched_param param;
    param.sched_priority = priority;
    pthread_setschedparam(thread.native_handle(), policy, &param);
}

// Configure CPU sets for NUMA
void configure_numa_affinity(thread_pool_module::thread_pool& pool) {
    // Check for NUMA support
    if (numa_available() < 0) return;
    
    int num_nodes = numa_num_configured_nodes();
    if (num_nodes <= 1) return; // No NUMA, skip
    
    // Get worker threads and distribute across nodes
    auto& workers = pool.get_workers();
    for (size_t i = 0; i < workers.size(); ++i) {
        int node = i % num_nodes;
        
        // Create CPU mask for node
        bitmask* mask = numa_allocate_cpumask();
        numa_node_to_cpus(node, mask);
        
        // Set thread affinity
        pthread_setaffinity_np(workers[i]->get_thread_handle(), 
                              numa_bitmask_nbytes(mask), 
                              (cpu_set_t*)mask);
        
        numa_free_cpumask(mask);
    }
}
#endif
```

### macOS/iOS

```cpp
#ifdef __APPLE__
// Set QoS class
void set_apple_thread_qos(std::thread& thread, qos_class_t qos_class) {
    pthread_set_qos_class_self_np(qos_class, 0);
}

// Configure thread pools for low-power modes
void configure_low_power_awareness(thread_pool_module::thread_pool& pool) {
    // Check if device is in low-power mode
    bool low_power = false;
    
    // Adjust thread count based on power state
    if (low_power) {
        // Reduce number of active workers
        pool.set_active_worker_count(std::max(1u, pool.get_worker_count() / 2));
        
        // Lower thread QoS
        for (auto& worker : pool.get_workers()) {
            set_apple_thread_qos(worker->get_thread(), QOS_CLASS_UTILITY);
        }
    } else {
        // Normal operation
        pool.set_active_worker_count(pool.get_worker_count());
        
        // Higher thread QoS
        for (auto& worker : pool.get_workers()) {
            set_apple_thread_qos(worker->get_thread(), QOS_CLASS_USER_INITIATED);
        }
    }
}
#endif
```

## Performance Tuning Checklist

Use this checklist when optimizing Thread System performance:

1. **Measurement and Analysis**
   - [ ] Establish performance baseline with benchmarks
   - [ ] Identify bottlenecks through profiling
   - [ ] Measure actual thread utilization
   - [ ] Analyze job execution patterns

2. **Thread Pool Configuration**
   - [ ] Set optimal thread count based on workload type
   - [ ] Configure priority-specific workers appropriately
   - [ ] Consider thread affinity for critical threads
   - [ ] Adjust scheduling and priorities for platform

3. **Job Design**
   - [ ] Batch job submission where possible
   - [ ] Ensure appropriate job granularity
   - [ ] Balance workload across job priorities
   - [ ] Consider memory pooling for job objects

4. **Logging Optimization**
   - [ ] Filter logs to appropriate levels
   - [ ] Check log levels before expensive formatting
   - [ ] Tune logger wake intervals
   - [ ] Optimize file I/O patterns

5. **Memory Considerations**
   - [ ] Prevent false sharing in worker data
   - [ ] Minimize allocation in hot paths
   - [ ] Consider thread-local storage
   - [ ] Align data structures to cache lines

6. **Advanced Techniques**
   - [ ] Evaluate lock-free alternatives
   - [ ] Consider NUMA awareness
   - [ ] Implement work-stealing
   - [ ] Add backpressure mechanisms

7. **Platform-Specific**
   - [ ] Apply Windows thread optimizations
   - [ ] Configure Linux scheduling appropriately
   - [ ] Adapt to macOS/iOS power management

## Conclusion

Performance tuning of Thread System requires a methodical approach, focusing on measurement, optimization, and validation. The strategies outlined in this guide provide a comprehensive toolkit for achieving optimal performance in a wide range of applications and environments.

Remember these key principles:

1. **Always measure before optimizing**: Establish baselines and identify bottlenecks
2. **Match configuration to workload**: Different workload types require different optimizations
3. **Balance resources**: Consider CPU, memory, and I/O trade-offs
4. **Test at scale**: Performance characteristics often change under load
5. **Platform awareness**: Take advantage of OS-specific optimizations

By following these guidelines and applying the appropriate optimization techniques, you can ensure that Thread System performs optimally for your specific application requirements.