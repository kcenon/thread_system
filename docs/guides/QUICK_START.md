---
doc_id: "THR-GUID-022"
doc_title: "Quick Start Guide"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "thread_system"
category: "GUID"
---

# Quick Start Guide

> **SSOT**: This document is the single source of truth for **Quick Start Guide**.

> **Language:** **English** | [한국어](QUICK_START.kr.md)

Get up and running with Thread System in 5 minutes.

---

## Prerequisites

- CMake 3.16 or later
- C++20 capable compiler (GCC 11+, Clang 14+, MSVC 2022+)
- Git
- **[common_system](https://github.com/kcenon/common_system)** - Required dependency (must be cloned alongside thread_system)

## Installation

### 1. Clone the Repositories

```bash
# Clone common_system first (required dependency)
git clone https://github.com/kcenon/common_system.git

# Clone thread_system alongside common_system
git clone https://github.com/kcenon/thread_system.git
cd thread_system
```

> **Note:** Both repositories must be in the same parent directory for the build to work correctly.

### 2. Install Dependencies

```bash
# Linux/macOS
./scripts/dependency.sh

# Windows
./scripts/dependency.bat
```

### 3. Build

```bash
# Linux/macOS
./scripts/build.sh

# Windows
./scripts/build.bat

# Or using CMake directly
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### 4. Verify Installation

```bash
# Run the sample application
./build/bin/minimal_thread_pool
```

---

## Your First Thread Pool

Create a simple thread pool application:

```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/thread_worker.h>

#include <algorithm>
#include <future>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

using namespace kcenon::thread;

int main() {
    // 1. Create a thread pool
    auto pool = std::make_shared<thread_pool>("MyFirstPool");

    // 2. Add workers (one per CPU core)
    std::vector<std::unique_ptr<thread_worker>> workers;
    const unsigned worker_count = std::max(1u, std::thread::hardware_concurrency());
    for (unsigned i = 0; i < worker_count; ++i) {
        workers.push_back(std::make_unique<thread_worker>());
    }
    pool->enqueue_batch(std::move(workers));

    // 3. Start the pool
    pool->start();

    // 4. Submit tasks; submit() returns a std::future for each one
    std::vector<std::future<void>> tasks;
    for (int i = 0; i < 10; ++i) {
        tasks.push_back(pool->submit([i]() {
            std::cout << "Processing task " << i << "\n";
        }));
    }
    for (auto& task : tasks) {
        task.get();  // Wait for each task to complete
    }

    // 5. Clean shutdown
    pool->stop(false);

    std::cout << "All tasks completed!\n";
    return 0;
}
```

### Building Your Application

Add to your `CMakeLists.txt`:

```cmake
# Using FetchContent: thread_system does not fetch common_system itself
include(FetchContent)
FetchContent_Declare(
    common_system
    GIT_REPOSITORY https://github.com/kcenon/common_system.git
    GIT_TAG v0.2.0
)
FetchContent_MakeAvailable(common_system)
set(COMMON_SYSTEM_INCLUDE_DIR "${common_system_SOURCE_DIR}/include")

FetchContent_Declare(
    thread_system
    GIT_REPOSITORY https://github.com/kcenon/thread_system.git
    GIT_TAG v1.0.0  # Pin to a specific release tag; do NOT use main
)
FetchContent_MakeAvailable(thread_system)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE thread_system)
```

---

## Key Concepts

### Thread Pool
The core component for managing worker threads and executing jobs.

```cpp
auto pool = std::make_shared<thread_pool>("PoolName");
pool->start();
// ... submit tasks ...
pool->stop(false);  // false = let workers finish their current job
```

### Workers
Threads that process jobs from the queue.

```cpp
// Create workers
std::vector<std::unique_ptr<thread_worker>> workers;
workers.push_back(std::make_unique<thread_worker>());
pool->enqueue_batch(std::move(workers));
```

### Jobs
Units of work to be executed.

```cpp
// Using submit(), which returns a std::future
auto future = pool->submit([]() {
    // Your work here
});

// Using callback_job for more control
pool->enqueue(std::make_unique<callback_job>([]() -> kcenon::common::VoidResult {
    // Your work here
    return kcenon::common::ok();
}));
```

---

## Common Patterns

### Parallel Processing

```cpp
std::atomic<int> counter{0};
std::vector<std::future<void>> futures;
for (int i = 0; i < 1000; ++i) {
    futures.push_back(pool->submit([&counter]() {
        counter++;
    }));
}
for (auto& f : futures) {
    f.get();  // Wait until every task has run
}
```

### Error Handling

```cpp
pool->enqueue(std::make_unique<callback_job>([]() -> kcenon::common::VoidResult {
    if (some_error_condition) {
        return kcenon::thread::make_error_result(kcenon::thread::error_code::job_execution_failed, "Task failed");
    }
    return kcenon::common::ok();
}));
```

### Graceful Shutdown

```cpp
// Wait on the futures of submitted tasks first; stop(false) then lets
// each worker finish its current job (queued jobs are not drained)
pool->stop(false);

// Or stop immediately; ongoing jobs may be interrupted
pool->stop(true);
```

---

## Next Steps

- **[Build Guide](BUILD_GUIDE.md)** - Detailed build instructions for all platforms
- **[User Guide](../advanced/USER_GUIDE.md)** - Comprehensive usage guide
- **[API Reference](../advanced/API_REFERENCE.md)** - Complete API documentation
- **[Examples](../../examples/)** - More sample applications

---

## Troubleshooting

### Common Issues

**Build fails with C++20 errors:**
```bash
# Ensure you have a compatible compiler
g++ --version  # Should be 11+
clang++ --version  # Should be 14+
```

**vcpkg installation fails:**
```bash
rm -rf vcpkg
./scripts/dependency.sh
```

**Tests fail to run:**
```bash
cd build && ctest --verbose
```

For more troubleshooting help, see [FAQ](FAQ.md).

---

*Last Updated: 2025-12-10*
