# Getting Started with thread_system

A step-by-step guide to using `thread_system` -- from installation through thread
pools, job submission, priority queues, DAG scheduling, and work stealing.

## Prerequisites

| Requirement | Minimum | Recommended |
|-------------|---------|-------------|
| C++ standard | C++20 | C++20 |
| CMake | 3.20 | 3.28+ |
| GCC | 13 | 13+ |
| Clang | 17 | 17+ |
| Apple Clang | 14 | 15+ |
| MSVC | 2022 (17.0) | 2022 (17.8+) |

**Required dependency**: [common_system](https://github.com/kcenon/common_system)
must be available to CMake (via `FetchContent`, `find_package`, or a sibling
directory).

## Installation

### Option A -- CMake FetchContent (recommended)

```cmake
include(FetchContent)

FetchContent_Declare(
    thread_system
    GIT_REPOSITORY https://github.com/kcenon/thread_system.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(thread_system)

target_link_libraries(your_target PRIVATE thread_system)
```

### Option B -- Clone and build locally

```bash
git clone https://github.com/kcenon/thread_system.git
cd thread_system

# Install ecosystem dependencies
./scripts/dependency.sh

# Build with the helper script
./scripts/build.sh

# Or use CMake directly
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Option C -- Add as a Git submodule

```bash
git submodule add https://github.com/kcenon/thread_system.git external/thread_system
```

Then in your `CMakeLists.txt`:

```cmake
add_subdirectory(external/thread_system)
target_link_libraries(your_target PRIVATE thread_system)
```

## First Thread Pool

Create a thread pool, start it, submit work, and shut down.

```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <iostream>

int main() {
    // Create a pool with a name
    auto pool = std::make_shared<kcenon::thread::thread_pool>("my_pool");

    // Start the pool (spawns worker threads)
    pool->start();

    // Submit a callable that returns a value
    auto future = pool->submit([] {
        return 42;
    });

    // Retrieve the result (blocks until done)
    std::cout << "Result: " << future.get() << "\n";

    // Graceful shutdown -- waits for pending jobs
    pool->stop();
    return 0;
}
```

### Builder Pattern

For more configuration, use `thread_pool_builder`:

```cpp
#include <kcenon/thread/core/thread_pool_builder.h>

auto pool = kcenon::thread::thread_pool_builder("worker_pool")
    .with_workers(8)
    .with_work_stealing()
    .with_autoscaling(kcenon::thread::autoscaling_policy{
        .min_workers = 2,
        .max_workers = 16,
        .scaling_mode = kcenon::thread::autoscaling_policy::mode::automatic
    })
    .with_circuit_breaker(kcenon::common::circuit_breaker_config{
        .failure_threshold = 5,
        .open_duration = std::chrono::seconds{30}
    })
    .build();

pool->start();
```

## Job Submission

### Single Task

```cpp
// Fire-and-forget
pool->enqueue(std::make_unique<my_job>());

// Submit with future
auto future = pool->submit([] { return compute_something(); });
auto result = future.get();

// Named job (for tracing/debugging)
auto future = pool->submit(
    [] { return 42; },
    kcenon::thread::submit_options{.name = "compute_task"}
);
```

### Batch Submission

```cpp
std::vector<std::function<int()>> tasks;
tasks.push_back([] { return 1; });
tasks.push_back([] { return 2; });
tasks.push_back([] { return 3; });

// Get individual futures
auto futures = pool->submit(std::move(tasks));

// Or wait for all results at once
auto results = pool->submit(std::move(tasks), kcenon::thread::submit_options::all());

// Or get the first completed result
auto first = pool->submit(std::move(tasks), kcenon::thread::submit_options::any());
```

### Custom Job Class

Subclass `job` and override `do_work()`:

```cpp
#include <kcenon/thread/core/job.h>

class download_job : public kcenon::thread::job {
public:
    download_job(const std::string& url)
        : job("download"), url_(url) {}

    kcenon::common::VoidResult do_work() override {
        // Perform the download
        auto data = http_get(url_);
        return kcenon::common::ok();
    }

private:
    std::string url_;
};

// Submit
pool->enqueue(std::make_unique<download_job>("https://example.com/data"));
```

### Job Composition (fluent API)

Jobs support composition for callbacks, priority, retry, and timeout:

```cpp
auto my_job = std::make_unique<compute_job>();
my_job->with_priority(kcenon::thread::job_priority::high)
      .with_on_complete([](auto result) {
          std::cout << "Job done\n";
      })
      .with_on_error([](const auto& err) {
          std::cerr << "Error: " << err.message << "\n";
      })
      .with_timeout(std::chrono::seconds{30})
      .with_retry(kcenon::thread::retry_policy{.max_retries = 3});

pool->enqueue(std::move(my_job));
```

### Cancellation

```cpp
#include <kcenon/thread/core/cancellation_token.h>

auto token = std::make_shared<kcenon::thread::cancellation_token>();

// Submit a cancellable job
auto future = pool->submit([token] {
    while (!token->is_cancelled()) {
        // do work...
    }
    return 0;
});

// Cancel from another thread
token->cancel();
```

## Priority Queues

The `typed_thread_pool` supports priority-based scheduling with job type routing
and automatic priority aging (to prevent starvation).

```cpp
#include <kcenon/thread/core/typed_thread_pool.h>

// Define job types
enum class my_job_type { compute, io, background };

auto pool = kcenon::thread::typed_thread_pool<my_job_type>::builder("priority_pool")
    .with_workers(8)
    .build();

pool->start();

// Submit with priority
pool->submit(my_job_type::compute, [] {
    return heavy_computation();
}, kcenon::thread::job_priority::high);

pool->submit(my_job_type::background, [] {
    cleanup_temp_files();
}, kcenon::thread::job_priority::low);
```

## DAG Scheduling

The `dag_scheduler` executes jobs respecting dependency order. Independent jobs
run in parallel automatically.

```cpp
#include <kcenon/thread/dag/dag_scheduler.h>
#include <kcenon/thread/dag/dag_job_builder.h>

auto pool = std::make_shared<kcenon::thread::thread_pool>("dag_pool");
pool->start();

kcenon::thread::dag_scheduler scheduler(pool);

// Define jobs with dependencies
auto fetch_id = scheduler.add_job(
    kcenon::thread::dag_job_builder("fetch_data")
        .work([] { return fetch_from_api(); })
        .build()
);

auto parse_id = scheduler.add_job(
    kcenon::thread::dag_job_builder("parse_data")
        .depends_on(fetch_id)
        .work([] { return parse_response(); })
        .build()
);

auto store_id = scheduler.add_job(
    kcenon::thread::dag_job_builder("store_data")
        .depends_on(parse_id)
        .work([] { return save_to_db(); })
        .build()
);

// Execute all -- respects dependency order, parallelizes where possible
scheduler.execute_all().wait();
```

The scheduler automatically detects cycles and supports multiple failure handling
policies.

## Work Stealing

Enable NUMA-aware work stealing for better load distribution across workers.

```cpp
// Via builder
auto pool = kcenon::thread::thread_pool_builder("stealing_pool")
    .with_workers(8)
    .with_work_stealing()
    .build();

pool->start();
```

Work stealing is off by default (`THREAD_ENABLE_WORK_STEALING=OFF`). Enable it
in CMake:

```cmake
set(THREAD_ENABLE_WORK_STEALING ON)
```

Each worker maintains a local deque. When a worker runs out of jobs, it
"steals" from another worker's deque, reducing idle time and improving
throughput for unbalanced workloads.

## Metrics and Diagnostics

```cpp
// Basic metrics
const auto& m = pool->metrics();
auto snap = m.snapshot();
std::cout << "Submitted: " << snap.tasks_submitted << "\n";
std::cout << "Executed:  " << snap.tasks_executed << "\n";
std::cout << "Failed:    " << snap.tasks_failed << "\n";

// Enhanced metrics (histograms, percentiles)
pool->set_enhanced_metrics_enabled(true);
auto esnap = pool->enhanced_metrics_snapshot();
```

## Next Steps

| Topic | Resource |
|-------|----------|
| Full API surface | [API_REFERENCE.md](API_REFERENCE.md) |
| API cheat sheet | [API_QUICK_REFERENCE.md](API_QUICK_REFERENCE.md) |
| Autoscaler configuration | [AUTOSCALER_GUIDE.md](AUTOSCALER_GUIDE.md) |
| Policy queues | [POLICY_QUEUE_GUIDE.md](POLICY_QUEUE_GUIDE.md) |
| NUMA topology | [NUMA_GUIDE.md](NUMA_GUIDE.md) |
| Diagnostics and metrics | [DIAGNOSTICS_METRICS_GUIDE.md](DIAGNOSTICS_METRICS_GUIDE.md) |
| Architecture | [ARCHITECTURE.md](ARCHITECTURE.md) |
| Benchmarks | [BENCHMARKS.md](BENCHMARKS.md) |
