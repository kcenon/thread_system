---
doc_id: "THR-ARCH-003a"
doc_title: "Thread System - Architecture Overview"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "thread_system"
category: "ARCH"
---

# Thread System - Architecture Overview

> **SSOT**: This document is the single source of truth for **Thread System - Architecture Overview** (high-level architecture diagrams).

> **See also**: [Architecture Details](ARCHITECTURE_DETAILS.md) for component deep-dives (queues, hazard pointers, cancellation, error handling).

## 1. System Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         EXTERNAL SYSTEMS & APPLICATIONS                     │
│                                                                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ Network      │  │ Data         │  │ Game         │  │ Scientific   │  │
│  │ System       │  │ Processing   │  │ Engine       │  │ Computing    │  │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  │
└─────────┼──────────────────┼──────────────────┼──────────────────┼──────────┘
          │                  │                  │                  │
          │ (via interfaces) │ (via interfaces) │ (via interfaces) │
          └──────────────────┼──────────────────┼──────────────────┘
                             │
        ┌────────────────────▼────────────────────┐
        │   THREAD SYSTEM (Core Foundation)      │
        │         ~2,700 lines of code            │
        │                                         │
        │  No external dependencies (standalone) │
        └────┬────────────────────┬──────────────┘
             │                    │
    ┌────────▼────────┐  ┌───────▼──────────┐
    │ CORE MODULE     │  │ IMPLEMENTATIONS  │
    │                 │  │                  │
    │ • thread_base   │  │ • thread_pool    │
    │ • thread_worker │  │ • typed_pool     │
    │ • job           │  │ • lockfree_queue │
    │ • job_queue     │  │ • adaptive_queue │
    │ • sync primitives   │                  │
    │ • cancellation  │  │                  │
    │ • error handling│  │                  │
    │ • service reg   │  │                  │
    └────────┬────────┘  └─────────┬────────┘
             │                     │
    ┌────────▼─────────────────────▼────────┐
    │      PUBLIC INTERFACES                │
    │                                        │
    │ • executor_interface                  │
    │ • scheduler_interface                 │
    │ • logger_interface (deprecated)       │
    │ • monitoring_interface (deprecated)   │
    └────────┬──────────────────────────────┘
             │
     ┌───────┴──────────┬──────────────┐
     │                  │              │
 ┌───▼────┐      ┌─────▼────┐   ┌────▼──────┐
 │Logger  │      │Monitoring│   │Integrated │
 │System  │      │System     │   │Examples   │
 │(opt)   │      │(opt)      │   │           │
 └────────┘      └───────────┘   └───────────┘
```

## 2. Core Module Component Hierarchy

```
┌──────────────────────────────────────────────────────────────────┐
│                      CORE MODULE                                 │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ BASE THREADING                                             │ │
│  ├────────────────────────────────────────────────────────────┤ │
│  │                                                             │ │
│  │ thread_base ──────┐                                         │ │
│  │   [start/stop]    │                                         │ │
│  │   [do_work]       │                                         │ │
│  │   [lifecycle]     │                                         │ │
│  │                   │                                         │ │
│  │                   └──▶ thread_worker                        │ │
│  │                          [process jobs]                     │ │
│  │                          [idle tracking]                    │ │
│  │                          [worker ID]                        │ │
│  │                                                             │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ JOB & QUEUE SYSTEM                                         │ │
│  ├────────────────────────────────────────────────────────────┤ │
│  │                                                             │ │
│  │ job ◀─────────────┐                                        │ │
│  │  [abstract]       │                                        │ │
│  │  [do_work]        │                                        │ │
│  │                   ├──▶ callback_job ◀─── [wrap callables]  │ │
│  │                   │                                        │ │
│  │                   └──▶ typed_job ◀────── [type metadata]   │ │
│  │                                                             │ │
│  │ job_queue                                                  │ │
│  │  [FIFO, mutex-based]                                       │ │
│  │  [enqueue/dequeue]                                         │ │
│  │  [condition_variable]                                      │ │
│  │                                                             │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ SYNCHRONIZATION & CONTROL                                  │ │
│  ├────────────────────────────────────────────────────────────┤ │
│  │                                                             │ │
│  │ sync_primitives ──┬──▶ scoped_lock_guard                   │ │
│  │  [RAII wrappers]  │     [timeout support]                  │ │
│  │                   │                                        │ │
│  │                   ├──▶ condition_variable_wrapper          │ │
│  │                   │     [predicates, notify]                │ │
│  │                   │                                        │ │
│  │                   ├──▶ atomic_flag_wrapper                 │ │
│  │                   │     [wait/notify ops]                  │ │
│  │                   │                                        │ │
│  │                   └──▶ shared_mutex_wrapper                │ │
│  │                        [reader-writer locks]               │ │
│  │                                                             │ │
│  │ cancellation_token                                         │ │
│  │  [cooperative cancellation]                                │ │
│  │  [linked tokens]                                           │ │
│  │  [callbacks]                                               │ │
│  │                                                             │ │
│  │ hazard_pointer                                             │ │
│  │  [lock-free memory safety]                                 │ │
│  │  [per-thread lists]                                        │ │
│  │  [global registry]                                         │ │
│  │                                                             │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ ERROR HANDLING                                             │ │
│  ├────────────────────────────────────────────────────────────┤ │
│  │                                                             │ │
│  │ result<T> ◀────────── [C++23 expected pattern]             │ │
│  │  [is_success]                                              │ │
│  │  [map, and_then]                                           │ │
│  │  [unwrap]                                                  │ │
│  │                                                             │ │
│  │ error_code ◀────────── [100+ typed codes]                  │ │
│  │  [thread, queue, job, resource errors]                     │ │
│  │                                                             │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ DEPENDENCY INJECTION                                       │ │
│  ├────────────────────────────────────────────────────────────┤ │
│  │                                                             │ │
│  │ service_registry                                           │ │
│  │  [static DI container]                                     │ │
│  │  [thread-safe (shared_mutex)]                              │ │
│  │  [type-safe via std::any]                                  │ │
│  │                                                             │ │
│  │ thread_context                                             │ │
│  │  [logger_interface]                                        │ │
│  │  [monitoring_interface]                                    │ │
│  │                                                             │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

## 3. Threading & Job Execution Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                    APPLICATION THREAD                           │
│                                                                 │
│  // Create pool                                                 │
│  auto pool = std::make_shared<thread_pool>("pool");             │
│                                                                 │
│  // Enqueue job (non-blocking)                                  │
│  pool->execute(std::make_unique<my_job>());                     │
│                                                                 │
│  // Job moves to queue                                          │
│  └──────────────┬──────────────────────────────────────────┐   │
└─────────────────┼──────────────────────────────────────────┼───┘
                  │                                          │
    ┌─────────────▼──────────────────────┐                  │
    │                                    │                  │
    │         SHARED JOB QUEUE            │                  │
    │                                    │                  │
    │  ┌──────────┐  ┌──────────┐       │                  │
    │  │ Job A    │  │ Job B    │  ◀────┘  (enqueue)       │
    │  └──────────┘  └──────────┘       │                  │
    │                                    │                  │
    └─────────────┬──────────────────────┘                  │
                  │ (dequeue)                               │
      ┌───────────┴──────────────┬─────────────┐           │
      │                          │             │           │
      ▼                          ▼             ▼           │
  ┌─────────┐             ┌─────────┐    ┌─────────┐      │
  │ Worker1 │             │ Worker2 │    │ WorkerN │      │
  │ (thread)│             │ (thread)│    │ (thread)│      │
  │         │             │         │    │         │      │
  │ while() │             │ while() │    │ while() │      │
  │ loop:   │             │ loop:   │    │ loop:   │      │
  │         │             │         │    │         │      │
  │ if job:◄┼─────────────┤─────────┼────┼─────────┼──┐   │
  │   run   │             │         │    │         │  │   │
  │   it    │             │         │    │         │  │   │
  │         │             │         │    │         │  │   │
  │ else:   │             │         │    │         │  │   │
  │  wait   │             │         │    │         │  │   │
  │  on CV  │             │         │    │         │  │   │
  └─────────┘             └─────────┘    └─────────┘  │   │
      │                       │             │         │   │
      │ (run)         (run)   │    (run)    │         │   │
      └─────┬──────────────────┼─────────────┘         │   │
            │                  │                       │   │
            ▼                  ▼                       │   │
         [Job A]            [Job B]           [dequeue] │   │
         result             result           cycles...└───┘
```

---

*Last Updated: 2025-12-04*
*Visual diagrams for Thread System architecture and key mechanisms*
