# Thread System Examples

This guide contains practical examples demonstrating how to use the Thread System framework in real-world scenarios.

## Quick Start Examples

### Hello World with Thread Pool

```cpp
#include "thread_pool/core/thread_pool.h"
#include "thread_base/jobs/callback_job.h"
#include "logger/core/logger.h"

int main() {
    // Start logger
    log_module::start();
    
    // Create thread pool
    auto pool = std::make_shared<thread_pool_module::thread_pool>();
    
    // Add workers
    std::vector<std::unique_ptr<thread_pool_module::thread_worker>> workers;
    for (int i = 0; i < 4; ++i) {
        workers.push_back(std::make_unique<thread_pool_module::thread_worker>());
    }
    pool->enqueue_batch(std::move(workers));
    
    // Start pool
    pool->start();
    
    // Submit job
    pool->enqueue(std::make_unique<thread_module::callback_job>(
        []() -> thread_module::result_void {
            log_module::write_information("Hello from thread pool!");
            return {};
        }
    ));
    
    // Wait for completion
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Clean up
    pool->stop();
    log_module::stop();
    
    return 0;
}
```

### Parallel Computation

```cpp
#include "thread_pool/core/thread_pool.h"
#include "thread_base/jobs/callback_job.h"
#include <numeric>
#include <vector>
#include <memory>

double parallel_sum(const std::vector<double>& data) {
    // Create thread pool
    auto pool = std::make_shared<thread_pool_module::thread_pool>();
    
    // Add 4 workers
    std::vector<std::unique_ptr<thread_pool_module::thread_worker>> workers;
    for (int i = 0; i < 4; ++i) {
        workers.push_back(std::make_unique<thread_pool_module::thread_worker>());
    }
    pool->enqueue_batch(std::move(workers));
    pool->start();
    
    const size_t chunk_size = data.size() / 4;
    std::vector<std::shared_ptr<double>> results;
    
    for (size_t i = 0; i < 4; ++i) {
        auto start = data.begin() + i * chunk_size;
        auto end = (i == 3) ? data.end() : start + chunk_size;
        
        auto result = std::make_shared<double>(0.0);
        results.push_back(result);
        
        pool->enqueue(std::make_unique<thread_module::callback_job>(
            [start, end, result]() -> thread_module::result_void {
                *result = std::accumulate(start, end, 0.0);
                return {};
            }
        ));
    }
    
    // Wait for all jobs to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    double total = 0.0;
    for (const auto& result : results) {
        total += *result;
    }
    
    pool->stop();
    return total;
}
```

## Basic Examples

### Simple Job Submission

```cpp
#include "thread_pool/core/thread_pool.h"
#include "thread_base/jobs/callback_job.h"
#include <iostream>

int main() {
    // Create and configure pool
    auto pool = std::make_shared<thread_pool_module::thread_pool>();
    
    // Add 2 workers
    std::vector<std::unique_ptr<thread_pool_module::thread_worker>> workers;
    for (int i = 0; i < 2; ++i) {
        workers.push_back(std::make_unique<thread_pool_module::thread_worker>());
    }
    
    if (auto error = pool->enqueue_batch(std::move(workers))) {
        std::cerr << "Failed to add workers: " << *error << std::endl;
        return 1;
    }
    
    if (auto error = pool->start()) {
        std::cerr << "Failed to start pool: " << *error << std::endl;
        return 1;
    }
    
    // Submit job with shared result
    auto result = std::make_shared<int>(0);
    pool->enqueue(std::make_unique<thread_module::callback_job>(
        [result]() -> thread_module::result_void {
            *result = 42;
            return {};
        }
    ));
    
    // Wait and get result
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "Result: " << *result << std::endl;
    
    pool->stop();
    return 0;
}
```

### Error Handling

```cpp
#include "thread_pool/core/thread_pool.h"
#include "thread_base/jobs/callback_job.h"
#include <iostream>

int main() {
    auto pool = std::make_shared<thread_pool_module::thread_pool>();
    
    // Add workers
    std::vector<std::unique_ptr<thread_pool_module::thread_worker>> workers;
    workers.push_back(std::make_unique<thread_pool_module::thread_worker>());
    workers.push_back(std::make_unique<thread_pool_module::thread_worker>());
    pool->enqueue_batch(std::move(workers));
    
    pool->start();
    
    // Submit job that fails
    pool->enqueue(std::make_unique<thread_module::callback_job>(
        []() -> thread_module::result_void {
            // Simulate error
            return thread_module::error_info(
                thread_module::error_type::runtime_error,
                "Something went wrong!"
            );
        }
    ));
    
    // The error will be logged by the worker
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    pool->stop();
    return 0;
}
```

### File Processing

```cpp
#include "thread_pool/core/thread_pool.h"
#include "thread_base/jobs/callback_job.h"
#include "logger/core/logger.h"
#include <fstream>
#include <vector>
#include <filesystem>

void process_files(const std::vector<std::filesystem::path>& files) {
    // Start logger
    log_module::start();
    
    // Create pool
    auto pool = std::make_shared<thread_pool_module::thread_pool>();
    
    // Add workers
    std::vector<std::unique_ptr<thread_pool_module::thread_worker>> workers;
    for (int i = 0; i < 4; ++i) {
        workers.push_back(std::make_unique<thread_pool_module::thread_worker>());
    }
    pool->enqueue_batch(std::move(workers));
    pool->start();
    
    // Process each file
    std::vector<std::shared_ptr<size_t>> line_counts;
    
    for (const auto& file : files) {
        auto count = std::make_shared<size_t>(0);
        line_counts.push_back(count);
        
        pool->enqueue(std::make_unique<thread_module::callback_job>(
            [file, count]() -> thread_module::result_void {
                std::ifstream ifs(file);
                if (!ifs) {
                    return thread_module::error_info(
                        thread_module::error_type::file_not_found,
                        "Cannot open file: " + file.string()
                    );
                }
                
                std::string line;
                while (std::getline(ifs, line)) {
                    (*count)++;
                }
                
                log_module::write_information("File {} has {} lines", 
                                            file.filename().string(), *count);
                return {};
            }
        ));
    }
    
    // Wait for all files to be processed
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Report total
    size_t total = 0;
    for (const auto& count : line_counts) {
        total += *count;
    }
    log_module::write_information("Total lines: {}", total);
    
    // Clean up
    pool->stop();
    log_module::stop();
}
```

## Type-Based Thread Pool Examples

### Basic Type Scheduling

```cpp
#include "typed_thread_pool/pool/typed_thread_pool.h"
#include "typed_thread_pool/jobs/callback_typed_job.h"
#include "logger/core/logger.h"

int main() {
    log_module::start();
    
    // Create typed thread pool
    auto pool = std::make_shared<typed_thread_pool_module::typed_thread_pool>();
    
    // Add workers with different type responsibilities
    // Worker 1: Only handles High priority
    pool->enqueue(std::make_unique<typed_thread_pool_module::typed_thread_worker_t<job_types>>(
        std::initializer_list<job_types>{job_types::High}
    ));
    
    // Worker 2: Handles Normal and Low priority
    pool->enqueue(std::make_unique<typed_thread_pool_module::typed_thread_worker_t<job_types>>(
        std::initializer_list<job_types>{job_types::Normal, job_types::Low}
    ));
    
    pool->start();
    
    // Submit jobs with different priorities
    for (int i = 0; i < 5; ++i) {
        pool->enqueue(std::make_unique<typed_thread_pool_module::callback_typed_job<job_types>>(
            job_types::High,
            [i]() -> thread_module::result_void {
                log_module::write_information("High priority job {}", i);
                return {};
            }
        ));
        
        pool->enqueue(std::make_unique<typed_thread_pool_module::callback_typed_job<job_types>>(
            job_types::Low,
            [i]() -> thread_module::result_void {
                log_module::write_information("Low priority job {}", i);
                return {};
            }
        ));
    }
    
    // High priority jobs will be processed first
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    pool->stop();
    log_module::stop();
    return 0;
}
```

### Custom Type Types

```cpp
#include "typed_thread_pool/pool/typed_thread_pool.h"
#include "typed_thread_pool/jobs/typed_job.h"

// Define custom priority types
enum class TaskPriority : uint8_t {
    Critical = 1,
    UserInteractive = 2,
    Background = 3,
    Maintenance = 4
};

// Custom job implementation
class MyCustomJob : public typed_thread_pool_module::typed_job_t<TaskPriority> {
private:
    std::string task_name_;
    
public:
    MyCustomJob(TaskPriority priority, const std::string& name)
        : typed_job_t<TaskPriority>(priority), task_name_(name) {}
    
    auto operator()() -> thread_module::result_void override {
        log_module::write_information("Executing {}: {}", 
                                    static_cast<int>(get_type()), task_name_);
        // Do actual work here
        return {};
    }
};

int main() {
    log_module::start();
    
    // Create pool with custom priority type
    auto pool = std::make_shared<typed_thread_pool_module::typed_thread_pool_t<TaskPriority>>();
    
    // Add workers
    pool->enqueue(std::make_unique<typed_thread_pool_module::typed_thread_worker_t<TaskPriority>>(
        std::initializer_list<TaskPriority>{TaskPriority::Critical, TaskPriority::UserInteractive}
    ));
    
    pool->start();
    
    // Submit custom jobs
    pool->enqueue(std::make_unique<MyCustomJob>(TaskPriority::Critical, "Database backup"));
    pool->enqueue(std::make_unique<MyCustomJob>(TaskPriority::Background, "Cache cleanup"));
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    pool->stop();
    log_module::stop();
    return 0;
}
```

## Logging Examples

### Comprehensive Logging Setup

```cpp
#include "logger/core/logger.h"
#include <iostream>

int main() {
    // Configure logger
    log_module::set_title("MyApplication");
    log_module::set_max_lines(10000);  // Max lines per file
    log_module::set_use_backup(true);  // Create backup files
    
    // Configure targets
    log_module::console_target(log_module::log_types::Information | 
                              log_module::log_types::Error);
    log_module::file_target(log_module::log_types::All);
    
    // Set up callback for custom handling
    log_module::callback_target(log_module::log_types::Error);
    log_module::message_callback(
        [](const log_module::log_types& type, 
           const std::string& datetime, 
           const std::string& message) {
            if (type == log_module::log_types::Error) {
                // Send error notifications, etc.
                std::cerr << "ALERT: " << message << std::endl;
            }
        }
    );
    
    // Start logger
    if (auto error = log_module::start()) {
        std::cerr << "Failed to start logger: " << *error << std::endl;
        return 1;
    }
    
    // Use logger
    log_module::write_information("Application started");
    log_module::write_debug("Debug mode enabled");
    log_module::write_error("Example error message");
    
    // Structured logging
    int user_id = 12345;
    std::string action = "login";
    log_module::write_sequence("User {} performed action: {}", user_id, action);
    
    // Stop logger
    log_module::stop();
    return 0;
}
```

### Performance Logging

```cpp
#include "logger/core/logger.h"
#include <chrono>

class PerformanceTimer {
private:
    std::string operation_;
    std::chrono::high_resolution_clock::time_point start_;
    
public:
    PerformanceTimer(const std::string& operation) 
        : operation_(operation), 
          start_(std::chrono::high_resolution_clock::now()) {
        log_module::write_sequence("Starting: {}", operation_);
    }
    
    ~PerformanceTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
        log_module::write_sequence("Completed: {} ({}ms)", operation_, duration.count());
    }
};

void some_operation() {
    PerformanceTimer timer("Database query");
    
    // Simulate work
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    
    // Timer destructor will log completion
}
```

## Real-World Scenarios

### Web Server Request Handler

```cpp
#include "thread_pool/core/thread_pool.h"
#include "typed_thread_pool/pool/typed_thread_pool.h"
#include "logger/core/logger.h"

class WebServer {
private:
    std::shared_ptr<typed_thread_pool_module::typed_thread_pool> request_pool_;
    std::shared_ptr<thread_pool_module::thread_pool> io_pool_;
    
public:
    WebServer() {
        // Request handling pool with priorities
        request_pool_ = std::make_shared<typed_thread_pool_module::typed_thread_pool>();
        
        // High priority worker for API requests
        request_pool_->enqueue(
            std::make_unique<typed_thread_pool_module::typed_thread_worker_t<job_types>>(
                std::initializer_list<job_types>{job_types::High}
            )
        );
        
        // Normal priority workers for regular requests
        for (int i = 0; i < 4; ++i) {
            request_pool_->enqueue(
                std::make_unique<typed_thread_pool_module::typed_thread_worker_t<job_types>>(
                    std::initializer_list<job_types>{job_types::Normal, job_types::Low}
                )
            );
        }
        
        // I/O pool for file operations
        io_pool_ = std::make_shared<thread_pool_module::thread_pool>();
        std::vector<std::unique_ptr<thread_pool_module::thread_worker>> io_workers;
        for (int i = 0; i < 2; ++i) {
            io_workers.push_back(std::make_unique<thread_pool_module::thread_worker>());
        }
        io_pool_->enqueue_batch(std::move(io_workers));
    }
    
    void start() {
        request_pool_->start();
        io_pool_->start();
        log_module::write_information("Web server started");
    }
    
    void handle_request(const std::string& path, bool is_api) {
        auto priority = is_api ? job_types::High : job_types::Normal;
        
        request_pool_->enqueue(
            std::make_unique<typed_thread_pool_module::callback_typed_job<job_types>>(
                priority,
                [this, path]() -> thread_module::result_void {
                    log_module::write_information("Processing request: {}", path);
                    
                    // Process request
                    if (path.starts_with("/static/")) {
                        serve_static_file(path.substr(8));
                    } else {
                        generate_response(path);
                    }
                    
                    return {};
                }
            )
        );
    }
    
    void stop() {
        request_pool_->stop();
        io_pool_->stop();
        log_module::write_information("Web server stopped");
    }
    
private:
    void serve_static_file(const std::string& file) {
        io_pool_->enqueue(std::make_unique<thread_module::callback_job>(
            [file]() -> thread_module::result_void {
                // Read and serve file
                log_module::write_debug("Serving static file: {}", file);
                return {};
            }
        ));
    }
    
    void generate_response(const std::string& path) {
        // Generate dynamic response
        log_module::write_debug("Generating response for: {}", path);
    }
};
```

### Data Processing Pipeline

```cpp
#include "typed_thread_pool/pool/typed_thread_pool.h"
#include "logger/core/logger.h"
#include <queue>
#include <mutex>

class DataPipeline {
private:
    struct DataItem {
        int id;
        std::vector<double> data;
    };
    
    std::shared_ptr<typed_thread_pool_module::typed_thread_pool> pool_;
    std::queue<DataItem> input_queue_;
    std::mutex queue_mutex_;
    
public:
    DataPipeline() {
        pool_ = std::make_shared<typed_thread_pool_module::typed_thread_pool>();
        
        // Stage 1: Data ingestion (high priority)
        pool_->enqueue(
            std::make_unique<typed_thread_pool_module::typed_thread_worker_t<job_types>>(
                std::initializer_list<job_types>{job_types::High}
            )
        );
        
        // Stage 2: Processing (normal priority)
        for (int i = 0; i < 3; ++i) {
            pool_->enqueue(
                std::make_unique<typed_thread_pool_module::typed_thread_worker_t<job_types>>(
                    std::initializer_list<job_types>{job_types::Normal}
                )
            );
        }
        
        // Stage 3: Cleanup (low priority)
        pool_->enqueue(
            std::make_unique<typed_thread_pool_module::typed_thread_worker_t<job_types>>(
                std::initializer_list<job_types>{job_types::Low}
            )
        );
    }
    
    void process_batch(const std::vector<DataItem>& items) {
        // Stage 1: Ingest data
        for (const auto& item : items) {
            pool_->enqueue(
                std::make_unique<typed_thread_pool_module::callback_typed_job<job_types>>(
                    job_types::High,
                    [this, item]() -> thread_module::result_void {
                        validate_and_queue(item);
                        return {};
                    }
                )
            );
        }
        
        // Stage 2: Process data
        pool_->enqueue(
            std::make_unique<typed_thread_pool_module::callback_typed_job<job_types>>(
                job_types::Normal,
                [this]() -> thread_module::result_void {
                    process_queued_items();
                    return {};
                }
            )
        );
        
        // Stage 3: Cleanup
        pool_->enqueue(
            std::make_unique<typed_thread_pool_module::callback_typed_job<job_types>>(
                job_types::Low,
                [this]() -> thread_module::result_void {
                    cleanup_processed_data();
                    return {};
                }
            )
        );
    }
    
private:
    void validate_and_queue(const DataItem& item) {
        log_module::write_debug("Validating item {}", item.id);
        
        // Validation logic
        if (item.data.empty()) {
            log_module::write_error("Invalid data for item {}", item.id);
            return;
        }
        
        std::lock_guard<std::mutex> lock(queue_mutex_);
        input_queue_.push(item);
    }
    
    void process_queued_items() {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        while (!input_queue_.empty()) {
            auto item = input_queue_.front();
            input_queue_.pop();
            
            // Process data
            double sum = std::accumulate(item.data.begin(), item.data.end(), 0.0);
            log_module::write_information("Processed item {}: sum = {}", item.id, sum);
        }
    }
    
    void cleanup_processed_data() {
        log_module::write_debug("Performing cleanup");
        // Cleanup logic
    }
};
```

## Best Practices

### Resource Management

```cpp
class ResourcePool {
private:
    std::shared_ptr<thread_pool_module::thread_pool> pool_;
    
public:
    ResourcePool() {
        pool_ = std::make_shared<thread_pool_module::thread_pool>();
    }
    
    // RAII pattern for automatic cleanup
    ~ResourcePool() {
        if (pool_) {
            pool_->stop();
        }
    }
    
    void process_with_timeout(std::function<void()> task, 
                             std::chrono::milliseconds timeout) {
        auto done = std::make_shared<std::atomic<bool>>(false);
        
        pool_->enqueue(std::make_unique<thread_module::callback_job>(
            [task, done]() -> thread_module::result_void {
                task();
                done->store(true);
                return {};
            }
        ));
        
        // Wait with timeout
        auto start = std::chrono::steady_clock::now();
        while (!done->load()) {
            if (std::chrono::steady_clock::now() - start > timeout) {
                log_module::write_error("Task timed out");
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
};
```

### Exception Safety

```cpp
template<typename T>
class SafeJobExecutor {
private:
    std::shared_ptr<thread_pool_module::thread_pool> pool_;
    
public:
    SafeJobExecutor(std::shared_ptr<thread_pool_module::thread_pool> pool) 
        : pool_(pool) {}
    
    std::future<std::optional<T>> execute_safe(std::function<T()> task) {
        auto promise = std::make_shared<std::promise<std::optional<T>>>();
        auto future = promise->get_future();
        
        pool_->enqueue(std::make_unique<thread_module::callback_job>(
            [task, promise]() -> thread_module::result_void {
                try {
                    T result = task();
                    promise->set_value(result);
                } catch (const std::exception& e) {
                    log_module::write_error("Job failed: {}", e.what());
                    promise->set_value(std::nullopt);
                } catch (...) {
                    log_module::write_error("Job failed: unknown error");
                    promise->set_value(std::nullopt);
                }
                return {};
            }
        ));
        
        return future;
    }
};
```