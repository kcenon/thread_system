#pragma once

/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2024, DongCheol Shin
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file cancellable_future.h
 * @brief Future wrapper with cancellation support
 *
 * This file provides the cancellable_future template class that wraps
 * std::future with cancellation_token integration for cooperative cancellation.
 */

#include <kcenon/thread/core/cancellation_token.h>

#include <chrono>
#include <future>
#include <optional>
#include <stdexcept>

namespace kcenon::thread {

/**
 * @class cancellable_future
 * @brief A future wrapper that supports cancellation
 *
 * @tparam R The result type of the future
 *
 * This class wraps a std::future and a cancellation_token together,
 * providing unified access to both the result and cancellation status.
 *
 * ### Thread Safety
 * - All methods are thread-safe.
 * - The cancellation token can be shared across multiple contexts.
 *
 * ### Usage Example
 * @code
 * auto [future, token] = pool->submit_cancellable([] {
 *     // Long-running work
 *     return compute_result();
 * });
 *
 * // Later, if we need to cancel:
 * future.cancel();
 *
 * // Or wait with timeout:
 * if (auto result = future.get_for(std::chrono::seconds(5))) {
 *     process(*result);
 * } else {
 *     handle_timeout();
 * }
 * @endcode
 */
template<typename R>
class cancellable_future {
public:
    using value_type = R;

    /**
     * @brief Construct a cancellable_future
     *
     * @param future The underlying std::future
     * @param token The cancellation token for this operation
     */
    cancellable_future(std::future<R> future, cancellation_token token)
        : future_(std::move(future))
        , token_(std::move(token))
    {}

    // Destructor
    ~cancellable_future() = default;

    // Delete copy operations
    cancellable_future(const cancellable_future&) = delete;
    cancellable_future& operator=(const cancellable_future&) = delete;

    // Allow move operations
    cancellable_future(cancellable_future&&) noexcept = default;
    cancellable_future& operator=(cancellable_future&&) noexcept = default;

    /**
     * @brief Wait for and retrieve the result
     *
     * @return The result value
     * @throws std::runtime_error if the operation was cancelled
     * @throws Any exception thrown by the callable
     *
     * This method blocks until the result is ready or the operation
     * is cancelled.
     */
    [[nodiscard]] auto get() -> R {
        if (token_.is_cancelled()) {
            throw std::runtime_error("Operation cancelled");
        }
        return future_.get();
    }

    /**
     * @brief Wait for the result with timeout
     *
     * @param timeout Maximum time to wait
     * @return The result if ready within timeout, std::nullopt otherwise
     * @throws std::runtime_error if the operation was cancelled
     *
     * This method waits up to the specified timeout for the result.
     * Returns std::nullopt if the timeout expires before the result is ready.
     */
    [[nodiscard]] auto get_for(std::chrono::milliseconds timeout)
        -> std::optional<R>
    {
        if (token_.is_cancelled()) {
            throw std::runtime_error("Operation cancelled");
        }

        auto status = future_.wait_for(timeout);
        if (status == std::future_status::ready) {
            return future_.get();
        }
        return std::nullopt;
    }

    /**
     * @brief Check if the result is ready
     *
     * @return true if the result is available, false otherwise
     */
    [[nodiscard]] auto is_ready() const -> bool {
        return future_.wait_for(std::chrono::seconds(0)) ==
               std::future_status::ready;
    }

    /**
     * @brief Check if the operation was cancelled
     *
     * @return true if cancellation was requested, false otherwise
     */
    [[nodiscard]] auto is_cancelled() const -> bool {
        return token_.is_cancelled();
    }

    /**
     * @brief Request cancellation of the operation
     *
     * This signals the associated job to stop. The job must
     * cooperatively check its cancellation token for this to
     * have effect.
     */
    void cancel() {
        token_.cancel();
    }

    /**
     * @brief Check if the future is valid
     *
     * @return true if the future has a shared state, false otherwise
     */
    [[nodiscard]] auto valid() const -> bool {
        return future_.valid();
    }

    /**
     * @brief Wait for the result to become ready
     *
     * Blocks until the result is available.
     */
    void wait() const {
        future_.wait();
    }

    /**
     * @brief Wait for the result with timeout
     *
     * @param timeout Maximum time to wait
     * @return The future status after waiting
     */
    template<typename Rep, typename Period>
    [[nodiscard]] auto wait_for(const std::chrono::duration<Rep, Period>& timeout) const
        -> std::future_status
    {
        return future_.wait_for(timeout);
    }

    /**
     * @brief Get the cancellation token
     *
     * @return A copy of the cancellation token
     */
    [[nodiscard]] auto get_token() const -> cancellation_token {
        return token_;
    }

private:
    mutable std::future<R> future_;
    cancellation_token token_;
};

/**
 * @brief Specialization for void return type
 */
template<>
class cancellable_future<void> {
public:
    using value_type = void;

    cancellable_future(std::future<void> future, cancellation_token token)
        : future_(std::move(future))
        , token_(std::move(token))
    {}

    ~cancellable_future() = default;

    cancellable_future(const cancellable_future&) = delete;
    cancellable_future& operator=(const cancellable_future&) = delete;
    cancellable_future(cancellable_future&&) noexcept = default;
    cancellable_future& operator=(cancellable_future&&) noexcept = default;

    void get() {
        if (token_.is_cancelled()) {
            throw std::runtime_error("Operation cancelled");
        }
        future_.get();
    }

    [[nodiscard]] auto get_for(std::chrono::milliseconds timeout) -> bool {
        if (token_.is_cancelled()) {
            throw std::runtime_error("Operation cancelled");
        }

        auto status = future_.wait_for(timeout);
        if (status == std::future_status::ready) {
            future_.get();
            return true;
        }
        return false;
    }

    [[nodiscard]] auto is_ready() const -> bool {
        return future_.wait_for(std::chrono::seconds(0)) ==
               std::future_status::ready;
    }

    [[nodiscard]] auto is_cancelled() const -> bool {
        return token_.is_cancelled();
    }

    void cancel() {
        token_.cancel();
    }

    [[nodiscard]] auto valid() const -> bool {
        return future_.valid();
    }

    void wait() const {
        future_.wait();
    }

    template<typename Rep, typename Period>
    [[nodiscard]] auto wait_for(const std::chrono::duration<Rep, Period>& timeout) const
        -> std::future_status
    {
        return future_.wait_for(timeout);
    }

    [[nodiscard]] auto get_token() const -> cancellation_token {
        return token_;
    }

private:
    mutable std::future<void> future_;
    cancellation_token token_;
};

} // namespace kcenon::thread
