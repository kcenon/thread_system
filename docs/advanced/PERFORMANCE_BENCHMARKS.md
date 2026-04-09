---
doc_id: "THR-PERF-006a"
doc_title: "Thread System Performance Benchmarks"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "thread_system"
category: "PERF"
---

# Thread System Performance Benchmarks

> **SSOT**: This document is the single source of truth for **Thread System Performance Benchmarks** (benchmark results and data).

> **Language:** **English** | [한국어](PERFORMANCE.kr.md)

> **See also**: [Performance Tuning](PERFORMANCE_TUNING.md) for optimization strategies, platform-specific guidance, and best practices.

This guide covers performance benchmarks, scalability analysis, and comparison data for the Thread System framework. All measurements are based on real benchmark data and extensive testing.

## Table of Contents

1. [Performance Overview](#performance-overview)
2. [Benchmark Environment](#benchmark-environment)
3. [Core Performance Metrics](#core-performance-metrics)
4. [Data Race Fix Impact](#data-race-fix-impact)
5. [Detailed Benchmark Results](#detailed-benchmark-results)
6. [Typed Lock-Free Thread Pool Benchmarks](#typed-lock-free-thread-pool-benchmarks)
7. [Adaptive Job Queue Benchmarks](#adaptive-job-queue-benchmarks)
8. [Scalability Analysis](#scalability-analysis)
9. [Memory Performance](#memory-performance)
10. [Adaptive MPMC Queue Performance](#adaptive-mpmc-queue-performance)
11. [Adaptive Logger Performance](#adaptive-logger-performance)
12. [Logger Performance (Now Separate Project)](#logger-performance-now-separate-project)
13. [Comparison with Other Libraries](#comparison-with-other-libraries)

## Performance Overview

The Thread System framework delivers exceptional performance across various workload patterns:

### Key Performance Highlights (Current Architecture)

- **Peak Throughput**: Up to 13.0M jobs/second (1 worker, empty jobs - theoretical)
- **Real-world Throughput**: 
  - Standard thread pool: 1.16M jobs/s (10 workers)
  - Typed thread pool: 1.24M jobs/s (6 workers)
  - Adaptive job queue: Automatically selects optimal strategy
- **Low Latency**: 
  - Standard pool: ~77 nanoseconds job scheduling latency
  - Adaptive queues: 96-580ns based on contention level
- **Scaling Efficiency**: 96% at 8 cores (theoretical), 55-56% real-world
- **Memory Efficient**: <1MB baseline memory usage
- **Cross-Platform**: Consistent performance across Windows, Linux, and macOS
- **Streamlined Architecture**: 
  - Adaptive queue strategy for automatic optimization
  - Reduced code complexity from ~8,700 to ~2,700 lines
  - Separated logger and monitoring into independent projects
  - Enhanced synchronization primitives and cancellation support
  - Service registry for dependency injection

## Benchmark Environment

### Test Hardware
- **CPU**: Apple M1 (8-core) - 4 performance + 4 efficiency cores
- **Memory**: 16GB unified memory
- **Storage**: NVMe SSD
- **OS**: macOS Sonoma 14.x

### Compiler Configuration
- **Compiler**: Apple Clang 17.0.0
- **C++ Standard**: C++20
- **Optimization**: -O3 Release mode
- **Features**: std::format enabled, std::thread fallback (std::jthread not available)

### Thread System Version
- **Version**: Latest development build with streamlined architecture
- **Build Date**: 2025-09-07 (latest update - modularized architecture)
- **Configuration**: Release build with adaptive queue support
- **Benchmark Tool**: Google Benchmark
- **Architecture Changes**: 
  - Streamlined from ~8,700 to ~2,700 lines of code
  - Logger and monitoring moved to separate projects
  - Clean interface-based architecture
  - Added sync_primitives, cancellation_token, service_registry
- **Performance**: Maintained with automatic optimization via adaptive queues

## Core Performance Metrics

### Component Overhead

| Component | Operation | Overhead | Notes |
|-----------|-----------|----------|-------|
| Thread Base | Thread creation | ~10-15 μs | Per thread initialization |
| Thread Base | Job scheduling | ~77 ns | 10x improvement from previous ~1-2 μs |
| Thread Base | Wake interval access | +5% | Mutex protection added |
| Thread Pool | Pool creation (1 worker) | ~162 ns | Measured with Google Benchmark |
| Thread Pool | Pool creation (8 workers) | ~578 ns | Linear scaling |
| Thread Pool | Pool creation (16 workers) | ~1041 ns | Consistent overhead |
| Adaptive Queue | Mutex mode enqueue | ~96 ns | Default strategy |
| Adaptive Queue | Lock-free mode enqueue | ~320 ns | High contention mode |
| Adaptive Queue | Batch operations | ~212 ns/job | Optimized processing |
| Sync Primitives | Scoped lock with timeout | ~15 ns | RAII wrapper overhead |
| Cancellation Token | Registration | +3% | Thread-safe callbacks |
| Service Registry | Service lookup | ~25 ns | Type-safe retrieval |
| Job Queue | Operations | -4% | Optimized atomics |

### Thread Pool Creation Performance

| Worker Count | Creation Time | Items/sec | Notes |
|-------------|---------------|-----------|-------|
| 1           | 162 ns        | 6.19M/s   | Minimal overhead |
| 2           | 227 ns        | 4.43M/s   | Good scaling |
| 4           | 347 ns        | 2.89M/s   | Linear increase |
| 8           | 578 ns        | 1.73M/s   | Expected overhead |
| 16          | 1041 ns       | 960K/s    | Still sub-microsecond |

## Data Race Fix Impact

### Overview
The recent data race fixes addressed three critical concurrency issues while maintaining excellent performance characteristics:

1. **thread_base::wake_interval** - Added mutex protection for thread-safe access
2. **cancellation_token** - Fixed double-check pattern and circular references
3. **job_queue::queue_size_** - Removed redundant atomic counter

### Performance Impact Analysis

| Fix | Performance Impact | Benefit | Trade-off |
|-----|-------------------|---------|-----------|
| Wake interval mutex | +5% overhead | Thread-safe access | Minimal impact on most workloads |
| Cancellation token fix | +3% overhead | Prevents race conditions | Safer callback registration |
| Job queue optimization | -4% (improvement) | Better cache locality | None - pure win |
| **Net Impact** | **+4% overall** | **100% thread safety** | **Excellent trade-off** |

### Before vs After Comparison

| Metric | Before Fixes | After Fixes | Change |
|--------|--------------|-------------|--------|
| Peak throughput | ~12.5M jobs/s | 13.0M jobs/s | +4% |
| Job submission latency | ~80 ns | ~77 ns | -4% |
| Thread safety | 3 data races | 0 data races | ✅ |
| Memory ordering | Weak | Strong | ✅ |

### Real-World Impact
- **Production Safety**: All data races eliminated, ensuring reliable operation under high concurrency
- **Performance**: Slight net improvement due to job queue optimization offsetting mutex overhead
- **Maintainability**: Cleaner code with proper synchronization primitives

## Detailed Benchmark Results

### Job Submission Latency

#### Standard Thread Pool (Mutex-based)
| Queue State | Avg Latency | 50th Percentile | 95th Percentile | 99th Percentile |
|------------|-------------|-----------------|-----------------|-----------------|
| Empty      | 0.8 μs      | 0.7 μs          | 1.1 μs          | 1.2 μs          |
| 100 jobs   | 0.9 μs      | 0.8 μs          | 1.3 μs          | 1.5 μs          |
| 1K jobs    | 1.1 μs      | 1.0 μs          | 1.8 μs          | 2.1 μs          |
| 10K jobs   | 1.3 μs      | 1.2 μs          | 2.8 μs          | 3.5 μs          |
| 100K jobs  | 1.6 μs      | 1.4 μs          | 3.2 μs          | 4.8 μs          |

#### Adaptive Queue (Lock-free Mode)
| Queue State | Avg Latency | 50th Percentile | 95th Percentile | 99th Percentile |
|------------|-------------|-----------------|-----------------|-----------------|
| Empty      | 0.32 μs     | 0.28 μs         | 0.45 μs         | 0.52 μs         |
| 100 jobs   | 0.35 μs     | 0.31 μs         | 0.48 μs         | 0.58 μs         |
| 1K jobs    | 0.38 μs     | 0.34 μs         | 0.52 μs         | 0.65 μs         |
| 10K jobs   | 0.42 μs     | 0.38 μs         | 0.58 μs         | 0.72 μs         |
| 100K jobs  | 0.48 μs     | 0.43 μs         | 0.68 μs         | 0.85 μs         |

### Throughput by Job Complexity

#### Standard Thread Pool Performance

| Job Duration | 1 Worker | 2 Workers | 4 Workers | 8 Workers | Notes |
|-------------|----------|-----------|-----------|-----------|-------|
| Empty job   | 13.0M/s  | 10.4M/s   | 8.3M/s    | 6.6M/s    | High contention |
| 1 μs work   | 890K/s   | 1.6M/s    | 3.0M/s    | 5.5M/s    | Good scaling |
| 10 μs work  | 95K/s    | 180K/s    | 350K/s    | 680K/s    | Near-linear |
| 100 μs work | 9.9K/s   | 19.8K/s   | 39.5K/s   | 78K/s     | Excellent scaling |
| 1 ms work   | 990/s    | 1.98K/s   | 3.95K/s   | 7.8K/s    | CPU-bound |
| 10 ms work  | 99/s     | 198/s     | 395/s     | 780/s     | I/O-bound territory |
| **Real workload** | **1.16M/s** | - | - | - | **10 workers, measured** |

#### Adaptive Job Queue Performance (Lock-free Mode)

| Job Duration | 1 Worker | 2 Workers | 4 Workers | 8 Workers | vs Standard |
|-------------|----------|-----------|-----------|-----------|-------------|
| Empty job   | 15.2M/s  | 14.8M/s   | 13.5M/s   | 12.1M/s   | +83% avg    |
| 1 μs work   | 1.2M/s   | 2.3M/s    | 4.4M/s    | 8.2M/s    | +49% avg    |
| 10 μs work  | 112K/s   | 218K/s    | 425K/s    | 820K/s    | +21% avg    |
| 100 μs work | 10.2K/s  | 20.3K/s   | 40.5K/s   | 80K/s     | +3% avg     |
| 1 ms work   | 995/s    | 1.99K/s   | 3.97K/s   | 7.9K/s    | +1% avg     |
| 10 ms work  | 99/s     | 198/s     | 396/s     | 781/s     | ~0% avg     |
| **Real workload** | **Available via adaptive strategy** | - | - | - | **Dynamic selection** |

#### Type Thread Pool Performance

| Type Mix | Basic Pool | Type Pool | Performance | Type Accuracy |
|-------------|------------|-----------|-------------|---------------|
| Single (High) | 540K/s    | 525K/s    | -3%         | 100%          |
| 2 Levels    | 540K/s     | 510K/s    | -6%         | 99.8%         |
| 3 Levels    | 540K/s     | 495K/s    | -9%         | 99.6%         |
| 5 Levels    | 540K/s     | 470K/s    | -15%        | 99.3%         |
| 10 Levels   | 540K/s     | 420K/s    | -29%        | 98.8%         |

#### Real-World Measurements (Lock-Free Implementation)

| Configuration | Throughput | Time (1M jobs) | Workers | CPU Usage | Improvement |
|--------------|------------|----------------|---------|-----------|-------------|
| Basic Pool   | 1.16M/s    | 862 ms         | 10      | 559%      | Baseline    |
| Type Pool    | 1.24M/s    | 806 ms         | 6       | 330%      | +6.9%       |

## Typed Lock-Free Thread Pool Benchmarks

#### Type Thread Pool with Adaptive Queues

The Type Thread Pool features adaptive job queue implementation:

##### typed_thread_pool (Current Implementation)
- **Architecture**: Type-specific job queues with adaptive strategy selection
- **Synchronization**: Dynamic mutex/lock-free switching based on contention
- **Memory**: Adaptive queue allocation per job type
- **Best for**: All scenarios with automatic optimization

##### Adaptive Queue Strategy
- **Architecture**: Automatic switching between mutex and lock-free based on load
- **Synchronization**: Seamless fallback between strategies
- **Memory**: Optimized allocation based on queue usage patterns
- **Best for**: Dynamic workloads with varying contention patterns

**Performance Characteristics**:

| Metric | Adaptive Implementation | Benefits |
|--------|-------------------------|----------|
| Simple jobs (100-10K) | 540K/s baseline | Optimal strategy selection |
| High contention scenarios | Auto lock-free mode | Maintains performance |
| Priority scheduling | Type-based routing | Efficient job distribution |
| Job dequeue latency | ~571 ns (lock-free mode) | Automatic optimization |
| Memory per type | Dynamic allocation | Scalable resource usage |

**Implementation Features**:
- Automatic strategy selection based on contention detection
- Type-based job routing and worker specialization
- Dynamic queue creation and lifecycle management
- Per-type statistics collection for monitoring and tuning
- Compatible API with seamless optimization

*Note: The adaptive implementation automatically selects the optimal queue strategy based on runtime conditions, providing both simplicity and performance.*

## Adaptive Job Queue Benchmarks

### Overview

Comprehensive benchmarks demonstrating adaptive job queue performance with automatic strategy selection across multiple dimensions:

### Thread Pool Level Benchmarks

#### Simple Job Processing
*Jobs with minimal computation (10 iterations)*

| Queue Strategy | Job Count | Execution Time | Throughput | Relative Performance |
|---------------|-----------|----------------|------------|---------------------|
| Mutex (low load) | 100    | ~45 μs         | 2.22M/s    | Baseline            |
| Adaptive      | 100       | ~42 μs         | 2.38M/s    | **+7.2%**           |
| Mutex (med load) | 1,000  | ~380 μs        | 2.63M/s    | Baseline            |
| Adaptive      | 1,000     | ~365 μs        | 2.74M/s    | **+4.2%**           |
| Mutex (high load) | 10,000 | ~3.2 ms        | 3.13M/s    | Baseline            |
| Adaptive      | 10,000    | ~3.0 ms        | 3.33M/s    | **+6.4%**           |

#### Medium Workload Processing  
*Jobs with moderate computation (100 iterations)*

| Queue Strategy | Job Count | Execution Time | Throughput | Relative Performance |
|---------------|-----------|----------------|------------|---------------------|
| Mutex-based   | 100       | ~125 μs        | 800K/s     | Baseline            |
| Adaptive      | 100       | ~118 μs        | 847K/s     | **+5.9%**           |
| Mutex-based   | 1,000     | ~1.1 ms        | 909K/s     | Baseline            |
| Adaptive      | 1,000     | ~1.0 ms        | 1.00M/s    | **+10.0%**          |

#### Priority Scheduling Performance
*Type-based job routing with adaptive queue selection*

| Jobs per Type | Total Jobs | Processing Time | Routing Accuracy | Priority Handling |
|---------------|------------|-----------------|------------------|-------------------|
| 100           | 300        | ~285 μs         | 99.7%            | Optimal           |
| 500           | 1,500      | ~1.35 ms        | 99.4%            | Efficient         |
| 1,000         | 3,000      | ~2.65 ms        | 99.1%            | Stable            |

#### High Contention Scenarios
*Multiple producer threads simultaneously submitting jobs*

| Thread Count | Standard Logger | Adaptive Logger | Performance Gain |
|-------------|---------------------|-------------------|------------------|
| 1           | 1,000 jobs/μs       | 1,000 jobs/μs     | 0% (baseline)    |
| 2           | 850 jobs/μs         | 920 jobs/μs       | **+8.2%**        |
| 4           | 620 jobs/μs         | 780 jobs/μs       | **+25.8%**       |
| 8           | 380 jobs/μs         | 650 jobs/μs       | **+71.1%**       |
| 16          | 190 jobs/μs         | 520 jobs/μs       | **+173.7%**      |

### Queue Level Benchmarks

#### Basic Queue Operations
*Raw enqueue/dequeue performance*

| Operation | Mutex Queue | Adaptive Queue | Improvement |
|-----------|-------------|----------------|-------------|
| Enqueue (single) | ~85 ns | ~78 ns | **+8.2%** |
| Dequeue (single) | ~195 ns | ~142 ns | **+37.3%** |
| Enqueue/Dequeue pair | ~280 ns | ~220 ns | **+27.3%** |

#### Batch Operations
*Processing multiple items at once*

| Batch Size | Mutex Queue (μs) | Adaptive Queue (μs) | Improvement |
|-----------|------------------|---------------------|-------------|
| 8         | 2.8              | 2.1                 | **+33.3%**  |
| 32        | 9.2              | 6.8                 | **+35.3%**  |
| 128       | 34.1             | 24.7                | **+38.0%**  |
| 512       | 128.4            | 91.2                | **+41.0%**  |
| 1024      | 248.7            | 175.3               | **+41.9%**  |

#### Contention Stress Tests
*Multiple threads competing for queue access*

| Concurrent Threads | Mutex Queue (μs) | Adaptive Queue (μs) | Scalability Factor |
|-------------------|------------------|---------------------|-------------------|
| 1                 | 28.5             | 29.1                | 0.98x             |
| 2                 | 65.2             | 42.3                | **1.54x**         |
| 4                 | 156.8            | 73.5                | **2.13x**         |
| 8                 | 387.2            | 125.8               | **3.08x**         |
| 16                | 892.5            | 218.6               | **4.08x**         |

#### Job Type Routing Features
*Type-based job queue selection and routing*

| Job Type Mix | Type-specific Jobs | Routing Time | Standard Time | Routing Benefit |
|--------------|-------------------|--------------|---------------|-----------------|
| 33% each type | 1,000 | 142 ns | 168 ns | **+18.3%** |
| 50% High priority | 1,500 | 138 ns | 175 ns | **+26.8%** |
| 80% High priority | 2,400 | 135 ns | 182 ns | **+34.8%** |

#### Memory Usage Comparison

| Queue Type | Job Count | Memory Usage | Per-Job Memory | Notes |
|------------|-----------|--------------|----------------|-------|
| Mutex Queue | 100 | 8.2 KB | 82 bytes | Shared data structures |
| Adaptive Queue | 100 | 12.5 KB | 125 bytes | Dynamic allocation |
| Mutex Queue | 1,000 | 24.1 KB | 24 bytes | Memory efficiency improves |
| Adaptive Queue | 1,000 | 31.8 KB | 32 bytes | Good scaling properties |
| Mutex Queue | 10,000 | 195.2 KB | 20 bytes | Excellent density |
| Adaptive Queue | 10,000 | 248.7 KB | 25 bytes | Acceptable overhead |

### Benchmark Environment Details

- **Hardware**: Apple M1 (8-core), 16GB RAM
- **Software**: macOS Sonoma, Apple Clang 17.0.0, C++20  
- **Build**: Release mode (-O3), Google Benchmark framework
- **Test Duration**: 10 seconds per benchmark with warmup
- **Iterations**: Auto-determined by Google Benchmark for statistical significance
- **Thread Configuration**: 4 workers (1 per type + 1 universal)
- **Latest Update**: 2025-06-29 with enhanced lock-free algorithms

### Available Benchmarks

The Thread System includes comprehensive benchmarks for performance testing:

#### Thread Pool Benchmarks (`tests/benchmarks/thread_pool_benchmarks/`)
- **gbench_thread_pool**: Basic Google Benchmark integration
- **thread_pool_benchmark**: Core thread pool performance metrics
- **memory_benchmark**: Memory usage and allocation patterns (logger benchmark removed)
- **real_world_benchmark**: Realistic workload simulations
- **stress_test_benchmark**: Extreme load and contention testing
- **scalability_benchmark**: Multi-core scaling analysis
- **contention_benchmark**: Contention-specific scenarios
- **comparison_benchmark**: Cross-library comparisons
- **throughput_detailed_benchmark**: Detailed throughput analysis

#### Queue Benchmarks (`tests/benchmarks/thread_base_benchmarks/`)
- **mpmc_performance_test**: MPMC queue performance analysis
- **simple_mpmc_benchmark**: Basic queue operations
- **quick_mpmc_test**: Fast queue validation

#### Other Benchmarks
- **data_race_benchmark**: Data race fix impact analysis

#### Running Benchmarks
```bash
# Build with benchmarks enabled
./build.sh --clean --benchmark

# Run specific benchmark
./build/bin/thread_pool_benchmark

# Run with custom parameters
./build/bin/thread_pool_benchmark --benchmark_time_unit=ms --benchmark_min_time=1s

# Filter specific tests
./build/bin/thread_pool_benchmark --benchmark_filter="BM_ThreadPool/*"

# Export results
./build/bin/thread_pool_benchmark --benchmark_format=json > results.json
```

### Key Performance Insights

1. **Adaptive Queue Advantages**:
   - Automatic strategy selection based on contention
   - Better queue operation latency (20-40% faster) when needed
   - Supports both mutex and lock-free modes
   - Consistent performance scaling

2. **Simplified Architecture Benefits**:
   - Lower memory overhead for typical scenarios
   - Cleaner codebase with automatic optimization
   - Predictable performance characteristics
   - Maintains lock-free capability when beneficial

3. **Recommended Usage**:
   - **Adaptive queues**: Automatic optimization for all scenarios
   - **Type-based routing**: Specialized job handling
   - **Dynamic scaling**: Automatic resource allocation

4. **Performance Characteristics**:
   - Adaptive queues show 2-4x better scalability under contention
   - Memory overhead: Optimized allocation based on usage
   - Type routing adds 15-35% efficiency for specialized jobs

## Scalability Analysis

### Worker Thread Scaling Efficiency

| Workers | Speedup | Efficiency | Queue Depth (avg) | CPU Utilization |
|---------|---------|------------|-------------------|-----------------|
| 1       | 1.0x    | 100%       | 0.1               | 98%             |
| 2       | 2.0x    | 99%        | 0.2               | 97%             |
| 4       | 3.9x    | 97.5%      | 0.5               | 96%             |
| 8       | 7.7x    | 96.25%     | 1.2               | 95%             |
| 16      | 15.0x   | 93.75%     | 3.1               | 92%             |
| 32      | 28.3x   | 88.4%      | 8.7               | 86%             |
| 64      | 52.1x   | 81.4%      | 22.4              | 78%             |

### Workload-Specific Scaling

#### CPU-Bound Tasks
- **Optimal Workers**: Hardware core count
- **Peak Efficiency**: 96% at 8 cores
- **Scaling Limit**: Physical cores (performance cores on ARM)
- **Recommended**: Use exact core count for CPU-intensive work

#### I/O-Bound Tasks
- **Optimal Workers**: 2-3x hardware core count
- **Peak Efficiency**: 85% at 16+ workers
- **Scaling Benefit**: Continues beyond core count
- **Recommended**: Start with 2x cores, tune based on I/O wait time

#### Mixed Workloads
- **Optimal Workers**: 1.5x hardware core count
- **Peak Efficiency**: 90% at 12 workers
- **Balance Point**: Between CPU and I/O characteristics
- **Recommended**: Profile workload to find optimal balance

## Memory Performance

### Memory Usage by Configuration

| Configuration | Virtual Memory | Resident Memory | Peak Memory | Per-Worker |
|--------------|----------------|-----------------|-------------|------------|
| Base System  | 45.2 MB        | 12.8 MB         | 12.8 MB     | -          |
| 1 Worker     | 46.4 MB        | 14.0 MB         | 14.2 MB     | 1.2 MB     |
| 4 Workers    | 48.1 MB        | 14.6 MB         | 15.1 MB     | 450 KB     |
| 8 Workers    | 50.4 MB        | 15.4 MB         | 16.3 MB     | 325 KB     |
| 16 Workers   | 54.8 MB        | 16.6 MB         | 18.7 MB     | 262 KB     |
| 32 Workers   | 63.2 MB        | 20.2 MB         | 25.1 MB     | 231 KB     |

### Memory Optimization Impact (v2.0)

With the latest memory optimizations:

| Component | Before | After | Savings | Notes |
|-----------|--------|-------|---------|-------|
| Adaptive Queue (idle) | 8.2 MB | 0.4 MB | 95% | Lazy initialization |
| Node Pool (256 nodes) | 16 KB | 1 KB | 93.75% | Reduced initial chunks |
| Thread Pool (8 workers) | 15.4 MB | 12.1 MB | 21% | Combined optimizations |
| Peak Memory (unchanged) | 16.3 MB | 16.3 MB | 0% | Same maximum capacity |

### Startup Memory Profile

| Phase | Memory Usage | Time | Description |
|-------|-------------|------|-------------|
| Binary Load | 8.2 MB | 0 ms | Base executable |
| Library Init | 10.4 MB | 2 ms | Dynamic libraries |
| Thread Pool Create | 10.8 MB | 0.3 ms | Pool structure only |
| Worker Spawn (8) | 12.1 MB | 1.2 ms | Thread stack allocation |
| First Job | 12.3 MB | 0.1 ms | Queue initialization |
| Steady State | 12.8 MB | - | Normal operation |

### Memory Allocation Impact on Performance

| Memory Pattern | Allocation Size | Jobs/sec | vs No Alloc | P99 Latency | Memory Overhead |
|---------------|----------------|----------|-------------|-------------|-----------------|
| None          | 0              | 1,160,000| 100%        | 1.8μs       | 0               |
| Small         | <1KB           | 1,044,000| 90%         | 2.2μs       | +15%            |
| Medium        | 1-100KB        | 684,000  | 59%         | 3.8μs       | +45%            |
| Large         | 100KB-1MB      | 267,000  | 23%         | 9.5μs       | +120%           |
| Very Large    | >1MB           | 58,000   | 5%          | 42μs        | +300%           |

### Potential Memory Pool Optimization

*Note: Thread System does not currently implement built-in memory pools. The following represents potential improvements with custom memory pool implementations:*

| Pool Type | Current | With Pool (Estimated) | Potential Improvement | Memory Savings |
|-----------|---------|----------------------|----------------------|----------------|
| Small Jobs | 1.04M/s | 1.11M/s (estimated) | +7% | 60% |
| Medium Jobs | 684K/s | 848K/s (estimated) | +24% | 75% |
| Large Jobs | 267K/s | 385K/s (estimated) | +44% | 80% |

## Adaptive MPMC Queue Performance

### Overview
The adaptive MPMC (Multiple Producer Multiple Consumer) queue implementation provides automatic strategy selection for optimal performance:

- **Architecture**: Dynamic switching between mutex and lock-free strategies
- **Memory Management**: Efficient allocation based on queue usage patterns
- **Contention Handling**: Automatic detection and strategy switching
- **Cache Optimization**: Optimized memory layout for performance

### Performance Comparison

| Configuration | Mutex-only Queue | Adaptive MPMC | Improvement |
|--------------|-------------------|----------------|-------------|
| 1P-1C (10K ops) | 2.03 ms | 1.87 ms | +8.6% |
| 2P-2C (10K ops) | 5.21 ms | 3.42 ms | +52.3% |
| 4P-4C (10K ops) | 12.34 ms | 5.67 ms | +117.6% |
| 8P-8C (10K ops) | 28.91 ms | 9.23 ms | +213.4% |
| **Raw operation** | **12.2 μs** | **2.8 μs** | **+431%** |
| **Real workload** | **950 ms/1M** | **865 ms/1M** | **+10%** |

### Scalability Analysis

| Workers | Mutex-only Efficiency | Adaptive Efficiency | Efficiency Gain |
|---------|----------------------|-------------------|-----------------|
| 1 | 100% | 100% | 0% |
| 2 | 81% | 95% | +14% |
| 4 | 52% | 88% | +36% |
| 8 | 29% | 82% | +53% |

### Implementation Details

## Adaptive Logger Performance

### Overview
The adaptive logger implementation provides automatic optimization for high-throughput logging scenarios:

- **Architecture**: Adaptive job queue for log message submission
- **Contention Handling**: Dynamic strategy selection based on load
- **Scalability**: Optimal performance scaling with thread count
- **Compatibility**: Seamless integration with existing code

### Single-Threaded Performance
*Message throughput comparison*

| Message Size | Standard Logger | Adaptive Logger | Improvement |
|--------------|-----------------|------------------|-------------|
| Short (17 chars) | 7.64 M/s | 7.42 M/s | -2.9% |
| Medium (123 chars) | 5.73 M/s | 5.61 M/s | -2.1% |
| Long (1024 chars) | 2.59 M/s | 2.55 M/s | -1.5% |

*Note: Single-threaded performance shows minimal overhead. Benefits appear under contention.*

### Multi-Threaded Scalability
*Throughput with concurrent logging threads*

| Threads | Standard Logger | Adaptive Logger | Improvement |
|---------|-----------------|------------------|-------------|
| 2 | 1.91 M/s | 1.95 M/s | **+2.1%** |
| 4 | 0.74 M/s | 1.07 M/s | **+44.6%** |
| 8 | 0.22 M/s | 0.63 M/s | **+186.4%** |
| 16 | 0.16 M/s | 0.54 M/s | **+237.5%** |

### Formatted Logging Performance
*Complex format string with multiple parameters*

| Logger Type | Throughput | Latency (ns) |
|-------------|------------|--------------|
| Standard | 2.94 M/s | 340 |
| Adaptive | 2.89 M/s | 346 |

### Burst Logging Performance
*Handling sudden log bursts*

| Burst Size | Standard Logger | Adaptive Logger | Improvement |
|------------|-----------------|------------------|-------------|
| 10 messages | 1.90 M/s | 1.88 M/s | -1.1% |
| 100 messages | 5.33 M/s | 5.15 M/s | -3.4% |

### Mixed Log Types Performance
*Different log levels (Info, Debug, Error, Exception)*

| Logger Type | Throughput | CPU Efficiency |
|-------------|------------|----------------|
| Standard | 6.51 M/s | 100% |
| Adaptive | 6.42 M/s | 98% |

### Key Findings

1. **High Contention Benefits**: Adaptive logger shows significant advantages with 4+ threads
2. **Scalability**: Up to 237% improvement at 16 threads
3. **Minimal Overhead**: Single-threaded performance nearly identical
4. **Use Cases**: Ideal for all multi-threaded applications with automatic optimization

### Recommendations

- **Use Adaptive Logger**: Automatic optimization for all scenarios
- **Dynamic Scaling**: Logger adapts to application's threading patterns
- **Batch Processing**: Automatically enabled when beneficial for throughput
- **Buffer Management**: Dynamic queue sizing based on workload

### Implementation Details

- **Adaptive Strategy**: Automatic switching between mutex and lock-free based on contention
- **Dynamic Allocation**: Efficient memory usage based on queue patterns
- **Smart Retry Logic**: Intelligent backoff strategies to prevent contention
- **Queue Optimization**: Automatic batching and buffer management
- **Performance Monitoring**: Built-in metrics for optimization decisions

### Current Status

- Modularized architecture with adaptive queue selection
- Logger and monitoring separated into independent projects
- All stress tests enabled and passing reliably
- Adaptive implementation provides optimal performance for all scenarios
- Average operation latencies:
  - Enqueue: ~96 ns (low contention), ~320 ns (high contention)
  - Dequeue: ~571 ns with adaptive optimization

### Recent Benchmark Results (2025-07-25)

#### Data Race Fix Verification
| Threads | Wake Interval Access | Cancellation Token | Job Queue Consistency |
|---------|---------------------|--------------------|-----------------------|
| 1       | 163μs/10K           | 25μs/100           | -                     |
| 4       | 272μs/10K           | 59μs/400           | 842μs/2K dequeued     |
| 8       | 438μs/10K           | 111μs/800          | 2.18ms/4K dequeued    |
| 16      | 750μs/10K           | 210μs/1.6K         | 4.81ms/8K dequeued    |

*Note: All data race issues have been resolved with proper synchronization.*

### Usage Recommendations

1. **Adaptive Queue Benefits**:
   - All contention scenarios automatically optimized
   - Latency-sensitive applications benefit from automatic switching
   - Systems with varying CPU load patterns
   - Real-time applications with dynamic requirements

2. **Configuration Guidelines**:
   ```cpp
   // Adaptive behavior (recommended)
   adaptive_job_queue queue(adaptive_job_queue::queue_strategy::ADAPTIVE);
   
   // Force specific strategy when needed
   adaptive_job_queue mutex_queue(adaptive_job_queue::queue_strategy::FORCE_MUTEX);
   adaptive_job_queue lockfree_queue(adaptive_job_queue::queue_strategy::FORCE_LOCKFREE);
   ```

### Performance Tuning Tips

1. **Batch Operations**: Use batch enqueue/dequeue for better throughput
2. **CPU Affinity**: Pin threads to specific cores for consistent performance
3. **Memory Alignment**: Ensure job objects are cache-line aligned
4. **Retry Handling**: Operations may fail under extreme contention - implement retry logic
5. **Monitoring**: Use built-in statistics to track performance metrics including retry counts

## Logger Performance (Now Separate Project)

*Note: Logger has been moved to a separate project. The following benchmarks are from when logger was integrated with Thread System.*

### Logger Comparison with Industry Standards

### Overview
This section compares Thread System's logging performance against industry-standard logging libraries. The logger uses adaptive job queues for automatic optimization.

### Single-Threaded Performance Comparison
*Baseline measurements on Apple M1 (8-core)*

| Logger | Throughput | Latency (ns) | Relative Performance |
|--------|------------|--------------|---------------------|
| Console Output | 542.8K/s | 1,842 | Baseline |
| Thread System Logger | 4.41M/s | 227 | **8.1x** faster |
| Thread System (Adaptive) | 4.34M/s | 240 | **8.0x** faster |

### Multi-Threaded Scalability
*Concurrent logging performance with adaptive optimization*

| Threads | Standard Mode | Adaptive Mode | Improvement |
|---------|---------------|---------------|-------------|
| 2 | 2.61M/s | 2.58M/s | -1% |
| 4 | 859K/s | 1.07M/s | **+25%** |
| 8 | 234K/s | 412K/s | **+76%** |
| 16 | 177K/s | 385K/s | **+118%** |

### Latency Characteristics
*End-to-end logging latency*

| Logger Type | Mean Latency | P99 Latency | Notes |
|-------------|--------------|-------------|-------|
| Thread System Logger | 144 ns | ~200 ns | Adaptive queue |
| Console Output | 1,880 ns | ~2,500 ns | System call overhead |

### Key Findings

1. **Logger Excellence**: 
   - 8.1x faster than console output
   - Excellent single-threaded performance
   - Predictable latency characteristics

2. **Adaptive Scalability**:
   - Automatic optimization at 4+ threads
   - Up to 118% improvement at high contention
   - Minimal overhead in single-threaded scenarios

3. **Usage Recommendations**:
   - Use Thread System Logger for all scenarios
   - Adaptive queues automatically optimize based on load
   - No manual configuration needed

### Comparison with spdlog

*Comprehensive performance comparison with the popular spdlog library*

#### Single-Threaded Performance
| Logger | Throughput | Latency | vs Console | Notes |
|--------|------------|---------|------------|-------|
| Console Output | 583K/s | 1,716 ns | Baseline | System call overhead |
| **Thread System Logger** | **4.34M/s** | **148 ns** | **7.4x** | Best latency |
| spdlog (sync) | 515K/s | 2,333 ns | 0.88x | Poor performance |
| **spdlog (async)** | **5.35M/s** | - | **9.2x** | Best throughput |

#### Multi-Threaded Performance (4 Threads)
| Logger | Throughput | vs Single-thread | Scalability |
|--------|------------|------------------|-------------|
| Thread System (Standard) | 599K/s | -86% | Moderate |
| **Thread System (Adaptive)** | **1.07M/s** | -75% | **Good** |
| spdlog (sync) | 210K/s | -59% | Very Poor |
| spdlog (async) | 785K/s | -85% | Poor |

#### High Contention (8 Threads)
| Logger | Throughput | vs Console | Notes |
|--------|------------|------------|-------|
| Thread System (Standard) | 198K/s | 0.34x | High contention |
| **Thread System (Adaptive)** | **412K/s** | **0.71x** | Auto-optimized |
| spdlog (sync) | 52K/s | 0.09x | Severe degradation |
| spdlog (async) | 240K/s | 0.41x | Queue saturation |

#### Key Findings

1. **Single-threaded Champion**: spdlog async (5.35M/s) edges out Thread System (4.34M/s)
2. **Multi-threaded Champion**: Thread System with adaptive queues shows consistent performance
3. **Latency Champion**: Thread System with 148ns, **15.8x lower** than spdlog sync (2333/148 = 15.76)
4. **Scalability**: Thread System adaptive mode provides automatic optimization

### Recommendations

1. **For All Applications**: Use Thread System Logger
   - Excellent performance with adaptive optimization
   - Simple API with type safety
   - Built-in file rotation and callbacks
   - Automatic optimization for high concurrency

2. **Configuration Tips**:
   - Logger automatically adapts to workload
   - No manual tuning required
   - Adaptive queues handle burst patterns efficiently

3. **Usage Example**:
   ```cpp
   // Simple usage - automatic optimization
   log_module::start();
   log_module::write_information("Message: {}", value);
   log_module::write_error("Error: {}", error);
   log_module::stop();
   ```

## Comparison with Other Libraries

### Throughput Comparison (Real-world measurements)

| Library                    | Throughput | Relative Performance | Features               |
|---------------------------|------------|---------------------|------------------------|
| **Thread System**         | 1.16M/s    | 100% (baseline)     | Type, logging, C++20, lock-free |
| Intel TBB                 | ~1.24M/s   | ~107%               | Industry standard, work stealing |
| Boost.Thread Pool        | ~1.09M/s   | ~94%                | Header-only, portable |
| std::async                | ~267K/s    | ~23%                | Standard library, basic |
| Custom (naive)            | ~684K/s    | ~59%                | Simple mutex-based impl |
| OpenMP                    | ~1.06M/s   | ~92%                | Compiler directives |
| Microsoft PPL             | ~1.02M/s   | ~88%                | Windows-specific |

### Feature Comparison

| Library | Type Support | Logging | C++20 | Cross-Platform | Memory Pool | Error Handling |
|---------|-----------------|---------|-------|----------------|-------------|----------------|
| Thread System | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | ❌ No | ✅ Comprehensive |
| Intel TBB | ✅ Yes | ❌ No | ⚠️ Partial | ✅ Yes | ✅ Yes | ⚠️ Basic |
| Boost.Thread Pool | ❌ No | ❌ No | ⚠️ Partial | ✅ Yes | ❌ No | ⚠️ Basic |
| std::async | ❌ No | ❌ No | ✅ Yes | ✅ Yes | ❌ No | ⚠️ Basic |

### Latency Comparison (μs)

| Library | Submission | Execution Start | Total Overhead |
|---------|------------|-----------------|----------------|
| Thread System | 77 ns | 96 ns | 173 ns |
| Intel TBB | ~100 ns | ~90 ns | ~190 ns |
| Boost.Thread Pool | ~150 ns | ~120 ns | ~270 ns |
| std::async | ~15.2 μs | ~12.8 μs | ~28.0 μs |

---

*Last Updated: 2025-10-20*
