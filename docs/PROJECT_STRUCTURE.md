---
doc_id: "THR-PROJ-004"
doc_title: "Thread System Project Structure"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "thread_system"
category: "PROJ"
---

# Thread System Project Structure

> **SSOT**: This document is the single source of truth for **Thread System Project Structure**.

**Version**: 1.0.0
**Last Updated**: 2026-01-11
**Language**: [English] | [한국어](PROJECT_STRUCTURE.kr.md)

---

## Overview

This document provides a comprehensive guide to the thread_system project structure, including detailed descriptions of all directories, files, and their purposes.

---

## Table of Contents

1. [Directory Organization](#directory-organization)
2. [Source Code Structure](#source-code-structure)
3. [Public Headers](#public-headers)
4. [Implementation Files](#implementation-files)
5. [Examples](#examples)
6. [Tests](#tests)
7. [Documentation](#documentation)
8. [Build Artifacts](#build-artifacts)
9. [Module Dependencies](#module-dependencies)

---

## Directory Organization

### Top-Level Structure

```
thread_system/
├── 📁 include/kcenon/thread/       # Public API headers
├── 📁 src/                         # Implementation files
├── 📁 examples/                    # Example applications
├── 📁 tests/                       # Test suites
├── 📁 docs/                        # Documentation
├── 📁 cmake/                       # CMake modules
├── 📁 scripts/                     # Build and utility scripts
├── 📄 CMakeLists.txt               # Root build configuration
├── 📄 vcpkg.json                   # Dependency manifest
├── 📄 LICENSE                      # BSD 3-Clause License
├── 📄 README.md                    # Project overview
└── 📄 README.kr.md                 # Korean documentation
```

---

## Source Code Structure

### Complete Directory Tree

```
thread_system/
├── 📁 include/kcenon/thread/       # Public headers
│   ├── 📁 core/                    # Core components
│   │   ├── thread_base.h           # Abstract thread class
│   │   ├── thread_pool.h           # Thread pool interface (umbrella header)
│   │   ├── thread_pool_impl.h      # Thread pool template implementations
│   │   ├── thread_pool_fmt.h       # Thread pool std::formatter specializations
│   │   ├── thread_worker.h         # Worker thread
│   │   ├── job.h                   # Job interface
│   │   ├── callback_job.h          # Function-based jobs
│   │   ├── job_queue.h             # Thread-safe queue
│   │   ├── backpressure_job_queue.h # Queue with backpressure
│   │   ├── hazard_pointer.h        # Memory reclamation
│   │   ├── node_pool.h             # Memory pool
│   │   ├── service_registry.h      # Dependency injection
│   │   ├── cancellation_token.h    # Cancellation support
│   │   ├── sync_primitives.h       # Synchronization wrappers
│   │   ├── error_handling.h        # Result<T> pattern
│   │   └── worker_policy.h         # Worker configuration
│   ├── 📁 lockfree/                # Lock-free implementations
│   │   └── lockfree_job_queue.h    # Lock-free MPMC queue
│   ├── 📁 queue/                   # Advanced queue module
│   │   └── adaptive_job_queue.h    # Adaptive dual-mode queue
│   ├── 📁 interfaces/              # Integration interfaces
│   │   ├── logger_interface.h      # Logger abstraction
│   │   ├── monitoring_interface.h  # Monitoring abstraction
│   │   ├── thread_context.h        # Thread context
│   │   └── service_container.h     # Service management
│   ├── 📁 utils/                   # Utilities
│   │   ├── formatter.h             # String formatting
│   │   ├── convert_string.h        # String conversions
│   │   └── span.h                  # Span utilities
│   └── compatibility.h             # Backward compatibility
│
├── 📁 src/                         # Implementation files
│   ├── 📁 core/                    # Core implementations
│   │   ├── thread_base.cpp         # Thread base implementation
│   │   ├── job.cpp                 # Job implementation
│   │   ├── callback_job.cpp        # Callback job implementation
│   │   ├── job_queue.cpp           # Queue implementation
│   │   ├── backpressure_job_queue.cpp # Backpressure queue impl
│   │   ├── hazard_pointer.cpp      # Hazard pointer impl
│   │   ├── node_pool.cpp           # Memory pool impl
│   │   └── cancellation_token.cpp  # Cancellation impl
│   ├── 📁 lockfree/                # Lock-free implementations
│   │   └── lockfree_job_queue.cpp  # Lock-free queue impl
│   ├── 📁 queue/                   # Advanced queue implementations
│   │   └── adaptive_job_queue.cpp  # Adaptive queue impl
│   ├── 📁 impl/                    # Concrete implementations
│   │   ├── 📁 thread_pool/         # Thread pool implementation
│   │   │   ├── thread_pool.cpp     # Pool implementation
│   │   │   └── thread_worker.cpp   # Worker implementation
│   │   └── 📁 typed_pool/          # Typed thread pool
│   │       ├── typed_thread_pool.h # Typed pool header
│   │       └── aging_typed_job_queue.h # Priority aging queue
│   ├── 📁 modules/                 # C++20 Module files (experimental)
│   │   ├── thread.cppm             # Primary module interface (kcenon.thread)
│   │   ├── core.cppm               # Core partition (thread_pool, jobs)
│   │   └── queue.cppm              # Queue partition (job_queue, adaptive)
│   └── 📁 utils/                   # Utility implementations
│       └── convert_string.cpp      # String conversion impl
│
├── 📁 examples/                    # Example applications
│   ├── 📁 thread_pool_sample/      # Basic thread pool usage
│   │   ├── main.cpp                # Entry point
│   │   └── CMakeLists.txt          # Build config
│   ├── 📁 typed_thread_pool_sample/ # Priority scheduling
│   │   ├── main.cpp                # Entry point
│   │   └── CMakeLists.txt          # Build config
│   ├── 📁 adaptive_queue_sample/   # Adaptive queue usage
│   │   ├── main.cpp                # Entry point
│   │   └── CMakeLists.txt          # Build config
│   ├── 📁 hazard_pointer_sample/   # Memory reclamation
│   │   ├── main.cpp                # Entry point
│   │   └── CMakeLists.txt          # Build config
│   └── 📁 integration_example/     # Integration examples
│       ├── main.cpp                # Entry point
│       └── CMakeLists.txt          # Build config
│
├── 📁 tests/                       # All tests
│   ├── 📁 unit/                    # Unit tests
│   │   ├── 📁 thread_base_test/    # Base functionality
│   │   │   ├── thread_base_test.cpp
│   │   │   └── CMakeLists.txt
│   │   ├── 📁 thread_pool_test/    # Pool tests
│   │   │   ├── thread_pool_test.cpp
│   │   │   ├── adaptive_queue_test.cpp
│   │   │   ├── lockfree_queue_test.cpp
│   │   │   └── CMakeLists.txt
│   │   ├── 📁 interfaces_test/     # Interface tests
│   │   │   ├── logger_interface_test.cpp
│   │   │   ├── monitoring_interface_test.cpp
│   │   │   └── CMakeLists.txt
│   │   └── 📁 utilities_test/      # Utility tests
│   │       ├── formatter_test.cpp
│   │       └── CMakeLists.txt
│   └── 📁 benchmarks/              # Performance tests
│       ├── 📁 thread_base_benchmarks/ # Core benchmarks
│       │   ├── thread_base_benchmark.cpp
│       │   └── CMakeLists.txt
│       ├── 📁 thread_pool_benchmarks/ # Pool benchmarks
│       │   ├── thread_pool_benchmark.cpp
│       │   ├── queue_comparison_benchmark.cpp
│       │   └── CMakeLists.txt
│       └── 📁 typed_thread_pool_benchmarks/ # Typed pool benchmarks
│           ├── typed_pool_benchmark.cpp
│           └── CMakeLists.txt
│
├── 📁 docs/                        # Documentation
│   ├── 📁 advanced/                # Advanced topics
│   │   ├── ARCHITECTURE.md         # System architecture
│   │   └── KNOWN_ISSUES.md         # Known issues
│   ├── 📁 performance/             # Performance documentation
│   │   └── BASELINE.md             # Performance baselines
│   ├── 📁 guides/                  # User guides
│   │   ├── API_REFERENCE.md        # API documentation
│   │   ├── USER_GUIDE.md           # User guide
│   │   ├── BUILD_GUIDE.md          # Build instructions
│   │   ├── QUICK_START.md          # Quick start
│   │   ├── INTEGRATION.md          # Integration guide
│   │   ├── BEST_PRACTICES.md       # Best practices
│   │   ├── TROUBLESHOOTING.md      # Troubleshooting
│   │   ├── FAQ.md                  # FAQ
│   │   └── MIGRATION_GUIDE.md      # Migration guide
│   ├── 📁 contributing/            # Contributing docs
│   │   ├── CONTRIBUTING.md         # How to contribute
│   │   └── CODE_OF_CONDUCT.md      # Code of conduct
│   ├── FEATURES.md                 # Detailed features
│   ├── BENCHMARKS.md               # Performance benchmarks
│   ├── PROJECT_STRUCTURE.md        # This file
│   └── PRODUCTION_QUALITY.md       # Quality metrics
│
├── 📁 cmake/                       # CMake modules
│   ├── CompilerWarnings.cmake      # Warning configuration
│   ├── Sanitizers.cmake            # Sanitizer setup
│   └── CodeCoverage.cmake          # Coverage tools
│
└── 📁 scripts/                     # Build and utility scripts
    ├── build.sh                    # Linux/macOS build script
    ├── build.bat                   # Windows build script
    ├── dependency.sh               # Linux/macOS dependency installer
    ├── dependency.bat              # Windows dependency installer
    ├── format.sh                   # Code formatting
    └── test.sh                     # Test runner
```

---

## Public Headers

### Core Module (`include/kcenon/thread/core/`)

#### thread_base.h

**Purpose**: Abstract base class for all thread operations

**Key Components**:
- `thread_base` class: Base thread abstraction
- Lifecycle management (start/stop)
- Customizable hooks (on_initialize, on_execute, on_destroy)
- Thread monitoring and state management

**Used By**: All worker threads, thread pools

---

#### thread_pool.h

**Purpose**: Main thread pool interface

**Key Components**:
- `thread_pool` class: Multi-worker thread pool
- Job submission API
- Worker management
- Adaptive queue support

**Dependencies**: thread_base, job_queue, thread_worker

---

#### thread_worker.h

**Purpose**: Worker thread implementation

**Key Components**:
- `thread_worker` class: Individual worker
- Batch processing support
- Worker statistics tracking

**Dependencies**: thread_base, job_queue

---

#### job.h

**Purpose**: Abstract interface for work units

**Key Components**:
- `job` class: Base job interface
- `execute()` method
- Result<T> return type

**Used By**: All job implementations

---

#### callback_job.h

**Purpose**: Lambda-based job implementation

**Key Components**:
- `callback_job` class: Function wrapper job
- `std::function<result_void()>` support

**Dependencies**: job.h

---

#### job_queue.h

**Purpose**: Thread-safe FIFO queue

**Key Components**:
- `job_queue` class: Mutex-based queue
- `enqueue()` and `dequeue()` methods
- Condition variable for blocking

**Used By**: thread_pool, thread_worker

---

#### backpressure_job_queue.h

**Purpose**: Queue with comprehensive backpressure mechanisms

**Key Components**:
- `backpressure_job_queue` class: Capacity-limited queue with pressure handling
- Multiple backpressure policies (block, drop_oldest, drop_newest, callback, adaptive)
- Rate limiting with token bucket
- Watermark-based pressure detection

**Use Cases**: Production systems, resource-constrained environments

> **Note**: For simple capacity limits, use `job_queue` with `max_size` parameter instead.

---

#### lockfree_job_queue.h

**Purpose**: High-performance lock-free MPMC queue

**Key Components**:
- `lockfree_job_queue` class: Lock-free queue
- Hazard pointer integration
- CAS operations
- Node pooling

**Performance**: 4x faster than mutex-based queues

---

#### adaptive_job_queue.h

**Purpose**: Intelligent dual-mode queue

**Key Components**:
- `adaptive_job_queue` class: Adaptive queue
- Automatic strategy selection
- Runtime performance monitoring
- Seamless mode switching

**Best For**: Variable workload patterns

---

#### hazard_pointer.h

**Purpose**: Safe memory reclamation for lock-free structures

**Key Components**:
- `hazard_pointer` class: Memory reclamation
- `hazard_pointer_guard` RAII wrapper
- Retire list management
- Periodic scanning

**Used By**: lockfree_job_queue, lock-free data structures

---

#### node_pool.h

**Purpose**: Memory pool for efficient node allocation

**Key Components**:
- `node_pool` class: Pre-allocated node cache
- Pool statistics
- Memory reuse

**Used By**: lockfree_job_queue, adaptive_job_queue

---

#### service_registry.h

**Purpose**: Lightweight dependency injection container

**Key Components**:
- `service_registry` class: Service container
- Type-safe registration
- Thread-safe access
- Global singleton pattern

**Use Cases**: Dependency injection, service management

---

#### cancellation_token.h

**Purpose**: Cooperative cancellation mechanism

**Key Components**:
- `cancellation_token` class: Cancellation support
- Linked token creation
- Callback registration
- Thread-safe operations

**Use Cases**: Long-running tasks, hierarchical cancellation

---

#### sync_primitives.h

**Purpose**: Enhanced synchronization wrappers

**Key Components**:
- `scoped_lock_guard`: RAII lock with timeout
- `condition_variable_wrapper`: Enhanced CV
- `atomic_flag_wrapper`: Extended atomic operations
- `shared_mutex_wrapper`: Reader-writer lock

**Use Cases**: Complex synchronization scenarios

---

#### error_handling.h

**Purpose**: Modern error handling with Result<T> pattern

**Key Components**:
- `result<T>` class: Success or error
- `result_void`: Void result type
- `error` class: Error information
- Monadic operations (and_then, or_else, etc.)

**Used Throughout**: All public APIs

---

#### worker_policy.h

**Purpose**: Fine-grained worker behavior configuration

**Key Components**:
- `worker_policy` struct: Policy configuration
- Predefined policies (default, high_performance, low_latency, power_efficient)
- Scheduling policies
- Idle strategies

**Use Cases**: Performance tuning, custom worker behavior

---

### Interfaces Module (`include/kcenon/thread/interfaces/`)

#### logger_interface.h

**Purpose**: Logger abstraction for integration

**Key Components**:
- `logger_interface` class: Abstract logger
- Log level support
- Thread-safe operations

**Implemented By**: External logger_system project

---

#### monitoring_interface.h

**Purpose**: Monitoring abstraction for metrics

**Key Components**:
- `monitoring_interface` class: Abstract monitor
- Metric registration
- Real-time data collection

**Implemented By**: External monitoring_system project

---

#### thread_context.h

**Purpose**: Thread-local context information

**Key Components**:
- `thread_context` class: Context data
- Thread ID tracking
- Custom data storage

---

#### service_container.h

**Purpose**: Service management interface

**Key Components**:
- `service_container` class: Container interface
- Service lifecycle management

---

### Utilities Module (`include/kcenon/thread/utils/`)

#### formatter.h

**Purpose**: String formatting utilities

**Key Components**:
- `format()` function: String formatting
- Cross-platform compatibility (std::format fallback)

---

#### convert_string.h

**Purpose**: String conversion utilities

**Key Components**:
- UTF-8 conversion
- Wide string support
- Platform-specific helpers

---

#### span.h

**Purpose**: Span utilities for array views

**Key Components**:
- `span<T>` class: Non-owning array view
- C++17/20 compatibility

---

## Implementation Files

### Core Implementations (`src/core/`)

| File | Purpose | Lines | Complexity |
|------|---------|-------|------------|
| thread_base.cpp | Thread base implementation | ~200 | Medium |
| job.cpp | Job interface implementation | ~50 | Low |
| callback_job.cpp | Callback job implementation | ~80 | Low |
| job_queue.cpp | Mutex queue implementation | ~150 | Medium |
| backpressure_job_queue.cpp | Backpressure queue implementation | ~350 | Medium |
| lockfree_job_queue.cpp | Lock-free queue implementation | ~400 | High |
| adaptive_job_queue.cpp | Adaptive queue implementation | ~300 | High |
| hazard_pointer.cpp | Memory reclamation implementation | ~350 | High |
| node_pool.cpp | Memory pool implementation | ~120 | Medium |
| cancellation_token.cpp | Cancellation implementation | ~100 | Low |

**Total Core Lines**: ~2,000 lines

---

### Thread Pool Implementations (`src/impl/thread_pool/`)

| File | Purpose | Lines | Complexity |
|------|---------|-------|------------|
| thread_pool.cpp | Thread pool implementation | ~350 | High |
| thread_worker.cpp | Worker implementation | ~200 | Medium |

**Total Thread Pool Lines**: ~550 lines

---

### Typed Thread Pool

**Headers** (`include/kcenon/thread/impl/typed_pool/`):

| File | Purpose | Lines | Complexity |
|------|---------|-------|------------|
| typed_thread_pool.h | Typed pool template header | ~300 | High |
| aging_typed_job_queue.h | Priority aging queue | ~280 | High |
| typed_thread_worker.h | Typed worker template | ~180 | Medium |
| typed_job.h | Typed job base | ~100 | Low |
| job_types.h | Job type definitions | ~120 | Low |
| config.h | Configuration types | ~80 | Low |

**Implementation** (`src/impl/typed_pool/`):

| File | Purpose | Lines | Complexity |
|------|---------|-------|------------|
| typed_thread_pool.cpp | Typed pool instantiation | ~400 | High |
| aging_typed_job_queue.cpp | Priority aging queue impl | ~350 | High |

**Total Typed Pool Lines**: ~750 lines

---

### Utilities (`src/utils/`)

| File | Purpose | Lines | Complexity |
|------|---------|-------|------------|
| convert_string.cpp | String conversion impl | ~150 | Low |

**Total Utilities Lines**: ~150 lines

---

### Code Statistics

**Total Production Code**: ~3,600 lines

**Breakdown by Module**:
- Core: ~2,000 lines (56%)
- Thread Pool: ~550 lines (15%)
- Typed Pool: ~900 lines (25%)
- Utilities: ~150 lines (4%)

**Code Reduction** (vs previous version):
- Previous: ~11,400 lines
- Current: ~2,700 lines (interface-based design)
- **Reduction**: ~8,700 lines (76%)

---

## Examples

### Example Applications

#### thread_pool_sample

**Purpose**: Demonstrates basic thread pool usage

**Features**:
- Worker creation
- Job submission
- Adaptive queue usage
- Performance monitoring

**File**: `examples/thread_pool_sample/main.cpp` (~150 lines)

---

#### typed_thread_pool_sample

**Purpose**: Priority-based task scheduling

**Features**:
- Type-based job routing
- Priority scheduling (RealTime, Batch, Background)
- Per-type queue optimization
- Statistics tracking

**File**: `examples/typed_thread_pool_sample/main.cpp` (~200 lines)

---

#### adaptive_queue_sample

**Purpose**: Adaptive queue fundamentals

**Features**:
- Queue mode switching
- Performance comparison
- Contention handling

**File**: `examples/adaptive_queue_sample/main.cpp` (~180 lines)

---

#### hazard_pointer_sample

**Purpose**: Safe memory reclamation demonstration

**Features**:
- Hazard pointer usage
- Lock-free data structure integration
- Memory safety validation

**File**: `examples/hazard_pointer_sample/main.cpp` (~220 lines)

---

#### integration_example

**Purpose**: Integration with logger and monitoring systems

**Features**:
- Logger integration
- Monitoring integration
- Service registry usage
- Complete application template

**File**: `examples/integration_example/main.cpp` (~300 lines)

---

## Tests

### Unit Tests (`tests/unit/`)

#### thread_base_test

**Tests**:
- Lifecycle (start/stop)
- State management
- Error handling
- Thread safety

**Files**: ~500 lines of tests

---

#### thread_pool_test

**Tests**:
- Worker management
- Job submission
- Adaptive queue behavior
- Lock-free queue correctness
- Batch processing
- Shutdown scenarios

**Files**: ~800 lines of tests

---

#### interfaces_test

**Tests**:
- Logger interface
- Monitoring interface
- Service registry
- Thread context

**Files**: ~300 lines of tests

---

#### utilities_test

**Tests**:
- Formatter
- String conversion
- Span utilities

**Files**: ~200 lines of tests

---

### Benchmarks (`tests/benchmarks/`)

#### thread_base_benchmarks

**Measures**:
- Thread creation time
- Start/stop overhead
- Lifecycle performance

**Files**: ~300 lines

---

#### thread_pool_benchmarks

**Measures**:
- Job throughput
- Queue performance (mutex vs lock-free vs adaptive)
- Worker scaling
- Latency distribution

**Files**: ~500 lines

---

#### typed_thread_pool_benchmarks

**Measures**:
- Type-based routing performance
- Priority scheduling overhead
- Adaptive typed queue performance

**Files**: ~400 lines

---

### Test Statistics

**Total Test Code**: ~3,000 lines

**Coverage**: ~52% (tracked by codecov)

---

## Documentation

### Design Documents (`docs/design/`)

| File | Purpose | Lines |
|------|---------|-------|
| QUEUE_POLICY_DESIGN.md | Queue policy interface design | ~450 |

---

### Advanced Documentation (`docs/advanced/`)

| File | Purpose | Lines |
|------|---------|-------|
| ARCHITECTURE.md | System architecture | ~800 |
| KNOWN_ISSUES.md | Known issues | ~300 |

### Performance Documentation (`docs/performance/`)

| File | Purpose | Lines |
|------|---------|-------|
| BASELINE.md | Performance baselines | ~500 |

---

### User Guides (`docs/guides/`)

| File | Purpose | Lines |
|------|---------|-------|
| API_REFERENCE.md | Complete API docs | ~1,200 |
| USER_GUIDE.md | User guide | ~600 |
| BUILD_GUIDE.md | Build instructions | ~400 |
| QUICK_START.md | Quick start | ~300 |
| INTEGRATION.md | Integration guide | ~500 |
| BEST_PRACTICES.md | Best practices | ~400 |
| TROUBLESHOOTING.md | Troubleshooting | ~350 |
| FAQ.md | FAQ | ~250 |
| MIGRATION_GUIDE.md | Migration guide | ~300 |

---

### Contributing (`docs/contributing/`)

| File | Purpose | Lines |
|------|---------|-------|
| CONTRIBUTING.md | How to contribute | ~400 |
| CODE_OF_CONDUCT.md | Code of conduct | ~150 |

---

### Top-Level Docs

| File | Purpose | Lines |
|------|---------|-------|
| FEATURES.md | Detailed features | ~1,000 |
| BENCHMARKS.md | Performance data | ~1,200 |
| PROJECT_STRUCTURE.md | This file | ~800 |
| PRODUCTION_QUALITY.md | Quality metrics | ~600 |

**Total Documentation**: ~9,050 lines

---

## Build Artifacts

### Build Directory Structure

```
build/
├── 📁 bin/                    # Executable files
│   ├── thread_pool_sample
│   ├── typed_thread_pool_sample
│   ├── adaptive_queue_sample
│   ├── hazard_pointer_sample
│   ├── integration_example
│   ├── thread_base_test
│   ├── thread_pool_test
│   ├── interfaces_test
│   ├── utilities_test
│   ├── thread_base_benchmark
│   ├── thread_pool_benchmark
│   └── typed_pool_benchmark
│
├── 📁 lib/                    # Static libraries
│   ├── libthread_base.a
│   ├── libthread_pool.a
│   ├── libtyped_thread_pool.a
│   └── libutilities.a
│
├── 📁 include/                # Public headers (for installation)
│   └── kcenon/
│       └── thread/
│           ├── core/
│           ├── interfaces/
│           └── utils/
│
└── 📁 CMakeFiles/             # CMake build metadata
```

---

### Build Output Sizes

**Binary Sizes** (Release build, stripped):

| Binary | Size | Description |
|--------|------|-------------|
| thread_pool_sample | ~80 KB | Basic example |
| typed_thread_pool_sample | ~95 KB | Typed pool example |
| thread_base_test | ~120 KB | Unit tests |
| thread_pool_test | ~150 KB | Pool tests |
| libthread_base.a | ~40 KB | Core library |
| libthread_pool.a | ~50 KB | Pool library |
| libtyped_thread_pool.a | ~60 KB | Typed pool library |

**Total Binary Size**: ~600 KB

---

## Module Dependencies

### Dependency Graph

```
utilities (no dependencies)
    │
    └──> thread_base
             │
             ├──> thread_pool
             │       │
             │       └──> thread_worker
             │
             └──> typed_thread_pool
                        │
                        └── aging_typed_job_queue (policy_queue based)
```

---

### External Dependencies

**Required**:
- C++20 compiler (GCC 13+, Clang 17+, MSVC 2022+)
- CMake 3.20+

**Optional** (via vcpkg):
- Google Test (for unit tests)
- Google Benchmark (for benchmarks)

**Optional Integration Projects**:
- logger_system (separate repository)
- monitoring_system (separate repository)
- integrated_thread_system (complete examples)

---

### Header Dependencies

**thread_pool.h depends on**:
- thread_base.h
- job_queue.h (or adaptive_job_queue.h)
- thread_worker.h
- error_handling.h

**typed_thread_pool.h depends on**:
- thread_base.h
- aging_typed_job_queue.h
- error_handling.h

**lockfree_job_queue.h depends on**:
- hazard_pointer.h
- node_pool.h
- error_handling.h

---

## File Naming Conventions

### Header Files

- **Interfaces**: `*_interface.h`
- **Templates**: `*.h` with `*.tpp` for implementation
- **Core classes**: `snake_case.h`
- **Utilities**: Descriptive names (e.g., `formatter.h`)

### Implementation Files

- **Class implementations**: Match header name with `.cpp` extension
- **Template implementations**: `*.tpp` (included in header)

### Tests

- **Unit tests**: `*_test.cpp`
- **Benchmarks**: `*_benchmark.cpp`

### Examples

- **Samples**: `*_sample/main.cpp`

---

## Code Organization Principles

### 1. Interface-Based Design

All major components provide abstract interfaces:
- `logger_interface.h`
- `monitoring_interface.h`
- `job` (abstract base)

**Benefits**:
- Clean separation of concerns
- Easy mocking for tests
- Pluggable implementations

---

### 2. Header-Only vs Compiled

**Header-Only**:
- Templates (typed_thread_pool.h)
- Utilities (formatter.h)

**Compiled**:
- Core classes (thread_base.cpp)
- Heavy implementations (lockfree_job_queue.cpp)

**Rationale**: Balance compile time vs flexibility

---

### 3. Modular Architecture

Each module is self-contained:
- Clear public API (`include/`)
- Hidden implementation details (`src/`)
- Minimal inter-module coupling

---

## Build Configuration

### CMake Structure

**Root CMakeLists.txt**:
- Project setup
- Global options
- Subdirectory inclusion

**Module CMakeLists.txt**:
- Library definitions
- Target properties
- Dependencies

**Example CMakeLists.txt**:
- Executable definitions
- Linking

**Test CMakeLists.txt**:
- Test executable definitions
- Google Test integration

---

## Summary

### Project Statistics

| Category | Count | Lines |
|----------|-------|-------|
| **Source Files** | ~30 | ~3,600 |
| **Header Files** | ~35 | ~2,500 |
| **Test Files** | ~15 | ~3,000 |
| **Example Files** | ~5 | ~1,050 |
| **Documentation** | ~20 | ~9,050 |
| **Total** | ~105 | ~19,200 |

---

### Key Directories

1. **`include/kcenon/thread/`**: Public API (use this)
2. **`src/`**: Implementation details (don't depend on directly)
3. **`examples/`**: Learn by example
4. **`tests/`**: Ensure correctness
5. **`docs/`**: Comprehensive documentation

---

**See Also**:
- [Feature Documentation](FEATURES.md)
- [Performance Benchmarks](BENCHMARKS.md)
- [Architecture Guide](advanced/ARCHITECTURE.md)
- [Build Guide](guides/BUILD_GUIDE.md)

---

**Last Updated**: 2026-01-11
**Maintained by**: kcenon@naver.com
