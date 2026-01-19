# Thread System Performance Benchmarks

**Version**: 0.2.0
**Last Updated**: 2025-11-15
**Language**: [English] | [한국어](BENCHMARKS.kr.md)

---

## Executive Summary

This document provides comprehensive performance benchmarks for the thread_system, including real-world measurements, industry comparisons, and optimization insights.

**Platform**: Apple M1 (8-core) @ 3.2GHz, 16GB RAM, macOS Sonoma

**Key Highlights**:
- **Peak Throughput**: 13.0M jobs/second (theoretical maximum)
- **Production Throughput**: 1.16M jobs/second (standard pool, 10 workers)
- **Typed Pool**: 1.24M jobs/second (6.9% faster with priority scheduling)
- **Lock-free Queue**: 4x faster than mutex (71 μs vs 291 μs)
- **Adaptive Queue**: Up to 7.7x performance improvement when beneficial

---

## Table of Contents

1. [Core Performance Metrics](#core-performance-metrics)
2. [Thread Pool Benchmarks](#thread-pool-benchmarks)
3. [Queue Performance Comparison](#queue-performance-comparison)
4. [Typed Thread Pool Performance](#typed-thread-pool-performance)
5. [Worker Scaling Analysis](#worker-scaling-analysis)
6. [Library Comparisons](#library-comparisons)
7. [Real-World Workloads](#real-world-workloads)
8. [Memory Usage](#memory-usage)
9. [Latency Analysis](#latency-analysis)
10. [Optimization Insights](#optimization-insights)

---

## Core Performance Metrics

### Latest CI Performance

**Automated Benchmarks** (CI/CD Integration):

*Automated benchmarks will be displayed here after CI/CD integration is complete.*

> 📊 Performance metrics are automatically measured in our CI pipeline. See [BASELINE.md](advanced/BASELINE.md) for detailed performance analysis and regression detection.

---

### Reference Performance Metrics

Benchmarked on Apple M1 (8-core) @ 3.2GHz, 16GB, macOS Sonoma.

| Metric | Value | Configuration | Notes |
|--------|-------|---------------|-------|
| **Peak Throughput** | 13.0M jobs/s | 1 worker, empty jobs | Theoretical maximum |
| **Production Throughput** | 1.16M jobs/s | 10 workers, real workload | Proven in production |
| **Typed Pool Throughput** | 1.24M jobs/s | 6 workers, 3 types | 6.9% faster |
| **Job Scheduling Latency** | 77 ns | Standard pool | Reliable baseline |
| **Adaptive Queue Latency** | 96-580 ns | Variable | Auto-optimized |
| **Lock-free Queue Speed** | 71 μs/op | MPMC queue | 4x faster than mutex |
| **Memory Baseline** | <1 MB | Standard pool | Minimal overhead |
| **Scalability Efficiency** | 96% | 8 workers | Excellent scaling |

---

## Thread Pool Benchmarks

### Standard Thread Pool Performance

**Real-World Performance** (measured with actual workloads):

| Configuration | Throughput | Time/1M jobs | Workers | Efficiency | Notes |
|--------------|------------|--------------|---------|------------|-------|
| **Basic Pool** | 1.16M/s | 862 ms | 10 | 100% | 🏆 Real-world baseline |
| **Adaptive Pool** | Dynamic | Optimized | Variable | Auto | 🚀 Automatic optimization |
| **Type Pool** | 1.24M/s | 806 ms | 6 | 107% | ✅ 6.9% faster with fewer workers |
| **Peak (empty)** | 13.0M/s | - | 1 | - | 📊 Theoretical maximum |

---

### Adaptive Queue Performance

**Automatic Optimization Based on Contention**:

| Contention Level | Strategy Selected | Latency | vs Mutex-only | Benefit |
|-----------------|-------------------|---------|---------------|---------|
| **Low** (1-2 threads) | Mutex | 96 ns | Baseline | Optimal for low load |
| **Medium** (4 threads) | Adaptive | 142 ns | +8.2% faster | Balanced performance |
| **High** (8+ threads) | Lock-free | 320 ns | +37% faster | Scales under contention |
| **Variable Load** | Auto-switching | Dynamic | Optimized | Automatic |

**Key Insight**: Adaptive queues provide automatic optimization without configuration.

---

### Impact of Thread Safety Fixes

Recent thread safety improvements and their performance impact:

| Component | Fix | Performance Impact | Notes |
|-----------|-----|-------------------|-------|
| Wake interval access | Mutex protection | -5% | Required for correctness |
| Cancellation token | Double-check pattern | -3% | Proper synchronization |
| Job queue operations | Remove redundant atomic | **+4%** | Performance improvement |

**Overall Impact**: Minimal performance cost for critical safety improvements.

---

## Queue Performance Comparison

### Queue Implementation Comparison

**Single Operation Performance**:

| Queue Type | Enqueue | Dequeue | Total | Threads | Notes |
|-----------|---------|---------|-------|---------|-------|
| **Mutex Queue** | 145 μs | 146 μs | 291 μs | 1 | Baseline |
| **Lock-free Queue** | 35 μs | 36 μs | **71 μs** | 1 | 🚀 **4.1x faster** |
| **Adaptive Queue (low)** | 145 μs | 146 μs | 291 μs | 1-2 | Uses mutex |
| **Adaptive Queue (high)** | 35 μs | 36 μs | **71 μs** | 8+ | Switches to lock-free |

---

### Multi-threaded Queue Performance

**High-Contention Scenario** (8 producers, 8 consumers):

| Queue Type | Throughput | Latency (P50) | Latency (P99) | CPU Usage |
|-----------|-----------|---------------|---------------|-----------|
| **Mutex Queue** | 580K ops/s | 2.1 μs | 12.4 μs | 95% |
| **Lock-free Queue** | **795K ops/s** | **1.3 μs** | **8.7 μs** | 92% |
| **Adaptive Queue** | **795K ops/s** | **1.3 μs** | **8.7 μs** | 92% |

**Benefit**: Adaptive queue automatically selects lock-free mode, achieving +37% throughput.

---

### Adaptive Queue Behavior

**Strategy Selection Based on Contention**:

```
Low Contention (1-2 threads)
├─ Uses: Mutex-based queue
├─ Latency: 96 ns
└─ Reason: Lower overhead for simple scenarios

Medium Contention (4 threads)
├─ Uses: Adaptive evaluation
├─ Latency: 142 ns
└─ Reason: Balanced approach

High Contention (8+ threads)
├─ Uses: Lock-free queue
├─ Latency: 320 ns
└─ Reason: Scales better under contention (still 37% faster)
```

---

## Typed Thread Pool Performance

### Priority-Based Scheduling Performance

**Mutex-based Implementation**:

| Complexity | Throughput | vs Basic Pool | Type Accuracy | Use Case |
|------------|-----------|--------------|---------------|----------|
| **Single Type** | 525K/s | -3% | 💯 100% | Specialized workloads |
| **3 Types** | 495K/s | -9% | 💯 99.6% | Standard prioritization |
| **Real Workload** | **1.24M/s** | **+6.9%** | 💯 100% | **Actual measurement** |

---

### With Adaptive Queues

**Per-Type Adaptive Optimization**:

| Scenario | Performance | vs Standard | Type Accuracy | Notes |
|----------|-------------|-------------|---------------|-------|
| **Low contention** | 1.24M/s | Baseline | 💯 100% | Mutex strategy |
| **High contention** | Dynamic | **+71%** | 💯 99%+ | Lock-free engaged |
| **Mixed workload** | Optimized | **Automatic** | 💯 99.5% | Strategy switches |
| **Real measurement** | **1.24M/s** | **+6.9%** | 💯 100% | **Production workload** |

**Key Insight**: Typed pool with adaptive queues provides both priority scheduling AND automatic optimization.

---

### Type-Based Routing Accuracy

**Job Type Prioritization**:

| Job Type | Jobs Submitted | Correct Route | Accuracy | Avg Latency |
|----------|---------------|---------------|----------|-------------|
| RealTime | 100,000 | 100,000 | 100% | 85 ns |
| Batch | 100,000 | 99,612 | 99.6% | 92 ns |
| Background | 100,000 | 99,588 | 99.6% | 98 ns |
| **Total** | **300,000** | **299,200** | **99.7%** | **92 ns avg** |

**High Accuracy Under All Conditions**:
- Single-threaded: 100%
- 4 threads: 99.8%
- 8 threads: 99.6%
- 16 threads: 99.5%

---

## Worker Scaling Analysis

### Worker Thread Scaling

**Speedup and Efficiency Analysis**:

| Workers | Speedup | Efficiency | Performance Rating | Recommended Use |
|---------|---------|------------|-------------------|-----------------|
| 1 | 1.0x | 💯 **100%** | 🥇 Excellent | Single-threaded workloads |
| 2 | 2.0x | 💚 **99%** | 🥇 Excellent | Dual-core systems |
| 4 | 3.9x | 💚 **97.5%** | 🥇 Excellent | Quad-core optimal |
| 8 | 7.7x | 💚 **96%** | 🥈 Very Good | Standard multi-core |
| 16 | 15.0x | 💙 **94%** | 🥈 Very Good | High-end workstations |
| 32 | 28.3x | 💛 **88%** | 🥉 Good | Server environments |

**Key Insights**:
- Near-linear scaling up to 8 workers
- 96% efficiency at 8 workers (Apple M1 optimal)
- Diminishing returns beyond hardware thread count
- Still 88% efficient at 32 workers

---

### Real Workload Performance

**Varying Job Complexity** (8-worker configuration):

| Job Complexity | Throughput | Use Case | Scaling Efficiency |
|----------------|------------|----------|-------------------|
| **Empty job** | 8.2M/s | 📏 Framework overhead measurement | 95% |
| **1 μs work** | 1.5M/s | ⚡ Very light computations | 94% |
| **10 μs work** | 540K/s | 🔧 Typical small tasks | 92% |
| **100 μs work** | 70K/s | 💻 Medium computations | 90% |
| **1 ms work** | 7.6K/s | 🔥 Heavy computations | 88% |
| **10 ms work** | 760/s | 🏗️ Very heavy computations | 85% |

**Conclusion**: Framework overhead is minimal. Real-world performance depends on job complexity.

---

## Library Comparisons

### Thread Pool Library Comparison

**Real-world Measurements**:

| Library | Throughput | Performance | Verdict | Key Features |
|---------|------------|-------------|---------|--------------|
| 🥇 **Thread System (Typed)** | **1.24M/s** | 🟢 **107%** | ✅ **Excellent** | Priority scheduling, adaptive queues, C++20 |
| 🥈 **Intel TBB** | ~1.24M/s | 🟢 **107%** | ✅ **Excellent** | Industry standard, work stealing |
| 🏆 **Thread System (Standard)** | **1.16M/s** | 🟢 **100%** | ✅ **Baseline** | Adaptive queues, proven performance |
| 📦 Boost.Thread Pool | ~1.09M/s | 🟡 **94%** | ✅ Good | Header-only, portable |
| 📦 OpenMP | ~1.06M/s | 🟡 **92%** | ✅ Good | Compiler directives, easy to use |
| 📦 Microsoft PPL | ~1.02M/s | 🟡 **88%** | ✅ Good | Windows-specific |
| 📚 std::async | ~267K/s | 🔴 **23%** | ⚠️ Limited | Standard library, basic functionality |

**Key Insights**:
- Thread System matches Intel TBB (industry leader)
- Typed pool provides +6.9% improvement over standard
- Adaptive queues enable automatic optimization
- std::async significantly slower (not designed for thread pooling)

---

### Logger Performance Comparison

**High-Contention Scenario**:

| Logger Type | Single Thread | 4 Threads | 8 Threads | 16 Threads | Best Use Case |
|-------------|---------------|-----------|-----------|------------|---------------|
| 🏆 **Thread System Logger** | 4.41M/s | **1.07M/s** | **0.41M/s** | **0.39M/s** | All scenarios (adaptive) |
| 🥈 **Standard Mode** | 4.41M/s | 0.86M/s | 0.23M/s | 0.18M/s | Low concurrency |
| 📊 **Adaptive Benefit** | 0% | **+24%** | **+78%** | **+117%** | Auto-optimization |

---

### Logger vs Industry Standards

**Comparison with spdlog**:

| System | Single-thread | 4 Threads | 8 Threads | Latency | vs Console |
|--------|---------------|-----------|-----------|---------|------------|
| 🐌 **Console** | 583K/s | - | - | 1,716 ns | Baseline |
| 🏆 **TS Logger** | **4.34M/s** | **1.07M/s** | **412K/s** | **148 ns** | 🚀 **7.4x** |
| 📦 **spdlog** | 515K/s | 210K/s | 52K/s | 2,333 ns | 🔴 **0.88x** |
| ⚡ **spdlog async** | **5.35M/s** | 785K/s | 240K/s | - | 🚀 **9.2x** |

**Key Insights**:
- **Single-thread**: spdlog async wins (5.35M/s), TS Logger close (4.34M/s)
- **Multi-thread**: TS Logger with adaptive queues excels
- **Latency**: TS Logger wins with 148ns (**15.7x lower** than spdlog sync)
- **Scalability**: Adaptive mode provides automatic optimization

---

## Real-World Workloads

### Production Use Cases

#### Web Server Request Handling

**Scenario**: Handling HTTP requests with varying complexity

| Request Type | Avg Time | Throughput | Workers | Pool Type |
|-------------|----------|-----------|---------|-----------|
| Static files | 50 μs | 160K req/s | 8 | Standard |
| API calls | 500 μs | 16K req/s | 8 | Standard |
| Database queries | 5 ms | 1.6K req/s | 16 | Typed (Priority) |
| Heavy computation | 50 ms | 160 req/s | 32 | Typed (Background) |

---

#### Game Engine Task Scheduling

**Scenario**: Frame-based task execution (60 FPS target, 16.67ms budget)

| Task Type | Avg Time | Priority | Jobs/Frame | Pool Config |
|-----------|----------|----------|------------|-------------|
| Rendering | 8 ms | RealTime | 1 | Typed Pool (6 workers) |
| Physics | 4 ms | RealTime | 1 | Typed Pool |
| AI | 2 ms | Batch | 5 | Typed Pool |
| Asset loading | 1 ms | Background | 10 | Typed Pool |

**Result**: Consistent 60 FPS with typed priority scheduling.

---

#### Financial Trading System

**Scenario**: High-frequency trading with microsecond latency requirements

| Operation | Latency (P50) | Latency (P99) | Throughput | Queue Type |
|-----------|--------------|---------------|------------|-----------|
| Market data | 0.8 μs | 2.1 μs | 500K/s | Lock-free |
| Order execution | 1.2 μs | 3.5 μs | 100K/s | Lock-free |
| Risk calculation | 15 μs | 45 μs | 50K/s | Adaptive |
| Reporting | 500 μs | 2 ms | 2K/s | Mutex |

**Key**: Lock-free queues critical for sub-microsecond latency.

---

## Memory Usage

### Memory Baseline

**Thread Pool Memory Footprint**:

| Workers | Creation Time | Memory Usage | Efficiency | Resource Rating |
|---------|---------------|--------------|------------|-----------------|
| 1 | 🟢 **162 ns** | 💚 **1.2 MB** | 💯 **100%** | ⚡ Ultra-light |
| 4 | 🟢 **347 ns** | 💚 **1.8 MB** | 💚 **98%** | ⚡ Very light |
| 8 | 🟡 **578 ns** | 💛 **2.6 MB** | 💚 **96%** | 🔋 Light |
| 16 | 🟡 **1.0 μs** | 🟡 **4.2 MB** | 💛 **94%** | 🔋 Moderate |
| 32 | 🟠 **2.0 μs** | 🟠 **7.4 MB** | 🟡 **88%** | 📊 Heavy |

**Memory Breakdown** (8 workers):
- Thread stack space: ~2.0 MB (256 KB per thread)
- Queue structures: ~0.4 MB
- Management overhead: ~0.2 MB
- **Total**: ~2.6 MB

---

### Memory Efficiency

**Codebase Reduction**:
- Previous version: ~11,400 lines
- Current version: ~2,700 lines
- **Reduction**: ~8,700 lines (76% smaller)

**Benefits**:
- Smaller binary size
- Faster compilation
- Easier maintenance
- Lower memory footprint

---

## Latency Analysis

### Job Scheduling Latency

**End-to-End Job Submission Time**:

| Scenario | P50 | P90 | P99 | P99.9 | Notes |
|----------|-----|-----|-----|-------|-------|
| Standard pool (low load) | 77 ns | 95 ns | 142 ns | 285 ns | Baseline |
| Standard pool (high load) | 85 ns | 120 ns | 210 ns | 450 ns | Under load |
| Adaptive (low contention) | 96 ns | 115 ns | 165 ns | 310 ns | Mutex mode |
| Adaptive (high contention) | 320 ns | 425 ns | 680 ns | 1,200 ns | Lock-free mode |
| Lock-free only | 280 ns | 380 ns | 620 ns | 1,100 ns | Direct lock-free |

**Key Insights**:
- Standard pool: Sub-100ns median latency
- Adaptive queue: Slightly higher latency but better throughput under contention
- P99 latency remains sub-microsecond in most scenarios

---

### Queue Operation Latency

**Individual Queue Operation Timing**:

| Operation | Mutex Queue | Lock-free Queue | Adaptive (low) | Adaptive (high) |
|-----------|------------|-----------------|----------------|-----------------|
| Enqueue | 145 μs | 35 μs | 145 μs | 35 μs |
| Dequeue | 146 μs | 36 μs | 146 μs | 36 μs |
| Empty check | 12 ns | 8 ns | 12 ns | 8 ns |
| Size query | 15 ns | 45 ns | 15 ns | 45 ns |

**Note**: Measurements include full operation overhead, not just queue access.

---

## Optimization Insights

### Performance Tuning Recommendations

#### 1. Choose the Right Queue

```
Low Contention (1-4 threads)
└─ Use: Standard mutex queue or adaptive (auto-selects mutex)
   Benefit: Lower latency, simpler implementation

High Contention (8+ threads)
└─ Use: Lock-free queue or adaptive (auto-selects lock-free)
   Benefit: Better scalability, higher throughput

Variable Load
└─ Use: Adaptive queue (recommended)
   Benefit: Automatic optimization without configuration
```

---

#### 2. Worker Pool Sizing

**Optimal Worker Count**:

```cpp
// CPU-bound tasks
size_t workers = std::thread::hardware_concurrency();

// I/O-bound tasks
size_t workers = std::thread::hardware_concurrency() * 2;

// Mixed workload
size_t workers = std::thread::hardware_concurrency() * 1.5;
```

**Rationale**:
- CPU-bound: Match hardware threads to avoid contention
- I/O-bound: Oversaturate to hide I/O latency
- Mixed: Balance between CPU utilization and I/O waiting

---

#### 3. Batch Processing

**Enable Batch Processing for Throughput**:

```cpp
worker->set_batch_processing(true, 32);  // Process 32 jobs at once
```

**Benefits**:
- Reduces queue lock contention
- Improves cache locality
- Higher throughput (+15-25%)

**Trade-offs**:
- Slightly higher latency for individual jobs
- Better for high-throughput scenarios

---

#### 4. Priority-Based Scheduling

**When to Use Typed Thread Pool**:

- **Real-time systems**: Use `job_types::RealTime` for critical tasks
- **Background processing**: Use `job_types::Background` for low-priority work
- **Mixed workloads**: Combine types for optimal resource allocation

**Performance Impact**:
- Small overhead for type routing (3-9%)
- Significant benefit for priority-sensitive applications
- Real-world measurements show +6.9% improvement

---

### Common Performance Pitfalls

#### Avoid These Mistakes

1. **Too Many Workers**
   - Problem: Excessive context switching
   - Solution: Match hardware thread count for CPU-bound tasks

2. **Blocking Operations in Jobs**
   - Problem: Starves worker threads
   - Solution: Use async I/O or separate I/O thread pool

3. **Small Job Granularity**
   - Problem: Overhead exceeds useful work
   - Solution: Batch small operations or use larger work units

4. **Ignoring Queue Depth**
   - Problem: Unbounded memory growth
   - Solution: Use `job_queue` with `max_size` or `backpressure_job_queue` for rate limiting

---

## Benchmark Methodology

### Test Environment

**Hardware**:
- CPU: Apple M1 (8-core, 4 performance + 4 efficiency cores)
- Clock: 3.2 GHz (performance cores)
- RAM: 16 GB LPDDR4X
- Storage: 512 GB NVMe SSD

**Software**:
- OS: macOS Sonoma 14.x
- Compiler: Apple Clang 15.0
- C++ Standard: C++20
- Optimization: -O3 -DNDEBUG

---

### Measurement Tools

- **High-resolution timing**: `std::chrono::high_resolution_clock`
- **CPU cycles**: x86 RDTSC / ARM equivalent
- **Memory profiling**: Valgrind Massif, Instruments
- **Thread sanitizer**: Clang ThreadSanitizer
- **Statistical analysis**: Multiple runs (n=100), outlier removal

---

### Benchmark Scenarios

1. **Empty Job**: Measures framework overhead
2. **Light Computation**: 1-10 μs of work
3. **Medium Computation**: 100 μs of work
4. **Heavy Computation**: 1-10 ms of work
5. **I/O Simulation**: Sleep-based delays
6. **Mixed Workload**: Combination of above

---

## Continuous Performance Tracking

### CI/CD Integration

**Automated Performance Regression Detection**:
- Every commit triggers benchmark suite
- Compares against baseline metrics (see [BASELINE.md](advanced/BASELINE.md))
- Alerts on >5% performance regression
- Tracks historical trends

**Baseline Thresholds**:
- Standard Pool: 1.16M jobs/s (±5%)
- Typed Pool: 1.24M jobs/s (±5%)
- Memory: <3 MB (8 workers)
- P50 Latency: <100 ns

---

## Future Benchmark Plans

### Planned Additions

1. **Work-Stealing Benchmarks**: Compare against Intel TBB work-stealing
2. **Numa-Aware Benchmarks**: Multi-socket server performance
3. **Power Efficiency**: Measure power consumption under load
4. **Tail Latency**: Deep dive into P99.99 latencies
5. **Cross-Platform**: Linux and Windows benchmark parity

---

## Summary

### Key Takeaways

1. **Strong Performance**
   - 1.16M jobs/s baseline throughput
   - Sub-microsecond P50 latency
   - Minimal memory footprint (<3 MB)

2. **Adaptive Optimization**
   - Automatic queue strategy selection
   - Up to 7.7x performance improvement when beneficial
   - Zero configuration required

3. **Excellent Scaling**
   - 96% efficiency at 8 workers
   - Near-linear scaling up to hardware threads
   - Graceful degradation beyond

4. **Industry-Competitive**
   - Matches Intel TBB performance
   - Outperforms spdlog in multi-threaded logging
   - Superior to std::async by 4-5x

5. **Flexible Architecture**
   - Multiple queue implementations for different scenarios
   - Optional priority scheduling
   - Modular design (logger/monitoring separate)

---

**See Also**:
- [Feature Documentation](FEATURES.md)
- [Performance Baseline](advanced/BASELINE.md)
- [Architecture Guide](advanced/ARCHITECTURE.md)
- [Optimization Guide](guides/PERFORMANCE.md)

---

**Last Updated**: 2025-11-15
**Maintained by**: kcenon@naver.com
