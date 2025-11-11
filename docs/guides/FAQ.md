# Thread System - Frequently Asked Questions

> **Version:** 1.0
> **Last Updated:** 2025-11-11
> **Audience:** Users, Developers

This FAQ addresses common questions about the thread_system, covering thread pools, job scheduling, synchronization, and performance.

---

## Table of Contents

1. [General Questions](#general-questions)
2. [Thread Pool Basics](#thread-pool-basics)
3. [Job Scheduling](#job-scheduling)
4. [Synchronization](#synchronization)
5. [Performance](#performance)
6. [Integration](#integration)

---

## General Questions

### 1. What is the thread_system?

A high-performance thread pool and job scheduling system for C++20:
- **Thread pools** with configurable size
- **Job scheduling** with priorities
- **Work stealing** for load balancing
- **Future/Promise** support
- **Thread-safe** by default

```cpp
#include <thread/thread_pool.hpp>

auto pool = thread_pool::create(4);  // 4 worker threads
auto future = pool->submit([]() {
    return compute_result();
});
auto result = future.get();
```

---

### 2. How do I create a thread pool?

```cpp
// Fixed size pool
auto pool = thread_pool::create(4);

// Auto-sized (hardware_concurrency)
auto pool = thread_pool::create();

// With configuration
auto pool = thread_pool::create({
    .thread_count = 8,
    .queue_size = 1000,
    .enable_work_stealing = true
});
```

---

### 3. How do I submit jobs?

```cpp
// Fire-and-forget
pool->submit([]() {
    do_work();
});

// With return value
auto future = pool->submit([]() {
    return 42;
});
int result = future.get();

// With parameters
pool->submit([](int x, int y) {
    return x + y;
}, 10, 20);
```

---

### 4. How do I handle priorities?

```cpp
// High priority job
pool->submit([]() {
    critical_work();
}, priority::high);

// Normal priority (default)
pool->submit([]() {
    normal_work();
});

// Low priority
pool->submit([]() {
    background_work();
}, priority::low);
```

---

### 5. What is the performance?

**Benchmarks** (4-core CPU):
- Job submission: <100 ns
- Context switch: ~1 μs
- Throughput: 1M jobs/s

---

### 6. Is it thread-safe?

**Yes**, all operations are thread-safe.

```cpp
// Multiple threads can submit concurrently
std::vector<std::thread> threads;
for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&pool]() {
        pool->submit([]() { /* work */ });
    });
}
```

---

### 7. How do I wait for completion?

```cpp
// Wait for specific job
auto future = pool->submit([]() { return 42; });
int result = future.wait();

// Wait for all jobs
pool->wait_for_all();

// Wait with timeout
if (pool->wait_for(std::chrono::seconds(5))) {
    // Completed
} else {
    // Timeout
}
```

---

### 8. How do I shutdown a pool?

```cpp
// Graceful shutdown (finish pending jobs)
pool->shutdown();
pool->wait_for_all();

// Immediate shutdown
pool->shutdown_now();
```

---

### 9. Can I integrate with monitoring_system?

```cpp
// Automatic metrics
auto pool = thread_pool::create(4, {
    .enable_monitoring = true
});

// Metrics available:
// - thread_pool_active_threads
// - thread_pool_queued_tasks
// - thread_pool_completed_tasks
```

---

### 10. Where can I find more information?

**Documentation:**
- [Architecture](../ARCHITECTURE.md)
- [API Reference](../API_REFERENCE.md)
- [Build Guide](../BUILD_GUIDE.md)

**Support:**
- [GitHub Issues](https://github.com/kcenon/thread_system/issues)

---

**Last Updated:** 2025-11-11
