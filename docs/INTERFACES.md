# Interfaces Overview

This document summarizes the public interfaces that decouple components in the Thread System and enable dependency injection.

## Executor Interface

Header: `interfaces/executor_interface.h`

- `auto execute(std::unique_ptr<thread_module::job>&& work) -> thread_module::result_void`
- `auto shutdown() -> thread_module::result_void`

Implemented by: `implementations/thread_pool/thread_pool`

Example:
```cpp
auto pool = std::make_shared<thread_pool_module::thread_pool>("pool");
pool->enqueue_batch({std::make_unique<thread_pool_module::thread_worker>(false)});
pool->start();
pool->execute(std::make_unique<thread_module::callback_job>([](){ return thread_module::result_void(); }));
pool->shutdown();
```

## Scheduler Interface

Header: `interfaces/scheduler_interface.h`

- `auto schedule(std::unique_ptr<thread_module::job>&& work) -> thread_module::result_void`
- `auto get_next_job() -> thread_module::result<std::unique_ptr<thread_module::job>>`

Implemented by: `core/jobs/job_queue` and its derivatives (`lockfree_job_queue`, `adaptive_job_queue`).

## Logging Interface and Registry

Header: `interfaces/logger_interface.h`

- `logger_interface::log(level, message[, file, line, function])`
- `logger_registry::set_logger(std::shared_ptr<logger_interface>)`
- Convenience macros: `THREAD_LOG_INFO("message")`, etc.

## Monitoring Interface

Header: `interfaces/monitoring_interface.h`

Data structures:
- `system_metrics`, `thread_pool_metrics` (supports `pool_name`/`pool_instance_id`), `worker_metrics`
- `metrics_snapshot`

Key methods:
- `update_system_metrics(const system_metrics&)`
- `update_thread_pool_metrics(const thread_pool_metrics&)`
- `update_thread_pool_metrics(const std::string& pool_name, std::uint32_t pool_instance_id, const thread_pool_metrics&)`
- `update_worker_metrics(std::size_t worker_id, const worker_metrics&)`
- `get_current_snapshot()`, `get_recent_snapshots(size_t)`

Utility:
- `null_monitoring` — no-op implementation
- `scoped_timer(std::atomic<std::uint64_t>& target)` — RAII measurement helper

## Monitorable Interface

Header: `interfaces/monitorable_interface.h`

- `auto get_metrics() -> monitoring_interface::metrics_snapshot`
- `void reset_metrics()`

Use to expose component metrics uniformly.

## Thread Context and Service Container

Headers: `interfaces/thread_context.h`, `interfaces/service_container.h`

`thread_context` provides access to:
- `std::shared_ptr<logger_interface> logger()`
- `std::shared_ptr<monitoring_interface::monitoring_interface> monitoring()`
- Helper methods to log and update metrics safely when services are available

`service_container` is a thread-safe DI container:
- `register_singleton<Interface>(std::shared_ptr<Interface>)`
- `register_factory<Interface>(std::function<std::shared_ptr<Interface>()>, lifetime)`
- `resolve<Interface>() -> std::shared_ptr<Interface>`

Use `scoped_service<Interface>` to register within a scope.

## Error Handling

Header: `core/sync/include/error_handling.h`, `interfaces/error_handler.h`

- Strongly typed `error_code`, `error`, `result<T>`/`result_void`
- `error_handler` interface and `default_error_handler` implementation

## Crash Handling

Header: `interfaces/crash_handler.h`

- Global `crash_handler::instance()` with registration of crash callbacks and cleanup routines
- Configurable safety level, core dumps, stack trace, and log path
- RAII helper `scoped_crash_callback`

## Typed Thread Pool Interfaces

Headers under `implementations/typed_thread_pool/include`:
- `typed_job_interface`, `job_types`, `type_traits`
- Lock-free/adaptive typed queues and typed workers/pools

These enable per-type routing and priority scheduling for heterogeneous workloads.

