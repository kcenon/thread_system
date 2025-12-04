/*
 * BSD 3-Clause License
 * Copyright (c) 2025, DongCheol Shin
 */

#pragma once

#include <kcenon/thread/interfaces/queue_capabilities.h>

namespace kcenon::thread {

/**
 * @class queue_capabilities_interface
 * @brief Mixin interface for queue capability introspection.
 *
 * This interface provides runtime capability queries for queue implementations.
 * Classes can optionally inherit from this to expose their capabilities,
 * enabling consumers to adapt their behavior accordingly.
 *
 * ### Non-Breaking Addition
 * This is an additive interface. Existing code that doesn't use this interface
 * continues to work unchanged. New code can optionally check for capabilities
 * using dynamic_cast or other type checking mechanisms.
 *
 * ### Thread Safety
 * All methods in this interface are const and return by value, making them
 * inherently thread-safe. Implementations should not introduce mutable state.
 *
 * ### Usage Example
 * @code
 * // Check if queue supports exact size (new code)
 * if (auto* cap = dynamic_cast<queue_capabilities_interface*>(queue.get())) {
 *     if (cap->has_exact_size()) {
 *         // Safe to use size() for decisions
 *         auto count = queue->size();
 *     }
 * }
 *
 * // Existing code continues to work (unchanged)
 * queue->enqueue(std::make_unique<my_job>());
 * @endcode
 */
class queue_capabilities_interface {
public:
    virtual ~queue_capabilities_interface() = default;

    /**
     * @brief Get capabilities of this queue implementation.
     * @return Queue capabilities struct describing this implementation
     *
     * ### Default Implementation
     * Returns capabilities matching mutex-based job_queue behavior,
     * ensuring backward compatibility for implementations that don't override.
     */
    [[nodiscard]] virtual auto get_capabilities() const -> queue_capabilities {
        return queue_capabilities{}; // Defaults match job_queue
    }

    /**
     * @brief Check if size() returns exact values.
     * @return true if size() provides accurate count
     *
     * Lock-free queues may return approximate sizes due to concurrent modifications.
     */
    [[nodiscard]] auto has_exact_size() const -> bool {
        return get_capabilities().exact_size;
    }

    /**
     * @brief Check if empty() check is atomic.
     * @return true if empty() provides consistent atomic check
     */
    [[nodiscard]] auto has_atomic_empty() const -> bool {
        return get_capabilities().atomic_empty_check;
    }

    /**
     * @brief Check if this is a lock-free implementation.
     * @return true if the queue uses lock-free algorithms
     *
     * Lock-free implementations provide better performance under high contention
     * but may have different semantics for size/empty checks.
     */
    [[nodiscard]] auto is_lock_free() const -> bool {
        return get_capabilities().lock_free;
    }

    /**
     * @brief Check if this is a wait-free implementation.
     * @return true if the queue uses wait-free algorithms
     *
     * Wait-free is a stronger guarantee than lock-free, ensuring bounded
     * execution time for all operations regardless of contention.
     */
    [[nodiscard]] auto is_wait_free() const -> bool {
        return get_capabilities().wait_free;
    }

    /**
     * @brief Check if batch operations are supported.
     * @return true if the queue supports batch enqueue/dequeue
     */
    [[nodiscard]] auto supports_batch() const -> bool {
        return get_capabilities().supports_batch;
    }

    /**
     * @brief Check if blocking wait is supported.
     * @return true if the queue supports blocking dequeue operations
     */
    [[nodiscard]] auto supports_blocking_wait() const -> bool {
        return get_capabilities().supports_blocking_wait;
    }

    /**
     * @brief Check if stop signaling is supported.
     * @return true if the queue supports stop() method for shutdown
     */
    [[nodiscard]] auto supports_stop() const -> bool {
        return get_capabilities().supports_stop;
    }
};

} // namespace kcenon::thread
