# Thread System Project

## Purpose

This project proposes a reusable Thread system for programmers who struggle with threading concepts. It aims to simplify complex multithreading tasks and enable efficient and safe concurrent programming.

## Key Components

### 1. [Thread Base (thread_module)](https://github.com/kcenon/thread_system/tree/main/sources/thread_base)

- `thread_base` class: The foundational class for all thread operations. It provides basic threading functionality that other components inherit and build upon.
- Supports both `std::jthread` (C++20) and `std::thread` through conditional compilation, allowing for broader compatibility and modern thread management.
- `job` class: Defines unit of work
- `job_queue` class: Manages work queue

### 2. [Logging System (log_module)](https://github.com/kcenon/thread_system/tree/main/sources/logger)

- `logger` class: Provides logging functionality using a singleton pattern
- `log_types` enum: Defines various log levels
- `log_job` class: Specialized job class for logging operations

### 3. [Thread Pool System (thread_pool_module)](https://github.com/kcenon/thread_system/tree/main/sources/thread_pool)

- `thread_worker` class: Worker thread that processes jobs, inherits from `thread_base`
- `thread_pool` class: Manages thread pool

### 4. [Priority-based Thread Pool System (priority_thread_pool_module)](https://github.com/kcenon/thread_system/tree/main/sources/priority_thread_pool)

- `job_priorities` enum: Defines job priority levels
- `priority_job` class: Defines job with priority, inherits from `job`
- `priority_job_queue` class: Priority-based job queue, inherits from `job_queue`
- `priority_thread_worker` class: Priority-based worker thread, inherits from `thread_base`
- `priority_thread_pool` class: Manages priority-based thread pool

## Key Features

- Hierarchical design with `thread_base` as the foundation
- Support for both `std::jthread` (C++20) and `std::thread`, controlled via the `USE_STD_JTHREAD` macro
- Flexible logging system
- Basic and priority-based job processing
- Dynamic thread pool management
- Error handling and reporting
- Type safety using templates
- Support for both `std::format` and `fmt::format`

## Usage Examples

Sample codes included in the project demonstrate the following use cases:
- [Basic logger usage](https://github.com/kcenon/thread_system/tree/main/samples/logger_sample)
- [Basic thread pool usage](https://github.com/kcenon/thread_system/tree/main/samples/thread_pool_sample)
- [Priority-based thread pool usage](https://github.com/kcenon/thread_system/tree/main/samples/priority_thread_pool_sample)

## Areas for Improvement

- Add mechanism to wait for job completion
- Utilize more diverse log levels
- Performance measurement and optimization
- Additional error handling and recovery mechanisms
- Further exploration of `std::jthread` features like automatic joining and cancellation

## License

This project is licensed under the BSD License.