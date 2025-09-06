# Interfaces Module

Public interfaces decoupling components and enabling dependency injection:

- logger_interface, thread_context, monitoring_interface
- executor_interface: submit work and shutdown
- scheduler_interface: enqueue/dequeue jobs
- monitorable_interface: expose metrics

Usage
- Implement interfaces in your application and inject via thread_context or service registries.

