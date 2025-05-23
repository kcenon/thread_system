[![CodeFactor](https://www.codefactor.io/repository/github/kcenon/thread_system/badge)](https://www.codefactor.io/repository/github/kcenon/thread_system)

[![Ubuntu-Clang](https://github.com/kcenon/thread_system/actions/workflows/build-ubuntu-clang.yaml/badge.svg)](https://github.com/kcenon/thread_system/actions/workflows/build-ubuntu-clang.yaml)
[![Ubuntu-GCC](https://github.com/kcenon/thread_system/actions/workflows/build-ubuntu-gcc.yaml/badge.svg)](https://github.com/kcenon/thread_system/actions/workflows/build-ubuntu-gcc.yaml)
[![Windows-MinGW](https://github.com/kcenon/thread_system/actions/workflows/build-windows-mingw.yaml/badge.svg)](https://github.com/kcenon/thread_system/actions/workflows/build-windows-mingw.yaml)
[![Windows-MSYS2](https://github.com/kcenon/thread_system/actions/workflows/build-windows-msys2.yaml/badge.svg)](https://github.com/kcenon/thread_system/actions/workflows/build-windows-msys2.yaml)
[![Windows-VisualStudio](https://github.com/kcenon/thread_system/actions/workflows/build-windows-vs.yaml/badge.svg)](https://github.com/kcenon/thread_system/actions/workflows/build-windows-vs.yaml)

# Thread System Project

## Project Overview

The Thread System Project is a comprehensive, production-ready C++20 multithreading framework designed to democratize concurrent programming. By providing intuitive abstractions and robust implementations, it empowers developers of all skill levels to build high-performance, thread-safe applications without the typical complexity and pitfalls of manual thread management.

## Project Purpose & Mission

This project addresses the fundamental challenge faced by developers worldwide: **making concurrent programming accessible, safe, and efficient**. Traditional threading approaches often lead to complex code, hard-to-debug race conditions, and performance bottlenecks. Our mission is to provide a comprehensive solution that:

- **Eliminates threading complexity** through intuitive, high-level abstractions
- **Ensures thread safety** by design, preventing common concurrency bugs
- **Maximizes performance** through optimized algorithms and modern C++ features
- **Promotes code reusability** across different platforms and use cases
- **Accelerates development** by providing ready-to-use threading components

## Core Advantages & Benefits

### 🚀 **Performance Excellence**
- **Zero-overhead abstractions**: Modern C++ design ensures minimal runtime cost
- **Optimized data structures**: Lock-free algorithms and cache-friendly designs
- **Adaptive scheduling**: Priority-based job processing for optimal resource utilization
- **Scalable architecture**: Linear performance scaling with hardware thread count

### 🛡️ **Production-Grade Reliability**
- **Thread-safe by design**: All components guarantee safe concurrent access
- **Comprehensive error handling**: Robust error reporting and recovery mechanisms
- **Memory safety**: RAII principles and smart pointers prevent leaks and corruption
- **Extensive testing**: 95%+ CI/CD success rate across multiple platforms and compilers

### 🔧 **Developer Productivity**
- **Intuitive API design**: Clean, self-documenting interfaces reduce learning curve
- **Rich documentation**: Comprehensive Doxygen documentation with examples
- **Flexible configuration**: Template-based customization for specific needs
- **Debugging support**: Built-in logging and monitoring capabilities

### 🌐 **Cross-Platform Compatibility**
- **Universal support**: Works on Windows, Linux, and macOS
- **Compiler flexibility**: Compatible with GCC, Clang, and MSVC
- **C++ standard adaptation**: Graceful fallback from C++20 to older standards
- **Architecture independence**: Optimized for both x86 and ARM processors

### 📈 **Enterprise-Ready Features**
- **Priority-based scheduling**: Sophisticated job prioritization for real-time systems
- **Asynchronous logging**: High-performance, non-blocking logging system
- **Resource monitoring**: Built-in performance metrics and health checks
- **Modular design**: Use individual components or the complete framework

## Real-World Impact & Use Cases

### 🎯 **Ideal Applications**
- **High-frequency trading systems**: Microsecond-level latency requirements
- **Game engines**: Real-time rendering and physics simulation
- **Web servers**: Concurrent request processing with priority handling
- **Scientific computing**: Parallel algorithm execution and data processing
- **Media processing**: Video encoding, image processing, and audio streaming
- **IoT systems**: Sensor data collection and real-time response systems

### 📊 **Performance Benchmarks**

#### Core Performance Metrics
- **Thread creation overhead**: ~10-15 microseconds per thread
- **Job scheduling latency**: ~1-2 microseconds per job  
- **Priority queue operations**: O(log n) complexity with optimized implementations
- **Memory efficiency**: <1MB baseline memory usage for typical configurations
- **Throughput**: Up to 2.1M jobs/second with 8 workers (empty jobs)

#### Detailed Performance Data

**Job Throughput by Complexity** (8-worker configuration):
| Job Duration | Throughput | Use Case |
|-------------|------------|----------|
| Empty job   | 2.1M/s     | Task distribution overhead measurement |
| 1 μs work   | 1.5M/s     | Very light computations |
| 10 μs work  | 540K/s     | Typical small tasks |
| 100 μs work | 70K/s      | Medium computations |
| 1 ms work   | 7.6K/s     | Heavy computations |

**Scaling Efficiency**:
| Workers | Speedup | Efficiency |
|---------|---------|------------|
| 1       | 1.0x    | 100%       |
| 2       | 2.0x    | 99%        |
| 4       | 3.9x    | 98%        |
| 8       | 7.7x    | 96%        |
| 16      | 15.0x   | 94%        |

**vs Other Libraries** (540K jobs/sec baseline):
- **Thread System**: 100% (baseline)
- **Intel TBB**: 107% (slightly faster)
- **Boost.Thread Pool**: 94%
- **std::async**: 23% (4x slower)

For comprehensive performance analysis and optimization techniques, see the [Performance Guide](docs/performance.md).

## Technology Stack & Architecture

### 🏗️ **Modern C++ Foundation**
- **C++20 features**: `std::jthread`, `std::format`, concepts, and ranges
- **Template metaprogramming**: Type-safe, compile-time optimizations
- **Memory management**: Smart pointers and RAII for automatic resource cleanup
- **Exception safety**: Strong exception safety guarantees throughout

### 🔄 **Design Patterns Implementation**
- **Command Pattern**: Job encapsulation for flexible task execution
- **Observer Pattern**: Event-driven logging and monitoring
- **Factory Pattern**: Configurable thread pool creation
- **Singleton Pattern**: Global logger access with thread safety
- **Template Method Pattern**: Customizable thread behavior

## Key Components

### 1. [Thread Base (thread_module)](https://github.com/kcenon/thread_system/tree/main/sources/thread_base)

- `thread_base` class: The foundational class for all thread operations. It provides basic threading functionality that other components inherit and build upon.
- Supports both `std::jthread` (C++20) and `std::thread` through conditional compilation, allowing for broader compatibility and modern thread management.
- `job` class: Defines unit of work
- `job_queue` class: Manages work queue

### 2. [Logging System (log_module)](https://github.com/kcenon/thread_system/tree/main/sources/logger)

- `logger` class: Provides logging functionality using a singleton pattern
- `log_types` enum: Defines various log levels (e.g., INFO, WARNING, ERROR)
- `log_job` class: Specialized job class for logging operations
- `log_collector` class: Manages the collection, processing, and distribution of log messages
- `message_job` class: Represents a logging message job derived from the base job class
- `console_writer` class: Handles writing log messages to the console in a separate thread
- `file_writer` class: Handles writing log messages to files in a separate thread
- `callback_writer` class: Handles writing log messages to callback in a separate thread

### 3. [Thread Pool System (thread_pool_module)](https://github.com/kcenon/thread_system/tree/main/sources/thread_pool)

- `thread_worker` class: Worker thread that processes jobs, inherits from `thread_base`
- `thread_pool` class: Manages thread pool

### 4. [Priority-based Thread Pool System (priority_thread_pool_module)](https://github.com/kcenon/thread_system/tree/main/sources/priority_thread_pool)

- `job_priorities` enum: Defines job priority levels
- `priority_job` class: Defines job with priority, inherits from `job`
- `priority_job_queue` class: Priority-based job queue, inherits from `job_queue`
- `priority_thread_worker` class: Priority-based worker thread, inherits from `thread_base`
- `priority_thread_pool` class: Manages priority-based thread pool

## Advanced Features & Capabilities

### 🎛️ **Intelligent Task Scheduling**
- **Priority-aware job distribution**: Workers can handle multiple priority levels with configurable responsibility lists
- **Dynamic priority adaptation**: Runtime adjustment of worker responsibilities based on workload patterns
- **FIFO guarantee**: Strict first-in-first-out ordering within same priority levels
- **Work stealing**: Automatic load balancing across worker threads

### 🔬 **Advanced Threading Features**
- **Hierarchical design**: Clean `thread_base` foundation with specialized derived classes
- **C++20 compatibility**: Full support for `std::jthread` with graceful fallback to `std::thread`
- **Cancellation support**: Cooperative task cancellation using `std::stop_token`
- **Custom thread naming**: Enhanced debugging with meaningful thread identification

### 📊 **Production Monitoring & Diagnostics**
- **Real-time metrics**: Job processing rates, queue depths, and worker utilization
- **Performance profiling**: Built-in timing and bottleneck identification
- **Health checks**: Automatic detection of thread failures and recovery
- **Comprehensive logging**: Multi-level, multi-target logging with asynchronous processing

### ⚙️ **Configuration & Customization**
- **Template-based flexibility**: Custom priority types and job implementations
- **Runtime configuration**: JSON-based configuration for deployment flexibility
- **Compile-time optimization**: Conditional feature compilation for minimal overhead
- **Builder pattern**: Fluent API for easy thread pool construction

### 🔒 **Safety & Reliability**
- **Exception safety**: Strong exception safety guarantees throughout the framework
- **Resource leak prevention**: Automatic cleanup using RAII principles
- **Deadlock prevention**: Careful lock ordering and timeout mechanisms
- **Memory corruption protection**: Smart pointer usage and bounds checking

## Quick Start & Usage Examples

### 🚀 **Getting Started in 5 Minutes**

```cpp
#include "priority_thread_pool.h"
using namespace priority_thread_pool_module;

// 1. Create a priority thread pool
auto pool = std::make_shared<priority_thread_pool_t<job_priorities>>();

// 2. Add workers with different responsibilities
pool->add_worker(job_priorities::High);      // High priority specialist
pool->add_worker({job_priorities::High, job_priorities::Normal}); // Multi-priority worker

// 3. Start processing
pool->start();

// 4. Submit jobs with priorities
pool->enqueue(make_priority_job([]() { 
    // Your high-priority task here
}, job_priorities::High));

// 5. Clean shutdown
pool->stop();
```

### 📚 **Comprehensive Sample Collection**

Our samples demonstrate real-world usage patterns and best practices:

- **[Asynchronous Logging](https://github.com/kcenon/thread_system/tree/main/samples/logger_sample/logger_sample.cpp)**: High-performance, multi-target logging system
- **[Basic Thread Pool](https://github.com/kcenon/thread_system/tree/main/samples/thread_pool_sample/thread_pool_sample.cpp)**: Simple job processing with automatic load balancing
- **[Priority Scheduling](https://github.com/kcenon/thread_system/tree/main/samples/priority_thread_pool_sample/priority_thread_pool_sample.cpp)**: Sophisticated priority-based task management
- **[Custom Priority Types](https://github.com/kcenon/thread_system/tree/main/samples/priority_thread_pool_sample_2/priority_thread_pool_sample_2.cpp)**: Extending the framework with domain-specific priorities

### 🛠️ **Build & Integration**

```bash
# Clone the repository
git clone https://github.com/kcenon/thread_system.git
cd thread_system

# Install dependencies via vcpkg
./dependency.sh  # Linux/macOS
./dependency.bat # Windows

# Build the project
./build.sh       # Linux/macOS
./build.bat      # Windows

# Run samples
./build/bin/priority_thread_pool_sample
```

## License

This project is licensed under the BSD License.
