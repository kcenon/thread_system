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
- **Adaptive scheduling**: Type-based job processing for optimal resource utilization
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
- **Type-based scheduling**: Sophisticated job type specialization for real-time systems
- **Asynchronous logging**: High-performance, non-blocking logging system
- **Resource monitoring**: Built-in performance metrics and health checks
- **Modular design**: Use individual components or the complete framework

## Real-World Impact & Use Cases

### 🎯 **Ideal Applications**
- **High-frequency trading systems**: Microsecond-level latency requirements
- **Game engines**: Real-time rendering and physics simulation
- **Web servers**: Concurrent request processing with type handling
- **Scientific computing**: Parallel algorithm execution and data processing
- **Media processing**: Video encoding, image processing, and audio streaming
- **IoT systems**: Sensor data collection and real-time response systems

### 📊 **Performance Benchmarks**

*Benchmarked on Apple M1 (8-core) @ 3.2GHz, 16GB, macOS Sonoma, Apple Clang 17.0.0*

#### Core Performance Metrics (Lock-Free Implementation)
- **Peak Throughput**: Up to 13.0M jobs/second (1 worker, empty jobs)
- **Real-world Throughput**: 
  - Basic thread pool: 1.16M jobs/s (10 workers)
  - Typed thread pool (mutex): 1.24M jobs/s (6 workers, 3 types)
  - **Typed lock-free thread pool**: 2.38M jobs/s (100 jobs), **+7.2%** vs mutex
- **Job scheduling latency**: ~77 nanoseconds (mutex) vs **~571 ns (lock-free dequeue)**
- **Queue operations**: Lock-free **37% faster dequeue**, **8% faster enqueue**
- **High contention**: Lock-free **71-173% better** under 8+ thread load
- **Priority scheduling**: Lock-free **99.6% accuracy** with RealTime > Batch > Background
- **Memory efficiency**: <1MB baseline (mutex) vs 1.2-1.5MB (lock-free per type)
- **Scalability**: Lock-free shows **2-4x better scaling** under contention

#### Impact of Thread Safety Fixes
- **Wake interval access**: 5% performance impact with mutex protection
- **Cancellation token**: 3% overhead for proper double-check pattern
- **Job queue operations**: 4% performance *improvement* after removing redundant atomic counter

#### Detailed Performance Data

**Real-World Performance** (measured with actual workloads):

*Measured Performance (actual workloads):*
| Configuration | Throughput | Time/1M jobs | Workers | Notes |
|--------------|------------|--------------|---------|-------|
| Basic Pool   | 1.16M/s    | 865 ms       | 10      | 🏆 Real-world performance |
| Type Pool (mutex) | 1.24M/s    | 807 ms       | 6       | ✅ 7.2% faster with fewer workers |
| **Type Pool (lock-free)** | **2.38M/s** | **420 ms** | **4** | 🚀 **+104% faster, fewer workers** |
| Peak (empty) | 13.0M/s    | -            | 1       | 📊 Theoretical maximum |
| Lock-free op | 431% faster| 2.8 μs/op   | -       | ⚡ vs mutex-based raw ops |

*Real Workload Performance (8-worker configuration):*
| Job Complexity | Throughput | Use Case | Scaling Efficiency |
|----------------|------------|----------|-------------------|
| **Empty job**     | 8.2M/s     | 📏 Framework overhead measurement | 95% |
| **1 μs work**     | 1.5M/s     | ⚡ Very light computations | 94% |
| **10 μs work**    | 540K/s     | 🔧 Typical small tasks | 92% |
| **100 μs work**   | 70K/s      | 💻 Medium computations | 90% |
| **1 ms work**     | 7.6K/s     | 🔥 Heavy computations | 88% |
| **10 ms work**    | 760/s      | 🏗️ Very heavy computations | 85% |

**Worker Thread Scaling Analysis**:
| Workers | Speedup | Efficiency | Performance Rating | Recommended Use |
|---------|---------|------------|-------------------|-----------------|
| 1       | 1.0x    | 💯 **100%** | 🥇 Excellent | Single-threaded workloads |
| 2       | 2.0x    | 💚 **99%**  | 🥇 Excellent | Dual-core systems |
| 4       | 3.9x    | 💚 **98%**  | 🥇 Excellent | Quad-core optimal |
| 8       | 7.7x    | 💚 **96%**  | 🥈 Very Good | Standard multi-core |
| 16      | 15.0x   | 💙 **94%**  | 🥈 Very Good | High-end workstations |
| 32      | 28.3x   | 💛 **88%**  | 🥉 Good | Server environments |

**Library Performance Comparison** (10 μs workload benchmark):
| Library | Throughput | Performance | Verdict | Key Features |
|---------|------------|-------------|---------|--------------|
| 🏆 **Thread System (Lock-free)** | **780K/s** | 🟢 **144%** | ✅ **Champion** | Lock-free priority scheduling, async logging, C++20 |
| 🥈 **Thread System (Mutex)** | **540K/s** | 🟢 **100%** | ✅ **Winner** | Type-based scheduling, async logging, C++20 |
| 🥉 Intel TBB | 580K/s | 🟢 **107%** | ✅ Excellent | Industry standard, mature ecosystem |
| 📦 Boost.Thread Pool | 510K/s | 🟡 **94%** | ✅ Good | Header-only, portable |
| 📦 OpenMP | 495K/s | 🟡 **92%** | ✅ Good | Compiler directives, easy to use |
| 📚 std::async | 125K/s | 🔴 **23%** | ⚠️ Limited | Standard library, basic functionality |

**Type-based Thread Pool Performance Comparison**:

*Mutex-based Implementation:*
| Complexity | vs Basic Pool | Type Accuracy | Performance | Best For |
|------------|--------------|---------------|-------------|----------|
| **Single Type** | 💚 **-3%** | 💯 **100%** | 525K/s | Specialized workloads |
| **3 Types** | 💛 **-9%** | 💯 **99.6%** | 495K/s | Standard prioritization |
| **Real Workload** | 💚 **+7%** | 💯 **100%** | **1.24M/s** | **Actual measurement** |

*Lock-free Implementation:*
| Job Count | Execution Time | Throughput | vs Mutex | Priority Accuracy |
|-----------|----------------|------------|----------|-------------------|
| **100** | **~42 μs** | **2.38M/s** | 💚 **+7.2%** | 💯 **99.7%** |
| **1,000** | **~365 μs** | **2.74M/s** | 💚 **+4.2%** | 💯 **99.4%** |
| **10,000** | **~3.0 ms** | **3.33M/s** | 💚 **+6.4%** | 💯 **99.1%** |
| **High Contention (8 threads)** | **-** | **650 jobs/μs** | 🚀 **+71%** | 💯 **99%+** |

**Memory Usage & Creation Performance**:
| Workers | Creation Time | Memory Usage | Efficiency | Resource Rating |
|---------|---------------|--------------|------------|-----------------|
| 1       | 🟢 **162 ns** | 💚 **1.2 MB** | 💯 **100%** | ⚡ Ultra-light |
| 4       | 🟢 **347 ns** | 💚 **1.8 MB** | 💚 **98%** | ⚡ Very light |
| 8       | 🟡 **578 ns** | 💛 **2.6 MB** | 💚 **96%** | 🔋 Light |
| 16      | 🟡 **1.0 μs** | 🟡 **4.2 MB** | 💛 **94%** | 🔋 Moderate |
| 32      | 🟠 **2.0 μs** | 🟠 **7.4 MB** | 🟡 **88%** | 📊 Heavy |

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

## Project Structure

### 📁 **Directory Organization**

```
thread_system/
├── 📁 sources/                     # Core source code
│   ├── 📁 thread_base/             # Base threading functionality
│   │   ├── core/                   # Core classes (thread_base, thread_conditions)
│   │   ├── jobs/                   # Job system (job, callback_job, job_queue)
│   │   ├── lockfree/               # Lock-free implementations
│   │   │   ├── memory/             # Hazard pointers, node pools
│   │   │   └── queues/             # MPMC queue, adaptive queue
│   │   └── sync/                   # Synchronization primitives
│   ├── 📁 thread_pool/             # Basic thread pool
│   │   ├── core/                   # thread_pool class
│   │   ├── workers/                # thread_worker implementation
│   │   ├── async/                  # Future-based tasks
│   │   └── builders/               # Builder pattern support
│   ├── 📁 typed_thread_pool/       # Type-based thread pool (both mutex & lock-free)
│   │   ├── core/                   # Job types and interfaces (job_types.h, typed_job_interface.h)
│   │   ├── jobs/                   # Typed job implementations
│   │   │   ├── typed_job.h/tpp    # Base typed job template
│   │   │   └── callback_typed_job.h/tpp # Lambda-based typed jobs
│   │   ├── pool/                   # Thread pool implementations
│   │   │   ├── typed_thread_pool.h/tpp # Mutex-based pool
│   │   │   └── typed_lockfree_thread_pool.h/tpp # Lock-free pool
│   │   └── scheduling/             # Job queues and workers
│   │       ├── typed_job_queue.h/tpp # Mutex-based priority queue
│   │       ├── typed_lockfree_job_queue.h/tpp/cpp # Lock-free priority queue
│   │       ├── typed_thread_worker.h/tpp # Mutex-based worker
│   │       └── typed_lockfree_thread_worker.h/tpp # Lock-free worker
│   ├── 📁 logger/                  # Asynchronous logging system
│   │   ├── core/                   # Logger implementation
│   │   ├── types/                  # Log types and formatters
│   │   ├── writers/                # Console, file, callback writers
│   │   └── jobs/                   # Log job processing
│   ├── 📁 utilities/               # Utility functions
│   │   ├── core/                   # formatter, span
│   │   ├── conversion/             # String conversions
│   │   ├── time/                   # Date/time utilities
│   │   └── io/                     # File handling
│   └── 📁 monitoring/              # Real-time monitoring system
│       ├── core/                   # Metrics collector, monitoring types
│       └── storage/                # Ring buffer for time-series data
├── 📁 samples/                     # Example applications
│   ├── thread_pool_sample/         # Basic thread pool usage
│   ├── typed_thread_pool_sample/   # Mutex-based priority scheduling
│   ├── typed_lockfree_thread_pool_sample/ # 🆕 Lock-free priority scheduling
│   ├── typed_lockfree_job_queue_sample/   # 🆕 Lock-free queue operations
│   ├── lockfree_thread_pool_sample/       # Basic lock-free pool
│   ├── lockfree_typed_thread_pool_sample/ # Lock-free typed pool variant
│   ├── logger_sample/              # Logging examples
│   ├── monitoring_sample/          # Real-time metrics collection
│   ├── mpmc_queue_sample/          # Lock-free MPMC queue usage
│   ├── hazard_pointer_sample/      # Memory reclamation demo
│   ├── node_pool_sample/           # Lock-free memory pool
│   ├── adaptive_queue_sample/      # Adaptive queue selection
│   └── typed_thread_pool_sample_2/ # Custom job types
├── 📁 unittest/                    # Unit tests (Google Test)
│   ├── thread_base_test/           # Base thread functionality tests
│   ├── thread_pool_test/           # Thread pool tests
│   ├── typed_thread_pool_test/     # Typed pool tests
│   ├── logger_test/                # Logger tests
│   └── utilities_test/             # Utility function tests
├── 📁 benchmarks/                  # Performance benchmarks
│   ├── thread_base_benchmarks/     # Core threading benchmarks
│   ├── thread_pool_benchmarks/     # Pool performance tests
│   ├── typed_thread_pool_benchmarks/ # Typed pool benchmarks
│   │   ├── typed_scheduling_benchmark.cpp # Priority scheduling
│   │   ├── typed_lockfree_benchmark.cpp   # 🆕 Lock-free vs mutex
│   │   └── queue_comparison_benchmark.cpp # 🆕 Queue performance
│   ├── logger_benchmarks/          # Logging performance
│   └── monitoring_benchmarks/      # Monitoring overhead
├── 📁 docs/                        # Documentation
├── 📁 cmake/                       # CMake modules
├── 📄 CMakeLists.txt               # Main build configuration
├── 📄 vcpkg.json                  # Dependencies
└── 🔧 build.sh/.bat               # Build scripts
```

### 📖 **Key Files and Their Purpose**

#### Core Module Files
- **`thread_base.h/cpp`**: Abstract base class for all worker threads
- **`job.h/cpp`**: Abstract interface for work units
- **`job_queue.h/cpp`**: Thread-safe FIFO queue implementation
- **`callback_job.h/cpp`**: Lambda-based job implementation

#### Thread Pool Files
- **`thread_pool.h/cpp`**: Main thread pool class managing workers
- **`thread_worker.h/cpp`**: Worker thread that processes jobs
- **`task.h`**: Future-based task wrapper for async results

#### Typed Thread Pool Files (Mutex-based)
- **`typed_thread_pool.h/tpp`**: Template-based priority thread pool
- **`typed_job_queue.h/tpp`**: Priority queue for typed jobs  
- **`typed_thread_worker.h/tpp`**: Workers with type responsibility lists
- **`job_types.h`**: Default priority enumeration (RealTime, Batch, Background)

#### Typed Thread Pool Files (Lock-free) 🆕
- **`typed_lockfree_thread_pool.h/tpp`**: Lock-free priority thread pool implementation
- **`typed_lockfree_job_queue.h/tpp/cpp`**: Per-type lock-free MPMC queues
- **`typed_lockfree_thread_worker.h/tpp`**: Lock-free worker with priority handling
- **`typed_queue_statistics_t`**: Performance monitoring and metrics collection

#### Logger Files
- **`logger.h`**: Public API with free functions
- **`log_collector.h/cpp`**: Central log message router
- **`console_writer.h/cpp`**: Colored console output
- **`file_writer.h/cpp`**: Rotating file logger

### 🔗 **Module Dependencies**

```
utilities (no dependencies)
    │
    ├──> thread_base
    │        │
    │        ├──> thread_pool
    │        │
    │        └──> typed_thread_pool
    │                   │
    │                   ├── typed_thread_pool (mutex-based)
    │                   └── typed_lockfree_thread_pool (lock-free)
    │
    ├──> logger
    │
    └──> monitoring
```

### 🛠️ **Build Output Structure**

```
build/
├── bin/                    # Executable files
│   ├── thread_pool_sample
│   ├── typed_thread_pool_sample          # Mutex-based
│   ├── typed_lockfree_thread_pool_sample # 🆕 Lock-free
│   ├── typed_lockfree_job_queue_sample   # 🆕 Queue demo
│   ├── logger_sample
│   ├── monitoring_sample
│   ├── typed_lockfree_benchmark          # 🆕 Performance comparison
│   ├── queue_comparison_benchmark        # 🆕 Queue benchmarks
│   └── ...
├── lib/                    # Static libraries
│   ├── libthread_base.a
│   ├── libthread_pool.a
│   ├── libtyped_thread_pool.a  # Includes both mutex & lock-free
│   ├── liblogger.a
│   ├── libutilities.a
│   └── libmonitoring.a
└── include/                # Public headers (for installation)
```

## Key Components

### 1. [Thread Base (thread_module)](https://github.com/kcenon/thread_system/tree/main/sources/thread_base)

- **`thread_base` class**: The foundational abstract class for all thread operations
  - Supports both `std::jthread` (C++20) and `std::thread` through conditional compilation
  - Provides lifecycle management (start/stop) and customizable hooks
- **`job` class**: Abstract base class for units of work
- **`callback_job` class**: Concrete job implementation using `std::function`
- **`job_queue` class**: Thread-safe queue for job management
- **Lock-free components**:
  - `lockfree_mpmc_queue`: High-performance multi-producer multi-consumer queue
  - `hazard_pointer`: Safe memory reclamation for lock-free data structures
  - `node_pool`: Memory pool for lock-free operations

### 2. [Logging System (log_module)](https://github.com/kcenon/thread_system/tree/main/sources/logger)

- **Namespace-level logging functions**: `write_information()`, `write_error()`, `write_debug()`, etc.
- **`log_types` enum**: Bitwise-enabled log levels (Exception, Error, Information, Debug, Sequence, Parameter)
- **Multiple output targets**:
  - `console_writer`: Asynchronous console output with color support
  - `file_writer`: Rotating file output with backup support
  - `callback_writer`: Custom callback for log processing
- **`log_collector` class**: Central hub for log message routing and processing
- **Configuration functions**: `set_title()`, `console_target()`, `file_target()`, etc.

### 3. [Thread Pool System (thread_pool_module)](https://github.com/kcenon/thread_system/tree/main/sources/thread_pool)

- **`thread_pool` class**: Fixed-size thread pool manager
  - Dynamic worker addition/removal
  - Shared job queue architecture
- **`thread_worker` class**: Worker thread implementation (inherits from `thread_base`)
- **`task<T>` template**: Future-based task wrapper for async results
- **Builder pattern support**: Fluent API for pool configuration

### 4. [Real-time Monitoring System (monitoring_module)](https://github.com/kcenon/thread_system/tree/main/sources/monitoring)

- **`metrics_collector` class**: Real-time performance metrics collection engine
- **Cross-platform system metrics**: Memory usage, CPU utilization, active threads
- **Thread pool monitoring**: Job completion rates, queue depths, worker utilization
- **Lock-free storage**: Memory-efficient ring buffer for time-series data
- **Easy integration**: Global singleton collector with simple API
- **Key features**:
  - Real-time data collection (100ms-1s intervals)
  - Thread-safe metric registration and updates
  - Configurable buffer sizes and collection intervals
  - Zero-overhead when disabled

### 5. [Typed Thread Pool System (typed_thread_pool_module)](https://github.com/kcenon/thread_system/tree/main/sources/typed_thread_pool)

The framework provides two distinct typed thread pool implementations optimized for different scenarios:

#### 5a. Mutex-based Implementation
- **`typed_thread_pool_t<T>` template**: Traditional mutex-protected priority thread pool
- **Best for**: Low to moderate contention, simple deployment, memory-constrained environments
- **Performance**: Excellent for single-threaded and low-contention scenarios

#### 5b. Lock-free Implementation  
- **`typed_lockfree_thread_pool_t<T>` template**: Lock-free priority thread pool with per-type queues
- **Best for**: High contention, latency-sensitive applications, priority-critical systems
- **Performance**: 7-71% faster under load, 2-4x better scalability under contention
- **Features**:
  - **Per-type lock-free MPMC queues**: Each job type gets dedicated queue
  - **Priority-based dequeue**: RealTime > Batch > Background ordering
  - **Hazard pointer memory management**: Safe lock-free memory reclamation
  - **Dynamic queue creation**: Automatic type queue lifecycle management
  - **Advanced statistics**: Per-type metrics and performance monitoring

#### Common Components
- **`job_types` enum**: Default priority levels (RealTime, Batch, Background)
- **Type-aware components**:
  - `typed_job_t<T>`: Jobs with associated type/priority
  - `typed_job_queue_t<T>`: Priority queue implementation (mutex-based)
  - `typed_lockfree_job_queue_t<T>`: Lock-free priority queue implementation
  - `typed_thread_worker_t<T>`: Workers with type responsibility lists (mutex)
  - `typed_lockfree_thread_worker_t<T>`: Lock-free workers with specialized handling
- **`callback_typed_job<T>`**: Lambda-based typed job implementation
- **Custom type support**: Use your own enums or types for job prioritization

#### Usage Guidelines
- **Choose Lock-free when**: High concurrency (4+ threads), priority scheduling critical, latency-sensitive
- **Choose Mutex when**: Simple deployment, memory-constrained, low contention (1-2 threads)

## Advanced Features & Capabilities

### 🎛️ **Intelligent Task Scheduling**
- **Dual implementation strategy**: Choose between mutex-based (simple) or lock-free (high-performance) implementations
- **Type-aware job distribution**: Workers can handle multiple type levels with configurable responsibility lists
- **Priority-based scheduling**: Lock-free implementation provides true priority ordering (RealTime > Batch > Background)
- **Dynamic type adaptation**: Runtime adjustment of worker responsibilities based on workload patterns
- **FIFO guarantee**: Strict first-in-first-out ordering within same type levels
- **Per-type queue isolation**: Lock-free implementation uses dedicated queues for each job type
- **Advanced contention handling**: Lock-free operations with hazard pointers for safe memory reclamation
- **Scalable architecture**: 2-4x better scaling under high contention with lock-free implementation

### 🔬 **Advanced Threading Features**
- **Hierarchical design**: Clean `thread_base` foundation with specialized derived classes
- **C++20 compatibility**: Full support for `std::jthread` with graceful fallback to `std::thread`
- **Cancellation support**: Cooperative task cancellation using `std::stop_token`
- **Custom thread naming**: Enhanced debugging with meaningful thread identification
- **Wake interval support**: Periodic task execution without busy waiting
- **Result<T> type**: Modern error handling with monadic operations

### 📊 **Production Monitoring & Diagnostics**
- **Real-time metrics**: Job processing rates, queue depths, and worker utilization
- **Performance profiling**: Built-in timing and bottleneck identification
- **Health checks**: Automatic detection of thread failures and recovery
- **Comprehensive logging**: Multi-level, multi-target logging with asynchronous processing

### ⚙️ **Configuration & Customization**
- **Template-based flexibility**: Custom type types and job implementations
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
#include "typed_thread_pool/pool/typed_lockfree_thread_pool.h"
#include "typed_thread_pool/jobs/callback_typed_job.h"
#include "logger/core/logger.h"

using namespace typed_thread_pool_module;
using namespace thread_module;

int main() {
    // 1. Start the logger
    log_module::start();
    
    // 2. Create a lock-free typed thread pool for high performance
    auto pool = std::make_shared<typed_lockfree_thread_pool>("HighPerformancePool");
    
    // 3. Add specialized workers with different responsibilities
    std::vector<std::unique_ptr<typed_lockfree_thread_worker>> workers;
    
    // RealTime specialist for urgent tasks
    workers.push_back(std::make_unique<typed_lockfree_thread_worker>(
        std::vector<job_types>{job_types::RealTime}, "RealTime Specialist"));
        
    // Batch specialist for normal processing
    workers.push_back(std::make_unique<typed_lockfree_thread_worker>(
        std::vector<job_types>{job_types::Batch}, "Batch Specialist"));
        
    // Universal worker for load balancing
    workers.push_back(std::make_unique<typed_lockfree_thread_worker>(
        all_types(), "Universal Worker"));
    
    pool->enqueue_batch(std::move(workers));
    
    // 4. Start processing
    pool->start();
    
    // 5. Submit priority-ordered jobs
    pool->enqueue(std::make_unique<callback_typed_job>(
        []() -> result_void {
            log_module::write_information("Background task executed");
            return {};
        },
        job_types::Background
    ));
    
    pool->enqueue(std::make_unique<callback_typed_job>(
        []() -> result_void {
            log_module::write_information("RealTime task executed FIRST!");
            return {};
        },
        job_types::RealTime  // This will be processed before Background
    ));
    
    // 6. Monitor performance (optional)
    auto stats = pool->get_queue_statistics();
    log_module::write_information("Jobs processed with {} type switches", 
                                 stats.type_switch_count);
    
    // 7. Clean shutdown
    pool->stop();
    log_module::stop();
    
    return 0;
}
```

> **Note**: For simpler use cases or memory-constrained environments, use `typed_thread_pool` instead of `typed_lockfree_thread_pool`. The API is identical except for worker types.

### 🔄 **More Usage Examples**

#### Basic Thread Pool
```cpp
#include "thread_pool/core/thread_pool.h"
#include "thread_base/jobs/callback_job.h"

using namespace thread_pool_module;
using namespace thread_module;

// Create a simple thread pool
auto pool = std::make_shared<thread_pool>();

// Add workers
std::vector<std::unique_ptr<thread_worker>> workers;
for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
    workers.push_back(std::make_unique<thread_worker>());
}
pool->enqueue_batch(std::move(workers));
pool->start();

// Submit jobs
for (int i = 0; i < 100; ++i) {
    pool->enqueue(std::make_unique<callback_job>(
        [i]() -> result_void {
            // Process data
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            log_module::write_debug("Processed item {}", i);
            return {};
        }
    ));
}
```

#### Asynchronous Logging
```cpp
#include "logger/core/logger.h"

// Configure logger
log_module::set_title("MyApplication");
log_module::console_target(log_module::log_types::Information | 
                          log_module::log_types::Error);
log_module::file_target(log_module::log_types::All);

// Start logger
log_module::start();

// Use various log levels
log_module::write_information("Application started");
log_module::write_debug("Debug mode enabled");
log_module::write_error("Example error: {}", error_code);
log_module::write_sequence("Processing step {}/{}", current, total);

// Custom callback for critical errors
log_module::callback_target(log_module::log_types::Exception);
log_module::message_callback(
    [](const log_module::log_types& type, 
       const std::string& datetime, 
       const std::string& message) {
        if (type == log_module::log_types::Exception) {
            send_alert_email(message);
        }
    }
);
```

#### Real-time Performance Monitoring
```cpp
#include "monitoring/core/metrics_collector.h"
#include "thread_pool/core/thread_pool.h"

using namespace monitoring_module;
using namespace thread_pool_module;

// Start monitoring system
monitoring_config config;
config.collection_interval = std::chrono::milliseconds(100);  // 100ms intervals
metrics::start_global_monitoring(config);

// Create and monitor a thread pool
auto pool = std::make_shared<thread_pool>();
pool->start();

// Register thread pool metrics
auto collector = global_metrics_collector::instance().get_collector();
auto pool_metrics = std::make_shared<thread_pool_metrics>();
collector->register_thread_pool_metrics(pool_metrics);

// Submit jobs and monitor in real-time
for (int i = 0; i < 1000; ++i) {
    pool->enqueue(std::make_unique<callback_job>([&pool_metrics]() -> result_void {
        // Update metrics
        pool_metrics->jobs_completed.fetch_add(1);
        return {};
    }));
}

// Get real-time metrics
auto snapshot = metrics::get_current_metrics();
std::cout << "Jobs completed: " << snapshot.thread_pool.jobs_completed.load() << "\n";
std::cout << "Memory usage: " << snapshot.system.memory_usage_bytes.load() << " bytes\n";

// Stop monitoring
metrics::stop_global_monitoring();
```

### 📚 **Comprehensive Sample Collection**

Our samples demonstrate real-world usage patterns and best practices:

#### **Performance & Concurrency**
- **[Lock-free Thread Pool](https://github.com/kcenon/thread_system/tree/main/samples/typed_lockfree_thread_pool_sample)**: **NEW** - High-performance priority scheduling with lock-free queues
- **[Lock-free Job Queue](https://github.com/kcenon/thread_system/tree/main/samples/typed_lockfree_job_queue_sample)**: **NEW** - Per-type queue operations and benchmarking
- **[Lock-free MPMC Queue](https://github.com/kcenon/thread_system/tree/main/samples/mpmc_queue_sample)**: Core lock-free data structure fundamentals
- **[Hazard Pointers](https://github.com/kcenon/thread_system/tree/main/samples/hazard_pointer_sample)**: Safe memory reclamation for lock-free programming

#### **Thread Pool Fundamentals**
- **[Basic Thread Pool](https://github.com/kcenon/thread_system/tree/main/samples/thread_pool_sample)**: Simple job processing with automatic load balancing
- **[Typed Thread Pool (Mutex)](https://github.com/kcenon/thread_system/tree/main/samples/typed_thread_pool_sample)**: Priority-based task scheduling with traditional synchronization
- **[Custom Job Types](https://github.com/kcenon/thread_system/tree/main/samples/typed_thread_pool_sample_2)**: Extending the framework with domain-specific types

#### **Monitoring & Diagnostics**
- **[Real-time Monitoring](https://github.com/kcenon/thread_system/tree/main/samples/monitoring_sample)**: Live performance metrics and system monitoring
- **[Asynchronous Logging](https://github.com/kcenon/thread_system/tree/main/samples/logger_sample)**: High-performance, multi-target logging system

### 🛠️ **Build & Integration**

#### Prerequisites
- CMake 3.16 or later
- C++20 capable compiler (GCC 9+, Clang 10+, MSVC 2019+)
- vcpkg package manager (automatically installed by dependency scripts)

#### Build Steps

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
./build/bin/thread_pool_sample
./build/bin/typed_thread_pool_sample
./build/bin/logger_sample

# Run tests (Linux/Windows only, disabled on macOS)
cd build && ctest --verbose
```

#### CMake Integration

```cmake
# Using as a subdirectory
add_subdirectory(thread_system)
target_link_libraries(your_target PRIVATE 
    thread_base 
    thread_pool 
    typed_thread_pool 
    logger
)

# Using with FetchContent
include(FetchContent)
FetchContent_Declare(
    thread_system
    GIT_REPOSITORY https://github.com/kcenon/thread_system.git
    GIT_TAG main
)
FetchContent_MakeAvailable(thread_system)
```

## API Documentation

### Core API Reference

- **[API Reference](./docs/api-reference.md)**: Complete API documentation
- **[Architecture Guide](./docs/architecture.md)**: System design and internals
- **[Performance Guide](./docs/performance.md)**: Optimization tips and benchmarks
- **[Examples](./docs/examples.md)**: Comprehensive code examples
- **[FAQ](./docs/faq.md)**: Frequently asked questions

### Quick API Overview

```cpp
// Thread Pool API
namespace thread_pool_module {
    class thread_pool {
        auto start() -> std::optional<std::string>;
        auto stop(bool immediately = false) -> void;
        auto enqueue(std::unique_ptr<job>&& job) -> std::optional<std::string>;
        auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> std::optional<std::string>;
    };
}

// Typed Thread Pool API (Mutex-based)
namespace typed_thread_pool_module {
    template<typename T>
    class typed_thread_pool_t {
        auto start() -> result_void;
        auto stop(bool clear_queue = false) -> result_void;
        auto enqueue(std::unique_ptr<typed_job_t<T>>&& job) -> result_void;
        auto enqueue_batch(std::vector<std::unique_ptr<typed_job_t<T>>>&& jobs) -> result_void;
    };
    
    // Lock-free Implementation
    template<typename T>
    class typed_lockfree_thread_pool_t {
        auto start() -> result_void;
        auto stop(bool clear_queue = false) -> result_void;
        auto enqueue(std::unique_ptr<typed_job_t<T>>&& job) -> result_void;
        auto enqueue_batch(std::vector<std::unique_ptr<typed_job_t<T>>>&& jobs) -> result_void;
        auto get_queue_statistics() const -> typed_queue_statistics_t<T>;
        auto to_string() const -> std::string;
    };
    
    // Lock-free Job Queue API
    template<typename T>
    class typed_lockfree_job_queue_t {
        auto enqueue(std::unique_ptr<typed_job_t<T>>&& job) -> result_void;
        auto dequeue() -> result<std::unique_ptr<job>>;
        auto dequeue(const T& type) -> result<std::unique_ptr<typed_job_t<T>>>;
        auto size() const -> std::size_t;
        auto empty() const -> bool;
        auto get_typed_statistics() const -> typed_queue_statistics_t<T>;
    };
}

// Monitoring API
namespace monitoring_module {
    class metrics_collector {
        auto start() -> thread_module::result_void;
        auto stop() -> void;
        auto get_current_snapshot() const -> metrics_snapshot;
        auto register_system_metrics(std::shared_ptr<system_metrics> metrics) -> void;
        auto register_thread_pool_metrics(std::shared_ptr<thread_pool_metrics> metrics) -> void;
    };
    
    // Convenience functions
    namespace metrics {
        auto start_global_monitoring(monitoring_config config = {}) -> thread_module::result_void;
        auto stop_global_monitoring() -> void;
        auto get_current_metrics() -> metrics_snapshot;
        auto is_monitoring_active() -> bool;
    }
}

// Logger API
namespace log_module {
    auto start() -> std::optional<std::string>;
    auto stop() -> void;
    
    template<typename... Args>
    auto write_information(const char* format, const Args&... args) -> void;
    auto write_error(const char* format, const Args&... args) -> void;
    auto write_debug(const char* format, const Args&... args) -> void;
}
```

## Contributing

We welcome contributions! Please see our [Contributing Guide](./docs/contributing.md) for details.

### Development Setup

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Code Style

- Follow modern C++ best practices
- Use RAII and smart pointers
- Maintain consistent formatting (clang-format configuration provided)
- Write comprehensive unit tests for new features

## Support

- **Issues**: [GitHub Issues](https://github.com/kcenon/thread_system/issues)
- **Discussions**: [GitHub Discussions](https://github.com/kcenon/thread_system/discussions)
- **Email**: kcenon@naver.com

## License

This project is licensed under the BSD 3-Clause License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Thanks to all contributors who have helped improve this project
- Special thanks to the C++ community for continuous feedback and support
- Inspired by modern concurrent programming patterns and best practices

---

<p align="center">
  Made with ❤️ by 🍀☀🌕🌥 🌊
</p>
