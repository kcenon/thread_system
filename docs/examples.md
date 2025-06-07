# Thread System Examples

This guide contains practical examples demonstrating how to use the Thread System framework in real-world scenarios.

## Quick Start Examples

### Hello World with Thread Pool

```cpp
#include "thread_pool.h"
#include "logger.h"

int main() {
    log_module::start();
    
    auto [pool, error] = thread_pool_module::create_default(4);
    if (!error) {
        pool->start();
        pool->add_job([] {
            log_module::logger()->log(log_types::info, "Hello from thread pool!");
        });
        pool->stop();
    }
    
    log_module::stop();
    return 0;
}
```

### Parallel Computation

```cpp
#include "thread_pool.h"
#include <numeric>
#include <vector>

double parallel_sum(const std::vector<double>& data) {
    auto [pool, error] = thread_pool_module::create_default(4);
    if (error) return std::accumulate(data.begin(), data.end(), 0.0);
    
    pool->start();
    
    const size_t chunk_size = data.size() / 4;
    std::vector<std::future<double>> futures;
    
    for (size_t i = 0; i < 4; ++i) {
        auto start = data.begin() + i * chunk_size;
        auto end = (i == 3) ? data.end() : start + chunk_size;
        
        futures.push_back(pool->submit_job([start, end] {
            return std::accumulate(start, end, 0.0);
        }));
    }
    
    double total = 0.0;
    for (auto& f : futures) {
        total += f.get();
    }
    
    pool->stop();
    return total;
}
```

## Basic Examples

### Simple Job Submission

```cpp
#include "thread_pool.h"
#include <iostream>

int main() {
    auto [pool, error] = thread_pool_module::create_default(2);
    if (error) {
        std::cerr << "Failed to create pool: " << *error << std::endl;
        return 1;
    }
    
    pool->start();
    
    // Submit job with return value
    auto future = pool->submit_job([] {
        return 42;
    });
    
    std::cout << "Result: " << future.get() << std::endl;
    
    pool->stop();
    return 0;
}
```

### Error Handling

```cpp
#include "thread_pool.h"
#include <iostream>
#include <stdexcept>

int main() {
    auto [pool, error] = thread_pool_module::create_default(2);
    if (error) return 1;
    
    pool->start();
    
    auto future = pool->submit_job([] {
        throw std::runtime_error("Something went wrong!");
        return 42;
    });
    
    try {
        int result = future.get();
    } catch (const std::exception& e) {
        std::cerr << "Job failed: " << e.what() << std::endl;
    }
    
    pool->stop();
    return 0;
}
```

### File Processing

```cpp
#include "thread_pool.h"
#include <fstream>
#include <vector>
#include <filesystem>

void process_files(const std::vector<std::filesystem::path>& files) {
    auto [pool, error] = thread_pool_module::create_default(4);
    if (error) return;
    
    pool->start();
    
    std::vector<std::future<size_t>> futures;
    
    for (const auto& file : files) {
        futures.push_back(pool->submit_job([file] {
            std::ifstream ifs(file);
            std::string line;
            size_t line_count = 0;
            
            while (std::getline(ifs, line)) {
                // Process line
                line_count++;
            }
            
            return line_count;
        }));
    }
    
    size_t total_lines = 0;
    for (auto& f : futures) {
        total_lines += f.get();
    }
    
    std::cout << "Total lines processed: " << total_lines << std::endl;
    
    pool->stop();
}
```

## Intermediate Examples

### Producer-Consumer Pattern

```cpp
#include "thread_pool.h"
#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class ThreadSafeQueue {
private:
    mutable std::mutex mutex_;
    std::queue<T> queue_;
    std::condition_variable cv_;
    bool done_ = false;
    
public:
    void push(T value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(value));
        }
        cv_.notify_one();
    }
    
    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty() || done_; });
        
        if (queue_.empty()) return false;
        
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }
    
    void done() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            done_ = true;
        }
        cv_.notify_all();
    }
};

void producer_consumer_example() {
    ThreadSafeQueue<int> queue;
    auto [pool, error] = thread_pool_module::create_default(4);
    if (error) return;
    
    pool->start();
    
    // Producer
    pool->add_job([&queue] {
        for (int i = 0; i < 100; ++i) {
            queue.push(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        queue.done();
    });
    
    // Consumers
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 3; ++i) {
        futures.push_back(pool->submit_job([&queue] {
            int sum = 0;
            int value;
            while (queue.pop(value)) {
                sum += value;
            }
            return sum;
        }));
    }
    
    int total = 0;
    for (auto& f : futures) {
        total += f.get();
    }
    
    std::cout << "Total sum: " << total << std::endl;
    
    pool->stop();
}
```

### Type-Based Task Scheduling

```cpp
#include "typed_thread_pool.h"
#include <iostream>

void type_example() {
    auto [pool, error] = typed_thread_pool_module::create_default(4);
    if (error) return;
    
    pool->start();
    
    // High type task (type 1)
    pool->submit_job(1, [] {
        std::cout << "High type task executed" << std::endl;
    });
    
    // Normal type tasks (type 5)
    for (int i = 0; i < 5; ++i) {
        pool->submit_job(5, [i] {
            std::cout << "Normal type task " << i << std::endl;
        });
    }
    
    // Low type task (type 10)
    pool->submit_job(10, [] {
        std::cout << "Low type task executed" << std::endl;
    });
    
    // Wait for completion
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    pool->stop();
}
```

### Batch Processing

```cpp
#include "thread_pool.h"
#include <vector>
#include <algorithm>

template<typename T, typename Func>
std::vector<T> parallel_transform(const std::vector<T>& input, Func func) {
    auto [pool, error] = thread_pool_module::create_default(4);
    if (error) {
        // Fallback to sequential processing
        std::vector<T> output;
        std::transform(input.begin(), input.end(), 
                      std::back_inserter(output), func);
        return output;
    }
    
    pool->start();
    
    const size_t batch_size = (input.size() + 3) / 4;
    std::vector<std::future<std::vector<T>>> futures;
    
    for (size_t i = 0; i < input.size(); i += batch_size) {
        auto begin = input.begin() + i;
        auto end = std::min(begin + batch_size, input.end());
        
        futures.push_back(pool->submit_job([begin, end, func] {
            std::vector<T> batch_output;
            std::transform(begin, end, std::back_inserter(batch_output), func);
            return batch_output;
        }));
    }
    
    std::vector<T> output;
    for (auto& f : futures) {
        auto batch = f.get();
        output.insert(output.end(), batch.begin(), batch.end());
    }
    
    pool->stop();
    return output;
}

// Usage example
void batch_processing_demo() {
    std::vector<int> numbers(1000);
    std::iota(numbers.begin(), numbers.end(), 0);
    
    auto squared = parallel_transform(numbers, [](int x) { return x * x; });
    
    std::cout << "First 10 squared numbers: ";
    for (int i = 0; i < 10; ++i) {
        std::cout << squared[i] << " ";
    }
    std::cout << std::endl;
}
```

## Advanced Examples

### Custom Worker Implementation

```cpp
#include "thread_base.h"
#include <atomic>

class CustomWorker : public thread_base {
private:
    std::atomic<int> counter_{0};
    std::atomic<bool> should_stop_{false};
    
protected:
    void run() override {
        while (!should_stop_) {
            // Custom work logic
            counter_++;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            if (counter_ % 10 == 0) {
                log_module::logger()->log(log_types::info, 
                    "Processed {} items", counter_.load());
            }
        }
    }
    
public:
    void stop() {
        should_stop_ = true;
        thread_base::stop();
    }
    
    int get_count() const { return counter_; }
};

void custom_worker_demo() {
    log_module::start();
    
    CustomWorker worker;
    worker.start();
    
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    worker.stop();
    
    std::cout << "Worker processed " << worker.get_count() << " items" << std::endl;
    
    log_module::stop();
}
```

### Performance Monitoring

```cpp
#include "thread_pool.h"
#include <chrono>
#include <atomic>

class PerformanceMonitor {
private:
    std::atomic<size_t> jobs_submitted_{0};
    std::atomic<size_t> jobs_completed_{0};
    std::chrono::steady_clock::time_point start_time_;
    
public:
    PerformanceMonitor() : start_time_(std::chrono::steady_clock::now()) {}
    
    void job_submitted() { jobs_submitted_++; }
    void job_completed() { jobs_completed_++; }
    
    void print_stats() const {
        auto elapsed = std::chrono::steady_clock::now() - start_time_;
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        
        if (seconds > 0) {
            std::cout << "Performance Stats:\n"
                     << "  Jobs submitted: " << jobs_submitted_ << "\n"
                     << "  Jobs completed: " << jobs_completed_ << "\n"
                     << "  Throughput: " << jobs_completed_ / seconds << " jobs/sec\n"
                     << "  Pending: " << jobs_submitted_ - jobs_completed_ << "\n";
        }
    }
};

void performance_monitoring_demo() {
    auto [pool, error] = thread_pool_module::create_default(8);
    if (error) return;
    
    pool->start();
    PerformanceMonitor monitor;
    
    // Submit many jobs
    for (int i = 0; i < 10000; ++i) {
        monitor.job_submitted();
        pool->add_job([&monitor, i] {
            // Simulate work
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            monitor.job_completed();
        });
    }
    
    // Monitor progress
    for (int i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        monitor.print_stats();
    }
    
    pool->stop();
    monitor.print_stats();
}
```

### Integration with Async Operations

```cpp
#include "thread_pool.h"
#include <future>
#include <vector>

class AsyncTaskManager {
private:
    std::shared_ptr<thread_pool> pool_;
    
public:
    AsyncTaskManager(size_t num_threads = std::thread::hardware_concurrency()) {
        auto [pool, error] = thread_pool_module::create_default(num_threads);
        if (!error) {
            pool_ = pool;
            pool_->start();
        }
    }
    
    ~AsyncTaskManager() {
        if (pool_) {
            pool_->stop();
        }
    }
    
    template<typename Func>
    auto execute_async(Func&& func) -> std::future<decltype(func())> {
        if (!pool_) {
            // Fallback to std::async
            return std::async(std::launch::async, std::forward<Func>(func));
        }
        return pool_->submit_job(std::forward<Func>(func));
    }
    
    template<typename Func>
    void execute_parallel(const std::vector<Func>& tasks) {
        std::vector<std::future<void>> futures;
        
        for (const auto& task : tasks) {
            futures.push_back(execute_async(task));
        }
        
        // Wait for all tasks
        for (auto& f : futures) {
            f.wait();
        }
    }
};

void async_integration_demo() {
    AsyncTaskManager manager(4);
    
    // Single async operation
    auto result = manager.execute_async([] {
        return 42;
    });
    
    // Multiple parallel operations
    std::vector<std::function<void()>> tasks;
    for (int i = 0; i < 10; ++i) {
        tasks.push_back([i] {
            std::cout << "Task " << i << " executing on thread " 
                     << std::this_thread::get_id() << std::endl;
        });
    }
    
    manager.execute_parallel(tasks);
    
    std::cout << "Async result: " << result.get() << std::endl;
}
```

## Running the Examples

1. **Build the examples**:
   ```bash
   cd thread_system
   ./build.sh
   ```

2. **Run sample applications**:
   ```bash
   ./build/bin/thread_pool_sample
   ./build/bin/typed_thread_pool_sample
   ./build/bin/logger_sample
   ```

3. **Create your own example**:
   - Copy one of the sample files
   - Modify for your use case
   - Add to CMakeLists.txt
   - Build and run

## Best Practices from Examples

1. **Always check for errors** when creating thread pools
2. **Start pools before submitting jobs** and stop them when done
3. **Use futures** to get results from jobs
4. **Handle exceptions** that may be thrown from jobs
5. **Choose appropriate pool sizes** based on workload
6. **Use type pools** when job ordering matters
7. **Initialize logging** at the start of your application

## See Also

- [Getting Started Guide](./getting-started.md)
- [API Reference](./api-reference.md)
- [Performance Guide](./performance.md)
- [Common Patterns](./patterns.md)