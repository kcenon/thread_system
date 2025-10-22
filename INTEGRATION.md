# Integration Guide - Thread System

> **Language:** **English** | [한국어](INTEGRATION_KO.md)

## Overview

This comprehensive guide describes how to integrate thread_system with other modules in the ecosystem. Thread System provides the foundational threading framework and interfaces that enable seamless integration with logger_system, monitoring_system, and other application components.

**Version:** 2.0.0
**Last Updated:** 2025-10-22
**Architecture**: Modular, Interface-Based Design

---

## Table of Contents

- [Quick Start](#quick-start)
- [Architecture Overview](#architecture-overview)
- [Integration with common_system](#integration-with-common_system)
- [Integration with logger_system](#integration-with-logger_system)
- [Integration with monitoring_system](#integration-with-monitoring_system)
- [Integration with network_system](#integration-with-network_system)
- [Build Configuration](#build-configuration)
- [Advanced Integration Patterns](#advanced-integration-patterns)
- [Performance Considerations](#performance-considerations)
- [Troubleshooting](#troubleshooting)
- [Examples](#examples)

---

## Quick Start

### Basic Thread Pool Usage

```cpp
#include <kcenon/thread/core/thread_pool.h>

int main() {
    // Create thread pool with 4 workers
    auto pool = std::make_shared<kcenon::thread::thread_pool>(4);

    // Submit a job
    auto future = pool->post([](const kcenon::thread::job_parameter& param) {
        std::cout << "Hello from thread pool!" << std::endl;
        return param;
    });

    // Wait for completion
    future.wait();

    // Shutdown gracefully
    pool->stop();

    return 0;
}
```

### CMake Integration

```cmake
find_package(thread_system CONFIG REQUIRED)

add_executable(your_app main.cpp)

target_link_libraries(your_app PRIVATE
    kcenon::thread_system
)
```

---

## Architecture Overview

### Modular Ecosystem

Thread System serves as the foundation for a modular ecosystem:

```
┌──────────────────────────────────────────────────────┐
│              Application Layer                       │
│         (Your Production Applications)               │
└────────────────────┬─────────────────────────────────┘
                     │
┌────────────────────▼─────────────────────────────────┐
│          integrated_thread_system                    │
│        (Complete Integration Examples)               │
└─────┬────────────────────────────────┬───────────────┘
      │                                │
      ▼                                ▼
┌─────────────┐  interfaces  ┌─────────────────────┐
│thread_system├─────────────►│ logger_system       │
│  (Core)     │              │ monitoring_system   │
│             │              │ network_system      │
└─────────────┘              └─────────────────────┘
```

### Key Interfaces

Thread System defines standard interfaces implemented by other systems:

```cpp
namespace kcenon::thread::interfaces {

    // Logger interface - implemented by logger_system
    class logger_interface {
    public:
        virtual void log(log_level level, const std::string& message) = 0;
        virtual void log(log_level level, const std::string& message,
                        const std::source_location& location) = 0;
    };

    // Monitoring interface - implemented by monitoring_system
    class monitoring_interface {
    public:
        virtual void record_metric(const std::string& name, double value) = 0;
        virtual void record_event(const std::string& event_name) = 0;
    };

    // Executor interface - implemented by thread pools
    class executor_interface {
    public:
        virtual auto post(std::function<job_parameter(const job_parameter&)> func)
            -> std::future<job_parameter> = 0;
        virtual void stop() = 0;
    };
}
```

---

## Integration with common_system

Thread System integrates with common_system for standardized interfaces and error handling.

### IExecutor Interface Implementation

```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/common/interfaces/executor_interface.h>

// Thread pool implements IExecutor
class thread_pool_executor : public common::interfaces::IExecutor {
private:
    std::shared_ptr<kcenon::thread::thread_pool> pool_;

public:
    explicit thread_pool_executor(std::shared_ptr<kcenon::thread::thread_pool> pool)
        : pool_(std::move(pool)) {}

    auto submit(std::function<void()> task)
        -> std::future<void> override {
        return pool_->post([task](const auto&) {
            task();
            return kcenon::thread::job_parameter{};
        }).then([](auto) { /* void return */ });
    }

    void shutdown() override {
        pool_->stop();
    }
};

// Usage
auto thread_pool = std::make_shared<kcenon::thread::thread_pool>(4);
auto executor = std::make_shared<thread_pool_executor>(thread_pool);

// Use with common_system APIs
executor->submit([]() {
    std::cout << "Task executed via IExecutor" << std::endl;
});
```

### Result<T> Pattern Integration

```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/common/patterns/result.h>

// Return Result<T> from thread pool jobs
auto process_with_error_handling(
    std::shared_ptr<kcenon::thread::thread_pool> pool)
    -> std::future<common::Result<int>> {

    return pool->post([](const auto&) -> common::Result<int> {
        try {
            // Perform operation
            int result = perform_computation();
            return common::ok(result);
        } catch (const std::exception& e) {
            return common::error<int>(
                common::error_codes::INTERNAL_ERROR,
                e.what(),
                "thread_pool_worker"
            );
        }
    });
}

// Usage with monadic operations
auto result_future = process_with_error_handling(pool);
auto result = result_future.get();

if (result.is_ok()) {
    int value = result.value();
    std::cout << "Success: " << value << std::endl;
} else {
    auto error = result.error();
    std::cerr << "Error: " << error.message << std::endl;
}
```

### Build Configuration

```cmake
# Enable common_system integration
find_package(common_system CONFIG REQUIRED)
find_package(thread_system CONFIG REQUIRED)

target_link_libraries(your_app PRIVATE
    kcenon::common_system
    kcenon::thread_system
)
```

---

## Integration with logger_system

Integrate thread_system with logger_system for comprehensive logging of thread operations.

### Logger Interface Implementation

The logger_system implements the `logger_interface` defined in thread_system:

```cpp
#include <kcenon/thread/interfaces/logger_interface.h>
#include <kcenon/logger/core/logger.h>

// Logger system implements thread system's logger interface
class logger_adapter : public kcenon::thread::interfaces::logger_interface {
private:
    std::shared_ptr<kcenon::logger::logger> logger_;

public:
    explicit logger_adapter(std::shared_ptr<kcenon::logger::logger> logger)
        : logger_(std::move(logger)) {}

    void log(log_level level, const std::string& message) override {
        logger_->log(convert_log_level(level), message);
    }

    void log(log_level level, const std::string& message,
            const std::source_location& location) override {
        logger_->log(convert_log_level(level), message, location);
    }

private:
    kcenon::logger::log_level convert_log_level(log_level level) {
        switch (level) {
            case log_level::trace: return kcenon::logger::log_level::trace;
            case log_level::debug: return kcenon::logger::log_level::debug;
            case log_level::info:  return kcenon::logger::log_level::info;
            case log_level::warn:  return kcenon::logger::log_level::warn;
            case log_level::error: return kcenon::logger::log_level::error;
            case log_level::fatal: return kcenon::logger::log_level::fatal;
            default: return kcenon::logger::log_level::info;
        }
    }
};
```

### Injecting Logger into Thread Pool

```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/logger/core/logger.h>

int main() {
    // Create logger
    auto logger = kcenon::logger::logger_builder()
        .set_log_level(kcenon::logger::log_level::debug)
        .add_console_writer()
        .add_file_writer("thread_pool.log")
        .build();

    // Adapt logger to thread system interface
    auto logger_adapter = std::make_shared<logger_adapter>(logger);

    // Create thread pool with logger
    auto pool = std::make_shared<kcenon::thread::thread_pool>(
        4,  // worker count
        logger_adapter,  // logger
        nullptr  // monitoring (optional)
    );

    // Thread pool operations are now logged
    pool->post([](const auto&) {
        // This execution will be logged
        std::cout << "Job executed" << std::endl;
        return kcenon::thread::job_parameter{};
    });

    pool->stop();
    return 0;
}
```

### Logging Thread Pool Events

With logger integration, thread_system automatically logs:

- Worker thread start/stop
- Job submission and execution
- Queue state changes
- Error conditions
- Performance warnings

Example log output:
```
[2025-10-22 19:30:15.123] [INFO] [thread_pool] Thread pool started with 4 workers
[2025-10-22 19:30:15.145] [DEBUG] [thread_worker] Worker thread 1 started
[2025-10-22 19:30:15.146] [DEBUG] [thread_worker] Worker thread 2 started
[2025-10-22 19:30:15.147] [DEBUG] [thread_worker] Worker thread 3 started
[2025-10-22 19:30:15.148] [DEBUG] [thread_worker] Worker thread 4 started
[2025-10-22 19:30:16.234] [INFO] [thread_pool] Job submitted to queue
[2025-10-22 19:30:16.235] [DEBUG] [thread_worker] Worker 2 executing job
[2025-10-22 19:30:16.345] [INFO] [thread_pool] Job completed successfully
[2025-10-22 19:30:20.567] [INFO] [thread_pool] Shutting down thread pool
[2025-10-22 19:30:20.789] [INFO] [thread_pool] All workers stopped gracefully
```

### Build Configuration

```cmake
find_package(thread_system CONFIG REQUIRED)
find_package(logger_system CONFIG REQUIRED)

target_link_libraries(your_app PRIVATE
    kcenon::thread_system
    kcenon::logger_system
)
```

---

## Integration with monitoring_system

Integrate thread_system with monitoring_system for real-time metrics collection and performance tracking.

### Monitoring Interface Implementation

The monitoring_system implements the `monitoring_interface` defined in thread_system:

```cpp
#include <kcenon/thread/interfaces/monitoring_interface.h>
#include <kcenon/monitoring/core/performance_monitor.h>

class monitoring_adapter : public kcenon::thread::interfaces::monitoring_interface {
private:
    std::shared_ptr<kcenon::monitoring::performance_monitor> monitor_;

public:
    explicit monitoring_adapter(
        std::shared_ptr<kcenon::monitoring::performance_monitor> monitor)
        : monitor_(std::move(monitor)) {}

    void record_metric(const std::string& name, double value) override {
        monitor_->record_counter(name, value);
    }

    void record_event(const std::string& event_name) override {
        monitor_->record_event(event_name);
    }

    void start_timer(const std::string& timer_name) override {
        monitor_->start_timer(timer_name);
    }

    void stop_timer(const std::string& timer_name) override {
        monitor_->stop_timer(timer_name);
    }
};
```

### Monitoring Thread Pool Performance

```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/monitoring/core/performance_monitor.h>

int main() {
    // Create performance monitor
    auto monitor = std::make_shared<kcenon::monitoring::performance_monitor>();

    // Adapt to thread system interface
    auto monitoring_adapter = std::make_shared<monitoring_adapter>(monitor);

    // Create thread pool with monitoring
    auto pool = std::make_shared<kcenon::thread::thread_pool>(
        4,  // workers
        nullptr,  // logger (optional)
        monitoring_adapter  // monitoring
    );

    // Thread pool metrics are automatically collected:
    // - Job submission rate
    // - Job execution time
    // - Queue depth
    // - Worker utilization
    // - Throughput

    // Execute jobs - metrics collected automatically
    for (int i = 0; i < 1000; ++i) {
        pool->post([i](const auto&) {
            // Simulate work
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return kcenon::thread::job_parameter{};
        });
    }

    // Retrieve metrics
    auto metrics = monitor->get_metrics();
    std::cout << "Total jobs executed: " << metrics.job_count << std::endl;
    std::cout << "Average execution time: " << metrics.avg_execution_time_ms << " ms" << std::endl;
    std::cout << "Worker utilization: " << metrics.worker_utilization * 100 << "%" << std::endl;

    pool->stop();
    return 0;
}
```

### Collected Metrics

With monitoring integration, thread_system automatically tracks:

| Metric Category | Metrics | Description |
|----------------|---------|-------------|
| **Job Metrics** | `jobs_submitted` | Total jobs submitted to pool |
|  | `jobs_completed` | Total jobs successfully completed |
|  | `jobs_failed` | Total jobs that threw exceptions |
|  | `job_execution_time` | Histogram of execution times |
| **Queue Metrics** | `queue_depth` | Current number of pending jobs |
|  | `queue_wait_time` | Time jobs spend waiting in queue |
|  | `queue_operations` | Enqueue/dequeue operation counts |
| **Worker Metrics** | `worker_idle_time` | Time workers spend idle |
|  | `worker_utilization` | Percentage of time workers are active |
|  | `worker_wake_count` | Number of times workers were woken |
| **Performance** | `throughput` | Jobs per second |
|  | `latency_p50` | 50th percentile latency |
|  | `latency_p95` | 95th percentile latency |
|  | `latency_p99` | 99th percentile latency |

### Build Configuration

```cmake
find_package(thread_system CONFIG REQUIRED)
find_package(monitoring_system CONFIG REQUIRED)

target_link_libraries(your_app PRIVATE
    kcenon::thread_system
    kcenon::monitoring_system
)
```

---

## Integration with network_system

Thread System provides the execution backend for network_system's asynchronous operations.

### Network Event Loop Integration

```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <network_system/messaging_server.h>

int main() {
    // Create thread pool for network operations
    auto thread_pool = std::make_shared<kcenon::thread::thread_pool>(8);

    // Create network server with thread pool backend
    auto server = network_system::messaging_server(
        "0.0.0.0",
        8080,
        thread_pool  // Thread pool handles async I/O
    );

    // Start server - uses thread pool for connection handling
    server.start();

    // Network connections handled by thread pool workers
    // Each connection gets dispatched to available worker

    server.stop();
    thread_pool->stop();

    return 0;
}
```

### Asynchronous Message Processing

```cpp
#include <kcenon/thread/core/typed_thread_pool.h>
#include <network_system/messaging_client.h>

// Define job types for different message priorities
enum class MessagePriority {
    HIGH,
    NORMAL,
    LOW
};

int main() {
    // Create typed thread pool for prioritized message processing
    auto pool = std::make_shared<kcenon::thread::typed_thread_pool<MessagePriority>>(
        6,  // workers
        std::map<MessagePriority, size_t>{
            {MessagePriority::HIGH, 2},    // 2 workers for high priority
            {MessagePriority::NORMAL, 3},  // 3 workers for normal
            {MessagePriority::LOW, 1}      // 1 worker for low priority
        }
    );

    auto client = network_system::messaging_client("localhost", 8080);

    // Process incoming messages with priority routing
    client.on_message_received([&pool](const auto& message) {
        auto priority = determine_priority(message);

        pool->post(priority, [message](const auto&) {
            process_message(message);
            return kcenon::thread::job_parameter{};
        });
    });

    client.connect();

    // High priority messages processed faster due to dedicated workers

    client.disconnect();
    pool->stop();

    return 0;
}
```

### Build Configuration

```cmake
find_package(thread_system CONFIG REQUIRED)
find_package(network_system CONFIG REQUIRED)

target_link_libraries(your_app PRIVATE
    kcenon::thread_system
    kcenon::network_system
)
```

---

## Build Configuration

### CMake Options

```cmake
# Thread system options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_BENCHMARKS "Build performance benchmarks" ON)
option(ENABLE_SANITIZERS "Enable address and thread sanitizers" OFF)

# Integration options (auto-detected if packages found)
option(WITH_LOGGER_SYSTEM "Enable logger_system integration" OFF)
option(WITH_MONITORING_SYSTEM "Enable monitoring_system integration" OFF)
option(WITH_COMMON_SYSTEM "Enable common_system integration" ON)
```

### Full Ecosystem Integration

```cmake
cmake_minimum_required(VERSION 3.16)
project(integrated_application)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find all ecosystem packages
find_package(common_system CONFIG REQUIRED)
find_package(thread_system CONFIG REQUIRED)
find_package(logger_system CONFIG REQUIRED)
find_package(monitoring_system CONFIG REQUIRED)
find_package(network_system CONFIG REQUIRED)

add_executable(my_app
    main.cpp
    application.cpp
)

target_link_libraries(my_app PRIVATE
    kcenon::common_system
    kcenon::thread_system
    kcenon::logger_system
    kcenon::monitoring_system
    kcenon::network_system
)

# Optional: Enable sanitizers for development
if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(my_app PRIVATE
        -fsanitize=address
        -fsanitize=thread
        -fno-omit-frame-pointer
    )
    target_link_options(my_app PRIVATE
        -fsanitize=address
        -fsanitize=thread
    )
endif()
```

---

## Advanced Integration Patterns

### 1. Service Registry Pattern

Use the service registry for dependency injection:

```cpp
#include <kcenon/thread/core/service_registry.h>
#include <kcenon/logger/core/logger.h>
#include <kcenon/monitoring/core/performance_monitor.h>

class Application {
private:
    std::shared_ptr<kcenon::thread::service_registry> registry_;

public:
    Application() : registry_(std::make_shared<kcenon::thread::service_registry>()) {
        // Register services
        registry_->register_service<kcenon::logger::logger>(
            "logger",
            create_logger()
        );

        registry_->register_service<kcenon::monitoring::performance_monitor>(
            "monitor",
            create_monitor()
        );

        registry_->register_service<kcenon::thread::thread_pool>(
            "thread_pool",
            create_thread_pool()
        );
    }

    void run() {
        // Retrieve services
        auto logger = registry_->get_service<kcenon::logger::logger>("logger");
        auto monitor = registry_->get_service<kcenon::monitoring::performance_monitor>("monitor");
        auto pool = registry_->get_service<kcenon::thread::thread_pool>("thread_pool");

        // Use services
        logger->log(kcenon::logger::log_level::info, "Application started");
        monitor->record_event("app_start");

        pool->post([logger](const auto&) {
            logger->log(kcenon::logger::log_level::debug, "Job executed");
            return kcenon::thread::job_parameter{};
        });
    }

private:
    auto create_logger() -> std::shared_ptr<kcenon::logger::logger> {
        return kcenon::logger::logger_builder()
            .set_log_level(kcenon::logger::log_level::debug)
            .add_console_writer()
            .build();
    }

    auto create_monitor() -> std::shared_ptr<kcenon::monitoring::performance_monitor> {
        return std::make_shared<kcenon::monitoring::performance_monitor>();
    }

    auto create_thread_pool() -> std::shared_ptr<kcenon::thread::thread_pool> {
        auto logger_adapter = std::make_shared<logger_adapter>(
            registry_->get_service<kcenon::logger::logger>("logger")
        );
        auto monitoring_adapter = std::make_shared<monitoring_adapter>(
            registry_->get_service<kcenon::monitoring::performance_monitor>("monitor")
        );

        return std::make_shared<kcenon::thread::thread_pool>(
            std::thread::hardware_concurrency(),
            logger_adapter,
            monitoring_adapter
        );
    }
};
```

### 2. Cancellation Token Pattern

```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/cancellation_token.h>

void long_running_task(
    std::shared_ptr<kcenon::thread::thread_pool> pool) {

    // Create cancellation token
    auto token = std::make_shared<kcenon::thread::cancellation_token>();

    // Submit long-running job
    auto future = pool->post([token](const auto&) -> int {
        for (int i = 0; i < 1000; ++i) {
            // Check for cancellation
            if (token->is_cancelled()) {
                std::cout << "Job cancelled at iteration " << i << std::endl;
                return -1;
            }

            // Perform work
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        return 1000;
    });

    // Cancel after 2 seconds
    std::this_thread::sleep_for(std::chrono::seconds(2));
    token->cancel();

    // Wait for cancellation to take effect
    int result = future.get();
    std::cout << "Result: " << result << std::endl;
}
```

### 3. Typed Thread Pool for Domain-Specific Processing

```cpp
#include <kcenon/thread/core/typed_thread_pool.h>

enum class TaskType {
    COMPUTE,
    IO,
    NETWORK
};

class DataProcessor {
private:
    std::shared_ptr<kcenon::thread::typed_thread_pool<TaskType>> pool_;

public:
    DataProcessor() {
        pool_ = std::make_shared<kcenon::thread::typed_thread_pool<TaskType>>(
            9,  // total workers
            std::map<TaskType, size_t>{
                {TaskType::COMPUTE, 4},   // 4 workers for compute-heavy tasks
                {TaskType::IO, 3},        // 3 workers for I/O tasks
                {TaskType::NETWORK, 2}    // 2 workers for network tasks
            }
        );
    }

    void process_data(const std::vector<Data>& data) {
        for (const auto& item : data) {
            TaskType type = classify_task(item);

            pool_->post(type, [item](const auto&) {
                process_item(item);
                return kcenon::thread::job_parameter{};
            });
        }
    }

    ~DataProcessor() {
        pool_->stop();
    }
};
```

---

## Performance Considerations

### 1. Worker Count Optimization

Choose worker count based on workload:

```cpp
// CPU-bound tasks: Use hardware concurrency
auto cpu_pool = std::make_shared<kcenon::thread::thread_pool>(
    std::thread::hardware_concurrency()
);

// I/O-bound tasks: Can exceed hardware threads
auto io_pool = std::make_shared<kcenon::thread::thread_pool>(
    std::thread::hardware_concurrency() * 2
);

// Mixed workload: Use typed thread pool
auto mixed_pool = std::make_shared<kcenon::thread::typed_thread_pool<WorkloadType>>(
    16,
    std::map<WorkloadType, size_t>{
        {WorkloadType::CPU, std::thread::hardware_concurrency()},
        {WorkloadType::IO, std::thread::hardware_concurrency()}
    }
);
```

### 2. Queue Selection

Thread System provides adaptive queues that automatically optimize:

```cpp
// Adaptive queue automatically selects best strategy
// - Low contention: Mutex-based (96 ns latency)
// - High contention: Lock-free (320 ns latency, 37% faster)
auto pool = std::make_shared<kcenon::thread::thread_pool>(8);

// Adaptive queue switches strategy based on load
// No manual configuration needed!
```

### 3. Minimize Job Overhead

```cpp
// BAD: Small jobs with high overhead
for (int i = 0; i < 1000000; ++i) {
    pool->post([i](const auto&) {
        return i + 1;  // Too small, overhead dominates
    });
}

// GOOD: Batch small jobs
const int batch_size = 1000;
for (int batch = 0; batch < 1000; ++batch) {
    pool->post([batch, batch_size](const auto&) {
        int sum = 0;
        for (int i = 0; i < batch_size; ++i) {
            sum += (batch * batch_size + i) + 1;
        }
        return sum;
    });
}
```

### 4. Memory Management

```cpp
// Use shared_ptr for thread-safe reference counting
auto shared_data = std::make_shared<LargeData>();

pool->post([shared_data](const auto&) {
    // Data kept alive by shared_ptr
    process(*shared_data);
    return kcenon::thread::job_parameter{};
});

// shared_data automatically freed when last reference drops
```

---

## Troubleshooting

### Common Issues

#### 1. "Thread pool not shutting down gracefully"

**Symptom**: Application hangs on exit

**Solution**: Ensure all jobs complete and call `stop()`

```cpp
// BAD: Infinite jobs
pool->post([](const auto&) {
    while (true) { /* ... */ }  // Never finishes!
    return kcenon::thread::job_parameter{};
});

// GOOD: Use cancellation token
auto token = std::make_shared<kcenon::thread::cancellation_token>();
pool->post([token](const auto&) {
    while (!token->is_cancelled()) { /* ... */ }
    return kcenon::thread::job_parameter{};
});

// Later: cancel and stop
token->cancel();
pool->stop();
```

#### 2. "Logger/Monitor integration not working"

**Symptom**: No logs or metrics collected

**Solution**: Ensure adapters are properly created and injected

```cpp
// Verify logger is set
auto pool = std::make_shared<kcenon::thread::thread_pool>(
    4,
    logger_adapter,  // Must not be nullptr
    monitoring_adapter  // Must not be nullptr
);

// Check adapter implementation
class MyLoggerAdapter : public kcenon::thread::interfaces::logger_interface {
    void log(log_level level, const std::string& message) override {
        // MUST be implemented!
        std::cout << message << std::endl;
    }
};
```

#### 3. "Poor performance with many workers"

**Symptom**: Adding more workers doesn't improve throughput

**Solution**: Check for contention and adjust worker count

```cpp
// Too many workers cause contention
auto bad_pool = std::make_shared<kcenon::thread::thread_pool>(64);  // Too many!

// Optimal: Match hardware
auto good_pool = std::make_shared<kcenon::thread::thread_pool>(
    std::thread::hardware_concurrency()
);

// Or use typed pool for workload separation
auto typed_pool = std::make_shared<kcenon::thread::typed_thread_pool<Type>>(
    std::thread::hardware_concurrency(),
    type_distribution
);
```

---

## Examples

### Complete Integration Example

See [examples/integration_example/](examples/integration_example/) for a complete application demonstrating:
- Thread pool with logger and monitoring
- Network server using thread pool backend
- Service registry for dependency injection
- Cancellation token for graceful shutdown
- Performance metrics collection

### Running Examples

```bash
cd thread_system
mkdir build && cd build
cmake .. -DBUILD_EXAMPLES=ON
make
./examples/integration_example
```

---

## Support

- **Documentation**: [docs/](docs/)
- **API Reference**: [docs/API_REFERENCE.md](docs/API_REFERENCE.md)
- **Architecture Guide**: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- **Issues**: [GitHub Issues](https://github.com/kcenon/thread_system/issues)
- **Email**: kcenon@naver.com

---

**Last Updated**: 2025-10-22
**Maintainer**: kcenon@naver.com
**Version**: 2.0.0
