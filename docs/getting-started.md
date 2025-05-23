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
#include "thread_pool.h"
#include "logger.h"
#include <iostream>
#include <vector>

using namespace thread_pool_module;

int main() {
    // Start the logger
    log_module::start();
    
    // Create a thread pool with 4 worker threads
    auto [pool, error] = create_default(4);
    if (error.has_value()) {
        std::cerr << "Failed to create pool: " << *error << std::endl;
        return 1;
    }
    
    // Submit some jobs
    std::vector<std::future<int>> results;
    
    for (int i = 0; i < 10; ++i) {
        results.emplace_back(
            pool->submit_job([i] {
                // Simulate some work
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                return i * i;
            })
        );
    }
    
    // Collect results
    std::cout << "Results: ";
    for (auto& future : results) {
        std::cout << future.get() << " ";
    }
    std::cout << std::endl;
    
    // Logger automatically stops on program exit
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
// Create with specific thread count
auto [pool, error] = thread_pool_module::create_default(4);

// Create with hardware concurrency
auto [pool2, error2] = thread_pool_module::create_default(
    std::thread::hardware_concurrency()
);
```

### 2. Job Submission

Submit various types of jobs:

```cpp
// Lambda function
auto future1 = pool->submit_job([] { return 42; });

// Function with parameters
auto future2 = pool->submit_job([](int x, int y) { return x + y; }, 10, 20);

// Member function
MyClass obj;
auto future3 = pool->submit_job(&MyClass::method, &obj, param);
```

### 3. Priority Scheduling

Use priority thread pool for priority-based execution:

```cpp
#include "priority_thread_pool.h"

using namespace priority_thread_pool_module;

// Create priority pool
auto [priority_pool, error] = create_default(4);

// Submit with priority (lower value = higher priority)
priority_pool->submit_job(1, [] { /* urgent task */ });
priority_pool->submit_job(5, [] { /* normal task */ });
priority_pool->submit_job(10, [] { /* background task */ });
```

### 4. Asynchronous Logging

High-performance thread-safe logging:

```cpp
#include "logger.h"

// Start logger (once per application)
log_module::start();

// Log messages
log_module::logger()->log(log_types::info, "Application started");
log_module::logger()->log(log_types::warning, "This is a warning");
log_module::logger()->log(log_types::error, "Error: {}", error_message);
```

## Common Use Cases

### Parallel Data Processing

```cpp
std::vector<int> data(1000000);
// ... fill data ...

const size_t num_threads = 4;
auto [pool, error] = thread_pool_module::create_default(num_threads);

const size_t chunk_size = data.size() / num_threads;
std::vector<std::future<double>> futures;

for (size_t i = 0; i < num_threads; ++i) {
    auto start = data.begin() + i * chunk_size;
    auto end = (i == num_threads - 1) ? data.end() : start + chunk_size;
    
    futures.emplace_back(
        pool->submit_job([start, end] {
            return std::accumulate(start, end, 0.0) / 
                   std::distance(start, end);
        })
    );
}

double total_average = 0;
for (auto& f : futures) {
    total_average += f.get();
}
total_average /= num_threads;
```

### Asynchronous I/O Operations

```cpp
// Create dedicated I/O pool
auto [io_pool, error] = thread_pool_module::create_default(2);

auto read_future = io_pool->submit_job([] {
    std::ifstream file("data.txt");
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    return content;
});

// Do other work while file is being read
do_other_work();

// Get the file content when needed
std::string content = read_future.get();
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
./build/bin/priority_thread_pool_sample
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