# Quick Start Guide

Get up and running with Thread System in just 5 minutes!

## Prerequisites Check

Ensure you have a modern C++ compiler:

```bash
# Check compiler version
g++ --version    # Should be 9.0 or later
clang++ --version # Should be 10.0 or later

# Check CMake
cmake --version  # Should be 3.16 or later
```

## Installation

```bash
# Clone and build
git clone https://github.com/kcenon/thread_system.git
cd thread_system

# One-command setup
./dependency.sh && ./build.sh  # Linux/macOS
./dependency.bat && ./build.bat # Windows

# Verify installation
./build/bin/thread_pool_sample
```

## Your First Program

### Example 1: Basic Thread Pool

```cpp
#include "thread_pool.h"
#include "logger.h"
#include <iostream>
#include <chrono>

using namespace thread_pool_module;

int main() {
    // Initialize logging
    log_module::start();
    
    // Create a thread pool with 4 workers
    auto [pool, error] = create_default(4);
    if (error.has_value()) {
        std::cerr << "Error: " << *error << std::endl;
        return 1;
    }
    
    // Create some jobs
    std::vector<std::unique_ptr<thread_module::job>> jobs;
    for (int i = 0; i < 10; ++i) {
        jobs.push_back(std::make_unique<thread_module::callback_job>(
            [i]() -> std::optional<std::string> {
                log_module::write_info("Processing job {}", i);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                return std::nullopt; // Success
            }
        ));
    }
    
    // Submit jobs and start processing
    pool->enqueue_batch(std::move(jobs));
    pool->start();
    
    // Wait for completion
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Clean shutdown
    pool->stop();
    log_module::stop();
    
    std::cout << "All jobs completed!" << std::endl;
    return 0;
}
```

**Compile and run:**
```bash
g++ -std=c++20 -I build/include first_program.cpp -L build/lib -lthread_system -o first_program
./first_program
```

### Example 2: Priority-Based Processing

```cpp
#include "priority_thread_pool.h"
#include "logger.h"
#include <iostream>

using namespace priority_thread_pool_module;

int main() {
    log_module::start();
    
    auto pool = std::make_shared<priority_thread_pool_t<job_priorities>>();
    
    // Add workers with different priority responsibilities
    std::vector<std::unique_ptr<priority_thread_worker_t<job_priorities>>> workers;
    
    // High priority specialist
    workers.push_back(std::make_unique<priority_thread_worker_t<job_priorities>>(
        std::vector<job_priorities>{job_priorities::High}
    ));
    
    // General worker
    workers.push_back(std::make_unique<priority_thread_worker_t<job_priorities>>(
        std::vector<job_priorities>{job_priorities::High, job_priorities::Normal}
    ));
    
    pool->enqueue_batch(std::move(workers));
    
    // Create jobs with different priorities
    std::vector<std::unique_ptr<priority_job_t<job_priorities>>> jobs;
    
    // High priority jobs
    for (int i = 0; i < 3; ++i) {
        jobs.push_back(std::make_unique<callback_priority_job_t<job_priorities>>(
            [i]() -> result_void {
                log_module::write_info("HIGH priority job {} processing", i);
                return {};
            },
            job_priorities::High
        ));
    }
    
    // Normal priority jobs
    for (int i = 0; i < 5; ++i) {
        jobs.push_back(std::make_unique<callback_priority_job_t<job_priorities>>(
            [i]() -> result_void {
                log_module::write_info("NORMAL priority job {} processing", i);
                return {};
            },
            job_priorities::Normal
        ));
    }
    
    pool->enqueue_batch(std::move(jobs));
    pool->start();
    
    // Wait for completion
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    pool->stop();
    log_module::stop();
    
    std::cout << "Priority-based processing completed!" << std::endl;
    return 0;
}
```

## Key Concepts in 60 Seconds

### 1. Thread Pool Pattern
```cpp
// Basic pattern: Create → Submit → Start → Stop
auto pool = create_thread_pool();
pool->enqueue(job);
pool->start();
pool->stop();
```

### 2. Priority Scheduling
```cpp
// Workers can handle specific priority levels
worker({job_priorities::High});                    // High only
worker({job_priorities::High, job_priorities::Normal}); // High then Normal
```

### 3. Error Handling
```cpp
// All operations return result<T> for safe error handling
auto [pool, error] = create_default(4);
if (error.has_value()) {
    // Handle error
}
```

### 4. Asynchronous Logging
```cpp
// Thread-safe logging across all threads
log_module::write_info("Message with {} formatting", value);
log_module::write_error("Error: {}", error.message());
```

## Common Usage Patterns

### Pattern 1: CPU-Intensive Tasks
```cpp
// Create pool with hardware thread count
auto worker_count = std::thread::hardware_concurrency();
auto [pool, error] = create_default(worker_count);

// Submit CPU-bound jobs
for (auto& data_chunk : large_dataset) {
    pool->enqueue(make_processing_job(data_chunk));
}
```

### Pattern 2: I/O-Bound Tasks
```cpp
// Create pool with more threads than CPU cores
auto worker_count = std::thread::hardware_concurrency() * 2;
auto [pool, error] = create_default(worker_count);

// Submit I/O jobs
for (auto& file_path : files_to_process) {
    pool->enqueue(make_file_processing_job(file_path));
}
```

## Quick Troubleshooting

### Build Issues
```bash
# Clean build if something goes wrong
rm -rf build/
./build.sh

# Verbose build for debugging
cmake --build build --verbose
```

### Runtime Issues
```bash
# Enable debug logging
log_module::console_target(log_module::log_types::All);

# Check thread counts
auto hw_threads = std::thread::hardware_concurrency();
std::cout << "Hardware threads: " << hw_threads << std::endl;
```

## Next Steps

Once you're comfortable with the basics:

1. **Deep Dive**: Read [architecture.md](architecture.md) for detailed design understanding
2. **Performance**: Study [performance_tuning.md](performance_tuning.md) for optimization techniques
3. **Advanced Usage**: Explore [patterns_and_antipatterns.md](patterns_and_antipatterns.md)
4. **Troubleshooting**: Reference [concurrency_troubleshooting.md](concurrency_troubleshooting.md)

Now you're ready to build high-performance, thread-safe applications with Thread System! 🎉