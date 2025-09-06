# Thread System User Guide

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Modules

- core: `thread_base`, `job`, `job_queue`
- implementations: `thread_pool`, `typed_thread_pool`, `lockfree`
- interfaces: logging, monitoring, executor/scheduler/monitorable
- utilities: formatting, string conversion, span

## Quick Start (Thread Pool)

```cpp
auto pool = std::make_shared<thread_pool_module::thread_pool>("pool");
std::vector<std::unique_ptr<thread_pool_module::thread_worker>> workers;
workers.push_back(std::make_unique<thread_pool_module::thread_worker>(false));
pool->enqueue_batch(std::move(workers));
pool->start();
pool->enqueue(std::make_unique<thread_module::callback_job>([]() -> thread_module::result_void {
  // do work
  return thread_module::result_void();
}));
pool->stop(false);
```

## Documentation

If Doxygen is installed:

```bash
cmake --build build --target docs
# Open thread_system/documents/html/index.html
```

## Samples

- minimal_thread_pool: minimal pool usage
- composition_example: DI with interfaces
- integration_example: external logger/monitoring integration
- multi_process_monitoring_integration: process-aware monitoring
- service_registry_sample: using service_registry

