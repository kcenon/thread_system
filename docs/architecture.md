# Thread System Architecture

## Overview

Thread System is a comprehensive C++20 threading framework designed to simplify concurrent programming, providing a hierarchical design that enables efficient thread management and task scheduling. This document outlines the architectural components, their relationships, and the design principles behind the system.

## Core Components

The Thread System architecture consists of four core components:

1. **Thread Base**: The foundational layer
2. **Logging System**: Concurrent logging framework
3. **Thread Pool**: General-purpose worker thread management
4. **Priority Thread Pool**: Priority-based task scheduling

These components are designed to work both independently and together, providing a flexible framework for concurrent programming.

## Component Relationships

```
                     ┌─────────────────┐
                     │   thread_base   │
                     └────────┬────────┘
                              │
                 ┌────────────┼────────────┐
                 │            │            │
        ┌────────▼────────┐   │   ┌────────▼─────────┐
        │  thread_worker  │   │   │priority_thread_worker│
        └────────┬────────┘   │   └────────┬──────────┘
                 │            │            │
        ┌────────▼────────┐   │   ┌────────▼──────────┐
        │   thread_pool   │   │   │priority_thread_pool│
        └─────────────────┘   │   └───────────────────┘
                              │
                     ┌────────▼────────┐
                     │  log_collector  │
                     └────────┬────────┘
                              │
                 ┌────────────┼────────────┐
                 │            │            │
        ┌────────▼────────┐   │   ┌────────▼────────┐
        │  console_writer │   │   │   file_writer   │
        └─────────────────┘   │   └─────────────────┘
                              │
                     ┌────────▼────────┐
                     │ callback_writer │
                     └─────────────────┘
```

## Detailed Component Architecture

### 1. Thread Base Module (`thread_module`)

The Thread Base Module provides the fundamental building blocks for all threading operations.

#### Key Classes:

- **`thread_base`**: An abstract base class that encapsulates a single thread's lifecycle management
- **`job`**: Represents a unit of work to be executed by a thread
- **`job_queue`**: A thread-safe queue for storing and retrieving jobs
- **`callback_job`**: A specialized job that executes a custom function
- **`thread_conditions`**: Enum defining thread states (e.g., stopped, running)
- **`cancellation_token`**: Provides a mechanism to request cancellation of operations

#### Responsibilities:

- Thread lifecycle management (creation, execution, termination)
- Provides customization points through virtual methods
- Supports wake intervals for periodic execution
- Handles thread synchronization via mutexes and condition variables
- Adapts to the available threading model (std::jthread or std::thread)

### 2. Logging Module (`log_module`)

The Logging Module builds on the Thread Base to provide thread-safe, asynchronous logging capabilities.

#### Key Classes:

- **`logger`**: Singleton interface for logging operations
- **`log_types`**: Enum defining log severity levels
- **`log_collector`**: Central hub for collecting and dispatching log messages
- **`log_job`**: Job implementation for logging operations
- **`message_job`**: Represents a single logging message
- **`console_writer`**: Handles console output in a dedicated thread
- **`file_writer`**: Manages log file output in a dedicated thread
- **`callback_writer`**: Processes logs through user-defined callbacks

#### Responsibilities:

- Thread-safe logging across multiple components
- Filtering based on log severity levels
- Output routing to multiple destinations
- Internationalization support (Unicode strings)
- Asynchronous log processing to minimize impact on application performance

### 3. Thread Pool Module (`thread_pool_module`)

The Thread Pool Module provides a pool of worker threads that can efficiently execute multiple jobs.

#### Key Classes:

- **`thread_pool`**: Manages a collection of worker threads
- **`thread_worker`**: Worker thread implementation derived from `thread_base`
- **`thread_pool_builder`**: Factory for creating thread pools with different configurations
- **`task`**: Higher-level abstraction for scheduled work

#### Responsibilities:

- Efficient distribution of jobs across multiple worker threads
- Dynamic management of thread lifecycle
- Batch job submission for improved performance
- Worker thread monitoring and status reporting

### 4. Priority Thread Pool Module (`priority_thread_pool_module`)

The Priority Thread Pool Module extends the Thread Pool concept to support job priorities.

#### Key Classes:

- **`priority_thread_pool`**: Manages workers with priority handling
- **`priority_thread_worker`**: Worker supporting priority-based job execution
- **`priority_job`**: Job with an associated priority level
- **`priority_job_queue`**: Queue that maintains jobs according to priority
- **`job_priorities`**: Enum defining standard priority levels
- **`callback_priority_job`**: Callback job with priority support

#### Responsibilities:

- Prioritized job execution
- Support for custom priority types
- Dynamic worker assignment based on priority
- Priority-based preemption policies

## Design Patterns

The Thread System architecture incorporates several design patterns:

1. **Singleton Pattern**: Used for the logger to provide a global access point
2. **Template Method Pattern**: Employed in thread_base for customization of behavior
3. **Factory Pattern**: Implemented in thread_pool_builder for creating thread pools
4. **Composition over Inheritance**: Components are designed for composition rather than deep inheritance hierarchies
5. **Bridge Pattern**: Separates abstractions (like logging) from implementations (writers)
6. **Command Pattern**: Jobs are commands that encapsulate actions to be performed
7. **Observer Pattern**: Callback mechanisms for notifications

## Cross-Cutting Concerns

Several aspects of the design address cross-cutting concerns:

### 1. Error Handling

- Consistent use of `std::optional<std::string>` or `result_void` for error reporting
- Structured error propagation through the call stack
- Detailed error messages for debugging

### 2. Thread Safety

- Thread-safe data structures with proper synchronization
- Clear documentation of thread safety guarantees
- Avoidance of shared mutable state where possible

### 3. Resource Management

- RAII principles for resource acquisition and release
- Smart pointers for memory management
- Explicit lifecycle management for threads

### 4. Extensibility

- Abstract base classes and virtual methods enabling customization
- Template-based generic programming for flexibility
- Explicit extension points for future features

## Implementation Considerations

The Thread System implementation accommodates different C++20 feature availability across compilers:

- Conditional compilation for std::format vs fmt::format
- Support for both std::jthread (C++20) and std::thread (pre-C++20)
- Platform-specific optimizations for different operating systems

## Conclusion

The Thread System architecture provides a robust foundation for concurrent programming, balancing simplicity of use with flexibility and performance. By understanding the component relationships and responsibilities, developers can effectively leverage this system for a wide range of concurrent programming scenarios.