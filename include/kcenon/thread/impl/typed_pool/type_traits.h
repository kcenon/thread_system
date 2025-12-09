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
 * @file type_traits.h
 * @brief Type traits for typed_thread_pool module
 *
 * This file contains type traits and compile-time utilities for the typed
 * thread pool. Concept definitions (JobType, JobCallable) are imported from
 * thread_concepts.h to avoid duplication.
 */

#include <kcenon/thread/concepts/thread_concepts.h>

#include <type_traits>
#include <string>

namespace kcenon::thread::detail {

    // JobType and JobCallable concepts are now imported from thread_concepts.h
    // via the using declarations in that header's detail namespace section

    /**
     * @brief Type traits for job types
     *
     * Provides compile-time information about job type characteristics.
     */
#ifdef USE_STD_CONCEPTS
    template<JobType T>
#else
    template<typename T, typename = std::enable_if_t<JobType<T>>>
#endif
    struct job_type_traits {
        using type = T;
        using underlying_type = std::conditional_t<std::is_enum_v<T>,
                                                    std::underlying_type_t<T>,
                                                    T>;

        static constexpr bool is_enum = std::is_enum_v<T>;
        static constexpr bool is_integral = std::is_integral_v<T>;
        static constexpr bool has_ordering = true;
        static constexpr bool is_signed = std::is_signed_v<underlying_type>;

        /**
         * @brief Converts job type to its underlying representation
         */
        static constexpr underlying_type to_underlying(T value) noexcept {
            if constexpr (is_enum) {
                return static_cast<underlying_type>(value);
            } else {
                return value;
            }
        }

        /**
         * @brief Creates job type from underlying representation
         */
        static constexpr T from_underlying(underlying_type value) noexcept {
            if constexpr (is_enum) {
                return static_cast<T>(value);
            } else {
                return value;
            }
        }
    };

    /**
     * @brief Helper to determine if a type can be used as a job priority
     */
    template<typename T>
    constexpr bool can_compare_priority() {
        if constexpr (JobType<T>) {
            using traits = job_type_traits<T>;
            return traits::has_ordering;
        }
        return false;
    }

    /**
     * @brief Compile-time priority comparison
     */
#ifdef USE_STD_CONCEPTS
    template<JobType T>
#else
    template<typename T, typename = std::enable_if_t<JobType<T>>>
#endif
    constexpr bool higher_priority(T lhs, T rhs) noexcept {
        using traits = job_type_traits<T>;

        // Assume lower numerical values = higher priority
        return traits::to_underlying(lhs) < traits::to_underlying(rhs);
    }

    /**
     * @brief Type alias for job type conversion
     */
#ifdef USE_STD_CONCEPTS
    template<JobType T>
    using job_underlying_t = typename job_type_traits<T>::underlying_type;
#else
    template<typename T, typename = std::enable_if_t<JobType<T>>>
    using job_underlying_t = typename job_type_traits<T, void>::underlying_type;
#endif

} // namespace kcenon::thread::detail
