# Job Cancellation System

## Overview

The thread_system now provides a comprehensive job cancellation mechanism that allows running jobs to be cooperatively cancelled when workers or pools are stopped. This implements a graceful shutdown pattern that gives jobs the opportunity to clean up resources and terminate early.

## Architecture

### Level 1: Worker-Job Connection

Each `thread_worker` maintains:
- A `cancellation_token` that is propagated to jobs during execution
- An atomic pointer to the currently executing job
- An `on_stop_requested()` hook that cancels the running job

When `thread_worker::do_work()` executes a job:
1. Sets the job's cancellation token
2. Tracks the current job atomically
3. Executes `job->do_work()`
4. Clears the current job pointer

When `thread_base::stop()` is called:
1. Sets the stop flag
2. Calls `on_stop_requested()` on derived classes
3. `thread_worker::on_stop_requested()` cancels the worker's token and the current job's token
4. The job detects cancellation on its next `is_cancelled()` check

### Level 2: Pool-Level Hierarchical Cancellation

The `thread_pool` maintains a pool-level `cancellation_token`. When `stop()` is called:
1. The pool's token is cancelled
2. This propagates to all workers (via `on_stop_requested()`)
3. Workers cancel their running jobs
4. All jobs receive the cancellation signal simultaneously

## Usage

### Implementing Cancellable Jobs

Jobs must cooperatively check for cancellation:

```cpp
class my_cancellable_job : public job {
public:
    result_void do_work() override {
        for (int i = 0; i < iterations; ++i) {
            // ✅ Check cancellation periodically
            if (cancellation_token_.is_cancelled()) {
                cleanup_partial_work();
                return error{error_code::operation_canceled,
                           "Cancelled at iteration " + std::to_string(i)};
            }

            // Do work
            process_item(i);
        }

        return {};
    }
};
```

### Best Practices

1. **Check cancellation frequently** - Every few iterations or after blocking operations
2. **Return operation_canceled error** - Use the standard error code for cancellation
3. **Clean up before returning** - Release resources, close files, etc.
4. **Don't ignore cancellation** - Jobs that never check will block shutdown

### Anti-Patterns

❌ **Never checking cancellation:**
```cpp
result_void do_work() override {
    for (int i = 0; i < 1000000; ++i) {
        expensive_operation(i);  // Never checks token!
    }
    return {};
}
```

❌ **Checking too frequently:**
```cpp
result_void do_work() override {
    for (int i = 0; i < 1000000; ++i) {
        if (cancellation_token_.is_cancelled()) return error{...};  // Every iteration!
        fast_operation(i);
    }
}
```

✅ **Good balance:**
```cpp
result_void do_work() override {
    for (int i = 0; i < 1000000; ++i) {
        if (i % 1000 == 0 && cancellation_token_.is_cancelled()) {
            return error{error_code::operation_canceled, "Cancelled"};
        }
        fast_operation(i);
    }
}
```

## Examples

See `examples/job_cancellation_example/` for comprehensive demonstrations of:
- Basic worker-level cancellation
- Non-cooperative jobs (anti-pattern)
- Pool-level multi-worker cancellation
- Immediate vs. graceful shutdown

## Performance Characteristics

- **Cancellation propagation latency**: < 1ms typically
- **Overhead**: Minimal - single atomic load per check
- **Shutdown time**: Depends on job cooperation
  - Cooperative jobs: Stop within one iteration (usually < 100ms)
  - Non-cooperative jobs: Must wait for completion (potentially seconds)

## Thread Safety

All cancellation operations are thread-safe:
- `cancellation_token::cancel()` - Can be called from any thread
- `cancellation_token::is_cancelled()` - Lock-free atomic load
- `thread_worker::on_stop_requested()` - Safe concurrent access with `do_work()`
- `thread_pool::stop()` - Atomically transitions state before cancellation

## Implementation Details

### Memory Ordering

- `current_job_` uses `memory_order_release` when storing, `memory_order_acquire` when loading
- Ensures job state is visible to cancellation thread
- `cancellation_token` uses `memory_order_acq_rel` for cancel flag

### Cancellation Token

The `cancellation_token` class provides:
- Atomic `is_cancelled` flag
- Callback registration (for advanced use cases)
- Linked tokens for hierarchical cancellation

### Worker Cancellation Flow

```
pool->stop()
    ↓
pool_cancellation_token_.cancel()
    ↓
thread_base::stop()
    ↓
on_stop_requested() [virtual hook]
    ↓
worker_cancellation_token_.cancel()
    ↓
current_job_->get_cancellation_token().cancel()
    ↓
job checks is_cancelled() → returns operation_canceled
```

## Future Enhancements

Potential future additions (not currently implemented):
- **Job handles**: Return handle from `enqueue()` to cancel individual jobs
- **Timeouts**: Automatic cancellation after time limit
- **Cancellation callbacks**: Register cleanup functions with token
- **Force-kill option**: Non-cooperative termination for stuck jobs (dangerous!)

## Related Classes

- `kcenon::thread::cancellation_token` - Core cancellation primitive
- `kcenon::thread::job` - Base job class with token support
- `kcenon::thread::thread_worker` - Worker with cancellation propagation
- `kcenon::thread::thread_pool` - Pool-level cancellation coordination
- `kcenon::thread::thread_base` - Base class with `on_stop_requested()` hook
