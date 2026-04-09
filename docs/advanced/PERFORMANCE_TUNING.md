---
doc_id: "THR-PERF-006b"
doc_title: "Thread System Performance Tuning"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "thread_system"
category: "PERF"
---

# Thread System Performance Tuning

> **SSOT**: This document is the single source of truth for **Thread System Performance Tuning** (optimization strategies, platform-specific guidance, best practices).

> **Language:** **English** | [한국어](PERFORMANCE.kr.md)

> **See also**: [Performance Benchmarks](PERFORMANCE_BENCHMARKS.md) for benchmark results, scalability analysis, and comparison data.

This guide covers optimization strategies, platform-specific tuning, and best practices for achieving optimal performance with the Thread System framework.

## Table of Contents

1. [Optimization Strategies](#optimization-strategies)
2. [Platform-Specific Optimizations](#platform-specific-optimizations)
3. [Best Practices](#best-practices)
4. [Performance Recommendations Summary (2025)](#performance-recommendations-summary-2025)
5. [Conclusion](#conclusion)

## Optimization Strategies

### 1. Optimal Thread Count Selection

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
            return static_cast<uint16_t>(hardware_threads * 1.5);
            
        case WorkloadType::RealTime:
            return hardware_threads - 1; // Reserve one core for OS
    }
    
    return hardware_threads;
}
```

### 2. Job Batching for Performance

Batching reduces scheduling overhead significantly:

| Batch Size | Overhead per Job | Recommended Use Case |
|------------|-----------------|---------------------|
| 1          | 77 ns           | Real-time tasks     |
| 10         | 25 ns           | Interactive tasks   |
| 100        | 8 ns            | Background processing|
| 1000       | 3 ns            | Batch processing    |
| 10000      | 2 ns            | Bulk operations     |

```cpp
// Efficient job batching
std::vector<std::unique_ptr<thread_module::job>> jobs;
jobs.reserve(batch_size);

for (int i = 0; i < batch_size; ++i) {
    jobs.push_back(create_job(data[i]));
}

pool->enqueue_batch(std::move(jobs));
```

### 3. Job Granularity Optimization

| Job Execution Time | Recommended Action | Reason |
|--------------------|-------------------|--------|
| < 10μs             | Batch 1000+ operations | Overhead dominates |
| 10-100μs           | Batch 100 operations | Balance overhead/parallelism |
| 100μs-1ms          | Batch 10 operations | Minimize coordination |
| 1ms-10ms           | Individual jobs | Good granularity |
| > 10ms             | Consider subdivision | Improve responsiveness |

### 4. Type Pool Configuration

```cpp
void configure_type_pool(std::shared_ptr<typed_thread_pool> pool,
                            const WorkloadProfile& profile) {
    const uint16_t hw_threads = std::thread::hardware_concurrency();
    
    // Allocate workers based on type distribution
    uint16_t high_workers = static_cast<uint16_t>(hw_threads * profile.high_type_ratio);
    uint16_t normal_workers = static_cast<uint16_t>(hw_threads * profile.normal_type_ratio);
    uint16_t low_workers = static_cast<uint16_t>(hw_threads * profile.low_type_ratio);
    
    // Ensure minimum coverage
    high_workers = std::max(1u, high_workers);
    normal_workers = std::max(1u, normal_workers);
    low_workers = std::max(1u, low_workers);
    
    // Add specialized workers
    add_type_workers(pool, job_types::High, high_workers);
    add_type_workers(pool, job_types::Normal, normal_workers);
    add_type_workers(pool, job_types::Low, low_workers);
}
```

### 4b. Adaptive Queue Configuration

```cpp
#include "typed_thread_pool/pool/typed_thread_pool.h"

auto create_optimal_pool(const std::string& name, 
                        size_t expected_concurrency,
                        bool priority_sensitive) -> std::shared_ptr<typed_thread_pool_t<job_types>> {
    
    // Create typed thread pool with adaptive queue strategy
    auto pool = std::make_shared<typed_thread_pool_t<job_types>>(name);
    
    // Configure adaptive queue strategy based on expected usage
    if (expected_concurrency > 4 || priority_sensitive) {
        // High contention - adaptive queue will automatically optimize
        pool->set_queue_strategy(queue_strategy::ADAPTIVE);
    }
    
    // Add specialized workers for each priority
    auto realtime_worker = std::make_unique<typed_thread_worker_t<job_types>>();
    realtime_worker->set_responsibilities({job_types::RealTime});
    pool->add_worker(std::move(realtime_worker));
    
    auto batch_worker = std::make_unique<typed_thread_worker_t<job_types>>();
    batch_worker->set_responsibilities({job_types::Batch});
    pool->add_worker(std::move(batch_worker));
    
    auto background_worker = std::make_unique<typed_thread_worker_t<job_types>>();
    background_worker->set_responsibilities({job_types::Background});
    pool->add_worker(std::move(background_worker));
    
    // Universal worker for load balancing
    auto universal_worker = std::make_unique<typed_thread_worker_t<job_types>>();
    universal_worker->set_responsibilities({job_types::RealTime, job_types::Batch, job_types::Background});
    pool->add_worker(std::move(universal_worker));
        
    return pool;
}

// Usage examples
auto high_concurrency_pool = create_optimal_pool(
    "HighConcurrency", 8, true);
    
auto simple_pool = create_optimal_pool(
    "Simple", 2, false);
```

### 5. Memory Optimization

#### Cache-Line Alignment
```cpp
// Prevent false sharing
struct alignas(64) WorkerData {
    std::atomic<uint64_t> processed_jobs{0};
    std::atomic<uint64_t> execution_time{0};
    char padding[64 - 2 * sizeof(std::atomic<uint64_t>)];
};
```

#### Memory Pool Implementation (Suggested Optimization)

*Note: This is a suggested optimization pattern for users who need memory pool functionality. Thread System does not currently include built-in memory pools.*

```cpp
template<typename JobType, size_t PoolSize = 1024>
class JobPool {
public:
    auto acquire() -> std::unique_ptr<JobType> {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!pool_.empty()) {
            auto job = std::move(pool_.back());
            pool_.pop_back();
            return job;
        }
        return std::make_unique<JobType>();
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

## Platform-Specific Optimizations

### macOS/ARM64 Optimizations

```cpp
#ifdef __APPLE__
// Leverage performance cores on Apple Silicon
void configure_for_apple_silicon(thread_pool_module::thread_pool& pool) {
    size_t performance_cores = 4; // M1 has 4 performance cores
    size_t efficiency_cores = 4;  // M1 has 4 efficiency cores
    
    // Prioritize performance cores for CPU-intensive work
    for (size_t i = 0; i < performance_cores; ++i) {
        auto worker = std::make_unique<thread_worker>(pool.get_job_queue());
        pthread_set_qos_class_self_np(QOS_CLASS_USER_INITIATED, 0);
        pool.enqueue(std::move(worker));
    }
}
#endif
```

### Linux Optimizations

```cpp
#ifdef __linux__
void set_thread_affinity(std::thread& thread, uint32_t core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set_t), &cpuset);
}

void configure_numa_awareness(thread_pool_module::thread_pool& pool) {
    // Distribute workers across NUMA nodes
    int numa_nodes = numa_max_node() + 1;
    auto workers = pool.get_workers();
    
    for (size_t i = 0; i < workers.size(); ++i) {
        int node = i % numa_nodes;
        numa_run_on_node(node);
        set_thread_affinity(workers[i]->get_thread(), i);
    }
}
#endif
```

### Windows Optimizations

```cpp
#ifdef _WIN32
void configure_windows_type(thread_pool_module::thread_pool& pool) {
    auto workers = pool.get_workers();
    
    for (auto& worker : workers) {
        SetThreadType(worker->get_thread_handle(), THREAD_PRIORITY_ABOVE_NORMAL);
    }
}

void configure_processor_groups(thread_pool_module::thread_pool& pool) {
    DWORD num_groups = GetActiveProcessorGroupCount();
    if (num_groups <= 1) return;
    
    auto workers = pool.get_workers();
    for (size_t i = 0; i < workers.size(); ++i) {
        WORD group = i % num_groups;
        GROUP_AFFINITY affinity = {0};
        affinity.Group = group;
        affinity.Mask = 1ULL << (i / num_groups);
        
        SetThreadGroupAffinity(workers[i]->get_thread_handle(), &affinity, nullptr);
    }
}
#endif
```

## Best Practices

### Performance Tuning Checklist

#### Measurement and Analysis
- [x] Establish performance baseline with benchmarks
- [x] Profile actual workload patterns
- [x] Measure thread utilization and queue depths
- [x] Identify bottlenecks through systematic analysis

#### Thread Pool Configuration
- [x] Set optimal thread count based on workload type
- [x] Configure type-specific workers appropriately
- [x] Consider thread affinity for critical applications
- [x] Adjust for platform-specific characteristics

#### Job Design
- [x] Batch job submission where possible
- [x] Ensure appropriate job granularity (>100μs recommended)
- [x] Balance workload across job types
- [x] Minimize memory allocation in job execution

#### Memory Considerations
- [x] Prevent false sharing with proper alignment
- [x] Consider implementing memory pools for frequently allocated objects (not built-in)
- [x] Consider thread-local storage for worker data
- [x] Monitor memory growth under sustained load

#### Advanced Techniques
- [x] Implement backpressure mechanisms for overload protection
- [x] Consider work-stealing for load balancing
- [x] Use lock-free data structures where appropriate
- [x] Implement circuit breakers for fault tolerance

### Real-World Performance Guidelines

#### Web Server Applications
- **Thread Count**: 2x hardware threads for I/O-heavy workloads
- **Job Granularity**: Keep request processing > 100μs
- **Type Usage**: High for interactive requests, Normal for API calls, Low for analytics
- **Memory**: Use connection pools and request object pools

#### Data Processing Pipelines
- **Thread Count**: Match physical core count
- **Batch Size**: Use large batches (1000+ items)
- **Memory**: Pre-allocate buffers, use memory-mapped files for large datasets
- **Optimization**: Pipeline stages with different thread pools

#### Real-Time Systems
- **Thread Count**: Reserve 1 core for OS, use remaining cores
- **Latency**: Target <10μs scheduling latency
- **Type**: Strict type separation with dedicated workers
- **Memory**: Pre-allocate all memory, avoid runtime allocation

#### Scientific Computing
- **Thread Count**: Use all available cores
- **Job Granularity**: Balance computation size with coordination overhead
- **Memory**: Consider NUMA topology and memory bandwidth
- **Optimization**: Use CPU-specific optimizations (SIMD, cache optimization)

### Monitoring and Diagnostics

#### Key Performance Indicators

| Metric | Target Range | Warning Threshold | Critical Threshold |
|--------|-------------|------------------|-------------------|
| Jobs/sec | >100K | <50K | <10K |
| Queue Depth | 0-10 | >50 | >200 |
| CPU Utilization | 80-95% | >98% | 100% sustained |
| Memory Growth | <1% per hour | >5% per hour | >10% per hour |
| Error Rate | <0.1% | >1% | >5% |

#### Diagnostic Tools

```cpp
class PerformanceMonitor {
public:
    struct Metrics {
        std::atomic<uint64_t> jobs_submitted{0};
        std::atomic<uint64_t> jobs_completed{0};
        std::atomic<uint64_t> total_execution_time{0};
        std::atomic<uint32_t> current_queue_depth{0};
        std::atomic<uint32_t> peak_queue_depth{0};
    };
    
    auto get_throughput() const -> double {
        auto duration = std::chrono::steady_clock::now() - start_time_;
        auto seconds = std::chrono::duration<double>(duration).count();
        return metrics_.jobs_completed.load() / seconds;
    }
    
    auto get_average_latency() const -> double {
        uint64_t completed = metrics_.jobs_completed.load();
        if (completed == 0) return 0.0;
        return static_cast<double>(metrics_.total_execution_time.load()) / completed;
    }
    
private:
    Metrics metrics_;
    std::chrono::steady_clock::time_point start_time_{std::chrono::steady_clock::now()};
};
```

## Future Performance Improvements

### Planned Optimizations

1. **Type Thread Pool Optimizations**:
   - Work stealing between type queues for better load balancing
   - Batch dequeue operations for reduced overhead
   - Type-aware scheduling policies

2. **Memory Pool Integration**:
   - Built-in memory pools for job objects
   - Reduce allocation overhead by 60-80%
   - Thread-local pools for cache efficiency

3. **Work Stealing for Type Pools**:
   - Allow idle workers to steal from other type queues
   - Better CPU utilization under uneven load
   - Configurable stealing policies

## Performance Recommendations Summary (2025)

### Quick Configuration Guide

#### 1. **For General Applications**
```cpp
// Use standard thread pool with adaptive queues
auto pool = std::make_shared<thread_pool>("MyPool");

// Add workers (hardware_concurrency for CPU-bound)
for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
    pool->enqueue(std::make_unique<thread_worker>());
}
pool->start();
```

#### 2. **For Priority-Sensitive Applications**
```cpp
// Use typed thread pool with adaptive queues
auto pool = std::make_shared<typed_thread_pool_t<job_types>>("PriorityPool");

// Add specialized workers
for (auto priority : {job_types::RealTime, job_types::Batch, job_types::Background}) {
    auto worker = std::make_unique<typed_thread_worker_t<job_types>>();
    worker->set_responsibilities({priority});
    pool->enqueue(std::move(worker));
}

// Add universal workers for load balancing
for (int i = 0; i < 2; ++i) {
    auto worker = std::make_unique<typed_thread_worker_t<job_types>>();
    worker->set_responsibilities({job_types::RealTime, job_types::Batch, job_types::Background});
    pool->enqueue(std::move(worker));
}
pool->start();
```

#### 3. **For High-Concurrency Scenarios**
```cpp
// Standard pool with batch processing
auto pool = std::make_shared<thread_pool>("HighConcurrency");

// Configure workers for batch processing
std::vector<std::unique_ptr<thread_worker>> workers;
for (int i = 0; i < std::thread::hardware_concurrency() * 2; ++i) {
    auto worker = std::make_unique<thread_worker>();
    worker->set_batch_processing(true, 32); // Process up to 32 jobs at once
    workers.push_back(std::move(worker));
}
pool->enqueue_batch(std::move(workers));
pool->start();
```

### Performance Tuning Quick Reference

| Scenario | Configuration | Expected Performance |
|----------|---------------|---------------------|
| **CPU-Bound Tasks** | Workers = hardware_concurrency() | 96% efficiency at 8 cores |
| **I/O-Bound Tasks** | Workers = hardware_concurrency() × 2 | Good overlap of I/O waits |
| **Mixed Workload** | Workers = hardware_concurrency() × 1.5 | Balanced performance |
| **Low Latency** | Standard pool, single jobs | ~77ns submission latency |
| **High Throughput** | Batch processing enabled | Up to 13M jobs/s theoretical |
| **Priority Scheduling** | Typed pool with 3-4 workers per type | 99.6% type accuracy |

### Common Pitfalls to Avoid

1. **Over-Threading**: Don't create more workers than 2× hardware threads
2. **Small Jobs**: Batch jobs < 10μs for better efficiency
3. **Memory Allocation**: Pre-allocate job objects when possible
4. **Queue Depth**: Monitor queue depth; > 1000 indicates backpressure needed
5. **Type Proliferation**: Keep priority types to 3-5 for optimal performance


## Conclusion

The Thread System framework provides exceptional performance characteristics with the simplified adaptive architecture:

1. **High Throughput**: 
   - Standard pool: 1.16M jobs/second (proven in production)
   - Adaptive queues: Automatic optimization for all scenarios
   - Typed pools: 1.24M jobs/second with priority specialization
2. **Low Latency**: 
   - Standard pool: 77ns scheduling overhead
   - Adaptive queues: 96-580ns with automatic strategy selection
   - Consistent performance across varying workloads
3. **Excellent Scalability**: 
   - Standard pool: 96% efficiency at 8 cores
   - Adaptive queues: Maintain performance under any contention level
   - Up to **3.46x improvement** under high contention
4. **Memory Efficiency**: 
   - Standard pool: <1MB baseline memory usage
   - Dynamic allocation based on actual usage
   - Reduced codebase by ~8,700+ lines without performance loss
5. **Platform Optimization**: 
   - Consistent performance across Windows, Linux, and macOS
   - Platform-specific optimizations where beneficial
6. **Simplified Architecture**:
   - Removed duplicate code and unused features
   - Maintained all performance capabilities
   - Cleaner, more maintainable codebase

### Key Success Factors

1. **Simplified Usage**:
   - Standard pools with adaptive queues work optimally out-of-the-box
   - No manual configuration required
   - Automatic optimization for all scenarios
2. **Profile Your Workload**: 
   - Use built-in benchmarks for baseline measurements
   - Monitor actual performance characteristics
   - Let adaptive queues handle optimization
3. **Clean Architecture Benefits**:
   - Reduced code complexity improves maintainability
   - Removed ~8,700+ lines (logger, monitoring, unused utilities)
   - Modular design with interface-based architecture
   - Performance maintained through smart design
4. **Monitor Performance**:
   - Track job throughput and latency
   - Monitor worker utilization
   - Observe adaptive queue behavior
5. **Best Practices**:
   - Use typed pools for priority-based workloads
   - Leverage batch operations for small jobs
   - Trust automatic optimization

By following the guidelines and techniques in this comprehensive performance guide, you can achieve optimal performance for your specific application requirements. The simplified adaptive architecture provides powerful optimization capabilities while maintaining the simplicity and reliability of the Thread System framework.
---

*Last Updated: 2025-10-20*
