# Thread Pool API Migration Guide

This guide helps you migrate from deprecated `thread_pool` methods to their recommended replacements. All deprecated methods will be removed in v2.0.

## Deprecated Methods Overview

| Deprecated Method | Replacement | Removal Version |
|-------------------|-------------|-----------------|
| `submit_task()` | `enqueue()` with job wrapper | v2.0 |
| `get_thread_count()` | `metrics().worker_count` or `get_active_worker_count()` | v2.0 |
| `shutdown_pool()` | `stop()` | v2.0 |

## Migration Examples

### 1. submit_task() -> enqueue()

**Before (deprecated):**
```cpp
auto pool = std::make_shared<thread_pool>("my_pool");
pool->start();

// Using deprecated submit_task
bool success = pool->submit_task([]() {
    std::cout << "Task executed\n";
});
```

**After (recommended):**
```cpp
#include <kcenon/thread/jobs/callback_job.h>

auto pool = std::make_shared<thread_pool>("my_pool");
pool->start();

// Using enqueue with callback_job
auto job = std::make_unique<callback_job>([]() -> common::VoidResult {
    std::cout << "Task executed\n";
    return common::ok();
});

auto result = pool->enqueue(std::move(job));
if (result.is_error()) {
    // Handle error with detailed information
    std::cerr << "Failed: " << result.error().message() << "\n";
}
```

**Alternative - Using submit_async for futures:**
```cpp
// If you need a future for the result
auto future = pool->submit_async([]() {
    return 42;
});
int result = future.get();  // Blocks until complete
```

### 2. get_thread_count() -> metrics().worker_count or get_active_worker_count()

**Before (deprecated):**
```cpp
auto pool = std::make_shared<thread_pool>("my_pool");
pool->start();

// Using deprecated get_thread_count
std::size_t count = pool->get_thread_count();
```

**After (recommended):**
```cpp
auto pool = std::make_shared<thread_pool>("my_pool");
pool->start();

// Option 1: Using metrics() - provides comprehensive pool statistics
const auto& m = pool->metrics();
std::size_t worker_count = m.worker_count;
std::size_t active_jobs = m.active_jobs;
std::size_t completed_jobs = m.jobs_completed;

// Option 2: Using get_active_worker_count() - simple active count
std::size_t active = pool->get_active_worker_count();
```

**Metrics provides additional information:**
```cpp
const auto& m = pool->metrics();

// Available metrics
std::cout << "Workers: " << m.worker_count << "\n";
std::cout << "Active jobs: " << m.active_jobs << "\n";
std::cout << "Completed: " << m.jobs_completed << "\n";
std::cout << "Failed: " << m.jobs_failed << "\n";
std::cout << "Queue size: " << m.queue_size << "\n";
```

### 3. shutdown_pool() -> stop()

**Before (deprecated):**
```cpp
auto pool = std::make_shared<thread_pool>("my_pool");
pool->start();
// ... use pool ...

// Using deprecated shutdown_pool
bool success = pool->shutdown_pool(false);  // Wait for tasks
// or
bool success_immediate = pool->shutdown_pool(true);  // Stop immediately
```

**After (recommended):**
```cpp
auto pool = std::make_shared<thread_pool>("my_pool");
pool->start();
// ... use pool ...

// Using stop() - provides detailed error information
auto result = pool->stop(false);  // Wait for tasks to complete
if (result.is_error()) {
    std::cerr << "Shutdown failed: " << result.error().message() << "\n";
}

// For immediate stop
auto result_immediate = pool->stop(true);
```

## Error Handling Improvements

The new API provides `common::VoidResult` return types that include detailed error information:

```cpp
auto result = pool->enqueue(std::move(job));
if (result.is_error()) {
    const auto& error = result.error();
    std::cerr << "Error code: " << static_cast<int>(error.code()) << "\n";
    std::cerr << "Message: " << error.message() << "\n";
}
```

## Compiler Warnings

When using deprecated methods, you will see compiler warnings like:

```
warning: 'submit_task' is deprecated: Use enqueue() with a job wrapper instead. This method will be removed in v2.0.
```

These warnings indicate code that needs to be updated before v2.0.

## Timeline

- **Current**: Deprecated methods generate compiler warnings but remain functional
- **v2.0**: Deprecated methods will be removed

## Questions?

If you have questions about migration, please open an issue on GitHub.
