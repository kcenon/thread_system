#pragma once

/**
 * @file forward.h
 * @brief Forward declarations for thread_system types
 *
 * This header provides forward declarations for commonly used types
 * in the thread_system module to reduce compilation dependencies.
 */

namespace kcenon::thread {

// Core classes
namespace core {
    class thread_pool;
    class task_scheduler;
    class worker_thread;
    class thread_context;
}

// Synchronization primitives
namespace sync {
    template<typename T> class atomic_wrapper;
    template<typename T> class lock_free_queue;
    template<typename T> class lock_free_stack;
    class barrier;
    class latch;
}

// Task management
namespace task {
    class task_base;
    template<typename T> class task;
    template<typename T> class future;
    template<typename T> class promise;
    class task_group;
}

// Utilities
namespace utils {
    class thread_id;
    class thread_local_storage;
    class cpu_affinity;
}

// Interfaces
namespace interfaces {
    class cancellable;
    class schedulable;
    class monitoring_interface;
}

} // namespace kcenon::thread