[![CI](https://github.com/kcenon/thread_system/actions/workflows/ci.yml/badge.svg?branch=phase-0-foundation)](https://github.com/kcenon/thread_system/actions/workflows/ci.yml)
[![Code Coverage](https://github.com/kcenon/thread_system/actions/workflows/coverage.yml/badge.svg?branch=phase-0-foundation)](https://github.com/kcenon/thread_system/actions/workflows/coverage.yml)
[![Static Analysis](https://github.com/kcenon/thread_system/actions/workflows/static-analysis.yml/badge.svg?branch=phase-0-foundation)](https://github.com/kcenon/thread_system/actions/workflows/static-analysis.yml)
[![codecov](https://codecov.io/gh/kcenon/thread_system/branch/main/graph/badge.svg)](https://codecov.io/gh/kcenon/thread_system)

# Thread System

> **Language:** **English** | [한국어](README.kr.md)

A modern C++20 multithreading framework designed to democratize concurrent programming.

---

## Overview

Thread System is a comprehensive multithreading framework that provides intuitive abstractions and robust implementations for building high-performance, thread-safe applications.

**Key Value Propositions**:
- **Well-Tested**: 95%+ CI/CD success rate, zero ThreadSanitizer warnings, 72% code coverage
- **High Performance**: 1.16M jobs/second baseline, 4x faster lock-free queues, adaptive optimization
- **Developer Friendly**: Intuitive API, comprehensive documentation, rich examples
- **Flexible Architecture**: Modular design with optional logger/monitoring integration
- **Cross-Platform**: Linux, macOS, Windows support with multiple compilers

**Latest Updates**:
- ✅ Queue API simplified: 8 implementations → 2 public types (adaptive_job_queue, job_queue)
- ✅ Hazard Pointer implementation completed - lock-free queue safe for production
- ✅ 4x performance improvement with lock-free queue (71 μs vs 291 μs)
- ✅ Enhanced synchronization primitives and cancellation tokens
- ✅ All CI/CD pipelines green (ThreadSanitizer & AddressSanitizer clean)

---

## Quick Start

### Requirements

- **C++20 Compiler**: GCC 13+ / Clang 17+ / MSVC 2022+
- **CMake 3.20+**
- **[common_system](https://github.com/kcenon/common_system)**: Required dependency (must be cloned alongside thread_system)

### Installation

```bash
# Clone repositories (common_system is required)
git clone https://github.com/kcenon/common_system.git
git clone https://github.com/kcenon/thread_system.git
cd thread_system

# Install dependencies
./scripts/dependency.sh  # Linux/macOS
./scripts/dependency.bat # Windows

# Build
./scripts/build.sh       # Linux/macOS
./scripts/build.bat      # Windows

# Run examples
./build/bin/thread_pool_sample
```

### Basic Usage

```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/jobs/callback_job.h>

using namespace kcenon::thread;

int main() {
    // Create thread pool
    auto pool = std::make_shared<thread_pool>("MyPool");

    // Add workers
    std::vector<std::unique_ptr<thread_worker>> workers;
    for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
        workers.push_back(std::make_unique<thread_worker>());
    }
    pool->enqueue_batch(std::move(workers));

    // Start processing
    pool->start();

    // Submit jobs (convenience API)
    for (int i = 0; i < 1000; ++i) {
        pool->submit_task([i]() {
            std::cout << "Processing job " << i << "\n";
        });
    }

    // Clean shutdown
    pool->shutdown_pool(false);  // Wait for completion
    return 0;
}
```

**📖 [Full Getting Started Guide →](docs/guides/QUICK_START.md)**

---

## Core Features

### Thread Pool System
- **Standard Thread Pool**: Multi-worker pool with adaptive queue support
- **Typed Thread Pool**: Priority-based scheduling with type-aware routing
- **Dynamic Worker Management**: Add/remove workers at runtime
- **Dual API**: Result-based (detailed errors) and convenience API (simple)

### Queue Implementations (Simplified to 2 Public Types)
Following Kent Beck's Simple Design principle, we now offer only 2 public queue types:
- **Adaptive Queue** (Recommended): Auto-optimizing queue that switches between mutex and lock-free modes
- **Standard Queue**: Mutex-based FIFO with blocking wait and exact size tracking (supports optional size limits)
- **Queue Factory**: Requirements-based queue creation with compile-time selection
- **Capability Introspection**: Runtime query for queue characteristics (exact_size, lock_free, etc.)

> **Note**: `bounded_job_queue` is now merged into `job_queue` with optional `max_size` parameter.
> Internal implementations (`lockfree_job_queue`, `concurrent_queue`) are in `detail::` namespace.

### Advanced Features
- **Hazard Pointers**: Safe memory reclamation for lock-free structures
- **Cancellation Tokens**: Cooperative cancellation with hierarchical support
- **Service Registry**: Lightweight dependency injection container
- **Synchronization Primitives**: Enhanced wrappers with timeouts and predicates
- **Worker Policies**: Fine-grained control (scheduling, idle behavior, CPU affinity)

### Error Handling
- `thread::result<T>` and `thread::result_void` wrap `common::Result` but keep thread-specific helpers (see `include/kcenon/thread/core/error_handling.h`).
- Use `result.has_error()` / `result.get_error()` when working inside the thread_system repo, and convert to `common::error_info` via `detail::to_common_error(...)` when crossing module boundaries.
- When updating shared documentation, call out that the thread-specific wrappers intentionally expose `.get_error()` for backward compatibility even though other systems rely on `.error()`.

**📚 [Detailed Features →](docs/FEATURES.md)**

---

## Performance Highlights

**Platform**: Apple M1 @ 3.2GHz, 16GB RAM, macOS Sonoma

| Metric | Value | Configuration |
|--------|-------|---------------|
| **Production Throughput** | 1.16M jobs/s | 10 workers, real workload |
| **Typed Pool** | 1.24M jobs/s | 6 workers, 6.9% faster |
| **Lock-free Queue** | 71 μs/op | 4x faster than mutex |
| **Job Latency (P50)** | 77 ns | Sub-microsecond |
| **Memory Baseline** | <1 MB | 8 workers |
| **Scaling Efficiency** | 96% | Up to 8 workers |

### Queue Performance Comparison

| Queue Type | Latency | Best For |
|-----------|---------|----------|
| Mutex Queue | 96 ns | Low contention (1-2 threads) |
| Adaptive (auto) | 96-320 ns | Variable workload |
| Lock-free | 320 ns | High contention (8+ threads), 37% faster |

### Worker Scaling

| Workers | Speedup | Efficiency | Rating |
|---------|---------|------------|--------|
| 2 | 2.0x | 99% | 🥇 Excellent |
| 4 | 3.9x | 97.5% | 🥇 Excellent |
| 8 | 7.7x | 96% | 🥈 Very Good |
| 16 | 15.0x | 94% | 🥈 Very Good |

**⚡ [Full Benchmarks →](docs/BENCHMARKS.md)**

---

## Architecture Overview

### Modular Design

```
┌─────────────────────────────────────────┐
│         Thread System Core              │
│  ┌───────────────────────────────────┐  │
│  │  Thread Pool & Workers            │  │
│  │  - Standard Pool                  │  │
│  │  - Typed Pool (Priority)          │  │
│  │  - Dynamic Worker Management      │  │
│  └───────────────────────────────────┘  │
│  ┌───────────────────────────────────┐  │
│  │  Queue Implementations            │  │
│  │  - Adaptive Queue (recommended)   │  │
│  │  - Standard Queue (blocking wait) │  │
│  │  - Internal: lock-free MPMC       │  │
│  └───────────────────────────────────┘  │
│  ┌───────────────────────────────────┐  │
│  │  Advanced Features                │  │
│  │  - Hazard Pointers                │  │
│  │  - Cancellation Tokens            │  │
│  │  - Service Registry               │  │
│  │  - Worker Policies                │  │
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘

Optional Integration Projects (Separate Repos):
┌──────────────────┐  ┌──────────────────┐
│  Logger System   │  │ Monitoring System│
│  - Async logging │  │ - Real-time      │
│  - Multi-target  │  │   metrics        │
│  - High-perf     │  │ - Observability  │
└──────────────────┘  └──────────────────┘
```

### Key Components

- **thread_base**: Abstract thread class with lifecycle management
- **thread_pool**: Multi-worker pool with adaptive queues
- **typed_thread_pool**: Priority scheduling with type-aware routing
- **adaptive_job_queue**: Auto-optimizing queue (recommended default)
- **job_queue**: Mutex-based queue with blocking wait support
- **hazard_pointer**: Safe memory reclamation for lock-free structures
- **cancellation_token**: Cooperative cancellation mechanism

**🏗️ [Architecture Guide →](docs/advanced/ARCHITECTURE.md)**

---

## Documentation

### Getting Started
- 📖 [Quick Start Guide](docs/guides/QUICK_START.md) - Get up and running in 5 minutes
- 🔧 [Build Guide](docs/guides/BUILD_GUIDE.md) - Detailed build instructions
- 🚀 [User Guide](docs/advanced/USER_GUIDE.md) - Comprehensive usage guide

### Core Documentation
- 📚 [Features](docs/FEATURES.md) - Detailed feature descriptions
- ⚡ [Benchmarks](docs/BENCHMARKS.md) - Comprehensive performance data
- 📋 [API Reference](docs/advanced/API_REFERENCE.md) - Complete API documentation
- 🏛️ [Architecture](docs/advanced/ARCHITECTURE.md) - System design and internals

### Advanced Topics
- 🔬 [Performance Baseline](docs/performance/BASELINE.md) - Baseline metrics and regression detection
- 🛡️ [Production Quality](docs/PRODUCTION_QUALITY.md) - CI/CD, testing, quality metrics
- 🧩 [C++20 Concepts](docs/advanced/CPP20_CONCEPTS.md) - Type-safe constraints for thread operations
- 📁 [Project Structure](docs/PROJECT_STRUCTURE.md) - Detailed codebase organization
- ⚠️ [Known Issues](docs/advanced/KNOWN_ISSUES.md) - Current limitations and workarounds
- 📗 [Queue Selection Guide](docs/advanced/QUEUE_SELECTION_GUIDE.md) - Choosing the right queue
- 🔄 [Queue Backward Compatibility](docs/QUEUE_BACKWARD_COMPATIBILITY.md) - Migration and compatibility

### Development
- 🤝 [Contributing](docs/contributing/CONTRIBUTING.md) - How to contribute
- 🔍 [Troubleshooting](docs/guides/TROUBLESHOOTING.md) - Common issues and solutions
- ❓ [FAQ](docs/guides/FAQ.md) - Frequently asked questions
- 🔄 [Migration Guide](docs/advanced/MIGRATION.md) - Upgrade from older versions

### API Documentation (Doxygen)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target docs
# Open documents/html/index.html
```

---

## Ecosystem Integration

### Project Ecosystem

This project is part of a modular ecosystem:

```
thread_system (core interfaces)
    ↑                    ↑
logger_system    monitoring_system
    ↑                    ↑
    └── integrated_thread_system ──┘
```

### Optional Components

- **[logger_system](https://github.com/kcenon/logger_system)**: High-performance asynchronous logging
- **[monitoring_system](https://github.com/kcenon/monitoring_system)**: Real-time metrics and monitoring
- **[integrated_thread_system](https://github.com/kcenon/integrated_thread_system)**: Complete integration examples

### Integration Benefits

- **Plug-and-play**: Use only the components you need
- **Interface-driven**: Clean abstractions enable easy swapping
- **Performance-optimized**: Each system optimized for its domain
- **Unified ecosystem**: Consistent API design

**🌐 [Ecosystem Integration Guide →](../ECOSYSTEM_INTEGRATION.md)**

---

## C++20 Module Support

Thread System provides C++20 module support as an alternative to the header-based interface.

### Requirements for Modules
- **CMake 3.28+**
- **Clang 16+, GCC 14+, or MSVC 2022 17.4+**
- **common_system** with module support

### Building with Modules

```bash
cmake -B build -DTHREAD_BUILD_MODULES=ON
cmake --build build
```

### Using Modules

```cpp
// Instead of includes:
// #include <kcenon/thread/core/thread_pool.h>

// Use module import:
import kcenon.thread;

int main() {
    using namespace kcenon::thread;

    auto pool = std::make_shared<thread_pool>("MyPool");
    pool->start();
    // ...
}
```

### Module Structure

| Module | Contents |
|--------|----------|
| `kcenon.thread` | Primary module (imports all partitions) |
| `kcenon.thread:core` | Thread pool, workers, jobs, cancellation |
| `kcenon.thread:queue` | Queue implementations (job_queue, adaptive_job_queue) |

> **Note**: C++20 modules are experimental. The header-based interface remains the primary API.

---

## CMake Integration

### Basic Integration

```cmake
# Using as subdirectory
add_subdirectory(thread_system)

target_link_libraries(your_target PRIVATE
    thread_base
    thread_pool
    utilities
)
```

### With FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
    thread_system
    GIT_REPOSITORY https://github.com/kcenon/thread_system.git
    GIT_TAG main
)
FetchContent_MakeAvailable(thread_system)

target_link_libraries(your_target PRIVATE thread_system)
```

### With vcpkg

The package is available as `kcenon-thread-system` in the kcenon vcpkg registry:

```json
{
  "dependencies": [
    "kcenon-thread-system"
  ]
}
```

> **Note**: This package requires [kcenon-common-system](https://github.com/kcenon/common_system)
> which must be cloned alongside thread_system. The vcpkg integration for common_system
> will be available once the kcenon vcpkg registry is established.

Optional features:
- `testing`: Includes gtest and benchmark for unit tests
- `logging`: Enables spdlog integration
- `development`: All testing and logging dependencies

```json
{
  "dependencies": [
    {
      "name": "kcenon-thread-system",
      "features": ["testing", "logging"]
    }
  ]
}
```

---

## Examples

### Sample Applications

- **[thread_pool_sample](examples/thread_pool_sample)**: Basic thread pool usage with adaptive queues
- **[typed_thread_pool_sample](examples/typed_thread_pool_sample)**: Priority-based task scheduling
- **[adaptive_queue_sample](examples/adaptive_queue_sample)**: Queue performance comparison
- **[queue_factory_sample](examples/queue_factory_sample)**: Requirements-based queue creation
- **[queue_capabilities_sample](examples/queue_capabilities_sample)**: Runtime capability introspection
- **[hazard_pointer_sample](examples/hazard_pointer_sample)**: Lock-free memory reclamation
- **[integration_example](examples/integration_example)**: Full integration with logger/monitoring

### Running Examples

```bash
# Build all examples
cmake -B build
cmake --build build

# Run specific example
./build/bin/thread_pool_sample
./build/bin/typed_thread_pool_sample
```

---

## Production Quality

### Quality Metrics

- ✅ **95%+ CI/CD Success Rate** across all platforms
- ✅ **72% Code Coverage** with comprehensive test suite
- ✅ **Zero ThreadSanitizer Warnings** in production code
- ✅ **Zero AddressSanitizer Leaks** - 100% RAII compliance
- ✅ **Multi-Platform Support**: Linux, macOS, Windows
- ✅ **Multiple Compilers**: GCC 11+, Clang 14+, MSVC 2022+

### Thread Safety

**70+ Thread Safety Tests** covering:
- Single producer/consumer
- Multi-producer/multi-consumer (MPMC)
- Adaptive queue mode switching
- Edge cases (shutdown, overflow, underflow)

**ThreadSanitizer Results**: ✅ CLEAN
- Zero data races
- Zero deadlocks
- Safe memory access patterns

### Resource Management

**RAII Compliance: Grade A**
- 100% smart pointer usage
- No manual memory management
- Exception-safe cleanup
- Zero memory leaks (AddressSanitizer verified)

**🛡️ [Production Quality Details →](docs/PRODUCTION_QUALITY.md)**

---

## Platform Support

### Supported Platforms

| Platform | Compilers | Status |
|----------|-----------|--------|
| **Linux** | GCC 11+, Clang 14+ | ✅ Fully supported |
| **macOS** | Apple Clang 14+, GCC 11+ | ✅ Fully supported |
| **Windows** | MSVC 2022+ | ✅ Fully supported |

### Architecture Support

| Architecture | Status |
|--------------|--------|
| x86-64 | ✅ Fully supported |
| ARM64 (Apple Silicon, Graviton) | ✅ Fully supported |
| ARMv7 | ⚠️ Untested |
| RISC-V | ⚠️ Untested |

---

## Contributing

We welcome contributions! Please see our [Contributing Guide](docs/contributing/CONTRIBUTING.md) for details.

### Development Workflow

1. Fork the repository
2. Create feature branch (`git checkout -b feature/amazing-feature`)
3. Make changes with tests
4. Run tests locally (`ctest --verbose`)
5. Commit changes (`git commit -m 'Add amazing feature'`)
6. Push to branch (`git push origin feature/amazing-feature`)
7. Open Pull Request

### Code Standards

- Follow modern C++ best practices
- Use RAII and smart pointers
- Write comprehensive unit tests
- Maintain consistent formatting (clang-format)
- Update documentation

---

## Support

- **Issues**: [GitHub Issues](https://github.com/kcenon/thread_system/issues)
- **Discussions**: [GitHub Discussions](https://github.com/kcenon/thread_system/discussions)
- **Email**: kcenon@naver.com

---

## License

This project is licensed under the BSD 3-Clause License - see the [LICENSE](LICENSE) file for details.

---

## Acknowledgments

- Inspired by modern concurrent programming patterns and best practices
- Built with C++20 features (GCC 11+, Clang 14+, MSVC 2022+) for maximum performance and safety
- Maintained by kcenon@naver.com

---

<p align="center">
  Made with ❤️ by 🍀☀🌕🌥 🌊
</p>
