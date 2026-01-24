# Thread System API Reference

> **Language:** [English](API_REFERENCE.md) | **한국어**

Thread System framework의 완전한 API 문서입니다.

## 목차

1. [개요](#개요)
2. [Core Module](#core-module-thread_module)
   - [thread_base Class](#thread_base-class)
   - [job Class](#job-class)
   - [job_queue Class](#job_queue-class)
   - [result Template](#resultt-template)
3. [Synchronization Module](#synchronization-module)
   - [sync_primitives](#sync_primitives)
   - [cancellation_token](#cancellation_token)
4. [Dependency Injection](#dependency-injection)
   - [service_container](#service_container)
   - [service_registry](#service_registry)
5. [Thread Pool Module](#thread-pool-module-thread_pool_module)
   - [thread_pool Class](#thread_pool-class)
   - [thread_worker Class](#thread_worker-class)
6. [Typed Thread Pool Module](#typed-thread-pool-module-typed_thread_pool_module)
   - [typed_thread_pool_t Template](#typed_thread_pool_t-template)
   - [typed_thread_worker_t Template](#typed_thread_worker_t-template)
   - [job_types Enumeration](#job_types-enumeration)
7. [Interfaces](#interfaces)
   - [개요](#interfaces-overview)
   - [Executor Interface](#executor-interface)
   - [Scheduler Interface](#scheduler-interface)
   - [Logging Interface](#logging-interface-and-registry)
   - [Monitoring Interface](#monitoring-interface)
   - [Monitorable Interface](#monitorable-interface)
   - [Thread Context and Service Container](#thread-context-and-service-container)
   - [Error Handling](#error-handling)
   - [Crash Handling](#crash-handling)
   - [Typed Thread Pool Interfaces](#typed-thread-pool-interfaces)
8. [External Modules](#external-modules)
   - [Logger System](#logger-system)
   - [Monitoring System](#monitoring-system)
9. [Utilities Module](#utilities-module-utility_module)
   - [formatter_macros](#formatter-macros)
   - [convert_string](#convert_string)
10. [빠른 참조](#빠른-참조)

## 개요

Thread System framework는 적응형 성능 최적화를 갖춘 멀티스레드 애플리케이션을 구축하기 위한 포괄적인 클래스 세트를 제공합니다.

### 핵심 구성 요소

- **Core Classes** (`thread_module` namespace)
  - `thread_base` - 모든 worker의 추상 기본 클래스
  - `job` - 작업 단위 표현
  - `job_queue` - Thread-safe job queue
  - `result<T>` - 오류 처리 타입
  - `callback_job` - lambda 지원이 있는 job 구현

### Thread Pool 구성 요소

- **Standard Thread Pool** (`thread_pool_module` namespace)
  - `thread_pool` - 적응형 queue 최적화를 갖춘 thread pool
  - `thread_worker` - Worker thread 구현

### Typed Thread Pool 구성 요소

- **Type-based Priority Pool** (`typed_thread_pool_module` namespace)
  - `typed_thread_pool_t<T>` - Template 기반 우선순위 pool
  - `typed_thread_worker_t<T>` - 우선순위 인식 worker
  - `typed_job_t<T>` - 타입 정보가 있는 job
  - `job_types` - 기본 우선순위 열거형 (RealTime, Batch, Background)

### 고급 기능

- **Adaptive Queue System**: mutex와 lock-free 전략 간 자동 전환
- **Enhanced Synchronization**: timeout 지원이 있는 포괄적인 동기화 래퍼
- **Cancellation Support**: 연결된 token 계층 구조를 사용한 협력적 취소
- **Dependency Injection**: thread-safe DI를 위한 `service_container` 권장. `service_registry`는 경량 header-only 대안
- **Error Handling**: 강력한 오류 관리를 위한 현대적 result<T> 패턴
- **Interface-Based Design**: 잘 정의된 interface를 통한 깔끔한 관심사 분리

## Core Module (thread_module)

### thread_base Class

시스템의 모든 worker thread를 위한 추상 기본 클래스입니다.

```cpp
class thread_base {
public:
    thread_base(const std::string& thread_title = "thread_base");
    virtual ~thread_base();

    // Thread control
    auto start() -> result_void;
    auto stop() -> result_void;

    // Configuration
    auto set_wake_interval(const std::optional<std::chrono::milliseconds>& interval) -> void;
    auto get_wake_interval() const -> std::optional<std::chrono::milliseconds>;

    // Status
    auto is_running() const -> bool;
    auto get_thread_title() const -> std::string;

protected:
    // Override in derived classes
    virtual auto before_start() -> result_void { return {}; }
    virtual auto do_work() -> result_void = 0;
    virtual auto after_stop() -> result_void { return {}; }
    virtual auto should_continue_work() const -> bool { return false; }
};
```

### job Class

모든 작업 단위를 위한 기본 클래스입니다.

```cpp
class job {
public:
    job(const std::string& name = "job");
    job(const std::vector<uint8_t>& data, const std::string& name = "data_job");
    virtual ~job();

    // Core functionality
    virtual auto do_work() -> result_void;

    // Cancellation support
    virtual auto set_cancellation_token(const cancellation_token& token) -> void;
    virtual auto get_cancellation_token() const -> cancellation_token;

    // Queue association
    virtual auto set_job_queue(const std::shared_ptr<job_queue>& queue) -> void;
    virtual auto get_job_queue() const -> std::shared_ptr<job_queue>;

    // Metadata
    auto get_name() const -> std::string;
    virtual auto to_string() const -> std::string;
};
```

### job_queue Class

job 관리를 위한 thread-safe queue입니다.

```cpp
class job_queue : public std::enable_shared_from_this<job_queue> {
public:
    job_queue();
    virtual ~job_queue();

    // Queue operations
    [[nodiscard]] virtual auto enqueue(std::unique_ptr<job>&& value) -> result_void;
    [[nodiscard]] virtual auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> result_void;
    [[nodiscard]] virtual auto dequeue() -> result<std::unique_ptr<job>>;
    [[nodiscard]] virtual auto dequeue_batch() -> std::deque<std::unique_ptr<job>>;

    // State management
    virtual auto clear() -> void;
    auto stop() -> void;

    // Status
    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> std::size_t;
    [[nodiscard]] auto is_stopped() const -> bool;

    // Configuration
    auto set_notify(bool notify) -> void;
};
```

### result<T> Template

`std::expected` 패턴 기반 오류 처리 타입입니다.

```cpp
template<typename T>
using result = std::expected<T, error>;

using result_void = result<void>;

struct error {
    error_code code;
    std::string message;
};

enum class error_code {
    success = 0,
    invalid_argument,
    resource_allocation_failed,
    operation_canceled,
    thread_start_failure,
    job_execution_failed,
    queue_full,
    queue_empty
};
```

## Synchronization Module

### sync_primitives

표준 라이브러리 primitive보다 향상된 기능을 제공하는 포괄적인 동기화 래퍼 유틸리티입니다.

```cpp
#include "core/sync/include/sync_primitives.h"

// Scoped lock guard with timeout support
class scoped_lock_guard {
public:
    template<typename Mutex>
    explicit scoped_lock_guard(Mutex& mutex,
                              std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    [[nodiscard]] bool owns_lock() const noexcept;
    explicit operator bool() const noexcept;
};

// Enhanced condition variable wrapper
class condition_variable_wrapper {
public:
    template<typename Predicate>
    void wait(std::unique_lock<std::mutex>& lock, Predicate pred);

    template<typename Rep, typename Period, typename Predicate>
    bool wait_for(std::unique_lock<std::mutex>& lock,
                  const std::chrono::duration<Rep, Period>& rel_time,
                  Predicate pred);

    void notify_one() noexcept;
    void notify_all() noexcept;
};

// Atomic flag with wait/notify support
class atomic_flag_wrapper {
public:
    void test_and_set(std::memory_order order = std::memory_order_seq_cst) noexcept;
    void clear(std::memory_order order = std::memory_order_seq_cst) noexcept;
    void wait(bool old, std::memory_order order = std::memory_order_seq_cst) const noexcept;
    void notify_one() noexcept;
    void notify_all() noexcept;
};

// Shared mutex wrapper for reader-writer locks
class shared_mutex_wrapper {
public:
    void lock();
    bool try_lock();
    void unlock();
    void lock_shared();
    bool try_lock_shared();
    void unlock_shared();
};
```

### cancellation_token

장시간 실행되는 작업을 위한 협력적 취소 메커니즘을 제공합니다.

```cpp
#include "core/sync/include/cancellation_token.h"

class cancellation_token {
public:
    // Create a new cancellation token
    static cancellation_token create();

    // Create a linked token (cancelled when any parent is cancelled)
    static cancellation_token create_linked(
        std::initializer_list<cancellation_token> tokens);

    // Cancel the operation
    void cancel();

    // Check cancellation status
    [[nodiscard]] bool is_cancelled() const;

    // Throw if cancelled
    void throw_if_cancelled() const;

    // Register callback for cancellation
    void register_callback(std::function<void()> callback);
};

// Usage example
auto token = cancellation_token::create();
auto linked = cancellation_token::create_linked({token, other_token});

token.register_callback([]() {
    std::cout << "Operation cancelled" << std::endl;
});

// In worker thread
while (!token.is_cancelled()) {
    // Do work
    token.throw_if_cancelled(); // Throws if cancelled
}
```

## Dependency Injection

### service_container

Thread System의 권장 DI: singleton/factory lifetime을 갖춘 thread-safe container입니다.

```cpp
#include "interfaces/service_container.h"
#include "interfaces/thread_context.h"

// Register services globally
auto& container = thread_module::service_container::global();
container.register_singleton<thread_module::logger_interface>(my_logger);
container.register_singleton<monitoring_interface::monitoring_interface>(my_monitoring);

// Resolve via thread_context (pools/workers use this context)
thread_module::thread_context ctx; // resolves from global container
```

### service_registry

간단한 시나리오를 위한 경량 header-only 대안입니다.

```cpp
#include "core/base/include/service_registry.h"

// Register a service instance
thread_module::service_registry::register_service<MyInterface>(std::make_shared<MyImpl>());

// Retrieve a service instance
auto svc = thread_module::service_registry::get_service<MyInterface>();
```

## Thread Pool Module (thread_pool_module)

### thread_pool Class

적응형 queue 최적화를 갖춘 메인 thread pool 구현입니다.

```cpp
class thread_pool : public std::enable_shared_from_this<thread_pool> {
public:
    thread_pool(const std::string& thread_title = "thread_pool");
    virtual ~thread_pool();

    // Pool control
    auto start() -> std::optional<std::string>;
    auto stop(const bool& immediately_stop = false) -> void;

    // Job management
    auto enqueue(std::unique_ptr<job>&& job) -> std::optional<std::string>;
    auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> std::optional<std::string>;

    // Worker management
    auto enqueue(std::unique_ptr<thread_worker>&& worker) -> std::optional<std::string>;
    auto enqueue_batch(std::vector<std::unique_ptr<thread_worker>>&& workers) -> std::optional<std::string>;

    // Access
    auto get_job_queue() -> std::shared_ptr<job_queue>;
    auto get_workers() const -> const std::vector<std::unique_ptr<thread_worker>>&;
};
```

### thread_worker Class

적응형 기능을 갖춘 worker thread 구현입니다.

```cpp
class thread_worker : public thread_base {
public:
    struct worker_statistics {
        uint64_t jobs_processed;
        uint64_t total_processing_time_ns;
        uint64_t batch_operations;
        uint64_t avg_processing_time_ns;
    };

    thread_worker(const std::string& name = "thread_worker");

    // Configuration
    auto set_batch_processing(bool enabled, size_t batch_size = 32) -> void;
    auto get_statistics() const -> worker_statistics;

    // Queue association
    auto set_job_queue(const std::shared_ptr<job_queue>& queue) -> void;

protected:
    auto do_work() -> result_void override;
};
```

## Typed Thread Pool Module (typed_thread_pool_module)

### typed_thread_pool_t Template

타입별 queue를 갖춘 우선순위 기반 thread pool입니다.

```cpp
template<typename T>
class typed_thread_pool_t : public std::enable_shared_from_this<typed_thread_pool_t<T>> {
public:
    typed_thread_pool_t(const std::string& name = "typed_thread_pool");

    // Pool control
    auto start() -> result_void;
    auto stop(bool clear_queue = false) -> result_void;

    // Job management
    auto enqueue(std::unique_ptr<typed_job_t<T>>&& job) -> result_void;
    auto enqueue_batch(std::vector<std::unique_ptr<typed_job_t<T>>>&& jobs) -> result_void;

    // Worker management
    auto add_worker(std::unique_ptr<typed_thread_worker_t<T>>&& worker) -> result_void;
    auto add_workers(std::vector<std::unique_ptr<typed_thread_worker_t<T>>>&& workers) -> result_void;

    // Queue strategy (adaptive mode)
    auto set_queue_strategy(queue_strategy strategy) -> void;
};
```

### typed_thread_worker_t Template

타입 기반 job 처리를 갖춘 worker입니다.

```cpp
template<typename T>
class typed_thread_worker_t : public thread_base {
public:
    typed_thread_worker_t(std::vector<T> types = get_all_job_types(),
                         const bool& use_time_tag = true,
                         const thread_context& context = thread_context());

    // Type responsibilities
    auto types() const -> std::vector<T>;

    // Queue association (priority aging 지원을 위해 aging queue 사용)
    auto set_aging_job_queue(std::shared_ptr<aging_typed_job_queue_t<T>> job_queue) -> void;

    // Context management
    auto set_context(const thread_context& context) -> void;
    auto get_context() const -> const thread_context&;
};
```

### job_types Enumeration

typed pool을 위한 기본 우선순위 레벨입니다.

```cpp
enum class job_types : uint8_t {
    Background = 0,  // Lowest priority
    Batch = 1,       // Medium priority
    RealTime = 2     // Highest priority
};
```

## Interfaces

이 섹션은 Thread System의 컴포넌트를 분리하고 dependency injection을 가능하게 하는 공용 interface의 포괄적인 개요를 제공합니다.

### Interfaces Overview

thread system은 깔끔한 관심사 분리와 쉬운 확장성을 위해 interface 기반 설계를 사용합니다. 이러한 interface는 dependency injection을 가능하게 하고 컴포넌트를 쉽게 교체하거나 확장할 수 있게 합니다.

### Executor Interface

Header: `interfaces/executor_interface.h`

모든 executor 구현(thread pool)을 위한 기본 interface입니다.

```cpp
#include "interfaces/executor_interface.h"

class executor_interface {
public:
    virtual ~executor_interface() = default;

    // Execute a job
    virtual auto execute(std::unique_ptr<thread_module::job>&& work) -> thread_module::result_void = 0;

    // Execute multiple jobs
    virtual auto execute_batch(std::vector<std::unique_ptr<job>> jobs) -> result_void = 0;

    // Shutdown the executor
    virtual auto shutdown() -> thread_module::result_void = 0;

    // Check if executor is running
    [[nodiscard]] virtual auto is_running() const -> bool = 0;
};
```

구현: `implementations/thread_pool/thread_pool`

예제:
```cpp
auto pool = std::make_shared<thread_pool_module::thread_pool>("pool");
pool->enqueue_batch({std::make_unique<thread_pool_module::thread_worker>(false)});
pool->start();
pool->execute(std::make_unique<thread_module::callback_job>([](){ return thread_module::result_void(); }));
pool->shutdown();
```

### Scheduler Interface

Header: `interfaces/scheduler_interface.h`

job 스케줄링 전략을 위한 interface입니다.

```cpp
#include "interfaces/scheduler_interface.h"

class scheduler_interface {
public:
    virtual ~scheduler_interface() = default;

    // Schedule a job
    virtual auto schedule(std::unique_ptr<thread_module::job>&& work) -> thread_module::result_void = 0;

    // Get next job to execute
    virtual auto get_next_job() -> thread_module::result<std::unique_ptr<thread_module::job>> = 0;

    // Check if there are pending jobs
    [[nodiscard]] virtual auto has_pending() const -> bool = 0;
};
```

구현: `core/jobs/job_queue` 및 파생 클래스 (`lockfree_job_queue`, `adaptive_job_queue`)

### Logging Interface and Registry

Header: `interfaces/logger_interface.h`

- `logger_interface::log(level, message[, file, line, function])`
- `logger_registry::set_logger(std::shared_ptr<logger_interface>)`
- 편의 매크로: `THREAD_LOG_INFO("message")` 등

### Monitoring Interface

Header: `interfaces/monitoring_interface.h`

데이터 구조:
- `system_metrics`, `thread_pool_metrics` (`pool_name`/`pool_instance_id` 지원), `worker_metrics`
- `metrics_snapshot`

주요 메서드:
- `update_system_metrics(const system_metrics&)`
- `update_thread_pool_metrics(const thread_pool_metrics&)`
- `update_thread_pool_metrics(const std::string& pool_name, std::uint32_t pool_instance_id, const thread_pool_metrics&)`
- `update_worker_metrics(std::size_t worker_id, const worker_metrics&)`
- `get_current_snapshot()`, `get_recent_snapshots(size_t)`

유틸리티:
- `null_monitoring` — no-op 구현
- `scoped_timer(std::atomic<std::uint64_t>& target)` — RAII 측정 helper

### Monitorable Interface

Header: `interfaces/monitorable_interface.h`

- `auto get_metrics() -> monitoring_interface::metrics_snapshot`
- `void reset_metrics()`

컴포넌트 metric을 균일하게 노출하는 데 사용합니다.

### Thread Context and Service Container

Headers: `interfaces/thread_context.h`, `interfaces/service_container.h`

`thread_context`는 다음에 대한 접근을 제공합니다:
- `std::shared_ptr<logger_interface> logger()`
- `std::shared_ptr<monitoring_interface::monitoring_interface> monitoring()`
- 서비스를 사용할 수 있을 때 안전하게 로그 및 metric을 업데이트하는 helper 메서드

`service_container`는 thread-safe DI container입니다:
- `register_singleton<Interface>(std::shared_ptr<Interface>)`
- `register_factory<Interface>(std::function<std::shared_ptr<Interface>()>, lifetime)`
- `resolve<Interface>() -> std::shared_ptr<Interface>`

범위 내에서 등록하려면 `scoped_service<Interface>`를 사용합니다.

### Error Handling

Header: `core/sync/include/error_handling.h`, `interfaces/error_handler.h`

- 강력한 타입의 `error_code`, `error`, `result<T>`/`result_void`
- `error_handler` interface 및 `default_error_handler` 구현

### Crash Handling

Header: `interfaces/crash_handler.h`

- crash callback 및 cleanup routine 등록이 있는 전역 `crash_handler::instance()`
- 구성 가능한 안전 수준, core dump, stack trace 및 로그 경로
- RAII helper `scoped_crash_callback`

### Typed Thread Pool Interfaces

`implementations/typed_thread_pool/include` 아래의 Header:
- `typed_job_interface`, `job_types`, `type_traits`
- Lock-free/adaptive typed queue 및 typed worker/pool

이를 통해 이기종 workload에 대한 타입별 라우팅 및 우선순위 스케줄링이 가능합니다.

## External Modules

더 나은 모듈성과 재사용성을 위해 다음 모듈이 별도 프로젝트로 이동되었습니다:

### Logger System

**Repository**: https://github.com/kcenon/logger_system

비동기 로깅 기능은 이제 `logger_interface`를 구현하는 별도 프로젝트로 제공됩니다. 다음을 제공합니다:

- **고성능 비동기 로깅**: 여러 출력 대상 지원
- **구조화된 로깅**: type-safe 형식 지정
- **로그 레벨 필터링**: bitwise 활성화 카테고리
- **파일 rotation**: 압축 지원
- **Thread-safe 작업**: 최소한의 성능 영향

### Monitoring System

**Repository**: https://github.com/kcenon/monitoring_system

성능 모니터링 및 metric 수집 시스템은 이제 `monitoring_interface`를 구현하는 별도 프로젝트입니다. 다음을 제공합니다:

- **실시간 성능 metric**: 수집 및 분석
- **시스템 리소스 모니터링**: CPU, 메모리, thread 사용량
- **Thread pool 통계**: 추적 및 보고
- **기록 데이터 보존**: 구성 가능한 기간
- **확장 가능한 metric framework**: 사용자 정의 측정

### Integration with Thread System

이러한 외부 모듈은 interface 패턴을 통해 원활하게 통합됩니다:

```cpp
// Example: Register external logger and create a pool
#include "interfaces/service_container.h"
#include "implementations/thread_pool/include/thread_pool.h"
#include <logger_system/logger.h>  // External project

// Register logger with the global container
thread_module::service_container::global()
  .register_singleton<thread_module::logger_interface>(
      std::make_shared<logger_module::logger>());

// Create a pool (thread_context resolves logger automatically)
auto pool = std::make_shared<thread_pool_module::thread_pool>("MyPool");
```

## Utilities Module (utility_module)

### formatter_macros

formatter 코드 중복을 줄이기 위한 매크로입니다.

```cpp
#include "utilities/core/formatter_macros.h"

// Generate formatter specializations for custom types
DECLARE_FORMATTER(my_namespace::my_class)
```

### convert_string

문자열 변환 유틸리티입니다.

```cpp
namespace convert_string {
    auto to_string(const std::wstring& str) -> std::string;
    auto to_wstring(const std::string& str) -> std::wstring;
    auto to_utf8(const std::wstring& str) -> std::string;
    auto from_utf8(const std::string& str) -> std::wstring;
}
```

## 빠른 참조

### 기본 Thread Pool 생성

```cpp
#include "implementations/thread_pool/include/thread_pool.h"
#include "core/jobs/include/callback_job.h"

// Create and configure pool
auto pool = std::make_shared<thread_pool>("MyPool");

// Add workers
for (int i = 0; i < 4; ++i) {
    pool->enqueue(std::make_unique<thread_worker>());
}

// Start pool
if (auto error = pool->start(); error.has_value()) {
    std::cerr << "Failed to start pool: " << *error << std::endl;
    return;
}

// Submit jobs with cancellation support
auto token = cancellation_token::create();
auto job = std::make_unique<callback_job>([]() -> result_void {
    // Do work
    return {};
});
job->set_cancellation_token(token);
pool->enqueue(std::move(job));

// Cancel if needed
token.cancel();

// Stop pool
pool->stop();
```

### Typed Thread Pool 사용

```cpp
// Create typed pool with priorities
auto typed_pool = std::make_shared<typed_thread_pool_t<job_types>>();

// Add specialized workers
auto realtime_worker = std::make_unique<typed_thread_worker_t<job_types>>();
realtime_worker->set_responsibilities({job_types::RealTime});
typed_pool->add_worker(std::move(realtime_worker));

// Start pool
typed_pool->start();

// Submit priority job
auto priority_job = std::make_unique<callback_typed_job<job_types>>(
    job_types::RealTime,
    []() -> result_void {
        // High priority work
        return {};
    }
);
typed_pool->enqueue(std::move(priority_job));
```

### 오류 처리 예제

```cpp
// Using result type for error handling
auto process_with_result() -> result_void {
    auto pool = std::make_shared<thread_pool>();

    // Start pool with error checking
    if (auto error = pool->start(); error.has_value()) {
        return thread_module::error{
            thread_module::error_code::thread_start_failure,
            *error
        };
    }

    // Submit job with error handling
    auto job = std::make_unique<callback_job>([]() -> result_void {
        // Simulate potential failure
        if (some_condition_fails()) {
            return thread_module::error{
                thread_module::error_code::job_execution_failed,
                "Operation failed due to invalid state"
            };
        }
        return {}; // Success
    });

    if (auto error = pool->enqueue(std::move(job)); error.has_value()) {
        return thread_module::error{
            thread_module::error_code::queue_full,
            *error
        };
    }

    return {}; // Success
}
```

### Batch 처리 예제

```cpp
// Configure worker for batch processing
auto worker = std::make_unique<thread_worker>("BatchWorker");
worker->set_batch_processing(true, 64); // Enable with batch size 64

// Submit multiple jobs for efficient batch processing
std::vector<std::unique_ptr<job>> batch;
for (int i = 0; i < 1000; ++i) {
    batch.push_back(std::make_unique<callback_job>([i]() -> result_void {
        // Process item i
        return {};
    }));
}

// Enqueue entire batch
pool->enqueue_batch(std::move(batch));
```

### Service Container 사용

```cpp
#include "interfaces/service_container.h"
#include "implementations/thread_pool/include/thread_pool.h"

// Register thread pool as executor service
auto pool = std::make_shared<thread_pool_module::thread_pool>("AppPool");
thread_module::service_container::global()
  .register_singleton<thread_module::executor_interface>(pool);

// Optional: register external logger implementation
// thread_module::service_container::global()
//   .register_singleton<thread_module::logger_interface>(
//       std::make_shared<external_logger>());

// Use services later via thread_context or resolve<>()
```

### 고급 동기화 예제

```cpp
#include "core/sync/include/sync_primitives.h"

// Using scoped lock with timeout
std::mutex resource_mutex;
{
    scoped_lock_guard lock(resource_mutex, std::chrono::milliseconds(100));
    if (lock.owns_lock()) {
        // Successfully acquired lock within timeout
        // Access protected resource
    } else {
        // Timeout occurred
        // Handle timeout scenario
    }
}

// Using enhanced condition variable
condition_variable_wrapper cv;
std::mutex cv_mutex;
bool ready = false;

// Producer thread
{
    std::unique_lock<std::mutex> lock(cv_mutex);
    ready = true;
    cv.notify_one();
}

// Consumer thread with timeout
{
    std::unique_lock<std::mutex> lock(cv_mutex);
    if (cv.wait_for(lock, std::chrono::seconds(1), []{ return ready; })) {
        // Condition met within timeout
    } else {
        // Timeout occurred
    }
}
```

---

*Last Updated: 2025-10-20*
