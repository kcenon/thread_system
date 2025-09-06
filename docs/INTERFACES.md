# Interfaces Overview

This document describes the new interfaces introduced in Phase 2.

- executor_interface: Submits work units and coordinates shutdown. Implemented by thread_pool and typed_thread_pool.
- scheduler_interface: Enqueues and fetches jobs. Implemented by job_queue.
- monitorable_interface: Exposes a metrics API contract. Intended for components reporting metrics.
- service_registry: Lightweight, header-only service lookup utility.

## executor_interface

- execute(std::unique_ptr<job>&&): submit a job
- shutdown(): cooperative shutdown

Implemented by:
- thread_pool: forwards to enqueue()/stop(false)
- typed_thread_pool: forwards to typed_job_queue->enqueue()/stop(false)

## scheduler_interface

- schedule(std::unique_ptr<job>&&): enqueue a job
- get_next_job(): dequeue a job

Implemented by:
- job_queue: forwards to enqueue()/dequeue()

## monitorable_interface

- get_metrics(): returns metrics_snapshot from monitoring_interface
- reset_metrics(): clears internal counters

Usage is optional and orthogonal to logger/monitoring integration.

## service_registry

Header: core/base/include/service_registry.h

```cpp
auto obj = std::make_shared<MyType>();
service_registry::register_service<std::shared_ptr<MyType>>(obj);
auto back = service_registry::get_service<std::shared_ptr<MyType>>();
```

Note: The existing thread_context continues to use service_container for DI.

