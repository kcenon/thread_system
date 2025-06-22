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

#### Core Performance Metrics (Latest Benchmarks - 2025-06-21)
- **Peak Throughput**: Up to 15.2M jobs/second (lock-free, 1 worker, empty jobs)
- **Real-world Throughput**: 
  - Standard thread pool: 1.16M jobs/s (10 workers)
  - **Lock-free thread pool**: 2.48M jobs/s (8 workers) - **2.14x improvement**
  - Typed thread pool (mutex): 1.24M jobs/s (6 workers, 3 types)
  - **Typed lock-free thread pool**: 2.38M jobs/s (100 jobs), **+7.2%** vs mutex
- **Job scheduling latency**: 
  - Standard pool: ~77 nanoseconds (mutex-based)
  - **Lock-free pool**: ~320 ns enqueue, ~580 ns dequeue (includes safety guarantees)
- **Queue operations**: Lock-free **7.7x faster enqueue**, **5.4x faster dequeue**
- **High contention**: Lock-free up to **3.46x better** with 16+ producers
- **Priority scheduling**: Lock-free **99.6% accuracy** with RealTime > Batch > Background
- **Memory efficiency**: <1MB baseline (standard) vs ~1.5MB (lock-free with hazard pointers)
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
| **Lock-free Pool** | **2.48M/s** | **403 ms** | **8** | 🚀 **+114% vs standard pool** |
| Peak (empty) | 15.2M/s    | -            | 1       | 📊 Theoretical maximum |

*Lock-free Thread Pool Performance Test Results:*
| Test Scenario | Workers | Jobs | Standard Time | Lock-free Time | Improvement |
|---------------|---------|------|---------------|----------------|-------------|
| Light Load    | 4       | 10K  | 45.2 ms      | 21.1 ms       | **2.14x**   |
| Medium Load   | 8       | 50K  | 312.5 ms     | 138.9 ms      | **2.25x**   |
| Heavy Load    | 16      | 100K | 892.4 ms     | 384.7 ms      | **2.32x**   |
| High Contention | 2     | 50K  | 523.7 ms     | 198.6 ms      | **2.64x**   |
| Stress Test   | 64      | 500K | 4,235.8 ms   | 1,876.3 ms    | **2.26x**   |

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
| 🏆 **Thread System (Lock-free Pool)** | **820K/s** | 🟢 **152%** | ✅ **Champion** | Lock-free MPMC queue, batch processing, C++20 |
| 🥇 **Thread System (Lock-free Typed)** | **780K/s** | 🟢 **144%** | ✅ **Excellent** | Lock-free priority scheduling, per-type queues |
| 🥈 **Thread System (Standard)** | **540K/s** | 🟢 **100%** | ✅ **Baseline** | Mutex-based, type scheduling, async logging |
| 🥉 Intel TBB | 580K/s | 🟢 **107%** | ✅ Very Good | Industry standard, work stealing |
| 📦 Boost.Thread Pool | 510K/s | 🟡 **94%** | ✅ Good | Header-only, portable |
| 📦 OpenMP | 495K/s | 🟡 **92%** | ✅ Good | Compiler directives, easy to use |
| 📚 std::async | 125K/s | 🔴 **23%** | ⚠️ Limited | Standard library, basic functionality |

**Logger Performance Comparison** (High-contention scenario):
| Logger Type | Single Thread | 4 Threads | 8 Threads | 16 Threads | Best Use Case |
|-------------|---------------|-----------|-----------|------------|---------------|
| 🏆 **Lock-free Logger** | 5.9M/s | **1.07M/s** | **0.63M/s** | **0.54M/s** | High-concurrency apps |
| 🥈 **Standard Logger** | 7.6M/s | 0.74M/s | 0.22M/s | 0.16M/s | Single-threaded apps |
| 📊 **Improvement** | -22% | **+45%** | **+186%** | **+238%** | 4+ threads = win |

**Logger vs Industry Standards** (spdlog comparison included):
| System | Single-thread | 4 Threads | 8 Threads | Latency | vs Console |
|--------|---------------|-----------|-----------|---------|------------|
| 🐌 **Console** | 583K/s | - | - | 1,716 ns | Baseline |
| 🏆 **TS Standard** | **4.34M/s** | 599K/s | 198K/s | **148 ns** | 🚀 **7.4x** |
| 🥈 **TS Lock-free** | 3.90M/s | **1.25M/s** | **583K/s** | 195 ns | 🚀 **6.7x** |
| 📦 **spdlog** | 515K/s | 210K/s | 52K/s | 2,333 ns | 🔴 **0.88x** |
| ⚡ **spdlog async** | **5.35M/s** | 785K/s | 240K/s | - | 🚀 **9.2x** |

**Key Insights**:
- 🏃 **Single-thread**: spdlog async wins (5.35M/s) but TS Standard close behind (4.34M/s)
- 🏋️ **Multi-thread**: TS Lock-free dominates (2.1x faster than spdlog async at 4 threads)
- ⏱️ **Latency**: TS Standard wins with 148ns (**15.7x lower** than spdlog)
- 📈 **Scalability**: Only TS Lock-free maintains performance under high contention

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
│   ├── 📁 thread_pool/             # Thread pool implementations
│   │   ├── core/                   # Pool classes
│   │   │   ├── thread_pool.h/cpp   # Standard mutex-based pool
│   │   │   └── lockfree_thread_pool.h/cpp # 🆕 Lock-free pool
│   │   ├── workers/                # Worker implementations
│   │   │   ├── thread_worker.h/cpp # Standard worker
│   │   │   └── lockfree_thread_worker.h/cpp # 🆕 Lock-free worker
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
│   │   │   ├── logger_implementation.h/cpp # Standard mutex-based logger
│   │   │   ├── lockfree_logger.h/cpp # 🆕 Lock-free logger
│   │   │   ├── log_collector.h/cpp # Standard log collector
│   │   │   └── lockfree_log_collector.h/cpp # 🆕 Lock-free collector
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
│   ├── lockfree_thread_pool_sample/       # 🆕 Basic lock-free pool usage
│   ├── logger_sample/              # Logging examples
│   ├── lockfree_logger_sample/     # 🆕 Lock-free logger performance
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
│   │   ├── thread_pool_benchmark.cpp      # Core pool metrics
│   │   ├── lockfree_comparison_benchmark.cpp # 🆕 Standard vs lock-free
│   │   ├── lockfree_performance_benchmark.cpp # 🆕 Detailed lock-free analysis
│   │   ├── memory_benchmark.cpp           # Memory usage patterns
│   │   ├── real_world_benchmark.cpp       # Realistic workloads
│   │   ├── stress_test_benchmark.cpp      # Extreme load testing
│   │   ├── scalability_benchmark.cpp      # Multi-core scaling
│   │   └── contention_benchmark.cpp       # Contention scenarios
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

#### Standard Thread Pool (Mutex-based)
- **`thread_pool` class**: Traditional thread pool with mutex synchronization
  - Dynamic worker addition/removal
  - Shared job queue architecture
  - Proven reliability for general workloads
- **`thread_worker` class**: Standard worker thread implementation

#### Lock-free Thread Pool 🆕
- **`lockfree_thread_pool` class**: High-performance lock-free implementation
  - **2.14x average performance improvement** over standard pool
  - Lock-free MPMC queue with hazard pointers
  - Exponential backoff for contention handling
  - Batch processing support for improved throughput
- **`lockfree_thread_worker` class**: Lock-free worker with advanced features
  - Per-worker statistics tracking
  - Optional batch processing mode
  - Configurable backoff strategies
  
#### Common Features
- **`task<T>` template**: Future-based task wrapper for async results
- **Builder pattern support**: Fluent API for pool configuration
- **Drop-in compatibility**: Same API for easy migration

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
#include "thread_pool/core/lockfree_thread_pool.h"
#include "thread_base/jobs/callback_job.h"
#include "logger/core/logger.h"

using namespace thread_pool_module;
using namespace thread_module;

int main() {
    // 1. Start the logger
    log_module::start();
    
    // 2. Create a high-performance lock-free thread pool
    auto pool = std::make_shared<lockfree_thread_pool>("HighPerformancePool");
    
    // 3. Add workers with batch processing enabled
    std::vector<std::unique_ptr<lockfree_thread_worker>> workers;
    for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
        auto worker = std::make_unique<lockfree_thread_worker>();
        worker->set_batch_processing(true, 32);  // Process up to 32 jobs at once
        workers.push_back(std::move(worker));
    }
    pool->enqueue_batch(std::move(workers));
    
    // 4. Start processing
    pool->start();
    
    // 5. Submit jobs - lock-free pool handles high contention efficiently
    std::atomic<int> counter{0};
    for (int i = 0; i < 100000; ++i) {
        pool->enqueue(std::make_unique<callback_job>(
            [&counter, i]() -> result_void {
                counter.fetch_add(1);
                if (i % 10000 == 0) {
                    log_module::write_information("Processed {} jobs", i);
                }
                return {};
            }
        ));
    }
    
    // 6. Wait for completion
    while (counter.load() < 100000) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // 7. Get performance statistics
    auto workers_list = pool->get_workers();
    for (const auto& worker : workers_list) {
        auto stats = static_cast<lockfree_thread_worker*>(worker.get())->get_statistics();
        log_module::write_information("Worker processed {} jobs, avg time: {} ns",
                                     stats.jobs_processed, 
                                     stats.avg_processing_time_ns);
    }
    
    // 8. Clean shutdown
    pool->stop();
    log_module::stop();
    
    return 0;
}
```

> **Performance Tip**: The lock-free pool excels with many concurrent producers. For simple workloads with low contention, use the standard `thread_pool` for lower memory usage.

### 🔄 **More Usage Examples**

#### Standard Thread Pool (Low Contention)
```cpp
#include "thread_pool/core/thread_pool.h"
#include "thread_base/jobs/callback_job.h"

using namespace thread_pool_module;
using namespace thread_module;

// Create a simple thread pool for low-contention workloads
auto pool = std::make_shared<thread_pool>("StandardPool");

// Add workers
std::vector<std::unique_ptr<thread_worker>> workers;
for (int i = 0; i < 4; ++i) {  // Few workers for simple tasks
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

#### Lock-free Thread Pool (High Contention)
```cpp
#include "thread_pool/core/lockfree_thread_pool.h"
#include "thread_base/jobs/callback_job.h"

using namespace thread_pool_module;
using namespace thread_module;

// Create lock-free pool for high-contention scenarios
auto pool = std::make_shared<lockfree_thread_pool>("LockfreePool");

// Configure workers for maximum throughput
std::vector<std::unique_ptr<lockfree_thread_worker>> workers;
for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
    auto worker = std::make_unique<lockfree_thread_worker>();
    
    // Enable batch processing for better throughput
    worker->set_batch_processing(true, 64);
    
    // Configure backoff strategy for contention handling
    lockfree_thread_worker::backoff_config config;
    config.min_backoff = std::chrono::nanoseconds(100);
    config.max_backoff = std::chrono::nanoseconds(10000);
    config.backoff_multiplier = 2.0;
    config.spin_count = 10;
    worker->set_backoff_config(config);
    
    workers.push_back(std::move(worker));
}
pool->enqueue_batch(std::move(workers));
pool->start();

// Submit jobs from multiple threads (high contention)
std::vector<std::thread> producers;
for (int t = 0; t < 8; ++t) {
    producers.emplace_back([&pool, t]() {
        for (int i = 0; i < 10000; ++i) {
            pool->enqueue(std::make_unique<callback_job>(
                [t, i]() -> result_void {
                    // Fast job execution
                    std::atomic<int> sum{0};
                    for (int j = 0; j < 100; ++j) {
                        sum.fetch_add(j);
                    }
                    return {};
                }
            ));
        }
    });
}

// Wait for all producers
for (auto& t : producers) {
    t.join();
}

// Get detailed statistics
auto workers_vec = pool->get_workers();
for (size_t i = 0; i < workers_vec.size(); ++i) {
    auto stats = static_cast<lockfree_thread_worker*>(
        workers_vec[i].get())->get_statistics();
    log_module::write_information(
        "Worker {}: {} jobs, {} μs avg, {} ns idle",
        i, stats.jobs_processed,
        stats.avg_processing_time_ns / 1000,
        stats.idle_time_ns
    );
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

#### High-Performance Lock-Free Logging 🆕
```cpp
#include "logger/core/lockfree_logger.h"

using namespace log_module::implementation;

// Use lock-free logger for high-concurrency scenarios
auto& logger = lockfree_logger::handle();
logger.set_title("HighPerformanceApp");
logger.console_target(log_types::Information);
logger.file_target(log_types::Information);

// Start the lock-free logger
if (auto error = logger.start(); error.has_value()) {
    std::cerr << "Failed to start logger: " << *error << std::endl;
    return 1;
}

// High-frequency logging from multiple threads
std::vector<std::thread> log_threads;
for (int t = 0; t < 16; ++t) {
    log_threads.emplace_back([&logger, t]() {
        for (int i = 0; i < 10000; ++i) {
            logger.write(log_types::Information, 
                        "Thread {} - High-frequency log message {}", t, i);
        }
    });
}

// Wait for all threads
for (auto& t : log_threads) {
    t.join();
}

// Lock-free logger provides superior performance:
// - No mutex contention in log queue
// - Wait-free enqueue operations
// - Up to 238% better throughput at 16 threads
// - Ideal for high-concurrency logging scenarios
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
- **[Lock-free Thread Pool](https://github.com/kcenon/thread_system/tree/main/samples/lockfree_thread_pool_sample)**: **NEW** - Basic lock-free pool with 2.14x performance improvement
- **[Typed Lock-free Thread Pool](https://github.com/kcenon/thread_system/tree/main/samples/typed_lockfree_thread_pool_sample)**: **NEW** - Priority scheduling with lock-free per-type queues
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
    // Standard mutex-based pool
    class thread_pool {
        auto start() -> std::optional<std::string>;
        auto stop(bool immediately = false) -> void;
        auto enqueue(std::unique_ptr<job>&& job) -> std::optional<std::string>;
        auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> std::optional<std::string>;
        auto get_workers() const -> const std::vector<std::shared_ptr<thread_worker>>&;
    };
    
    // Lock-free pool (2.14x faster on average)
    class lockfree_thread_pool {
        auto start() -> std::optional<std::string>;
        auto stop(bool immediately = false) -> void;
        auto enqueue(std::unique_ptr<job>&& job) -> std::optional<std::string>;
        auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> std::optional<std::string>;
        auto get_workers() const -> const std::vector<std::shared_ptr<thread_worker>>&;
    };
    
    // Lock-free worker with statistics
    class lockfree_thread_worker : public thread_worker {
        struct worker_statistics {
            uint64_t jobs_processed;
            uint64_t total_processing_time_ns;
            uint64_t idle_time_ns;
            uint64_t avg_processing_time_ns;
        };
        
        auto set_batch_processing(bool enabled, size_t batch_size = 32) -> void;
        auto set_backoff_config(const backoff_config& config) -> void;
        auto get_statistics() const -> worker_statistics;
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
