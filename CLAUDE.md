# thread_system

## Overview

Modern C++20 multithreading framework providing thread pool, job queue, and concurrent
programming abstractions. Offers high-performance lock-free primitives behind an intuitive,
type-safe API following Kent Beck's Simple Design principle.

## Architecture

```
include/kcenon/thread/
  core/          - thread_pool, thread_worker, job, job_queue, cancellation_token,
                   hazard_pointer, sync_primitives, error_handling (~42 headers)
  queue/         - adaptive_job_queue, queue_factory (2 public queue types)
  lockfree/      - lockfree_job_queue, work_stealing_deque (internal)
  dag/           - DAG scheduler (dag_job, dag_scheduler, dag_config)
  stealing/      - NUMA-aware work stealing (7 headers)
  scaling/       - Autoscaler, autoscaling_policy
  diagnostics/   - thread_pool_diagnostics, health_status, bottleneck_report
  metrics/       - enhanced_metrics, latency_histogram, sliding_window_counter
  policies/      - sync_policies, overflow_policies, policy_queue
  pool_policies/ - autoscaling, circuit_breaker, work_stealing pool policies
  adapters/      - common_executor_adapter, job_queue_adapter
  concepts/      - C++20 concepts for thread domain
```

Key abstractions:
- `thread_pool` — Multi-worker pool with adaptive queue and submit_task API
- `typed_thread_pool` — Priority-based scheduling with job type routing and aging
- `adaptive_job_queue` — Auto-switches between mutex and lock-free modes (recommended)
- `job_queue` — Mutex-based FIFO with optional bounded size
- `hazard_pointer` / `safe_hazard_pointer` — Lock-free memory reclamation
- `cancellation_token` — Cooperative cancellation with hierarchy support
- `dag_scheduler` — Dependency-based job scheduling

## Build & Test

```bash
# Build scripts
./scripts/dependency.sh && ./scripts/build.sh

# CMake presets
cmake --preset debug && cmake --build --preset debug
cd build-debug && ctest --output-on-failure

# Manual CMake
cmake -B build -G Ninja -DBUILD_WITH_COMMON_SYSTEM=ON && cmake --build build
```

Key CMake options:
- `BUILD_WITH_COMMON_SYSTEM` (ON) — Required common_system integration
- `BUILD_TESTS` (ON) — Unit tests (Google Test 1.17.0)
- `THREAD_ENABLE_WORK_STEALING` (OFF) — NUMA-aware work stealing
- `THREAD_BUILD_MODULES` (OFF) — C++20 modules (CMake 3.28+)

Presets: `default`, `debug`, `release`, `core-debug`, `core-release`, `asan`, `tsan`, `ubsan`, `ci`, `vcpkg`

Modular build: `thread_core` (always), `thread_stealing`, `thread_resilience`, `thread_scaling` (optional modules).

CI: Multi-platform (Ubuntu GCC/Clang, macOS, Windows MSVC), coverage, static analysis,
sanitizers, stress tests, benchmarks, Valgrind (Linux), CVE scan.

## Key Patterns

- **Thread pool** — Workers enqueued via `enqueue_batch()`, lifecycle: `start()` -> `submit_task()` -> `shutdown_pool()`
- **Queue strategies** — 2 public types: `adaptive_job_queue` (auto mutex/lockfree), `job_queue` (mutex FIFO)
- **Priority scheduling** — `typed_thread_pool` with `job_types` enum routing and priority aging
- **Hazard pointers** — Thread-local hazard arrays (4 slots), automatic reclamation, used by lockfree queue
- **Lock-free queue** — Michael-Scott algorithm with hazard pointer memory reclamation
- **DAG scheduling** — Dependency graph-based job execution with `dag_job_builder`
- **Result wrappers** — `thread::result<T>` / `thread::result_void` wrap `common::Result`

## Ecosystem Position

**Tier 1** — Core Services layer, depends only on common_system (Tier 0).

| Downstream | Dependency Type |
|------------|----------------|
| logger_system | Optional (async logging) |
| monitoring_system | Required |
| network_system | Required |
| database_system | Optional |

## Dependencies

**Required**: kcenon-common-system
**External**: simdutf 5.2.5 (Unicode conversion)
**Dev/test**: Google Test 1.17.0, Google Benchmark 1.9.5, spdlog 1.15.3 (benchmark comparison)

## Known Constraints

- C++20 required; **higher compiler baseline**: GCC 13+, Clang 17+, MSVC 2022+ (due to `std::format`)
- C++20 modules experimental (CMake 3.28+, Clang 16+/GCC 14+)
- ARMv7 and RISC-V untested; UWP/Xbox unsupported
- `thread::result<T>` uses `.get_error()` (not `.error()`) for backward compatibility
- Work-stealing scheduler OFF by default (experimental)
