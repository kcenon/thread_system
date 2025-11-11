# Thread System: 패턴, 모범 사례 및 문제 해결 가이드

> **Language:** [English](PATTERNS.md) | **한국어**

이 포괄적인 가이드는 Thread System 작업 시 패턴, 모범 사례, 피해야 할 안티패턴 및 일반적인 동시성 문제에 대한 해결책을 다룹니다. 이러한 지침을 따르면 효율적이고 유지 관리가 가능하며 버그 없는 동시 애플리케이션을 작성할 수 있습니다.

## 목차

1. [모범 사례](#모범-사례)
2. [일반 패턴](#일반-패턴)
3. [피해야 할 안티패턴](#피해야-할-안티패턴)
4. [일반적인 문제 해결](#일반적인-문제-해결)
5. [고급 동시성 패턴](#고급-동시성-패턴)
6. [동시성 코드 디버깅](#동시성-코드-디버깅)
7. [성능 최적화](#성능-최적화)
8. [외부 모듈 통합](#외부-모듈-통합)

## 모범 사례

### 1. Thread Base 사용

#### ✅ DO:
- 사용자 정의 worker thread를 구현할 때 `thread_base`에서 파생
- `before_start()`, `do_work()`, `after_stop()`을 재정의하여 동작 사용자 정의
- `after_stop()` 메서드에서 적절한 정리 구현
- 주기적인 작업을 위해 `set_wake_interval()` 메서드 사용
- 장시간 실행되는 작업에서 정기적으로 `should_continue_work()` 확인

#### ❌ DON'T:
- 생성자 내에서 `start()` 호출
- 기본 thread를 직접 조작
- 조건 확인 없이 긴밀한 루프 구현
- `start()` 및 `stop()`의 반환 값 무시
- `do_work()` 내에서 적절한 동기화 없이 thread-unsafe 작업 사용

### 2. Thread Pool 사용

#### ✅ DO:
- CPU 바운드 작업에 `thread_pool` 사용
- 적절한 수의 worker thread 생성(일반적으로 코어 수 또는 코어 수 + 1)
- 가능한 경우 `enqueue_batch()`를 사용하여 batch 제출
- 대부분의 작업에 callback job 사용
- job 실행에서 반환된 오류를 적절히 처리

#### ❌ DON'T:
- 너무 많은 thread pool 생성(애플리케이션당 하나면 충분한 경우가 많음)
- 과도한 수의 worker thread 생성(context switching 오버헤드 발생 가능)
- 신중한 고려 없이 I/O 바운드 작업에 thread pool 사용
- 긴밀한 루프에서 개별 job 제출(대신 batch 제출 사용)
- 장시간 실행되는 동기 작업으로 worker thread 차단

### 3. Type Thread Pool 사용

#### ✅ DO:
- 다양한 유형의 작업에 대해 별개의 type 레벨 사용
- 중요한 type 레벨을 위한 전용 worker 생성
- 백그라운드 또는 유지 관리 작업에 낮은 type 사용
- 도메인별 스케줄링을 위한 사용자 정의 type 타입 고려
- type별 queue 크기를 모니터링하여 균형 잡힌 실행 보장

#### ❌ DON'T:
- 모든 작업에 높은 type 할당(목적 상실)
- 너무 많은 type 레벨 생성(3-5 레벨이 일반적으로 충분)
- type inversion 문제 무시
- 애플리케이션 전체에서 일관되지 않게 type 사용
- 적절한 job을 할당하지 않고 type worker 생성

### 4. 오류 처리

#### ✅ DO:
- 항상 Thread System 함수의 반환 값 확인
- `result<T>` 또는 `std::optional<std::string>` 오류 패턴 사용
- 필요한 경우 사용자 정의 오류 handler 구현
- 컨텍스트가 있는 의미 있는 오류 메시지 제공
- 각 레이어에서 오류를 적절히 처리

#### ❌ DON'T:
- 오류 반환 값 무시
- worker thread에서 처리 없이 예외가 전파되도록 허용
- 컨텍스트 없는 일반 오류 메시지 사용
- 작업이 항상 성공할 것이라고 가정
- 다른 오류 처리 패턴을 일관되지 않게 혼합

## 일반 패턴

### 1. Worker Thread 패턴

```cpp
class MyWorker : public thread_module::thread_base {
protected:
    auto before_start() -> result_void override {
        // Initialize resources
        return {};
    }

    auto do_work() -> result_void override {
        // Perform work here
        return {};
    }

    auto should_continue_work() const -> bool override {
        // Logic to determine if more work is needed
        return !work_queue_.empty();
    }

    auto after_stop() -> result_void override {
        // Cleanup resources
        return {};
    }

private:
    // Worker-specific members
    std::queue<WorkItem> work_queue_;
};
```

### 2. Thread Pool 작업 처리 패턴

```cpp
// Create a thread pool
auto [pool, error] = create_default(thread_counts_);
if (error.has_value()) {
    // Handle error
    return;
}

// Create a batch of jobs
std::vector<std::unique_ptr<thread_module::job>> jobs;
jobs.reserve(task_count);

for (auto i = 0; i < task_count; ++i) {
    jobs.push_back(std::make_unique<thread_module::callback_job>(
        [i]() -> std::optional<std::string> {
            // Process task
            return std::nullopt; // Success
        }
    ));
}

// Submit jobs as a batch for efficiency
error = pool->enqueue_batch(std::move(jobs));
if (error.has_value()) {
    // Handle error
    return;
}

// Start processing
error = pool->start();
if (error.has_value()) {
    // Handle error
    return;
}

// When done, stop the pool
pool->stop();
```

### 3. Type 기반 Job 실행 패턴

```cpp
// Create a type thread pool with different type workers
auto [pool, error] = create_type_pool(
    high_type_workers_,
    normal_type_workers_,
    low_type_workers_
);

// Creating jobs with different types
std::vector<std::unique_ptr<typed_thread_pool_module::typed_job>> jobs;
jobs.reserve(job_count);

// High type critical tasks
jobs.push_back(std::make_unique<typed_thread_pool_module::callback_typed_job>(
    []() -> result_void {
        // Critical operation
        return {};
    },
    typed_thread_pool_module::job_types::High
));

// Normal type regular tasks
jobs.push_back(std::make_unique<typed_thread_pool_module::callback_typed_job>(
    []() -> result_void {
        // Regular operation
        return {};
    },
    typed_thread_pool_module::job_types::Normal
));

// Low type background tasks
jobs.push_back(std::make_unique<typed_thread_pool_module::callback_typed_job>(
    []() -> result_void {
        // Background operation
        return {};
    },
    typed_thread_pool_module::job_types::Low
));
```

### 4. Error Handler 패턴

```cpp
// Implement a custom error handler for thread operations
class ApplicationErrorHandler : public thread_module::error_handler {
public:
    void handle_error(const std::string& error_message) override {
        // Handle errors according to your application needs
        // Could write to a log file, send to monitoring system, etc.
        std::cerr << "Thread error: " << error_message << std::endl;

        // If using external logger module:
        // if (logger_) {
        //     logger_->write_error("Thread error: {}", error_message);
        // }
    }

private:
    // Optional: reference to external logger
    // std::shared_ptr<logger_interface> logger_;
};

// Use the error handler
auto error_handler = std::make_shared<ApplicationErrorHandler>();
thread_pool->set_error_handler(error_handler);
```

### 5. Producer-Consumer 패턴

```cpp
// Consumer thread worker
class ConsumerWorker : public thread_module::thread_base {
protected:
    auto do_work() -> result_void override {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        // Wait for data or stop signal
        data_condition_.wait(lock, [this]() {
            return !data_queue_.empty() || !should_continue_work();
        });

        // Check if should terminate
        if (!should_continue_work()) {
            return {};
        }

        // Process data
        auto data = std::move(data_queue_.front());
        data_queue_.pop();

        lock.unlock();
        process_data(data);

        return {};
    }

private:
    std::mutex queue_mutex_;
    std::condition_variable data_condition_;
    std::queue<Data> data_queue_;

    void process_data(const Data& data) {
        // Process the data
    }
};
```

### 6. Task 분할 패턴

```cpp
void process_large_dataset(const std::vector<Data>& dataset) {
    const size_t thread_count = std::thread::hardware_concurrency();
    const size_t chunk_size = (dataset.size() + thread_count - 1) / thread_count;

    // Create thread pool
    auto [pool, error] = create_default(thread_count);
    if (error.has_value()) {
        // Handle error appropriately - could use external logger if available
        std::cerr << "Failed to create thread pool: " << error.value() << std::endl;
        return;
    }

    // Submit chunks as separate jobs
    std::vector<std::unique_ptr<thread_module::job>> jobs;
    for (size_t i = 0; i < thread_count; ++i) {
        size_t start_idx = i * chunk_size;
        size_t end_idx = std::min(start_idx + chunk_size, dataset.size());

        jobs.push_back(std::make_unique<thread_module::callback_job>(
            [&dataset, start_idx, end_idx]() -> std::optional<std::string> {
                for (size_t j = start_idx; j < end_idx; ++j) {
                    process_item(dataset[j]);
                }
                return std::nullopt;
            }
        ));
    }

    pool->enqueue_batch(std::move(jobs));
    pool->start();
    pool->stop(); // Wait for all jobs to complete
}
```

## 피해야 할 안티패턴

### 1. Thread Explosion 안티패턴

❌ **문제가 있는 접근 방식**:
```cpp
// Creating a new thread for each small task
for (const auto& item : items) {
    auto thread = std::make_unique<thread_module::thread_base>();
    thread->start();
    // Process item in thread
}
```

✅ **더 나은 접근 방식**:
```cpp
// Create a single thread pool
auto thread_pool = std::make_shared<thread_pool_module::thread_pool>();

// Submit all items as jobs
std::vector<std::unique_ptr<thread_module::job>> jobs;
for (const auto& item : items) {
    jobs.push_back(std::make_unique<thread_module::callback_job>(
        [item]() -> std::optional<std::string> {
            // Process item
            return std::nullopt;
        }
    ));
}

// Process all items with the thread pool
thread_pool->enqueue_batch(std::move(jobs));
thread_pool->start();
```

### 2. Busy Waiting 안티패턴

❌ **문제가 있는 접근 방식**:
```cpp
auto do_work() -> result_void override {
    // Continuously check if work is available without yielding
    while (!work_available()) {
        // Tight loop consuming CPU
    }
    process_work();
    return {};
}
```

✅ **더 나은 접근 방식**:
```cpp
auto do_work() -> result_void override {
    // Use condition variables and wake intervals
    if (!work_available()) {
        // Return and let the thread sleep until next wake interval
        return {};
    }
    process_work();
    return {};
}
```

### 3. Type Abuse 안티패턴

❌ **문제가 있는 접근 방식**:
```cpp
// Marking all jobs as high type
for (auto i = 0; i < job_count; ++i) {
    jobs.push_back(std::make_unique<callback_typed_job>(
        [i]() -> result_void {
            // Regular task
            return {};
        },
        job_types::High // All jobs set to high type
    ));
}
```

✅ **더 나은 접근 방식**:
```cpp
// Assign appropriate types based on task importance
for (auto i = 0; i < job_count; ++i) {
    // Determine appropriate type based on task characteristics
    auto type = determine_appropriate_type(i);

    jobs.push_back(std::make_unique<callback_typed_job>(
        [i]() -> result_void {
            // Regular task
            return {};
        },
        type
    ));
}
```

### 4. Blocking Thread Pool 안티패턴

❌ **문제가 있는 접근 방식**:
```cpp
// Submitting I/O-bound or blocking operations to thread pool
pool->enqueue(std::make_unique<thread_module::callback_job>(
    []() -> std::optional<std::string> {
        // Perform long-running I/O operation that blocks
        std::this_thread::sleep_for(std::chrono::seconds(10));
        return std::nullopt;
    }
));
```

✅ **더 나은 접근 방식**:
```cpp
// Use asynchronous I/O or dedicated threads for blocking operations
// For I/O-bound operations, consider async I/O or a dedicated thread pool
auto io_thread = std::make_unique<thread_module::thread_base>("io_thread");
io_thread->start();

// Use the main thread pool for CPU-bound work only
pool->enqueue(std::make_unique<thread_module::callback_job>(
    []() -> std::optional<std::string> {
        // CPU-bound computation
        return std::nullopt;
    }
));
```

### 5. Performance Monitoring 안티패턴

❌ **문제가 있는 접근 방식**:
```cpp
for (int i = 0; i < 1000000; i++) {
    // Recording metrics for every single operation
    auto start = std::chrono::steady_clock::now();
    // Process item
    auto end = std::chrono::steady_clock::now();
    // Recording every single operation creates overhead
    record_metric("item_processed", end - start);
}
```

✅ **더 나은 접근 방식**:
```cpp
// Record metrics at appropriate intervals or aggregated
std::atomic<size_t> processed_count{0};
auto batch_start = std::chrono::steady_clock::now();

for (int i = 0; i < 1000000; i++) {
    // Process item
    processed_count++;

    // Record metrics periodically
    if (i % 10000 == 0) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = now - batch_start;
        // Record aggregated metrics
        record_batch_metrics(10000, elapsed);
        batch_start = now;
    }
}
```

## 일반적인 문제 해결

### 1. Race Condition

#### 증상
- 일관되지 않거나 예상치 못한 결과
- 실행 간 프로그램 동작이 달라짐
- 결과가 타이밍 또는 시스템 부하에 따라 달라짐
- 간헐적인 충돌 또는 데이터 손상

#### 해결 접근 방식
1. **mutex 보호 사용:**
   ```cpp
   std::mutex counter_mutex_;

   // In thread code
   std::lock_guard<std::mutex> lock(counter_mutex_);
   counter_++;
   ```

2. **atomic 변수 사용:**
   ```cpp
   std::atomic<int> counter_{0};

   // In thread code (no mutex needed)
   counter_++;
   ```

3. **job 기반 설계 사용:**
   ```cpp
   // Define a counter modification job
   auto increment_job = std::make_unique<thread_module::callback_job>(
       [this]() -> std::optional<std::string> {
           counter_++;
           return std::nullopt;
       }
   );

   // Submit to thread pool
   thread_pool->enqueue(std::move(increment_job));
   ```

### 2. Deadlock

#### 증상
- 프로그램이 멈추거나 중단됨
- 여러 thread가 응답하지 않음
- 프로그램이 실행되는 것처럼 보이지만 CPU 사용이 없음
- deadlock 감지 도구가 lock 순환을 보고함

#### 해결 접근 방식
1. **일관된 lock 순서:**
   ```cpp
   // Always acquire locks in the same order
   std::lock_guard<std::mutex> lock1(mutex_a_); // Always first
   std::lock_guard<std::mutex> lock2(mutex_b_); // Always second
   ```

2. **여러 lock에 std::lock 사용:**
   ```cpp
   std::unique_lock<std::mutex> lock_a(mutex_a_, std::defer_lock);
   std::unique_lock<std::mutex> lock_b(mutex_b_, std::defer_lock);
   std::lock(lock_a, lock_b); // Atomic acquisition of both locks
   ```

3. **가능한 경우 중첩 lock 피하기:**
   ```cpp
   // Instead of nested locks, acquire all needed locks upfront
   {
       std::lock_guard<std::mutex> lock(mutex_);
       // Do all work requiring the lock here
   }
   // Then do work not requiring the lock
   ```

4. **deadlock 감지를 위한 lock timeout 사용:**
   ```cpp
   std::unique_lock<std::mutex> lock(mutex_, std::chrono::seconds(5));
   if (!lock) {
       // Handle potential deadlock - log error if logger is available
       handle_error("Potential deadlock detected - could not acquire lock within timeout");
       // Implement recovery strategy
   }
   ```

### 3. Type Inversion

#### 증상
- 높은 type 작업이 예상치 못한 지연을 경험함
- 시스템 응답성이 일관되지 않음
- 중요한 job이 낮은 type job보다 오래 걸림

#### 해결 접근 방식
1. **type 경계를 넘어 리소스 공유 최소화:**
   ```cpp
   // Design your system to minimize cases where high and low type
   // threads need to share resources. Use separate resources when possible.
   auto high_type_resources = std::make_unique<ResourcePool>("high");
   auto low_type_resources = std::make_unique<ResourcePool>("low");
   ```

2. **type ceiling 사용:**
   ```cpp
   // When a low-type thread acquires a critical resource,
   // temporarily boost its type
   auto original_type = get_current_type();
   set_current_type(high_type);

   // Critical section with shared resource
   {
       std::lock_guard<std::mutex> lock(shared_mutex_);
       // Work with shared resource
   }

   // Restore original type
   set_current_type(original_type);
   ```

### 4. Thread Starvation

#### 증상
- 특정 작업이 완료되지 않거나 극심한 지연을 경험함
- 일부 thread가 CPU 시간을 받지 못함
- 시스템이 사용 가능한 작업의 일부에만 집중하는 것처럼 보임

#### 해결 접근 방식
1. **각 type 레벨에 worker 전용 할당:**
   ```cpp
   // Create workers specifically for low-type tasks
   auto low_worker = std::make_unique<typed_thread_worker>(
       std::vector<job_types>{job_types::Low},
       "low_type_worker"
   );

   type_pool->enqueue(std::move(low_worker));
   ```

2. **낮은 type 작업을 위한 aging 구현:**
   ```cpp
   class AgingJob : public typed_job {
   public:
       AgingJob(std::function<result_void()> func)
           : typed_job(job_types::Low), func_(func),
             creation_time_(std::chrono::steady_clock::now()) {}

       auto get_type() const -> job_types override {
           auto age = std::chrono::steady_clock::now() - creation_time_;
           if (age > std::chrono::minutes(5)) {
               return job_types::High;
           } else if (age > std::chrono::minutes(1)) {
               return job_types::Normal;
           }
           return job_types::Low;
       }

   private:
       std::function<result_void()> func_;
       std::chrono::steady_clock::time_point creation_time_;
   };
   ```

### 5. False Sharing

#### 증상
- 멀티스레드 코드에서 예상치 못한 낮은 성능
- 더 많은 코어를 사용할수록 성능이 저하됨
- CPU cache profiling이 높은 cache coherence traffic을 보여줌

#### 해결 접근 방식
1. **false sharing을 피하기 위해 데이터 구조 패딩:**
   ```cpp
   struct alignas(64) PaddedCounter {
       std::atomic<int> value{0};
       char padding[64 - sizeof(std::atomic<int>)];
   };

   PaddedCounter counter1_;
   PaddedCounter counter2_;
   ```

2. **counter에 thread-local storage 사용:**
   ```cpp
   thread_local int local_counter_ = 0;

   // Each thread updates its own counter
   auto job = std::make_unique<thread_module::callback_job>(
       []() -> std::optional<std::string> {
           local_counter_++;
           return std::nullopt;
       }
   );
   ```

### 6. Memory Visibility 문제

#### 증상
- Thread가 다른 thread의 업데이트를 "보지" 못함
- 오래된 데이터가 계산에 사용됨
- 공유 변수에 대한 non-atomic 작업이 손상을 유발함

#### 해결 접근 방식
1. **flag에 atomic 변수 사용:**
   ```cpp
   std::atomic<bool> done_{false};

   // Thread 1
   done_.store(true, std::memory_order_release);

   // Thread 2
   while (!done_.load(std::memory_order_acquire)) {
       std::this_thread::yield();
   }
   ```

2. **적절한 동기화 primitive 사용:**
   ```cpp
   std::mutex mutex_;
   bool done_ = false;
   std::condition_variable cv_;

   // Thread 1
   {
       std::lock_guard<std::mutex> lock(mutex_);
       done_ = true;
   }
   cv_.notify_all();

   // Thread 2
   {
       std::unique_lock<std::mutex> lock(mutex_);
       cv_.wait(lock, [this]() { return done_; });
   }
   ```

## 고급 동시성 패턴

### 1. 이벤트 기반 통신

```cpp
class EventSystem {
public:
    using EventHandler = std::function<void(const Event&)>;

    void subscribe(EventType type, EventHandler handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_[type].push_back(handler);
    }

    void publish(const Event& event) {
        std::vector<EventHandler> handlers_to_call;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = handlers_.find(event.type);
            if (it != handlers_.end()) {
                handlers_to_call = it->second;
            }
        }

        // Submit each handler to the thread pool
        for (const auto& handler : handlers_to_call) {
            thread_pool_->enqueue(std::make_unique<thread_module::callback_job>(
                [handler, event]() -> std::optional<std::string> {
                    handler(event);
                    return std::nullopt;
                }
            ));
        }
    }

private:
    std::mutex mutex_;
    std::map<EventType, std::vector<EventHandler>> handlers_;
    std::shared_ptr<thread_pool_module::thread_pool> thread_pool_;
};
```

### 2. Work Stealing 패턴

```cpp
class WorkStealingPool {
public:
    WorkStealingPool(size_t worker_count) {
        // Create per-worker queues
        queues_.resize(worker_count);

        // Create workers
        for (size_t i = 0; i < worker_count; ++i) {
            workers_.push_back(std::make_unique<WorkStealingWorker>(
                i, queues_, *this
            ));
        }
    }

    auto enqueue(std::unique_ptr<thread_module::job> job, size_t preferred_worker) -> void {
        if (preferred_worker >= queues_.size()) {
            preferred_worker = 0;
        }

        queues_[preferred_worker].enqueue(std::move(job));
    }

private:
    std::vector<std::unique_ptr<WorkStealingWorker>> workers_;
    std::vector<JobQueue> queues_;
};

class WorkStealingWorker : public thread_module::thread_base {
protected:
    auto do_work() -> result_void override {
        // Try to get job from own queue
        auto job = queues_[worker_id_].dequeue();

        // If no job, try to steal from other queues
        if (!job) {
            for (size_t i = 0; i < queues_.size(); ++i) {
                if (i == worker_id_) continue;

                job = queues_[i].try_steal();
                if (job) break;
            }
        }

        // If we got a job, execute it
        if (job) {
            job->execute();
            return {};
        }

        // No job found, let the thread sleep
        return {};
    }

private:
    size_t worker_id_;
    std::vector<JobQueue>& queues_;
};
```

### 3. Read-Write Lock 패턴

```cpp
class ReadWriteLock {
public:
    void read_lock() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (write_count_ > 0 || write_waiting_ > 0) {
            read_cv_.wait(lock);
        }
        read_count_++;
    }

    void read_unlock() {
        std::unique_lock<std::mutex> lock(mutex_);
        read_count_--;
        if (read_count_ == 0 && write_waiting_ > 0) {
            write_cv_.notify_one();
        }
    }

    void write_lock() {
        std::unique_lock<std::mutex> lock(mutex_);
        write_waiting_++;
        while (read_count_ > 0 || write_count_ > 0) {
            write_cv_.wait(lock);
        }
        write_waiting_--;
        write_count_++;
    }

    void write_unlock() {
        std::unique_lock<std::mutex> lock(mutex_);
        write_count_--;
        if (write_waiting_ > 0) {
            write_cv_.notify_one();
        } else {
            read_cv_.notify_all();
        }
    }

private:
    std::mutex mutex_;
    std::condition_variable read_cv_;
    std::condition_variable write_cv_;
    int read_count_ = 0;
    int write_count_ = 0;
    int write_waiting_ = 0;
};
```

## 동시성 코드 디버깅

### 진단을 효과적으로 사용하기

1. **디버깅을 위한 thread ID 추적:**
   ```cpp
   // Custom diagnostic helper
   class ThreadDiagnostics {
   public:
       static void record_event(const std::string& event) {
           auto thread_id = std::this_thread::get_id();
           auto timestamp = std::chrono::steady_clock::now();

           // Store or output diagnostic information
           std::ostringstream oss;
           oss << "[" << timestamp.time_since_epoch().count()
               << "][Thread " << thread_id << "] " << event;

           // Output to console, file, or external logger if available
           std::cout << oss.str() << std::endl;
       }
   };
   ```

2. **상태 전환 추적:**
   ```cpp
   auto do_work() -> result_void override {
       ThreadDiagnostics::record_event("Worker state: entering critical section");
       {
           std::lock_guard<std::mutex> lock(mutex_);
           // Critical section
           ThreadDiagnostics::record_event(
               "Worker state: in critical section, count=" + std::to_string(count_)
           );
       }
       ThreadDiagnostics::record_event("Worker state: exited critical section");
       return {};
   }
   ```

3. **이벤트 순서 지정을 위한 시퀀스 번호 사용:**
   ```cpp
   std::atomic<uint64_t> global_seq_{0};

   void record_sequenced_event(const std::string& event) {
       uint64_t seq = global_seq_++;
       std::ostringstream oss;
       oss << "[SEQ:" << seq << "] " << event;
       ThreadDiagnostics::record_event(oss.str());
   }
   ```

### Thread Sanitizer 사용

1. **빌드에서 ThreadSanitizer 활성화:**
   ```bash
   # For GCC/Clang
   g++ -fsanitize=thread -g mycode.cpp

   # For MSVC
   # Use /fsanitize=address in recent versions
   ```

2. **thread sanitizer가 감지하는 일반적인 문제:**
   - Data race
   - Deadlock
   - Double-locking
   - 동시 컨텍스트에서 use-after-free

### 일반적인 Thread System 디버깅 단계

1. **thread pool 시작 확인:**
   ```cpp
   auto error = pool->start();
   if (error.has_value()) {
       // Handle startup error
       std::cerr << "Thread pool failed to start: "
                 << error.value_or("unknown error") << std::endl;
       // Handle error
   } else {
       // Record successful startup
       std::cout << "Thread pool started successfully with "
                 << pool->get_worker_count() << " workers" << std::endl;
   }
   ```

2. **job 실행 확인:**
   ```cpp
   // Add debugging to your job
   auto job = std::make_unique<thread_module::callback_job>(
       [](void) -> std::optional<std::string> {
           ThreadDiagnostics::record_event("Job started");

           // Your job logic here

           ThreadDiagnostics::record_event("Job completed");
           return std::nullopt;
       }
   );
   ```

## 성능 최적화

### Thread Pool 크기 지정 지침

1. **CPU 바운드 작업의 경우:**
   ```cpp
   // Use hardware concurrency as a baseline
   auto thread_count = std::thread::hardware_concurrency();
   ```

2. **I/O 바운드 작업의 경우:**
   ```cpp
   // Consider using more threads than cores
   auto thread_count = std::thread::hardware_concurrency() * 2;
   ```

3. **혼합 workload의 경우:**
   ```cpp
   // Create separate pools for different workload types
   auto cpu_pool = create_default(std::thread::hardware_concurrency());
   auto io_pool = create_default(std::thread::hardware_concurrency() * 2);
   ```

### Batch Job 제출

개별 job 대기열 대신 항상 batch 제출을 선호합니다:

```cpp
// ❌ 비효율적
for (const auto& task : tasks) {
    pool->enqueue(create_job(task));
}

// ✅ 효율적
std::vector<std::unique_ptr<thread_module::job>> jobs;
for (const auto& task : tasks) {
    jobs.push_back(create_job(task));
}
pool->enqueue_batch(std::move(jobs));
```

### Wake Interval 최적화

workload에 따라 wake interval을 구성합니다:

```cpp
// For high-frequency tasks
worker.set_wake_interval(std::chrono::milliseconds(10));

// For periodic maintenance
worker.set_wake_interval(std::chrono::seconds(1));

// For rare events
worker.set_wake_interval(std::chrono::minutes(1));
```

## 외부 모듈 통합

### Logger 통합 패턴

Thread System은 외부 로깅 라이브러리와 함께 작동하도록 설계되었습니다. 통합 방법은 다음과 같습니다:

```cpp
// Example: Integrating an external logger
class LoggerAdapter : public thread_module::error_handler {
public:
    LoggerAdapter(std::shared_ptr<external::Logger> logger)
        : logger_(logger) {}

    void handle_error(const std::string& error_message) override {
        if (logger_) {
            logger_->error("Thread System: {}", error_message);
        }
    }

private:
    std::shared_ptr<external::Logger> logger_;
};

// Usage
auto external_logger = external::Logger::create("app.log");
auto logger_adapter = std::make_shared<LoggerAdapter>(external_logger);
thread_pool->set_error_handler(logger_adapter);
```

### Monitoring 통합 패턴

성능 모니터링 및 metric 수집의 경우:

```cpp
// Example: Integrating external monitoring
class MonitoringAdapter {
public:
    MonitoringAdapter(std::shared_ptr<external::MetricsCollector> collector)
        : collector_(collector) {}

    void record_job_execution(const std::string& job_type,
                             std::chrono::nanoseconds duration) {
        if (collector_) {
            collector_->record_histogram("thread_system.job_duration",
                                       duration.count(),
                                       {{"job_type", job_type}});
        }
    }

    void record_pool_size(size_t active_workers, size_t total_workers) {
        if (collector_) {
            collector_->record_gauge("thread_system.active_workers",
                                   active_workers);
            collector_->record_gauge("thread_system.total_workers",
                                   total_workers);
        }
    }

private:
    std::shared_ptr<external::MetricsCollector> collector_;
};
```

### 모듈식 아키텍처 모범 사례

1. **Dependency Injection 사용**:
   ```cpp
   class Application {
   public:
       Application(std::shared_ptr<thread_module::error_handler> error_handler = nullptr)
           : error_handler_(error_handler) {
           // Create thread pool
           auto [pool, error] = create_default(4);
           if (!error.has_value() && error_handler_) {
               pool->set_error_handler(error_handler_);
           }
           thread_pool_ = std::move(pool);
       }

   private:
       std::shared_ptr<thread_module::error_handler> error_handler_;
       std::unique_ptr<thread_pool_module::thread_pool> thread_pool_;
   };
   ```

2. **추상 Interface 생성**:
   ```cpp
   // Define your own interfaces for optional dependencies
   class ILogger {
   public:
       virtual ~ILogger() = default;
       virtual void log(const std::string& message) = 0;
   };

   class IMonitor {
   public:
       virtual ~IMonitor() = default;
       virtual void record_metric(const std::string& name, double value) = 0;
   };
   ```

3. **선택적 종속성에 Null Object 패턴 사용**:
   ```cpp
   class NullLogger : public ILogger {
   public:
       void log(const std::string&) override {
           // No-op implementation
       }
   };

   // Use null logger when no real logger is provided
   auto logger = external_logger ? external_logger : std::make_shared<NullLogger>();
   ```

## 결론

이러한 패턴을 따르고 안티패턴을 피하면 Thread System을 효과적으로 사용할 수 있습니다. 기억해야 할 핵심 원칙은 다음과 같습니다:

1. **작업에 적합한 도구 사용**: 요구 사항에 따라 적절한 컴포넌트 선택
2. **동시성을 위한 설계**: 처음부터 thread 안전성을 고려
3. **과도한 엔지니어링 피하기**: 요구 사항을 충족하는 가장 간단한 동시성 패턴 사용
4. **모니터링 및 측정**: 항상 threading 설계의 성능 이점 검증
5. **오류 처리**: 항상 반환 값을 확인하고 오류를 적절히 처리
6. **체계적으로 디버그**: 진단, debugger 및 thread sanitizer를 사용하여 문제 식별
7. **모듈식 유지**: 외부 로깅/모니터링이 있거나 없어도 작동하도록 시스템 설계

이러한 지침을 따르면 Thread System으로 강력하고 효율적이며 유지 관리 가능한 동시 애플리케이션을 만들 수 있습니다.

---

*Last Updated: 2025-10-20*
