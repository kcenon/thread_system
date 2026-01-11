/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, DongCheol Shin
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#pragma once

#include <chrono>
#include <memory>

#include <kcenon/thread/core/error_handling.h>
#include <kcenon/thread/core/job.h>

namespace kcenon::thread::policies {

/**
 * @brief Tag type for overflow policy identification
 */
struct overflow_policy_tag {};

/**
 * @class overflow_reject_policy
 * @brief Policy that rejects new items when queue is full
 *
 * When the queue is full, enqueue operations immediately fail
 * with an error. The new item is not added to the queue.
 *
 * ### Use Cases
 * - Fast-fail scenarios where dropping new work is acceptable
 * - Load shedding where overflow should be handled by caller
 * - Non-blocking producers that check result
 *
 * ### Usage
 * @code
 * policy_queue<bounded_policy, mutex_sync_policy, overflow_reject_policy> queue(100);
 * auto result = queue.enqueue(make_job());
 * if (result.is_err()) {
 *     // Handle overflow - job was rejected
 * }
 * @endcode
 */
class overflow_reject_policy {
public:
    using policy_tag = overflow_policy_tag;

    /**
     * @brief Handle overflow by rejecting the new item
     * @param value The job that would be added (will be dropped)
     * @return Error indicating queue is full
     */
    [[nodiscard]] auto handle_overflow(std::unique_ptr<job>&& /*value*/)
        -> common::VoidResult {
        return common::error_info{-120, "queue is full, rejecting new item", "thread_system"};
    }

    /**
     * @brief Check if this policy blocks on overflow
     * @return false (never blocks)
     */
    [[nodiscard]] static constexpr auto blocks() noexcept -> bool {
        return false;
    }

    /**
     * @brief Get a descriptive name for this policy
     * @return Policy name string
     */
    [[nodiscard]] static constexpr auto name() noexcept -> const char* {
        return "overflow_reject";
    }
};

/**
 * @class overflow_block_policy
 * @brief Policy that blocks until space is available
 *
 * When the queue is full, the enqueue operation blocks until
 * another thread dequeues an item, making space available.
 *
 * ### Use Cases
 * - Backpressure scenarios where producers should slow down
 * - Bounded buffers between producer-consumer pairs
 * - Flow control where no items should be lost
 *
 * ### Thread Safety
 * - Thread-safe blocking using condition variables
 * - Safe for multiple concurrent producers
 *
 * @warning This policy requires the sync policy to support blocking waits.
 *          It will not work correctly with lock-free sync policies.
 */
class overflow_block_policy {
public:
    using policy_tag = overflow_policy_tag;

    /**
     * @brief Handle overflow by indicating need to wait
     * @param value The job to be added (returned unchanged)
     * @return Special result indicating blocking is needed
     *
     * @note The actual blocking is implemented by the queue template
     */
    [[nodiscard]] auto handle_overflow(std::unique_ptr<job>&& value)
        -> std::pair<common::VoidResult, std::unique_ptr<job>> {
        // Return the job back to the caller for retry after waiting
        return {common::error_info{-123, "queue full, waiting for space", "thread_system"},
                std::move(value)};
    }

    /**
     * @brief Check if this policy blocks on overflow
     * @return true (always blocks)
     */
    [[nodiscard]] static constexpr auto blocks() noexcept -> bool {
        return true;
    }

    /**
     * @brief Get a descriptive name for this policy
     * @return Policy name string
     */
    [[nodiscard]] static constexpr auto name() noexcept -> const char* {
        return "overflow_block";
    }
};

/**
 * @class overflow_drop_oldest_policy
 * @brief Policy that drops the oldest item when queue is full
 *
 * When the queue is full, the oldest item (front of queue) is removed
 * and discarded, making room for the new item.
 *
 * ### Use Cases
 * - Real-time systems where recent data is more valuable
 * - Telemetry/metrics where latest values matter most
 * - Caching scenarios with LRU-like behavior
 *
 * ### Thread Safety
 * - Thread-safe for concurrent access
 * - Oldest item removal is atomic with new item insertion
 *
 * @note The dropped job's destructor will be called immediately
 */
class overflow_drop_oldest_policy {
public:
    using policy_tag = overflow_policy_tag;

    /**
     * @brief Handle overflow by requesting oldest item removal
     * @param value The job to be added
     * @return Result indicating oldest should be dropped
     */
    [[nodiscard]] auto handle_overflow(std::unique_ptr<job>&& value)
        -> std::pair<bool, std::unique_ptr<job>> {
        // Return true to indicate oldest should be dropped
        return {true, std::move(value)};
    }

    /**
     * @brief Check if this policy blocks on overflow
     * @return false (never blocks)
     */
    [[nodiscard]] static constexpr auto blocks() noexcept -> bool {
        return false;
    }

    /**
     * @brief Check if this policy drops items on overflow
     * @return true (drops oldest)
     */
    [[nodiscard]] static constexpr auto drops_oldest() noexcept -> bool {
        return true;
    }

    /**
     * @brief Get a descriptive name for this policy
     * @return Policy name string
     */
    [[nodiscard]] static constexpr auto name() noexcept -> const char* {
        return "overflow_drop_oldest";
    }
};

/**
 * @class overflow_drop_newest_policy
 * @brief Policy that rejects new item when queue is full (same as reject)
 *
 * When the queue is full, the new item is dropped instead of being added.
 * This is semantically equivalent to overflow_reject_policy but with
 * different naming convention for clarity in some contexts.
 *
 * ### Use Cases
 * - Burst handling where excess items should be silently dropped
 * - Scenarios where existing work is more important than new work
 * - Simple overflow handling without error propagation
 *
 * @note Unlike overflow_reject_policy, this policy returns success
 *       even though the item was dropped.
 */
class overflow_drop_newest_policy {
public:
    using policy_tag = overflow_policy_tag;

    /**
     * @brief Handle overflow by dropping the new item silently
     * @param value The job that would be added (will be dropped)
     * @return Success (item was handled, by dropping)
     */
    [[nodiscard]] auto handle_overflow(std::unique_ptr<job>&& /*value*/)
        -> common::VoidResult {
        // Silently drop the item
        return common::ok();
    }

    /**
     * @brief Check if this policy blocks on overflow
     * @return false (never blocks)
     */
    [[nodiscard]] static constexpr auto blocks() noexcept -> bool {
        return false;
    }

    /**
     * @brief Check if this policy drops items on overflow
     * @return true (drops newest)
     */
    [[nodiscard]] static constexpr auto drops_newest() noexcept -> bool {
        return true;
    }

    /**
     * @brief Get a descriptive name for this policy
     * @return Policy name string
     */
    [[nodiscard]] static constexpr auto name() noexcept -> const char* {
        return "overflow_drop_newest";
    }
};

/**
 * @class overflow_timeout_policy
 * @brief Policy that blocks for a limited time when queue is full
 *
 * When the queue is full, the enqueue operation blocks for up to
 * the configured timeout duration. If space doesn't become available
 * within the timeout, the operation fails.
 *
 * ### Use Cases
 * - Bounded waits where indefinite blocking is unacceptable
 * - Timeout-based flow control
 * - Interactive systems with response time requirements
 *
 * @warning Requires sync policy with blocking wait support
 */
class overflow_timeout_policy {
public:
    using policy_tag = overflow_policy_tag;

    /**
     * @brief Construct with timeout duration
     * @param timeout Maximum time to wait for space
     */
    explicit overflow_timeout_policy(std::chrono::milliseconds timeout)
        : timeout_(timeout) {}

    /**
     * @brief Default constructor with 1 second timeout
     */
    overflow_timeout_policy() : timeout_(std::chrono::seconds(1)) {}

    /**
     * @brief Get the configured timeout
     * @return Timeout duration
     */
    [[nodiscard]] auto timeout() const noexcept -> std::chrono::milliseconds {
        return timeout_;
    }

    /**
     * @brief Set new timeout duration
     * @param timeout New timeout value
     */
    auto set_timeout(std::chrono::milliseconds timeout) noexcept -> void {
        timeout_ = timeout;
    }

    /**
     * @brief Handle overflow by indicating need to wait with timeout
     * @param value The job to be added
     * @return Result with job for retry
     */
    [[nodiscard]] auto handle_overflow(std::unique_ptr<job>&& value)
        -> std::pair<common::VoidResult, std::unique_ptr<job>> {
        return {common::error_info{-123, "queue full, waiting with timeout", "thread_system"},
                std::move(value)};
    }

    /**
     * @brief Check if this policy blocks on overflow
     * @return true (blocks with timeout)
     */
    [[nodiscard]] static constexpr auto blocks() noexcept -> bool {
        return true;
    }

    /**
     * @brief Get a descriptive name for this policy
     * @return Policy name string
     */
    [[nodiscard]] static constexpr auto name() noexcept -> const char* {
        return "overflow_timeout";
    }

private:
    std::chrono::milliseconds timeout_;
};

} // namespace kcenon::thread::policies
