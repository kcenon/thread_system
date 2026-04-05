# API Quick Reference -- thread_system

Cheat-sheet for the most common `thread_system` APIs.
For full details see [API_REFERENCE.md](API_REFERENCE.md) and the Doxygen-generated docs.

---

## Header and Namespace

```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/thread_pool_builder.h>

using namespace kcenon::thread;
```

---

## Thread Pool Creation

### Direct Construction

```cpp
// Default (hardware_concurrency workers)
auto pool = std::make_shared<thread_pool>("my_pool");

// With custom queue
auto pool = std::make_shared<thread_pool>("my_pool", custom_queue);

// With policy-based queue
auto pool = std::make_shared<thread_pool>("my_pool",
    make_lockfree_queue_adapter());
```

### Builder Pattern

```cpp
auto pool = thread_pool_builder("my_pool")
    .with_workers(8)
    .with_work_stealing()
    .with_circuit_breaker(circuit_breaker_config{
        .failure_threshold = 5,
        .open_duration = std::chrono::seconds{30}
    })
    .with_autoscaling(autoscaling_policy{
        .min_workers = 2,
        .max_workers = 16,
        .scaling_mode = autoscaling_policy::mode::automatic
    })
    .build();
```

### Lifecycle

```cpp
pool->start();                      // Start workers
pool->stop();                       // Graceful shutdown
pool->stop(/*immediately=*/true);   // Force shutdown
```

---

## Job Submission

### Unified Submit API

```cpp
// Single task -> future
auto future = pool->submit([] { return 42; });
int result = future.get();

// Named task
auto future = pool->submit([] { return 42; },
    submit_options{.name = "compute"});

// Named task (factory)
auto future = pool->submit([] { return 42; },
    submit_options::named("compute"));
```

### Batch Submit

```cpp
std::vector<std::function<int()>> tasks = { ... };

// Individual futures
auto futures = pool->submit(std::move(tasks));

// Wait for all results
auto results = pool->submit(std::move(tasks), submit_options::all());

// First completed result
auto result = pool->submit(std::move(tasks), submit_options::any());
```

### Job Objects

```cpp
// Single job
pool->enqueue(std::make_unique<my_job>());

// Batch
pool->enqueue_batch(std::move(job_vector));
```

### Job Composition (fluent API)

```cpp
job->with_priority(job_priority::high)
   .with_on_complete([](auto r) { /* ... */ })
   .with_on_error([](const auto& e) { /* ... */ })
   .with_timeout(std::chrono::seconds{30})
   .with_retry(retry_policy{.max_retries = 3});
```

---

## Job Cancellation

```cpp
#include <kcenon/thread/core/cancellation_token.h>

auto token = std::make_shared<cancellation_token>();

// Check in job
while (!token->is_cancelled()) { /* work */ }

// Cancel from outside
token->cancel();
```

---

## Job Priority

```cpp
enum class job_priority {
    lowest   = 0,
    low      = 1,
    normal   = 2,   // default
    high     = 3,
    highest  = 4,
    realtime = 5
};
```

---

## Priority Queues (typed_thread_pool)

```cpp
#include <kcenon/thread/core/typed_thread_pool.h>

enum class my_type { compute, io, background };

auto pool = typed_thread_pool<my_type>::builder("typed_pool")
    .with_workers(8)
    .build();

pool->start();
pool->submit(my_type::compute, [] { return 42; }, job_priority::high);
```

---

## DAG Scheduling

```cpp
#include <kcenon/thread/dag/dag_scheduler.h>
#include <kcenon/thread/dag/dag_job_builder.h>

dag_scheduler scheduler(pool);

auto a = scheduler.add_job(
    dag_job_builder("fetch")
        .work([] { return fetch_data(); })
        .build());

auto b = scheduler.add_job(
    dag_job_builder("process")
        .depends_on(a)
        .work([] { return process(); })
        .build());

scheduler.execute_all().wait();
```

### dag_job_builder Methods

| Method | Description |
|---|---|
| `.work(callable)` | Set the work function |
| `.depends_on(job_id)` | Add dependency on another job |
| `.build()` | Create the dag_job |

---

## Autoscaler Configuration

```cpp
#include <kcenon/thread/scaling/autoscaler.h>
#include <kcenon/thread/scaling/autoscaling_policy.h>

autoscaling_policy policy{
    .min_workers = 2,
    .max_workers = 32,
    .scaling_mode = autoscaling_policy::mode::automatic,
};

auto scaler = std::make_shared<autoscaler>(*pool, policy);
scaler->start();
// ... pool auto-scales based on load ...
scaler->stop();
```

### Via Builder

```cpp
auto pool = thread_pool_builder("auto_pool")
    .with_workers(4)
    .with_autoscaling(autoscaling_policy{
        .min_workers = 2,
        .max_workers = 16
    })
    .build();
```

---

## Metrics

### Basic Metrics

```cpp
const auto& m = pool->metrics();
auto snap = m.snapshot();

snap.tasks_submitted;      // uint64_t
snap.tasks_enqueued;       // uint64_t
snap.tasks_executed;       // uint64_t
snap.tasks_failed;         // uint64_t
snap.total_busy_time_ns;   // uint64_t
snap.total_idle_time_ns;   // uint64_t
```

### Enhanced Metrics (histograms, percentiles)

```cpp
pool->set_enhanced_metrics_enabled(true);

auto snap = pool->enhanced_metrics_snapshot();
pool->reset_metrics();
```

### Pool Status

```cpp
pool->get_idle_worker_count();   // size_t
pool->report_metrics();          // report to monitoring
pool->to_string();               // human-readable description
```

---

## Queue Types

| Queue | Header | Description |
|---|---|---|
| `adaptive_job_queue` | `queue/adaptive_job_queue.h` | Auto-switches mutex/lockfree (recommended) |
| `job_queue` | `core/job_queue.h` | Mutex-based FIFO, optional bounded size |
| `standard_queue` | `core/thread_pool.h` | Alias: mutex + unbounded + reject overflow |
| `policy_lockfree_queue` | `core/thread_pool.h` | Alias: lockfree + unbounded + reject overflow |
| `backpressure_job_queue` | `core/backpressure_job_queue.h` | Rate limiting and flow control |

---

## Policy Queue Adapters

```cpp
#include <kcenon/thread/adapters/policy_queue_adapter.h>

auto pool = std::make_shared<thread_pool>("pool",
    make_standard_queue_adapter());      // mutex-based

auto pool = std::make_shared<thread_pool>("pool",
    make_lockfree_queue_adapter());      // lock-free
```

---

## Quick Recipe

```cpp
#include <kcenon/thread/core/thread_pool_builder.h>
#include <kcenon/thread/dag/dag_scheduler.h>
#include <kcenon/thread/dag/dag_job_builder.h>

// Build pool
auto pool = thread_pool_builder("pipeline")
    .with_workers(4)
    .with_work_stealing()
    .build();
pool->start();

// Submit single task
auto answer = pool->submit([] { return 42; });

// DAG pipeline
dag_scheduler dag(pool);
auto step1 = dag.add_job(
    dag_job_builder("load").work([] { return load_data(); }).build());
auto step2 = dag.add_job(
    dag_job_builder("transform").depends_on(step1)
        .work([] { return transform(); }).build());
dag.execute_all().wait();

// Shutdown
pool->stop();
```
