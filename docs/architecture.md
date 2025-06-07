# Thread System Architecture

This document provides detailed technical architecture information for the Thread System, focusing on internal design decisions, implementation details, and architectural patterns that complement the high-level overview in the main README.

## Component Relationships

```
                     ┌─────────────────┐
                     │   thread_base   │
                     └────────┬────────┘
                              │
                 ┌────────────┼────────────┐
                 │            │            │
        ┌────────▼────────┐   │   ┌────────▼─────────┐
        │  thread_worker  │   │   │typed_thread_worker│
        └────────┬────────┘   │   └────────┬──────────┘
                 │            │            │
        ┌────────▼────────┐   │   ┌────────▼──────────┐
        │   thread_pool   │   │   │typed_thread_pool│
        └─────────────────┘   │   └───────────────────┘
                              │
                     ┌────────▼────────┐
                     │  log_collector  │
                     └────────┬────────┘
                              │
                 ┌────────────┼────────────┐
                 │            │            │
        ┌────────▼────────┐   │   ┌────────▼────────┐
        │  console_writer │   │   │   file_writer   │
        └─────────────────┘   │   └─────────────────┘
                              │
                     ┌────────▼────────┐
                     │ callback_writer │
                     └─────────────────┘
```

## Internal Architecture Details

### Thread Base Implementation Details

The `thread_base` class employs several key design decisions:

#### Thread Model Adaptation
```cpp
#ifdef __cpp_lib_jthread
    std::jthread _thread;
#else
    std::thread _thread;
    std::atomic<bool> _stop_requested{false};
#endif
```
This conditional compilation ensures compatibility across C++ standards while leveraging modern features when available.

#### Virtual Method Pattern
The class uses protected virtual methods (`prepare()`, `process()`, `stop()`) following the Template Method pattern, allowing derived classes to customize behavior while maintaining thread lifecycle control in the base class.

#### Synchronization Strategy
- Uses `std::condition_variable` with `std::unique_lock` for efficient thread wake-up
- Implements wake intervals through `wait_for()` for periodic tasks
- Employs RAII for lock management to prevent deadlocks

### Job Queue Implementation

The `job_queue` class implements a thread-safe queue with specific optimizations:

#### Lock-Free Fast Path
```cpp
bool empty() const noexcept {
    // Atomic read without lock for common case
    return _jobs.empty();
}
```

#### Batch Operations
```cpp
void enqueue(std::vector<std::shared_ptr<job>>&& jobs) {
    std::lock_guard<std::mutex> lock(_mutex);
    _jobs.insert(_jobs.end(), 
                 std::make_move_iterator(jobs.begin()),
                 std::make_move_iterator(jobs.end()));
    _condition.notify_all();
}
```
Batch enqueue reduces lock contention by acquiring the mutex once for multiple jobs.

### Type Queue Optimization

The `typed_job_queue` uses a bucketed approach for O(1) average-case performance:

#### Internal Structure
```cpp
template<typename Type>
class typed_job_queue {
private:
    std::map<Type, std::queue<std::shared_ptr<typed_job<Type>>>> _type_buckets;
    mutable std::mutex _mutex;
};
```

#### Dequeue Strategy
Jobs are dequeued in type order (highest first), with FIFO ordering within each type level. This guarantees fairness while maintaining type semantics.

### Logger Architecture Internals

#### Asynchronous Processing Pipeline
1. **Producer threads**: Call `logger::log()` which creates `message_job` instances
2. **Log collector thread**: Dequeues messages and dispatches to writers
3. **Writer threads**: Process messages independently (console, file, callback)

#### Memory Efficiency
- Uses move semantics throughout to minimize copying
- Pre-allocates string buffers for formatting
- Implements circular buffer for high-frequency logging scenarios

### Type Worker Specialization

#### Responsibility Lists
Each `typed_thread_worker` maintains a list of types it can handle:
```cpp
std::vector<Type> _responsibilities;
```

This allows fine-grained control over worker specialization:
- Dedicated high-type workers for critical tasks
- Multi-type workers for load balancing
- Type-exclusive workers for isolation

## Design Patterns and Implementation Rationale

### Template Method Pattern in `thread_base`

The base class defines the algorithm skeleton while allowing subclasses to override specific steps:

```cpp
class thread_base {
protected:
    virtual void prepare() { }  // Optional setup
    virtual void process() = 0; // Required implementation
    virtual void stop() { }     // Optional cleanup
    
private:
    void run() {
        prepare();
        while (!stop_requested()) {
            process();
        }
        stop();
    }
};
```

**Rationale**: This pattern ensures consistent thread lifecycle management while providing flexibility for specialized behavior.

### Command Pattern for Jobs

Jobs encapsulate requests as objects, allowing parameterization and queuing:

```cpp
class job {
public:
    virtual ~job() = default;
    virtual void execute() = 0;
};
```

**Benefits**:
- Decouples job producers from consumers
- Enables job persistence and replay
- Facilitates undo/redo operations
- Supports job serialization for distributed systems

### Singleton with Thread Safety

The logger implements a thread-safe singleton using Meyer's pattern:

```cpp
logger& logger::instance() {
    static logger instance;
    return instance;
}
```

**Thread Safety Guarantees**:
- C++11 guarantees thread-safe static initialization
- No explicit locking required
- Lazy initialization on first use

## Memory Management Strategy

### Smart Pointer Usage Patterns

The system uses specific smart pointer patterns for different scenarios:

#### Shared Ownership for Jobs
```cpp
std::shared_ptr<job> // Jobs may be referenced by multiple queues
```
**Rationale**: Jobs might be moved between queues or referenced by multiple components during their lifecycle.

#### Unique Ownership for Workers
```cpp
std::vector<std::unique_ptr<thread_worker>> _workers;
```
**Rationale**: Thread pool has exclusive ownership of workers, preventing accidental sharing.

#### Weak References for Callbacks
```cpp
std::weak_ptr<callback_writer> // Prevents circular dependencies
```
**Rationale**: Callback systems can create cycles; weak_ptr breaks potential circular references.

### Memory Pool Considerations

For high-frequency job creation scenarios, consider implementing object pools:

```cpp
template<typename T>
class object_pool {
    std::queue<std::unique_ptr<T>> _pool;
    std::mutex _mutex;
    
public:
    std::shared_ptr<T> acquire() {
        std::lock_guard lock(_mutex);
        if (!_pool.empty()) {
            auto obj = std::move(_pool.front());
            _pool.pop();
            return std::shared_ptr<T>(obj.release(), 
                [this](T* p) { release(std::unique_ptr<T>(p)); });
        }
        return std::make_shared<T>();
    }
    
    void release(std::unique_ptr<T> obj) {
        std::lock_guard lock(_mutex);
        _pool.push(std::move(obj));
    }
};
```

## Synchronization Primitives

### Lock Hierarchy

To prevent deadlocks, the system follows a strict lock ordering:

1. **Global locks** (logger singleton)
2. **Pool locks** (thread pool mutex)
3. **Queue locks** (job queue mutex)
4. **Worker locks** (individual thread state)

### Condition Variable Patterns

#### Spurious Wakeup Protection
```cpp
std::unique_lock lock(_mutex);
_condition.wait(lock, [this] { 
    return !_jobs.empty() || _stop_requested; 
});
```

#### Timed Wait with Predicate
```cpp
_condition.wait_for(lock, _wake_interval, [this] {
    return !_jobs.empty() || _stop_requested;
});
```

## Platform-Specific Optimizations

### Thread Affinity (Linux)
```cpp
#ifdef __linux__
void set_thread_affinity(std::thread& t, int core) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);
    pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &cpuset);
}
#endif
```

### High-Resolution Timing (Windows)
```cpp
#ifdef _WIN32
class high_resolution_timer {
    LARGE_INTEGER frequency;
    LARGE_INTEGER start_time;
public:
    high_resolution_timer() {
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start_time);
    }
    
    double elapsed_microseconds() {
        LARGE_INTEGER end_time;
        QueryPerformanceCounter(&end_time);
        return (end_time.QuadPart - start_time.QuadPart) * 1000000.0 / frequency.QuadPart;
    }
};
#endif
```

### Memory Allocation Strategies

#### NUMA-Aware Allocation (Future Enhancement)
```cpp
#ifdef NUMA_AVAILABLE
void* numa_alloc(size_t size, int node) {
    return numa_alloc_onnode(size, node);
}
#endif
```

## Performance Optimization Techniques

### Cache-Line Alignment

Critical data structures are aligned to prevent false sharing:

```cpp
struct alignas(64) worker_stats {
    std::atomic<uint64_t> jobs_processed{0};
    std::atomic<uint64_t> total_wait_time{0};
    std::atomic<uint64_t> total_execution_time{0};
};
```

### Lock-Free Optimizations

#### Atomic Status Checks
```cpp
class thread_pool {
    std::atomic<size_t> _active_workers{0};
    
    bool has_idle_workers() const noexcept {
        return _active_workers.load(std::memory_order_relaxed) < _workers.size();
    }
};
```

#### Double-Checked Locking
```cpp
std::shared_ptr<job> try_dequeue() {
    if (_jobs.empty()) return nullptr;  // Fast path
    
    std::lock_guard lock(_mutex);       // Slow path
    if (_jobs.empty()) return nullptr;
    
    auto job = _jobs.front();
    _jobs.pop();
    return job;
}
```

### Batch Processing Optimizations

#### Vectorized Job Submission
```cpp
void enqueue_batch(std::vector<std::shared_ptr<job>>&& jobs) {
    if (jobs.empty()) return;
    
    {
        std::lock_guard lock(_mutex);
        _jobs.reserve(_jobs.size() + jobs.size()); // Pre-allocate
        _jobs.insert(_jobs.end(), 
                     std::make_move_iterator(jobs.begin()),
                     std::make_move_iterator(jobs.end()));
    }
    
    // Wake multiple workers for batch
    _condition.notify_all();
}
```

## Testing Architecture

### Thread Safety Testing

#### Race Condition Detection
```cpp
TEST(ThreadPoolTest, ConcurrentEnqueue) {
    constexpr int num_producers = 10;
    constexpr int jobs_per_producer = 1000;
    
    thread_pool pool(4);
    std::atomic<int> counter{0};
    std::vector<std::thread> producers;
    
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&pool, &counter]() {
            for (int j = 0; j < jobs_per_producer; ++j) {
                pool.enqueue([&counter]() { counter++; });
            }
        });
    }
    
    for (auto& t : producers) t.join();
    pool.wait_for_completion();
    
    EXPECT_EQ(counter.load(), num_producers * jobs_per_producer);
}
```

#### Stress Testing with ThreadSanitizer
```cmake
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(thread_system_tests PRIVATE -fsanitize=thread)
    target_link_options(thread_system_tests PRIVATE -fsanitize=thread)
endif()
```

### Performance Benchmarking

#### Microbenchmarks
```cpp
void BM_JobEnqueue(benchmark::State& state) {
    thread_pool pool(state.range(0));
    pool.start();
    
    for (auto _ : state) {
        auto job = std::make_shared<callback_job>([](){});
        pool.enqueue(job);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_JobEnqueue)->Range(1, 16);
```

## Advanced Configuration

### Compile-Time Configuration

```cpp
// config.hpp
#define THREAD_SYSTEM_ENABLE_METRICS 1
#define THREAD_SYSTEM_MAX_PRIORITY_LEVELS 64
#define THREAD_SYSTEM_JOB_QUEUE_INITIAL_CAPACITY 1024
```

### Runtime Configuration

```cpp
struct thread_pool_config {
    size_t initial_workers = std::thread::hardware_concurrency();
    size_t max_workers = initial_workers * 2;
    size_t job_queue_size_warning = 10000;
    std::chrono::milliseconds worker_idle_timeout{60000};
    bool enable_metrics = true;
};
```

## Security Considerations

### Job Validation
```cpp
template<typename Func>
std::shared_ptr<job> make_validated_job(Func&& func) {
    // Validate function signature
    static_assert(std::is_invocable_v<Func>, "Function must be callable");
    
    // Wrap with exception handling
    return std::make_shared<callback_job>([func = std::forward<Func>(func)]() {
        try {
            func();
        } catch (const std::exception& e) {
            logger::instance().log(log_types::Error, 
                fmt::format("Job execution failed: {}", e.what()));
        }
    });
}
```

### Resource Limits
```cpp
class resource_limited_pool : public thread_pool {
    std::atomic<size_t> _memory_usage{0};
    const size_t _max_memory;
    
public:
    bool enqueue(std::shared_ptr<job> job) override {
        size_t job_size = estimate_job_size(job);
        if (_memory_usage + job_size > _max_memory) {
            return false; // Reject job
        }
        _memory_usage += job_size;
        return thread_pool::enqueue(job);
    }
};
```