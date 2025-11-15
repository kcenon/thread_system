# Thread System Integration Guide

## Overview

This directory contains integration guides for using thread_system with other KCENON systems.

## Integration Guides

- [With Common System](with-common-system.md) - IExecutor interface implementation
- [With Logger System](with-logger.md) - Thread-safe logging
- [With Monitoring System](with-monitoring.md) - Thread pool metrics
- [With Network System](with-network-system.md) - Async I/O operations
- [With Database System](with-database-system.md) - Connection pool management

## Quick Start

### Basic Thread Pool

```cpp
#include "thread_system/ThreadPool.h"

int main() {
    thread_system::ThreadPool pool(4);  // 4 worker threads

    // Submit task
    auto future = pool.enqueue([]() {
        return compute_result();
    });

    // Wait for result
    auto result = future.get();
}
```

### Work Stealing Thread Pool

```cpp
auto pool = thread_system::WorkStealingThreadPool::create({
    .num_threads = std::thread::hardware_concurrency(),
    .enable_work_stealing = true
});

for (int i = 0; i < 1000; ++i) {
    pool->submit([i]() {
        process_item(i);
    });
}
```

## Integration Patterns

### Async Operations

```cpp
class AsyncProcessor {
public:
    AsyncProcessor(std::shared_ptr<IExecutor> executor)
        : executor_(std::move(executor)) {}

    Future<Result> processAsync(const Data& data) {
        return executor_->submit([data]() {
            return process(data);
        });
    }

private:
    std::shared_ptr<IExecutor> executor_;
};
```

### Task Queues

```cpp
// Lock-free task queue
auto queue = thread_system::LockFreeQueue<Task>::create();

// Producer
queue->push(task);

// Consumer
auto task = queue->pop();
if (task) {
    task->execute();
}
```

## Common Use Cases

### 1. Parallel Data Processing

```cpp
std::vector<Data> items = load_data();

thread_system::parallel_for_each(items.begin(), items.end(),
    [](auto& item) {
        process(item);
    },
    pool
);
```

### 2. Task Dependencies

```cpp
auto task1 = pool->submit([]() { return step1(); });
auto task2 = pool->submit([task1]() {
    auto result1 = task1.get();
    return step2(result1);
});

auto final_result = task2.get();
```

### 3. Periodic Tasks

```cpp
auto scheduler = thread_system::Scheduler::create();

scheduler->schedule_periodic(
    std::chrono::seconds(60),
    []() {
        cleanup_old_data();
    }
);
```

## Best Practices

- Size thread pools based on workload (CPU-bound vs I/O-bound)
- Use futures for async results
- Avoid blocking operations in thread pool tasks
- Monitor thread pool utilization
- Use lock-free structures when possible

## Additional Resources

- [Thread System API Reference](../API_REFERENCE.md)
- [Ecosystem Integration Guide](../../../ECOSYSTEM_INTEGRATION.md)
