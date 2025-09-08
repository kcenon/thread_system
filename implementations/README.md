# Implementations Module

Concrete implementations built on top of the core module:

## Thread Pool (`implementations/thread_pool`)
- A pool of `thread_worker` instances consuming from a shared `job_queue`
- Implements `executor_interface` (`execute`, `shutdown`)
- Integrates with `thread_context` for logging/monitoring
- `worker_policy` for future scheduling/idle behaviors

Key APIs
- `thread_pool::enqueue(std::unique_ptr<job>&&)`
- `thread_pool::enqueue_batch(std::vector<std::unique_ptr<job>>&&)`
- `thread_pool::enqueue(std::unique_ptr<thread_worker>&&)`
- `thread_pool::start()` / `thread_pool::stop(bool immediately)` / `shutdown()`

## Lock-Free and Adaptive Queues (`implementations/lockfree`)
- `lockfree_job_queue`: Michael & Scott MPMC with hazard pointers and node pool
- `adaptive_job_queue`: runtime switching between mutex-based and lock-free
  strategies based on contention/latency (auto/forced strategies supported)

Factory
- `thread_module::create_job_queue(strategy)` creates an appropriate queue instance

## Typed Thread Pool (`implementations/typed_thread_pool`)
- Type/priority-aware jobs (`job_types` or custom enum/integral types)
- Per-type queues with lock-free/adaptive variants
- Typed workers/pool for heterogeneous workload isolation and QoS

Usage
- Include headers from each implementation's `include` folder.
- Link against `thread_pool`, `lockfree`, and/or `typed_thread_pool` targets.
