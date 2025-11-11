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
 * @file synchronization.h
 * @brief C++17-compatible synchronization primitives with C++20 fallback
 *
 * Provides latch and barrier implementations that work in C++17,
 * with automatic fallback to std:: versions when available in C++20.
 */

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

#ifdef HAS_STD_LATCH
#include <latch>
#include <barrier>
#endif

namespace kcenon::thread {

#ifdef HAS_STD_LATCH
    // C++20: Use standard library implementations
    using std::latch;
    using std::barrier;
#else
    /**
     * @brief C++17-compatible latch implementation
     *
     * A downward counter which can be used to synchronize threads.
     * The value of the counter is initialized on creation. Threads may
     * block on the latch until the counter is decremented to zero.
     */
    class latch {
    public:
        /**
         * @brief Constructs a latch with the given count
         * @param count The initial value of the counter (must be >= 0)
         */
        explicit latch(std::ptrdiff_t count)
            : count_(count) {}

        /**
         * @brief Decrements the counter in a non-blocking manner
         * @param n The value to subtract from the counter (default: 1)
         */
        void count_down(std::ptrdiff_t n = 1) {
            std::unique_lock<std::mutex> lock(mutex_);
            count_ -= n;
            if (count_ <= 0) {
                cv_.notify_all();
            }
        }

        /**
         * @brief Tests if the internal counter equals zero
         * @return true if the counter has reached zero, false otherwise
         */
        bool try_wait() const noexcept {
            std::unique_lock<std::mutex> lock(mutex_);
            return count_ <= 0;
        }

        /**
         * @brief Blocks until the counter reaches zero
         */
        void wait() const {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return count_ <= 0; });
        }

        /**
         * @brief Decrements the counter and blocks until it reaches zero
         * @param n The value to subtract from the counter (default: 1)
         */
        void arrive_and_wait(std::ptrdiff_t n = 1) {
            count_down(n);
            wait();
        }

    private:
        mutable std::mutex mutex_;
        mutable std::condition_variable cv_;
        std::ptrdiff_t count_;
    };

    /**
     * @brief C++17-compatible barrier implementation
     *
     * A reusable thread coordination mechanism that blocks a set of threads
     * until all threads have arrived at the barrier.
     *
     * @tparam CompletionFunction A callable invoked when all threads arrive
     */
    template<typename CompletionFunction = std::function<void()>>
    class barrier {
    public:
        /**
         * @brief Constructs a barrier for the given number of threads
         * @param count The number of threads to synchronize (must be > 0)
         * @param completion Optional function called when all threads arrive
         */
        explicit barrier(std::ptrdiff_t count,
                        CompletionFunction completion = [](){})
            : threshold_(count)
            , count_(count)
            , generation_(0)
            , completion_(std::move(completion)) {}

        /**
         * @brief Arrives at the barrier and blocks until all threads arrive
         *
         * When the last thread arrives:
         * 1. The completion function is called
         * 2. All waiting threads are unblocked
         * 3. The barrier resets for the next phase
         */
        void arrive_and_wait() {
            std::unique_lock<std::mutex> lock(mutex_);
            auto gen = generation_;

            if (--count_ == 0) {
                // Last thread to arrive
                generation_++;
                count_ = threshold_;

                // Call completion function (if provided)
                if constexpr (!std::is_same_v<CompletionFunction, std::function<void()>>) {
                    completion_();
                } else if (completion_) {
                    completion_();
                }

                cv_.notify_all();
            } else {
                // Wait for all threads to arrive
                cv_.wait(lock, [this, gen] { return gen != generation_; });
            }
        }

        /**
         * @brief Decrements the counter without blocking
         *
         * Equivalent to arrive_and_drop() in C++20.
         * This thread will not participate in future barrier phases.
         */
        void arrive_and_drop() {
            std::unique_lock<std::mutex> lock(mutex_);
            --threshold_;
            arrive_and_wait();
        }

    private:
        mutable std::mutex mutex_;
        mutable std::condition_variable cv_;
        std::ptrdiff_t threshold_;  // Total number of threads
        std::ptrdiff_t count_;       // Remaining threads in current phase
        std::size_t generation_;     // Current phase number
        CompletionFunction completion_;
    };

#endif // HAS_STD_LATCH

} // namespace kcenon::thread
