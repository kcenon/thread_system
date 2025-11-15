# Thread System - Architecture Diagrams & Visual Reference

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

## 4. Queue Implementation Strategy Comparison

```
┌──────────────────────────────────────────────────────────────────┐
│                    QUEUE IMPLEMENTATIONS                         │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ job_queue (Mutex-Based FIFO)                               │ │
│  ├────────────────────────────────────────────────────────────┤ │
│  │  ENQUEUE                    DEQUEUE                        │ │
│  │  ┌─────────────────┐        ┌──────────────────┐          │ │
│  │  │ lock_guard      │        │ lock_guard       │          │ │
│  │  │ queue.push(job) │        │ if queue.empty   │          │ │
│  │  │ notify_one()    │        │   wait(cond_var) │          │ │
│  │  │ unlock          │        │ job = queue.pop()│          │ │
│  │  └─────────────────┘        │ unlock           │          │ │
│  │                             │ return job       │          │ │
│  │  ✅ Simple, reliable        └──────────────────┘          │ │
│  │  ✅ Baseline performance                                  │ │
│  │  ⚠️  Contention under high load                           │ │
│  │                                                            │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ lockfree_job_queue (Michael-Scott + Hazard Pointers)      │ │
│  ├────────────────────────────────────────────────────────────┤ │
│  │  ALGORITHM: Michael-Scott Queue (1996)                    │ │
│  │  MEMORY: Hazard Pointers (Michael, 2004)                  │ │
│  │                                                            │ │
│  │  ENQUEUE (wait-free)      DEQUEUE (lock-free)            │ │
│  │  ┌──────────────────┐     ┌──────────────────┐           │ │
│  │  │ create node      │     │ get hazard ptr   │           │ │
│  │  │ CAS tail.next    │     │ read head        │           │ │
│  │  │ CAS tail pointer │     │ CAS head pointer │           │ │
│  │  │ no locks!        │     │ retire old node  │           │ │
│  │  └──────────────────┘     │ return job       │           │ │
│  │                           └──────────────────┘           │ │
│  │  MEMORY RECLAMATION:                                     │ │
│  │  • Per-thread hazard list (MAX 4)                        │ │
│  │  • Global hazard registry                                │ │
│  │  • Scan all threads before deletion                      │ │
│  │                                                            │ │
│  │  ✅ 4x faster (71 μs vs 291 μs per op)                    │ │
│  │  ✅ Production-safe (no TLS bugs)                         │ │
│  │  ✅ Zero contention overhead                             │ │
│  │  ⚠️  ~256 bytes per thread                                │ │
│  │                                                            │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ adaptive_job_queue (Auto-Switching Strategy)               │ │
│  ├────────────────────────────────────────────────────────────┤ │
│  │                                                            │ │
│  │  MONITOR:                 SELECT STRATEGY:               │ │
│  │  • Contention ratio  ──┐                                  │ │
│  │  • Operation latency  ├──▶ Low contention:                │ │
│  │  • Operation count    │    Use Mutex (simpler)            │ │
│  │                       │                                   │ │
│  │                       └──▶ High contention:               │ │
│  │                          Use Lock-Free (faster)           │ │
│  │                                                            │ │
│  │  ✅ Up to 7.7x improvement under contention               │ │
│  │  ✅ Automatic optimization                                │ │
│  │  ✅ Zero configuration                                    │ │
│  │  ✅ Best of both worlds                                   │ │
│  │                                                            │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ bounded_job_queue (Ring Buffer with Size Limit)           │ │
│  ├────────────────────────────────────────────────────────────┤ │
│  │  • Fixed-size ring buffer                                 │ │
│  │  • Memory-bounded (predictable)                           │ │
│  │  • Wraparound on full                                     │ │
│  │                                                            │ │
│  │  ✅ Memory-predictable                                    │ │
│  │  ⚠️  Bounded capacity (enqueue may fail)                  │ │
│  │                                                            │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

## 5. Type-Based Thread Pool Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│            TYPED THREAD POOL (Type-Routed Scheduling)           │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  APPLICATION LAYER                                              │
│  ┌──────────────────────┐  ┌──────────────────────┐             │
│  │ enqueue(RealTime job)│  │ enqueue(Batch job)   │             │
│  └──────────┬───────────┘  └──────────┬───────────┘             │
│             │                         │                        │
│    ┌────────▼─────────────────────────▼────────┐               │
│    │   TYPE-BASED JOB DISPATCH                 │               │
│    │   (Enqueue-time decision)                 │               │
│    └────┬─────────────┬───────────┬─────────┘               │
│         │             │           │                        │
│    ┌────▼────┐   ┌────▼────┐  ┌──▼────┐                   │
│    │RealTime │   │  Normal │  │Background               │
│    │Queue    │   │  Queue  │  │Queue   │                   │
│    │(FIFO)   │   │ (FIFO)  │  │(FIFO)  │                   │
│    └────┬────┘   └────┬────┘  └──┬────┘                   │
│         │             │          │                        │
│    ┌────▼─────────────▼──────────▼────┐                  │
│    │     WORKER ASSIGNMENT             │                  │
│    │  (Type-Aware Pool Partitioning)   │                  │
│    └────┬─────────────┬────────────────┘                  │
│         │             │         │                         │
│    ┌────▼────┐   ┌────▼────┐  ┌──▼────────┐              │
│    │Workers  │   │Workers  │  │Workers    │              │
│    │Set A    │   │Set B    │  │Set C      │              │
│    │(R.Time) │   │(Normal) │  │(Background               │
│    │         │   │         │  │)          │              │
│    │ ┌─────┐ │   │ ┌─────┐ │  │ ┌──────┐  │              │
│    │ │W₁   │ │   │ │W₄   │ │  │ │W₇    │  │              │
│    │ └─────┘ │   │ └─────┘ │  │ └──────┘  │              │
│    │ ┌─────┐ │   │ ┌─────┐ │  │ ┌──────┐  │              │
│    │ │W₂   │ │   │ │W₅   │ │  │ │W₈    │  │              │
│    │ └─────┘ │   │ └─────┘ │  │ └──────┘  │              │
│    │ ┌─────┐ │   │ ┌─────┐ │  │ ┌──────┐  │              │
│    │ │W₃   │ │   │ │W₆   │ │  │ │W₉    │  │              │
│    │ └─────┘ │   │ └─────┘ │  │ └──────┘  │              │
│    │         │   │         │  │           │              │
│    └─────────┘   └─────────┘  └───────────┘              │
│                                                            │
│  BENEFITS:                                               │
│  ✅ Type-accuracy >99% under all conditions              │
│  ✅ Per-type FIFO ordering preserved                     │
│  ✅ Optimal resource allocation                          │
│  ✅ Priority-aware scheduling                            │
│                                                            │
└─────────────────────────────────────────────────────────────┘
```

## 6. Hazard Pointer Memory Reclamation Flow

```
┌──────────────────────────────────────────────────────────────────┐
│        HAZARD POINTER MEMORY RECLAMATION MECHANISM               │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  THREAD 1            THREAD 2            THREAD 3               │
│  ┌───────────┐       ┌───────────┐      ┌───────────┐           │
│  │Hazard List│       │Hazard List│      │Hazard List│           │
│  │           │       │           │      │           │           │
│  │ [ptr→node]│       │ [NULL]    │      │ [NULL]    │           │
│  │ [NULL]    │       │ [NULL]    │      │ [NULL]    │           │
│  │ [NULL]    │       │ [NULL]    │      │ [NULL]    │           │
│  │ [NULL]    │       │ [NULL]    │      │ [NULL]    │           │
│  │           │       │           │      │           │           │
│  │ active=T  │       │ active=T  │      │ active=T  │           │
│  └───────┬───┘       └────┬──────┘      └────┬──────┘           │
│          │                │                  │                  │
│          └────────────────┼──────────────────┘                  │
│                           │                                     │
│                 ┌─────────▼─────────┐                           │
│                 │Global Hazard Ptr  │                           │
│                 │  Registry         │                           │
│                 │                   │                           │
│                 │ Head ──▶ [T1] ──▶ │                           │
│                 │         ↓         │                           │
│                 │        [T2] ──▶   │                           │
│                 │         ↓         │                           │
│                 │        [T3]       │                           │
│                 │         ↓         │                           │
│                 │        NULL       │                           │
│                 └────────┬──────────┘                           │
│                          │                                      │
│  ┌───────────────────────┴────────────────────────┐             │
│  │ DELETION PROCESS                              │             │
│  ├─────────────────────────────────────────────────┤             │
│  │                                                │             │
│  │  1. Mark node for retirement:                 │             │
│  │     retire_list.push(node_ptr)                │             │
│  │                                                │             │
│  │  2. Scan all thread hazard lists:             │             │
│  │     protected = scan_hazards()                │             │
│  │     (Check: is node in any thread's list?)    │             │
│  │                                                │             │
│  │  3. Reclaim if safe:                          │             │
│  │     for (auto node : retire_list) {           │             │
│  │       if (node not in protected) {            │             │
│  │         delete node;  // SAFE TO DELETE       │             │
│  │       } else {                                 │             │
│  │         defer_delete(node);  // Try later     │             │
│  │       }                                        │             │
│  │     }                                          │             │
│  │                                                │             │
│  │  4. Thread cleanup:                           │             │
│  │     When thread exits:                        │             │
│  │     mark_inactive()  // Sets active=false     │             │
│  │     (TLS destructor no longer blocks delete)  │             │
│  │                                                │             │
│  └────────────────────────────────────────────────┘             │
│                                                                  │
│  KEY BENEFITS:                                                 │
│  ✅ True ABA prevention                                        │
│  ✅ No use-after-free                                          │
│  ✅ No memory leaks                                            │
│  ✅ Safe TLS destruction (no ordering dependency)              │
│  ✅ Production-safe                                            │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

## 7. Cancellation Token Hierarchy

```
┌──────────────────────────────────────────────────────────────────┐
│            CANCELLATION TOKEN HIERARCHY & PROPAGATION            │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  SCOPE 1: System-level Shutdown                                │
│  ┌──────────────────────────┐                                   │
│  │  system_shutdown_token   │                                   │
│  │                          │                                   │
│  │  is_cancelled() = false  │                                   │
│  │  [callbacks = {}]        │                                   │
│  └──────────┬───────────────┘                                   │
│             │                                                   │
│             │ (create_linked)                                   │
│             │                                                   │
│      ┌──────┴──────┬─────────────┬─────────────┐              │
│      │             │             │             │              │
│  ┌───▼────┐    ┌───▼────┐   ┌───▼────┐   ┌───▼────┐         │
│  │Token 1 │    │Token 2 │   │Token 3 │   │Token 4 │         │
│  │(DB Op) │    │(Network│   │(Compute)   │(Cache) │         │
│  │        │    │ Op)    │   │       │    │        │         │
│  │callback│    │callback│   │callback  │callback  │         │
│  │cleanup │    │cleanup │   │cleanup │ │cleanup │         │
│  │database│    │conns   │   │threads │ │entries │         │
│  └────────┘    └────────┘   └────────┘ └────────┘         │
│                                                               │
│  PROPAGATION FLOW:                                           │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │ User: system_shutdown_token.cancel()                   │ │
│  │       │                                                  │ │
│  │       └─▶ set atomic flag                               │ │
│  │           │                                              │ │
│  │           └─▶ notify() on all registered callbacks       │ │
│  │               │                                          │ │
│  │               ├─▶ Token 1: cleanup()                    │ │
│  │               │   └─▶ close DB connections              │ │
│  │               │                                          │ │
│  │               ├─▶ Token 2: cleanup()                    │ │
│  │               │   └─▶ close network sockets              │ │
│  │               │                                          │ │
│  │               ├─▶ Token 3: cleanup()                    │ │
│  │               │   └─▶ terminate worker threads           │ │
│  │               │                                          │ │
│  │               └─▶ Token 4: cleanup()                    │ │
│  │                   └─▶ flush cache entries                │ │
│  │                                                           │ │
│  │       All linked tokens observe is_cancelled() = true   │ │
│  │                                                           │ │
│  └─────────────────────────────────────────────────────────┘ │
│                                                                  │
│  WEAK POINTER SAFETY:                                          │
│  • Token stores shared_ptr<token_state>                       │
│  • Callbacks store weak_ptr<token_state>                      │
│  ✅ Prevents circular reference cycles                        │
│  ✅ Safe automatic cleanup                                    │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

## 8. Error Handling Flow

```
┌──────────────────────────────────────────────────────────────────┐
│              RESULT<T> ERROR HANDLING PATTERN                    │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  OPERATION WITH ERROR HANDLING:                                │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ result<int> compute_with_error() {                      │   │
│  │   // Try operation                                       │   │
│  │   if (invalid_input) {                                  │   │
│  │     return error{                                       │   │
│  │       error_code::invalid_argument,                     │   │
│  │       "Input value out of range"                        │   │
│  │     };                                                  │   │
│  │   }                                                     │   │
│  │                                                          │   │
│  │   // Success                                            │   │
│  │   return result<int>(42);                              │   │
│  │ }                                                       │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                  │
│  CONSUMING THE RESULT:                                         │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ auto result = compute_with_error();                     │   │
│  │                                                          │   │
│  │ if (result.is_success()) {                              │   │
│  │   int value = result.value();  // Use value            │   │
│  │ } else {                                                │   │
│  │   auto err = result.error_value();                      │   │
│  │   handle_error(err);            // Handle error        │   │
│  │ }                                                       │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                  │
│  MONADIC OPERATIONS (Chaining):                               │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ // map: Transform value if success                      │   │
│  │ auto squared = result                                  │   │
│  │   .map([](int x) { return x * x; })  // 42*42=1764     │   │
│  │   .map([](int x) { return x + 1; }); // 1765           │   │
│  │                                                          │   │
│  │ // and_then: Chain dependent operations                │   │
│  │ auto chained = parse_input()                           │   │
│  │   .and_then([](auto val) {                             │   │
│  │     return validate(val);  // result<T>                │   │
│  │   })                                                    │   │
│  │   .and_then([](auto val) {                             │   │
│  │     return process(val);   // result<T>                │   │
│  │   });                                                  │   │
│  │                                                          │   │
│  │ // unwrap: Extract value or throw on error            │   │
│  │ int value = result.unwrap();  // Throws if error       │   │
│  │                                                          │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                  │
│  ERROR CODE HIERARCHY:                                         │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ enum class error_code {                                 │   │
│  │   success = 0,                                          │   │
│  │   unknown_error,                                        │   │
│  │   operation_canceled,                                   │   │
│  │   operation_timeout,                                    │   │
│  │                                                          │   │
│  │   thread_already_running = 100,    // Thread (100+)     │   │
│  │   thread_not_running,                                   │   │
│  │   thread_start_failure,                                 │   │
│  │   thread_join_failure,                                  │   │
│  │                                                          │   │
│  │   queue_full = 200,                // Queue (200+)      │   │
│  │   queue_empty,                                          │   │
│  │   queue_stopped,                                        │   │
│  │                                                          │   │
│  │   job_creation_failed = 300,       // Job (300+)        │   │
│  │   job_execution_failed,                                 │   │
│  │   job_invalid,                                          │   │
│  │                                                          │   │
│  │   resource_allocation_failed = 400, // Resource (400+)  │   │
│  │   resource_limit_reached,                               │   │
│  │                                                          │   │
│  │   mutex_error = 500,               // Sync (500+)       │   │
│  │   deadlock_detected,                                    │   │
│  │   condition_variable_error,                             │   │
│  │                                                          │   │
│  │   io_error = 600,                  // IO (600+)         │   │
│  │   ...                                                   │   │
│  │ };                                                      │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                  │
│  BENEFITS:                                                     │
│  ✅ No exceptions in critical paths                           │
│  ✅ Type-safe error information                               │
│  ✅ Explicit error handling                                   │
│  ✅ Composable & chainable operations                         │
│  ✅ Clear error propagation paths                             │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

---

*Last Updated: 2025-11-14*
*Visual diagrams for Thread System architecture and key mechanisms*
