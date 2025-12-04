/*
 * BSD 3-Clause License
 * Copyright (c) 2025, DongCheol Shin
 */

#pragma once

namespace kcenon::thread {

/**
 * @struct queue_capabilities
 * @brief Runtime-queryable queue capabilities descriptor.
 *
 * This struct provides a standardized way to describe the capabilities
 * of different queue implementations at runtime. It enables code to
 * adapt its behavior based on the underlying queue's characteristics.
 *
 * ### Default Values
 * All fields have sensible defaults matching mutex-based job_queue behavior,
 * ensuring backward compatibility when adding this capability to existing queues.
 *
 * ### Usage Example
 * @code
 * auto caps = queue->get_capabilities();
 * if (caps.exact_size) {
 *     // Safe to use size() for precise decisions
 *     auto count = queue->size();
 * }
 * if (caps.lock_free) {
 *     // Can expect better performance under high contention
 * }
 * @endcode
 */
struct queue_capabilities {
    /// @brief size() returns exact value (vs approximate for lock-free queues)
    bool exact_size = true;

    /// @brief empty() check is atomic and consistent
    bool atomic_empty_check = true;

    /// @brief Implementation uses lock-free algorithms
    bool lock_free = false;

    /// @brief Implementation uses wait-free algorithms (stronger than lock-free)
    bool wait_free = false;

    /// @brief Supports batch enqueue/dequeue operations
    bool supports_batch = true;

    /// @brief Supports blocking dequeue with wait
    bool supports_blocking_wait = true;

    /// @brief Supports stop() method to signal shutdown
    bool supports_stop = true;

    /**
     * @brief Equality comparison operator.
     * @param other Another queue_capabilities to compare
     * @return true if all capabilities match
     */
    [[nodiscard]] constexpr auto operator==(const queue_capabilities& other) const noexcept -> bool = default;
};

} // namespace kcenon::thread
