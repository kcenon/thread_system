# Quick Start Guide

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
./build/bin/thread_pool_sample
```

---

## Your First Thread Pool

Create a simple thread pool application:

```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/jobs/callback_job.h>

#include <iostream>
#include <memory>
#include <vector>

using namespace kcenon::thread;

int main() {
    // 1. Create a thread pool
    auto pool = std::make_shared<thread_pool>("MyFirstPool");

    // 2. Add workers (one per CPU core)
    std::vector<std::unique_ptr<thread_worker>> workers;
    for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
        workers.push_back(std::make_unique<thread_worker>());
    }
    pool->enqueue_batch(std::move(workers));

    // 3. Start the pool
    pool->start();

    // 4. Submit tasks
    for (int i = 0; i < 10; ++i) {
        pool->submit_task([i]() {
            std::cout << "Processing task " << i << "\n";
        });
    }

    // 5. Clean shutdown (wait for all tasks to complete)
    pool->shutdown_pool(false);

    std::cout << "All tasks completed!\n";
    return 0;
}
```

### Building Your Application

Add to your `CMakeLists.txt`:

```cmake
# Using FetchContent
include(FetchContent)
FetchContent_Declare(
    thread_system
    GIT_REPOSITORY https://github.com/kcenon/thread_system.git
    GIT_TAG main
)
FetchContent_MakeAvailable(thread_system)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE
    thread_base
    thread_pool
    utilities
)
```

---

## Key Concepts

### Thread Pool
The core component for managing worker threads and executing jobs.

```cpp
auto pool = std::make_shared<thread_pool>("PoolName");
pool->start();
// ... submit tasks ...
pool->shutdown_pool(false);  // false = wait for completion
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
// Using convenience API
pool->submit_task([]() {
    // Your work here
});

// Using callback_job for more control
pool->execute(std::make_unique<callback_job>([]() -> kcenon::common::VoidResult {
    // Your work here
    return kcenon::common::ok();
}));
```

---

## Common Patterns

### Parallel Processing

```cpp
std::atomic<int> counter{0};
for (int i = 0; i < 1000; ++i) {
    pool->submit_task([&counter]() {
        counter++;
    });
}
```

### Error Handling

```cpp
pool->execute(std::make_unique<callback_job>([]() -> kcenon::common::VoidResult {
    if (some_error_condition) {
        return kcenon::thread::make_error_result(kcenon::thread::error_code::operation_failed, "Task failed");
    }
    return kcenon::common::ok();
}));
```

### Graceful Shutdown

```cpp
// Wait for all tasks to complete
pool->shutdown_pool(false);

// Or force immediate stop
pool->shutdown_pool(true);
```

---

## Next Steps

- **[Build Guide](BUILD_GUIDE.md)** - Detailed build instructions for all platforms
- **[User Guide](../advanced/USER_GUIDE.md)** - Comprehensive usage guide
- **[API Reference](../advanced/02-API_REFERENCE.md)** - Complete API documentation
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
