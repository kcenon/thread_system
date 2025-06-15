# Getting Started with Thread System

Welcome to Thread System! This comprehensive guide covers installation and basic usage to get you up and running quickly.

## System Requirements

| Component | Requirement |
|-----------|-------------|
| **Compiler** | GCC 9+, Clang 10+, MSVC 2019+ |
| **C++ Standard** | C++17 (C++20 recommended) |
| **CMake** | 3.16 or later |
| **Memory** | 512 MB RAM for compilation |
| **Disk Space** | 200 MB for full build |

### Supported Platforms

| Platform | Architecture | Status |
|----------|-------------|---------|
| **Linux** | x86_64, ARM64 | ✅ Fully Supported |
| **Windows** | x86_64 | ✅ Fully Supported |
| **macOS** | x86_64, ARM64 (M1/M2) | ✅ Fully Supported |

## Quick Installation

### One-Command Setup

```bash
# Clone and build
git clone https://github.com/kcenon/thread_system.git
cd thread_system

# Install dependencies and build
./dependency.sh && ./build.sh    # Linux/macOS
./dependency.bat && ./build.bat  # Windows
```

## Platform-Specific Instructions

### Linux (Ubuntu/Debian)

```bash
# Prerequisites
sudo apt update
sudo apt install -y build-essential cmake git curl zip unzip tar

# Build
export CC=gcc
export CXX=g++
./dependency.sh
./build.sh
```

### Windows (Visual Studio)

```cmd
# Set environment for Visual Studio
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

# Build project
.\dependency.bat
.\build.bat
```

### macOS

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Build project
./dependency.sh
./build.sh
```

## Your First Thread System Program

Let's create a simple program that demonstrates the basic features:

```cpp
#include "thread_pool/core/thread_pool.h"
#include "thread_base/jobs/callback_job.h"
#include "logger/core/logger.h"
#include "utilities/core/formatter.h"
#include <iostream>
#include <vector>
#include <memory>

using namespace thread_pool_module;
using namespace thread_module;
using namespace utility_module;

int main() {
    // Start the logger
    log_module::start();
    
    // Create a thread pool
    auto pool = std::make_shared<thread_pool>();
    
    // Add 4 worker threads
    std::vector<std::unique_ptr<thread_worker>> workers;
    for (int i = 0; i < 4; ++i) {
        workers.push_back(std::make_unique<thread_worker>());
    }
    pool->enqueue_batch(std::move(workers));
    
    // Start the pool
    auto start_error = pool->start();
    if (start_error.has_value()) {
        std::cerr << "Failed to start pool: " << *start_error << std::endl;
        return 1;
    }
    
    // Submit some jobs
    std::vector<std::shared_ptr<int>> results;
    for (int i = 0; i < 10; ++i) {
        auto result = std::make_shared<int>(0);
        results.push_back(result);
        
        pool->enqueue(std::make_unique<callback_job>(
            [i, result]() -> result_void {
                // Simulate some work
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                *result = i * i;
                return {};
            }
        ));
    }
    
    // Wait for jobs to complete
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Collect results
    std::cout << "Results: ";
    for (const auto& result : results) {
        std::cout << *result << " ";
    }
    std::cout << std::endl;
    
    // Stop the pool and logger
    pool->stop();
    log_module::stop();
    
    return 0;
}
```

### Compile and Run

```bash
# Build the project first
./build.sh  # or build.bat on Windows

# Compile your program
g++ -std=c++20 -I build/include first_program.cpp -L build/lib -lthread_pool -llogger -pthread -o first_program

# Run
./first_program
```

Expected output:
```
Results: 0 1 4 9 16 25 36 49 64 81
```

## Core Concepts

### 1. Thread Pool

The thread pool manages worker threads that execute submitted jobs:

```cpp
// Create thread pool
auto pool = std::make_shared<thread_pool>();

// Add specific number of workers
std::vector<std::unique_ptr<thread_worker>> workers;
for (size_t i = 0; i < 4; ++i) {
    workers.push_back(std::make_unique<thread_worker>());
}
pool->enqueue_batch(std::move(workers));

// Start the pool
pool->start();
```

### 2. Job Submission

Submit various types of jobs:

```cpp
// Lambda function as callback job
pool->enqueue(std::make_unique<callback_job>(
    []() -> result_void { 
        // Do work
        return {}; 
    }
));

// Job with captured values
int x = 10, y = 20;
pool->enqueue(std::make_unique<callback_job>(
    [x, y]() -> result_void { 
        int result = x + y;
        log_module::write_information("Result: {}", result);
        return {}; 
    }
));
```

### 3. Type Scheduling

Use type thread pool for type-based execution:

```cpp
#include "typed_thread_pool/pool/typed_thread_pool.h"
#include "typed_thread_pool/jobs/callback_typed_job.h"

using namespace typed_thread_pool_module;

// Create typed thread pool
auto typed_pool = std::make_shared<typed_thread_pool_t<job_types>>();

// Add workers with different type responsibilities
typed_pool->enqueue(std::make_unique<typed_thread_worker_t<job_types>>(
    std::initializer_list<job_types>{job_types::High}
));
typed_pool->enqueue(std::make_unique<typed_thread_worker_t<job_types>>(
    std::initializer_list<job_types>{job_types::Normal, job_types::Low}
));

// Start the pool
typed_pool->start();

// Submit jobs with different types
typed_pool->enqueue(std::make_unique<callback_typed_job<job_types>>(
    job_types::High,
    []() -> result_void { /* urgent task */ return {}; }
));
typed_pool->enqueue(std::make_unique<callback_typed_job<job_types>>(
    job_types::Normal,
    []() -> result_void { /* normal task */ return {}; }
));
```

### 4. Asynchronous Logging

High-performance thread-safe logging:

```cpp
#include "logger/core/logger.h"

// Start logger (once per application)
log_module::start();

// Log messages using convenience functions
log_module::write_information("Application started");
log_module::write_debug("Debug message: {}", debug_info);
log_module::write_error("Error: {}", error_message);

// Configure log targets
log_module::console_target(log_types::Information | log_types::Error);
log_module::file_target(log_types::All);
```

## Common Use Cases

### Parallel Data Processing

```cpp
std::vector<int> data(1000000);
// ... fill data ...

const size_t num_threads = 4;
auto pool = std::make_shared<thread_pool>();

// Add workers
std::vector<std::unique_ptr<thread_worker>> workers;
for (size_t i = 0; i < num_threads; ++i) {
    workers.push_back(std::make_unique<thread_worker>());
}
pool->enqueue_batch(std::move(workers));
pool->start();

const size_t chunk_size = data.size() / num_threads;
std::vector<std::shared_ptr<double>> results;

for (size_t i = 0; i < num_threads; ++i) {
    auto start = data.begin() + i * chunk_size;
    auto end = (i == num_threads - 1) ? data.end() : start + chunk_size;
    
    auto result = std::make_shared<double>(0.0);
    results.push_back(result);
    
    pool->enqueue(std::make_unique<callback_job>(
        [start, end, result]() -> result_void {
            *result = std::accumulate(start, end, 0.0) / 
                      std::distance(start, end);
            return {};
        }
    ));
}

// Wait for completion
std::this_thread::sleep_for(std::chrono::milliseconds(500));

double total_average = 0;
for (const auto& result : results) {
    total_average += *result;
}
total_average /= num_threads;
```

### Asynchronous I/O Operations

```cpp
// Create dedicated I/O pool
auto io_pool = std::make_shared<thread_pool>();
std::vector<std::unique_ptr<thread_worker>> io_workers;
for (int i = 0; i < 2; ++i) {
    io_workers.push_back(std::make_unique<thread_worker>());
}
io_pool->enqueue_batch(std::move(io_workers));
io_pool->start();

auto content_ptr = std::make_shared<std::string>();

io_pool->enqueue(std::make_unique<callback_job>(
    [content_ptr]() -> result_void {
        std::ifstream file("data.txt");
        *content_ptr = std::string((std::istreambuf_iterator<char>(file)),
                                  std::istreambuf_iterator<char>());
        return {};
    }
));

// Do other work while file is being read
do_other_work();

// Wait and get the file content when needed
std::this_thread::sleep_for(std::chrono::milliseconds(100));
std::string content = *content_ptr;
```

## Build Options

### CMake Configuration

```bash
# Basic configuration
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake

# Advanced configuration
cmake -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DSET_STD_FORMAT=ON \
    -DSET_STD_JTHREAD=ON \
    -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
```

### Build Script Options

```bash
./build.sh --clean      # Clean rebuild
./build.sh --docs       # Generate documentation
./build.sh --clean-docs # Clean and regenerate docs
```

## Verification

### Test Installation

```bash
# Run sample applications
./build/bin/thread_pool_sample
./build/bin/typed_thread_pool_sample
./build/bin/logger_sample
```

### Run Tests (Linux only)

```bash
cd build
ctest --verbose
```

**Note**: Tests are disabled on macOS due to pthread/gtest linkage issues.

## Troubleshooting

### Common Issues

#### CMake can't find vcpkg toolchain

```bash
export CMAKE_TOOLCHAIN_FILE=$(pwd)/../vcpkg/scripts/buildsystems/vcpkg.cmake
cmake -B build
```

#### Compiler not found

```bash
export CXX=g++
export CC=gcc
cmake -B build
```

#### Missing dependencies

```bash
rm -rf vcpkg/
./dependency.sh
```

## Integration Options

### Using as Submodule

```bash
# Add as git submodule
git submodule add https://github.com/kcenon/thread_system.git external/thread_system

# In your CMakeLists.txt
add_subdirectory(external/thread_system)
target_link_libraries(your_target PRIVATE thread_pool logger)
```

### Using with CMake FetchContent

```cmake
include(FetchContent)

FetchContent_Declare(
    thread_system
    GIT_REPOSITORY https://github.com/kcenon/thread_system.git
    GIT_TAG v1.0.0
)

FetchContent_MakeAvailable(thread_system)
target_link_libraries(your_target PRIVATE thread_pool logger)
```

## Best Practices

1. **Choose the Right Pool Size**: Use `std::thread::hardware_concurrency()` as a starting point
2. **Avoid Blocking Operations**: Use dedicated pools for I/O operations
3. **Handle Exceptions**: Jobs can throw exceptions that will be captured by futures
4. **Clean Shutdown**: Thread pools automatically clean up on destruction
5. **Start Logger Once**: Call `log_module::start()` once at application startup

## What's Next?

- Explore the [API Reference](./api-reference.md) for complete documentation
- Learn about [Performance Optimization](./performance.md)
- Check out [Common Patterns and Best Practices](./patterns.md)
- Read the [Architecture Overview](./architecture.md)

## Need Help?

- Check our [FAQ](./FAQ.md)
- Review [Common Patterns](./patterns.md) for troubleshooting
- Visit the project [GitHub page](https://github.com/kcenon/thread_system)

---

Congratulations! You're now ready to build high-performance concurrent applications with Thread System! 🎉