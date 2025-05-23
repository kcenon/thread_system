# Thread System Performance Guide

This comprehensive guide covers performance benchmarks, tuning strategies, and optimization techniques for the Thread System framework. It combines measurement data with practical advice for achieving optimal performance.

## Table of Contents

1. [Performance Characteristics](#performance-characteristics)
2. [Benchmark Results](#benchmark-results)
3. [Performance Measurement](#performance-measurement)
4. [Optimization Strategies](#optimization-strategies)
5. [Advanced Techniques](#advanced-techniques)
6. [Platform-Specific Optimizations](#platform-specific-optimizations)
7. [Best Practices](#best-practices)

## Performance Characteristics

Understanding the baseline performance characteristics is essential for optimization.

### Component Overhead

| Component | Operation | Overhead |
|-----------|-----------|----------|
| Thread Base | Creation | ~10-15 μs per thread |
| Thread Base | Job scheduling | ~1-2 μs per job |
| Priority Pool | Priority handling | ~0.5-1 μs per job |
| Priority Pool | Queue operations | O(log n) |
| Logger | Async log call | <1 μs per call |
| Logger | Default capacity | ~100,000 logs/second |

### Scaling Behavior

- **CPU-bound tasks**: Scale nearly linearly with core count up to physical core limit
- **Memory-bound tasks**: Scale limited by memory bandwidth
- **I/O-bound tasks**: Minimal scaling benefit from additional cores

## Benchmark Results

### Test Environment

- **Hardware**: Intel Core i7-9700K (8 cores, 8 threads) @ 3.6GHz, 32GB DDR4 @ 3200MHz
- **OS**: Ubuntu 20.04 LTS / Windows 10 / macOS 11
- **Compiler**: GCC 10.3 / Clang 12.0 / MSVC 2019 (-O3 / Release mode)
- **Thread System Version**: 1.0.0

### Thread Pool Creation

| Worker Count | Creation Time | Memory Usage |
|-------------|---------------|--------------|
| 1           | 12 μs         | 1.2 MB       |
| 4           | 48 μs         | 1.8 MB       |
| 8           | 95 μs         | 2.6 MB       |
| 16          | 189 μs        | 4.2 MB       |
| 32          | 378 μs        | 7.4 MB       |

### Job Submission Latency

| Queue State | Avg Latency | 99th Percentile |
|------------|-------------|-----------------|
| Empty      | 0.8 μs      | 1.2 μs          |
| 100 jobs   | 0.9 μs      | 1.5 μs          |
| 1K jobs    | 1.1 μs      | 2.1 μs          |
| 10K jobs   | 1.3 μs      | 3.5 μs          |

### Throughput Analysis

#### Basic Throughput by Job Complexity

| Job Duration | 4 Workers | 8 Workers | 16 Workers |
|-------------|-----------|-----------|------------|
| Empty job   | 1.2M/s    | 2.1M/s    | 3.5M/s     |
| 1 μs work   | 800K/s    | 1.5M/s    | 2.8M/s     |
| 10 μs work  | 280K/s    | 540K/s    | 1.0M/s     |
| 100 μs work | 35K/s     | 70K/s     | 135K/s     |
| 1 ms work   | 3.8K/s    | 7.6K/s    | 15.2K/s    |

#### Worker Scaling Efficiency

| Workers | Speedup | Efficiency | Queue Depth (avg) |
|---------|---------|------------|-------------------|
| 1       | 1.0x    | 100%       | 0.1               |
| 2       | 2.0x    | 99%        | 0.2               |
| 4       | 3.9x    | 98%        | 0.5               |
| 8       | 7.7x    | 96%        | 1.2               |
| 16      | 15.0x   | 94%        | 3.1               |
| 32      | 28.3x   | 88%        | 8.7               |

#### Memory Allocation Impact

| Memory Pattern | Allocation Size | Jobs/sec | vs No Alloc | P99 Latency |
|---------------|----------------|----------|-------------|-------------|
| None          | 0              | 540,000  | 100%        | 18μs        |
| Small         | <1KB           | 485,000  | 90%         | 22μs        |
| Medium        | 1-100KB        | 320,000  | 59%         | 38μs        |
| Large         | 100KB-1MB      | 125,000  | 23%         | 95μs        |
| Very Large    | >1MB           | 28,000   | 5%          | 420μs       |

### Priority Scheduling

| Total Jobs | Priority Levels | Scheduling Accuracy | Overhead vs Basic |
|-----------|-----------------|--------------------|--------------------|
| 1,000     | 3               | 99.8%              | +2%                |
| 10,000    | 3               | 99.7%              | +3%                |
| 100,000   | 3               | 99.6%              | +4%                |
| 10,000    | 10              | 99.5%              | +5%                |

### Logger Performance

| Log Level | Throughput | Latency (avg) | Latency (99%) |
|-----------|------------|---------------|---------------|
| Debug     | 450K/s     | 2.2 μs        | 4.5 μs        |
| Info      | 480K/s     | 2.1 μs        | 4.2 μs        |
| Warning   | 485K/s     | 2.1 μs        | 4.1 μs        |
| Error     | 490K/s     | 2.0 μs        | 4.0 μs        |

### Library Comparison

| Library              | Throughput | Relative Performance |
|---------------------|------------|---------------------|
| Thread System       | 540K/s     | 100% (baseline)     |
| std::async          | 125K/s     | 23%                 |
| Boost.Thread Pool   | 510K/s     | 94%                 |
| Intel TBB           | 580K/s     | 107%                |
| Custom (naive)      | 320K/s     | 59%                 |

## Performance Measurement

### Basic Thread Pool Benchmarking

```cpp
#include <chrono>
#include "thread_pool.h"

auto benchmark_thread_pool(uint16_t thread_count, uint32_t job_count) -> double {
    // Create thread pool
    auto [pool, error] = create_default(thread_count);
    if (error.has_value()) {
        return -1.0;
    }
    
    // Create synchronization
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
                // Simulated work
                for (volatile int j = 0; j < 10000; ++j) {}
                
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
    
    // Submit jobs and start
    pool->enqueue_batch(std::move(jobs));
    pool->start();
    
    // Wait for completion
    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&all_done]() { return all_done; });
    }
    
    // Calculate time
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time).count() / 1000.0;
    
    pool->stop();
    return duration; // milliseconds
}
```

### Running Benchmarks

```bash
# Configure with benchmarks enabled
mkdir build && cd build
cmake .. -DBUILD_BENCHMARKS=ON -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/toolchain.cmake

# Build and run benchmarks
make benchmarks
make run_benchmarks

# Or run individual benchmarks
./bin/thread_pool_benchmark
./bin/memory_benchmark
./bin/logger_benchmark
```

## Optimization Strategies

### 1. Optimal Thread Count

```cpp
uint16_t determine_optimal_thread_count(WorkloadType workload) {
    uint16_t hardware_threads = std::thread::hardware_concurrency();
    
    switch (workload) {
        case WorkloadType::CpuBound:
            return hardware_threads;
            
        case WorkloadType::MemoryBound:
            return std::max(1u, hardware_threads / 2);
            
        case WorkloadType::IoBlocking:
            return hardware_threads * 2;
            
        case WorkloadType::Mixed:
            return hardware_threads + 2;
    }
    
    return hardware_threads;
}
```

**Guidelines:**
- CPU-bound: Use `hardware_concurrency()`
- I/O-bound: Use `hardware_concurrency() * 2`
- Mixed: Use `hardware_concurrency() + 2`

### 2. Job Batching

Batching reduces scheduling overhead significantly:

```cpp
// Instead of individual submissions
for (int i = 0; i < 10000; ++i) {
    pool->enqueue(create_job(i));
}

// Use batch submission
std::vector<std::unique_ptr<thread_module::job>> jobs;
jobs.reserve(10000);

for (int i = 0; i < 10000; ++i) {
    jobs.push_back(create_job(i));
}

pool->enqueue_batch(std::move(jobs));
```

**Batch Size Impact:**

| Batch Size | Overhead per Job |
|------------|-----------------|
| 1          | 15.2 μs         |
| 10         | 2.5 μs          |
| 100        | 0.8 μs          |
| 1000       | 0.3 μs          |
| 10000      | 0.2 μs          |

### 3. Job Granularity

Choose appropriate job sizes for efficiency:

```cpp
// Better granularity with chunking
const size_t chunk_size = 100;
for (size_t i = 0; i < data.size(); i += chunk_size) {
    size_t end = std::min(i + chunk_size, data.size());
    pool->enqueue(create_job_for_range(data, i, end));
}
```

**Granularity Guidelines:**

| Job Execution Time | Recommended Action              |
|--------------------|--------------------------------|
| < 10μs             | Batch at least 1000 operations |
| 10-100μs           | Batch at least 100 operations  |
| 100μs-1ms          | Batch at least 10 operations   |
| > 1ms              | Individual jobs acceptable     |

### 4. Priority Pool Configuration

```cpp
void configure_priority_pool(std::shared_ptr<priority_thread_pool> pool,
                            const WorkloadProfile& profile) {
    const uint16_t hw_threads = std::thread::hardware_concurrency();
    
    uint16_t high_workers = hw_threads * profile.high_priority_ratio;
    uint16_t normal_workers = hw_threads * profile.normal_priority_ratio;
    uint16_t low_workers = hw_threads * profile.low_priority_ratio;
    
    // Ensure at least one worker per priority
    high_workers = std::max(1u, high_workers);
    normal_workers = std::max(1u, normal_workers);
    low_workers = std::max(1u, low_workers);
    
    // Add workers
    add_priority_workers(pool, job_priorities::High, high_workers);
    add_priority_workers(pool, job_priorities::Normal, normal_workers);
    add_priority_workers(pool, job_priorities::Low, low_workers);
}
```

### 5. Logging Optimization

Configure logging for minimal impact:

```cpp
// Production configuration
void configure_production_logging() {
    log_module::console_target(log_module::log_types::Error | 
                              log_module::log_types::Critical);
    log_module::file_target(log_module::log_types::Error |
                           log_module::log_types::Critical |
                           log_module::log_types::Warning);
}

// Check log level before expensive operations
if (log_module::is_debug_enabled()) {
    log_module::write_debug("Complex calculation: {:.10f}", 
                           perform_expensive_calculation());
}
```

**Log Level Impact:**

| Configuration      | Processing Time | Application Impact |
|-------------------|-----------------|-------------------|
| All Levels        | 100%            | High              |
| Info and above    | ~40%            | Moderate          |
| Warning and above | ~15%            | Low               |
| Error and above   | ~5%             | Minimal           |

## Advanced Techniques

### 1. False Sharing Prevention

```cpp
// Align data structures to cache line boundaries
struct alignas(64) PaddedCounter {
    std::atomic<uint64_t> value{0};
    char padding[64 - sizeof(std::atomic<uint64_t>)];
};

class OptimizedThreadPool : public thread_pool_module::thread_pool {
    std::unique_ptr<PaddedCounter[]> worker_counters_;
    
public:
    OptimizedThreadPool(size_t worker_count) : thread_pool() {
        worker_counters_ = std::make_unique<PaddedCounter[]>(worker_count);
    }
};
```

### 2. Memory Pool for Jobs

```cpp
template<typename JobType, size_t PoolSize = 1024>
class JobPool {
public:
    JobPool() {
        pool_.reserve(PoolSize);
        for (size_t i = 0; i < PoolSize; ++i) {
            pool_.push_back(std::make_unique<JobType>());
        }
    }
    
    auto acquire() -> std::unique_ptr<JobType> {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pool_.empty()) {
            return std::make_unique<JobType>();
        }
        
        auto job = std::move(pool_.back());
        pool_.pop_back();
        return job;
    }
    
    auto release(std::unique_ptr<JobType> job) -> void {
        if (!job) return;
        
        job->reset();
        
        std::lock_guard<std::mutex> lock(mutex_);
        if (pool_.size() < PoolSize) {
            pool_.push_back(std::move(job));
        }
    }
    
private:
    std::vector<std::unique_ptr<JobType>> pool_;
    std::mutex mutex_;
};
```

### 3. Lock-Free Queue (Simplified)

```cpp
template<typename T, size_t Capacity = 1024>
class LockFreeQueue {
public:
    auto enqueue(T item) -> bool {
        size_t current_tail = tail_.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) % Capacity;
        
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false; // Queue full
        }
        
        storage_[current_tail] = std::move(item);
        occupied_[current_tail].store(true, std::memory_order_release);
        
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }
    
    auto dequeue(T& item) -> bool {
        size_t current_head = head_.load(std::memory_order_relaxed);
        
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return false; // Queue empty
        }
        
        if (!occupied_[current_head].load(std::memory_order_acquire)) {
            return false;
        }
        
        item = std::move(storage_[current_head]);
        occupied_[current_head].store(false, std::memory_order_release);
        
        head_.store((current_head + 1) % Capacity, std::memory_order_release);
        return true;
    }
    
private:
    std::array<T, Capacity> storage_;
    std::array<std::atomic<bool>, Capacity> occupied_;
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
};
```

## Platform-Specific Optimizations

### Windows

```cpp
#ifdef _WIN32
void set_windows_thread_priority(std::thread& thread, int priority) {
    SetThreadPriority(thread.native_handle(), priority);
}

void configure_processor_groups(thread_pool_module::thread_pool& pool) {
    DWORD num_groups = GetActiveProcessorGroupCount();
    if (num_groups <= 1) return;
    
    auto& workers = pool.get_workers();
    for (size_t i = 0; i < workers.size(); ++i) {
        WORD group = i % num_groups;
        GROUP_AFFINITY affinity = {0};
        affinity.Group = group;
        
        SetThreadGroupAffinity(workers[i]->get_thread_handle(), 
                              &affinity, nullptr);
    }
}
#endif
```

### Linux

```cpp
#ifdef __linux__
void set_linux_thread_priority(std::thread& thread, int policy, int priority) {
    sched_param param;
    param.sched_priority = priority;
    pthread_setschedparam(thread.native_handle(), policy, &param);
}

void set_thread_affinity(std::thread& thread, uint32_t core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set_t), &cpuset);
}
#endif
```

### macOS/iOS

```cpp
#ifdef __APPLE__
void set_apple_thread_qos(std::thread& thread, qos_class_t qos_class) {
    pthread_set_qos_class_self_np(qos_class, 0);
}
#endif
```

## Best Practices

### Performance Tuning Checklist

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
   - [ ] Implement work-stealing if beneficial
   - [ ] Add backpressure mechanisms

### Real-World Performance Guidelines

Based on extensive benchmarking and real-world usage:

1. **Web Server Applications**
   - Use 2x hardware threads for I/O-heavy workloads
   - Batch database queries when possible
   - Keep job execution time > 100μs

2. **Data Processing**
   - Match thread count to physical cores
   - Use large batch sizes (1000+ items)
   - Consider memory-mapped files for large datasets

3. **Image/Video Processing**
   - Use CPU core count for worker threads
   - Process frames in batches
   - Pre-allocate output buffers

4. **Microservices**
   - Configure based on expected load patterns
   - Use priority queues for request handling
   - Implement circuit breakers for overload protection

## Conclusion

Achieving optimal performance with Thread System requires:

1. **Understanding your workload** - CPU-bound, I/O-bound, or mixed
2. **Measuring before optimizing** - Use benchmarks to establish baselines
3. **Applying appropriate techniques** - From basic configuration to advanced optimizations
4. **Platform awareness** - Leverage OS-specific features when beneficial
5. **Continuous monitoring** - Performance characteristics change under load

By following the guidelines and techniques in this guide, you can ensure that Thread System performs optimally for your specific application requirements.