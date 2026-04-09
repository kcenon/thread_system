---
doc_id: "THR-PERF-006"
doc_title: "Thread System Performance Guide"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "thread_system"
category: "PERF"
---

# Thread System Performance Guide

> **SSOT**: This document is the single source of truth for **Thread System Performance Guide**.

> **Language:** **English** | [한국어](PERFORMANCE.kr.md)

This guide has been split into two focused documents for easier navigation:

| Document | Description | Topics |
|----------|-------------|--------|
| **[Performance Benchmarks](PERFORMANCE_BENCHMARKS.md)** | Benchmark results and data | Performance overview, benchmark environment, core metrics, data race fix impact, detailed benchmarks, typed pool benchmarks, adaptive queue benchmarks, scalability analysis, memory performance, MPMC queue, logger performance, library comparisons |
| **[Performance Tuning](PERFORMANCE_TUNING.md)** | Tuning guidance and best practices | Optimization strategies (thread count, batching, granularity, type pool, adaptive queue, memory), platform-specific optimizations (macOS, Linux, Windows), best practices, performance recommendations, conclusion |

## Quick Links

### Benchmarks
- [Performance Overview](PERFORMANCE_BENCHMARKS.md#performance-overview)
- [Benchmark Environment](PERFORMANCE_BENCHMARKS.md#benchmark-environment)
- [Core Performance Metrics](PERFORMANCE_BENCHMARKS.md#core-performance-metrics)
- [Data Race Fix Impact](PERFORMANCE_BENCHMARKS.md#data-race-fix-impact)
- [Detailed Benchmark Results](PERFORMANCE_BENCHMARKS.md#detailed-benchmark-results)
- [Adaptive Job Queue Benchmarks](PERFORMANCE_BENCHMARKS.md#adaptive-job-queue-benchmarks)
- [Scalability Analysis](PERFORMANCE_BENCHMARKS.md#scalability-analysis)
- [Memory Performance](PERFORMANCE_BENCHMARKS.md#memory-performance)
- [Comparison with Other Libraries](PERFORMANCE_BENCHMARKS.md#comparison-with-other-libraries)

### Tuning
- [Optimization Strategies](PERFORMANCE_TUNING.md#optimization-strategies)
- [Platform-Specific Optimizations](PERFORMANCE_TUNING.md#platform-specific-optimizations)
- [Best Practices](PERFORMANCE_TUNING.md#best-practices)
- [Performance Recommendations Summary](PERFORMANCE_TUNING.md#performance-recommendations-summary-2025)
- [Conclusion](PERFORMANCE_TUNING.md#conclusion)
