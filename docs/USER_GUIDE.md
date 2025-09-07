# Thread System User Guide

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Modules

- core: `thread_base`, `job`, `job_queue`, `sync`
- implementations: `thread_pool`, `typed_thread_pool`, `lockfree` (lock-free + adaptive queues)
- interfaces: logging, monitoring, executor/scheduler/monitorable, service container/context
- utilities: formatting, string conversion, span

## Quick Start: Thread Pool

```cpp
using namespace thread_pool_module;
using thread_module::result_void;

auto pool = std::make_shared<thread_pool>("pool");
std::vector<std::unique_ptr<thread_worker>> workers;
workers.emplace_back(std::make_unique<thread_worker>(/*use_time_tag=*/false));
pool->enqueue_batch(std::move(workers));
pool->start();

pool->execute(std::make_unique<thread_module::callback_job>([]() -> result_void {
  // do work
  return result_void();
}));

pool->shutdown(); // or pool->stop(false)
```

## Quick Start: Typed Thread Pool (priority/type routing)

```cpp
using namespace typed_thread_pool_module;

// See samples/typed_thread_pool_sample for a complete example
// Define jobs by type (e.g., job_types::RealTime / Batch / Background)
// and submit to the typed pool to route by type/priority.
```

## Adaptive Job Queue (automatic strategy)

`implementations/lockfree` provides an adaptive queue that switches between
mutex-based and lock-free MPMC internally based on contention and latency.

```cpp
thread_module::adaptive_job_queue q{
  thread_module::adaptive_job_queue::queue_strategy::ADAPTIVE
};

q.enqueue(std::make_unique<thread_module::callback_job>([](){ return thread_module::result_void(); }));
auto job = q.dequeue();
```

## Dependency Injection and Context

Use `service_container` to register optional services (logger, monitoring), then
`thread_context` retrieves them automatically for pools/workers.

```cpp
// Register services
thread_module::service_container::global()
  .register_singleton<thread_module::logger_interface>(my_logger);
thread_module::service_container::global()
  .register_singleton<monitoring_interface::monitoring_interface>(my_monitoring);

// Context-aware pool/worker will log and report metrics when services exist
thread_pool_module::thread_worker w{true, thread_module::thread_context{}};
```

## Monitoring and Metrics

- `monitoring_interface` defines `system_metrics`, `thread_pool_metrics` (with
  `pool_name`/`pool_instance_id`), `worker_metrics`, and snapshot APIs.
- `thread_pool` reports pool metrics (workers, idle count, queue size) via context.
- Use `null_monitoring` when monitoring is not configured.

## Error Handling and Cancellation

- Return `result_void` / `result<T>` from operations, with `error_code` on failure.
- Use `cancellation_token` for cooperative cancellation (linkable tokens, callbacks).

## Documentation

If Doxygen is installed:

```bash
cmake --build build --target docs
# Open thread_system/documents/html/index.html
```

See also: `docs/INTERFACES.md` for API-level interface details.

## Samples

- minimal_thread_pool: minimal pool usage
- thread_pool_sample: end-to-end pool lifecycle
- adaptive_queue_sample: adaptive vs lock-free vs mutex queue
- typed_thread_pool_sample: typed/persistent priority routing
- composition_example: DI with interfaces
- integration_example: external logger/monitoring integration
- multi_process_monitoring_integration: process-aware monitoring
