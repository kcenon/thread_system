# Interfaces Module

Public interfaces decoupling components and enabling dependency injection:

- `executor_interface` — submit work and shutdown (implemented by thread pools)
- `scheduler_interface` — enqueue/dequeue jobs (implemented by job queues)
- `queue_capabilities`/`queue_capabilities_interface` — runtime queue capability queries
- `monitoring_interface`/`monitorable_interface` — metrics reporting/query
- `thread_context` — unified access to optional services (logger/monitoring)
- `service_container` — thread-safe DI container (singleton/factory lifetimes)
- `error_handler` — pluggable error handling (with `default_error_handler`)
- `crash_handler` — process-wide crash safety hooks (optional)

> **Note**: As of v3.0.0, `logger_interface` and `logger_registry` have been removed.
> Use `kcenon::common::interfaces::ILogger` from common_system instead.
> See `docs/guides/LOGGER_INTERFACE_MIGRATION_GUIDE.md` for migration instructions.

See `docs/INTERFACES.md` for full API details and examples.

Usage
- Register services in `service_container::global()` and use `thread_context`
  in pools/workers to access them.
- For logging, use `common::interfaces::GlobalLoggerRegistry` to register an ILogger implementation.
