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

#include <cstddef>
#include <limits>
#include <optional>

namespace kcenon::thread::policies {

/**
 * @brief Tag type for bound policy identification
 */
struct bound_policy_tag {};

/**
 * @class unbounded_policy
 * @brief Policy that allows unlimited queue size
 *
 * This policy does not impose any size limits on the queue.
 * Memory is the only constraint on queue capacity.
 *
 * ### Usage
 * @code
 * policy_queue<unbounded_policy, mutex_sync_policy> queue;
 * // Queue can grow indefinitely
 * @endcode
 */
class unbounded_policy {
public:
    using policy_tag = bound_policy_tag;

    /**
     * @brief Construct unbounded policy
     */
    constexpr unbounded_policy() noexcept = default;

    /**
     * @brief Check if queue is at capacity
     * @param current_size Current queue size
     * @return Always false (never at capacity)
     */
    [[nodiscard]] constexpr auto is_full(std::size_t /*current_size*/) const noexcept -> bool {
        return false;
    }

    /**
     * @brief Get maximum size
     * @return nullopt (no limit)
     */
    [[nodiscard]] constexpr auto max_size() const noexcept -> std::optional<std::size_t> {
        return std::nullopt;
    }

    /**
     * @brief Check if this is a bounded policy
     * @return false
     */
    [[nodiscard]] static constexpr auto is_bounded() noexcept -> bool {
        return false;
    }

    /**
     * @brief Get remaining capacity
     * @param current_size Current queue size (ignored)
     * @return Maximum size_t value
     */
    [[nodiscard]] constexpr auto remaining_capacity(std::size_t /*current_size*/) const noexcept -> std::size_t {
        return std::numeric_limits<std::size_t>::max();
    }
};

/**
 * @class bounded_policy
 * @brief Policy that limits queue size to a maximum
 *
 * This policy enforces a maximum queue size. When the queue is full,
 * enqueue operations may fail or block depending on the overflow policy.
 *
 * ### Thread Safety
 * - Thread-safe for concurrent access
 * - Size checks are based on provided current_size parameter
 *
 * ### Usage
 * @code
 * bounded_policy bound(1000);  // Max 1000 items
 * policy_queue<bounded_policy, mutex_sync_policy> queue(bound);
 * @endcode
 */
class bounded_policy {
public:
    using policy_tag = bound_policy_tag;

    /**
     * @brief Construct bounded policy with max size
     * @param max Maximum queue size
     */
    explicit constexpr bounded_policy(std::size_t max) noexcept
        : max_size_(max) {}

    /**
     * @brief Check if queue is at capacity
     * @param current_size Current queue size
     * @return true if at or above max size
     */
    [[nodiscard]] constexpr auto is_full(std::size_t current_size) const noexcept -> bool {
        return current_size >= max_size_;
    }

    /**
     * @brief Get maximum size
     * @return The configured maximum size
     */
    [[nodiscard]] constexpr auto max_size() const noexcept -> std::optional<std::size_t> {
        return max_size_;
    }

    /**
     * @brief Check if this is a bounded policy
     * @return true
     */
    [[nodiscard]] static constexpr auto is_bounded() noexcept -> bool {
        return true;
    }

    /**
     * @brief Get remaining capacity
     * @param current_size Current queue size
     * @return Number of items that can still be added
     */
    [[nodiscard]] constexpr auto remaining_capacity(std::size_t current_size) const noexcept -> std::size_t {
        if (current_size >= max_size_) {
            return 0;
        }
        return max_size_ - current_size;
    }

    /**
     * @brief Set new maximum size
     * @param new_max New maximum size
     *
     * @note If new max is smaller than current size, queue becomes over-capacity.
     *       Overflow handling depends on the overflow policy.
     */
    auto set_max_size(std::size_t new_max) noexcept -> void {
        max_size_ = new_max;
    }

private:
    std::size_t max_size_;
};

/**
 * @class dynamic_bounded_policy
 * @brief Policy with dynamically adjustable size limit
 *
 * Similar to bounded_policy but allows runtime changes to the limit
 * and can be switched to unbounded mode.
 *
 * ### Usage
 * @code
 * dynamic_bounded_policy bound(1000);
 * // Later:
 * bound.set_max_size(2000);  // Increase limit
 * bound.set_unbounded();     // Remove limit entirely
 * @endcode
 */
class dynamic_bounded_policy {
public:
    using policy_tag = bound_policy_tag;

    /**
     * @brief Construct with optional max size
     * @param max Optional maximum size (nullopt for unbounded)
     */
    explicit constexpr dynamic_bounded_policy(
        std::optional<std::size_t> max = std::nullopt) noexcept
        : max_size_(max) {}

    /**
     * @brief Check if queue is at capacity
     * @param current_size Current queue size
     * @return true if bounded and at or above max size
     */
    [[nodiscard]] constexpr auto is_full(std::size_t current_size) const noexcept -> bool {
        if (!max_size_.has_value()) {
            return false;
        }
        return current_size >= max_size_.value();
    }

    /**
     * @brief Get maximum size
     * @return The configured maximum size, or nullopt if unbounded
     */
    [[nodiscard]] constexpr auto max_size() const noexcept -> std::optional<std::size_t> {
        return max_size_;
    }

    /**
     * @brief Check if this is currently bounded
     * @return true if max size is set
     */
    [[nodiscard]] constexpr auto is_bounded() const noexcept -> bool {
        return max_size_.has_value();
    }

    /**
     * @brief Get remaining capacity
     * @param current_size Current queue size
     * @return Number of items that can still be added
     */
    [[nodiscard]] constexpr auto remaining_capacity(std::size_t current_size) const noexcept -> std::size_t {
        if (!max_size_.has_value()) {
            return std::numeric_limits<std::size_t>::max();
        }
        if (current_size >= max_size_.value()) {
            return 0;
        }
        return max_size_.value() - current_size;
    }

    /**
     * @brief Set new maximum size
     * @param new_max New maximum size
     */
    auto set_max_size(std::size_t new_max) noexcept -> void {
        max_size_ = new_max;
    }

    /**
     * @brief Remove size limit (become unbounded)
     */
    auto set_unbounded() noexcept -> void {
        max_size_ = std::nullopt;
    }

private:
    std::optional<std::size_t> max_size_;
};

} // namespace kcenon::thread::policies
