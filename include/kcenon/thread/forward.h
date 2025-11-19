/**
 * @file forward.h
 * @brief Forward declarations for thread system types
 * @date 2025-11-20
 */

#pragma once

namespace kcenon {
namespace thread {

// Core types
class ThreadPool;
class ThreadWorker;
class ThreadBase;
class Job;
class JobQueue;
class CancellationToken;

// Typed thread pool types
class TypedThreadPool;
class TypedThreadWorker;
class TypedJob;
class TypedJobQueue;

// Task related types
class TaskQueue;
enum class TaskPriority;

// Synchronization primitives
class Mutex;
class Semaphore;
class ConditionVariable;
class SpinLock;
class ReadWriteLock;

// Async operation types
template<typename T> class Future;
template<typename T> class Promise;
template<typename T> class SharedFuture;

// Thread utilities
template<typename T> class ThreadSafeQueue;
template<typename K, typename V> class ThreadSafeMap;
class ThreadLocalStorage;

// Timer and scheduling
class Timer;
class PeriodicTimer;
class Scheduler;

// Lockfree structures
class LockfreeJobQueue;
template<typename T> class LockfreeQueue;

// Error types
class ThreadError;
class ThreadPoolError;

} // namespace thread
} // namespace kcenon