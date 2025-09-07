# Implementations Module

Concrete implementations built on top of the core module:

- thread_pool: standard worker pool processing jobs from a shared queue
- lockfree: lock-free MPMC queues and hazard pointer memory reclamation
- typed_thread_pool: priority/typed scheduling with per-type queues

Usage
- Include headers from the specific implementation's `include` folder.
- Link against `thread_pool`, `lockfree`, and/or `typed_thread_pool` targets.

