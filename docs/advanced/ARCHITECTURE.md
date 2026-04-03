---
doc_id: "THR-ARCH-005"
doc_title: "Threading Ecosystem Architecture"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "thread_system"
category: "ARCH"
---

# Threading Ecosystem Architecture

> **SSOT**: This document is the single source of truth for **Threading Ecosystem Architecture**.

> **Language:** **English** | [한국어](ARCHITECTURE.kr.md)

## Table of Contents

- [🏗️ Ecosystem Overview](#-ecosystem-overview)
- [📋 Project Roles & Responsibilities](#-project-roles-responsibilities)
  - [1. thread_system (Foundation)](#1-thread_system-foundation)
  - [2. logger_system (Logging)](#2-logger_system-logging)
  - [3. monitoring_system (Metrics)](#3-monitoring_system-metrics)
  - [4. integrated_thread_system (Integration Hub)](#4-integrated_thread_system-integration-hub)
- [🔄 Dependency Flow & Interface Contracts](#-dependency-flow-interface-contracts)
- [📁 Directory Structure (Overview)](#-directory-structure-overview)
- [🚀 Recent Architectural Highlights](#-recent-architectural-highlights)
  - [Enhanced Synchronization Primitives 🆕](#enhanced-synchronization-primitives-)
  - [Improved Cancellation Support 🆕](#improved-cancellation-support-)
  - [Service Registry Pattern 🆕](#service-registry-pattern-)
  - [Adaptive Job Queue](#adaptive-job-queue)
  - [Interface-Driven Integration](#interface-driven-integration)
  - [Error Handling Excellence](#error-handling-excellence)
  - [Typed Thread Pool](#typed-thread-pool)

A comprehensive overview of the modular threading ecosystem and inter-project relationships.

## 🏗️ Ecosystem Overview

The threading ecosystem consists of four interconnected projects designed to provide a complete, high-performance concurrent programming solution:

```
                    ┌─────────────────────────────┐
                    │   Application Layer         │
                    │                             │
                    │   Your Production Apps      │
                    └─────────────┬───────────────┘
                                  │
                    ┌─────────────▼───────────────┐
                    │ integrated_thread_system    │
                    │ (Integration Hub)           │
                    │                             │
                    │ • Complete Examples         │
                    │ • Integration Tests         │
                    │ • Best Practices           │
                    │ • Migration Guides         │
                    └─────────────┬───────────────┘
                                  │ uses all
        ┌─────────────────────────┼─────────────────────────┐
        │                         │                         │
        ▼                         ▼                         ▼
┌───────────────┐     ┌───────────────┐     ┌─────────────────┐
│ thread_system │────▶│ logger_system │     │monitoring_system│
│   (Core)      │     │ (Logging)     │     │  (Metrics)      │
│               │     │               │     │                 │
│ Foundation    │     │ Implements    │     │ Implements      │
│ interfaces    │     │ logger_       │     │ monitoring_     │
│ and core      │     │ interface     │     │ interface       │
│ threading     │     │               │     │                 │
└───────────────┘     └───────────────┘     └─────────────────┘
```

## 📋 Project Roles & Responsibilities

### 1. thread_system (Foundation)
**Repository**: https://github.com/kcenon/thread_system  
**Role**: Core threading framework and interface provider  
**Code Size**: ~2,700 lines (streamlined from 8,700+ through coroutine removal)

Responsibilities:
- Interface Definitions: `logger_interface`, `monitoring_interface`, `executor_interface`
- Core Threading: worker pools, job queues, thread management
- Synchronization Primitives: Enhanced wrappers and utilities
- Service Infrastructure: Dependency injection and service registry
- Cross-Platform Support: Windows, Linux, macOS

Key Components:
```cpp
namespace thread_module {
    // Interfaces
    class logger_interface;           // Implemented by logger_system
    class monitoring_interface;       // Implemented by monitoring_system
    class executor_interface;         // Job execution contract
    
    // Core Threading
    class thread_pool;                // Main thread pool implementation
    class thread_worker;              // Worker thread management
    class job_queue;                  // Thread-safe job distribution
    class callback_job;               // Job wrapper for callbacks
    
    // Synchronization (NEW)
    class cancellation_token;         // Cooperative cancellation
    class scoped_lock_guard;          // RAII lock with timeout
    class condition_variable_wrapper; // Enhanced condition variable
    class service_registry;           // Dependency injection container
    
    // Adaptive Components
    class adaptive_job_queue;         // Dual-mode queue optimization
    class hazard_pointer_manager;     // Lock-free memory reclamation
}
```

Dependencies:
- External: None (standalone)
- Internal: Self-contained

---

### 2. logger_system (Logging)
**Repository**: https://github.com/kcenon/logger_system  
**Role**: High-performance asynchronous logging implementation

Responsibilities:
- Implements `thread_module::logger_interface`
- Asynchronous logging with high throughput
- Multiple writers (console/file/custom)
- Thread-safe

---

### 3. monitoring_system (Metrics)
**Repository**: https://github.com/kcenon/monitoring_system  
**Role**: Real-time performance monitoring and metrics collection

Responsibilities:
- Implements `monitoring_interface::monitoring_interface`
- System, thread pool, and worker metrics
- Low-overhead collection and ring buffers

---

### 4. integrated_thread_system (Integration Hub)
**Repository**: https://github.com/kcenon/integrated_thread_system  
**Role**: Complete integration examples and testing framework

Responsibilities:
- Integration examples and best practices
- Cross-system integration tests
- Migration guides

---

## 🔄 Dependency Flow & Interface Contracts

Interface Hierarchy:
```
thread_module::logger_interface
    ↑ implements
logger_module::logger

monitoring_interface::monitoring_interface
    ↑ implements
monitoring_module::monitoring
```

Dependency Graph:
```
┌─────────────────┐
│  thread_system  │ ← No external dependencies (foundation)
└─────────┬───────┘
          │ provides interfaces
          ├─────────────────────┬─────────────────────┐
          ▼                     ▼                     ▼
┌─────────────────┐   ┌─────────────────┐   ┌─────────────────┐
│  logger_system  │   │monitoring_system│   │integrated_thread│
│                 │   │                 │   │    _system      │
│ depends on:     │   │ depends on:     │   │                 │
│ - thread_system │   │ - thread_system │   │ depends on:     │
│   (interfaces)  │   │   (interfaces)  │   │ - thread_system │
└─────────────────┘   └─────────────────┘   │ - logger_system │
                                            │ - monitoring_   │
                                            │   system        │
                                            └─────────────────┘
```

## 📁 Directory Structure (Overview)

Project layout after modularization (~2,700 lines):

```
thread_system/
├── core/                          # Core threading foundation
│   ├── base/                      # Thread base, service registry
│   │   ├── include/
│   │   │   ├── thread_base.h
│   │   │   ├── service_registry.h  # 🆕 DI container
│   │   │   └── thread_conditions.h
│   │   └── src/
│   ├── jobs/                      # Job system
│   │   ├── include/
│   │   │   ├── job.h               # With cancellation
│   │   │   ├── callback_job.h
│   │   │   └── job_queue.h
│   │   └── src/
│   └── sync/                      # Synchronization
│       ├── include/
│       │   ├── sync_primitives.h   # 🆕 Enhanced wrappers
│       │   ├── cancellation_token.h # 🆕 Cooperative cancellation
│       │   └── error_handling.h    # Result<T> pattern
│       └── src/
├── interfaces/                    # Public contracts
│   ├── executor_interface.h
│   ├── scheduler_interface.h
│   ├── logger_interface.h
│   └── monitoring_interface.h
├── implementations/
│   ├── thread_pool/{include,src}
│   ├── typed_thread_pool/{include,src}
│   └── lockfree/{include,src}
├── utilities/{include,src}
├── tests/benchmarks/
├── samples/
├── docs/
└── cmake/
```

Design rules:
- core exposes public headers under `include/` and implementations under `src/`
- implementations depend on core and interfaces
- utilities is standalone; interfaces depend only on core/base

---

## 🚀 Recent Architectural Highlights

### Enhanced Synchronization Primitives 🆕
- **`sync_primitives.h`**: Comprehensive synchronization wrappers
  - `scoped_lock_guard`: RAII with timeout support
  - `condition_variable_wrapper`: Predicates and timeouts
  - `atomic_flag_wrapper`: Wait/notify operations
  - `shared_mutex_wrapper`: Reader-writer locks

### Improved Cancellation Support 🆕
- **`cancellation_token`**: Cooperative cancellation mechanism
  - Linked token creation for hierarchical cancellation
  - Thread-safe callback registration
  - Automatic signal propagation
  - Weak pointer usage to prevent cycles

### Service Registry Pattern 🆕
- **`service_registry`**: Lightweight dependency injection
  - Type-safe service registration/retrieval
  - Thread-safe with shared_mutex
  - Automatic lifetime management
  - Header-only implementation

### Adaptive Job Queue
- Runtime switching between mutex-based and lock-free MPMC strategies
- Uses lightweight metrics (latency, contention ratio, operation count)
- Automatic optimization based on workload characteristics
- Up to 7.7x performance improvement under high contention

### Interface-Driven Integration
- `executor_interface` implemented by thread pools (`execute`, `shutdown`)
- `scheduler_interface` implemented by job queues (enqueue/dequeue)
- `monitoring_interface` provides pool/worker/system metrics
- `logger_interface` keeps logging pluggable and optional

Ecosystem Integration Note
- network_system integrates with external thread pools via its `thread_integration_manager` and adapters; there is no hard compile-time dependency on thread_system.

### Error Handling Excellence
- **`result<T>` pattern**: Modern error handling similar to C++23 std::expected
  - Type-safe error codes
  - Monadic operations (map, and_then)
  - Zero-overhead abstractions
  - Clear error propagation

### Typed Thread Pool
- Per-type queues with lock-free/adaptive variants
- Priority/type-aware scheduling for heterogeneous workloads
- Maintains 99%+ type accuracy under all conditions

---

*Last Updated: 2025-10-20*
