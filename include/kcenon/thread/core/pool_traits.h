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
 * @file pool_traits.h
 * @brief Type traits and metaprogramming utilities for thread pool
 *
 * This file contains type traits and compile-time utilities that help ensure
 * type safety and provide better error messages. Concept definitions have been
 * moved to thread_concepts.h to avoid duplication.
 */

#include <kcenon/thread/concepts/thread_concepts.h>

#include <type_traits>
#include <functional>
#include <chrono>
#include <algorithm>

namespace kcenon::thread::detail {

    // Concepts are now imported from thread_concepts.h via the using declarations
    // in that header's detail namespace section

    /**
     * @brief Type trait for function signature analysis
     */
    template<typename F>
    struct function_traits;

    // Specialization for function pointers
    template<typename R, typename... Args>
    struct function_traits<R(*)(Args...)> {
        using return_type = R;
        using argument_types = std::tuple<Args...>;
        static constexpr size_t arity = sizeof...(Args);
        static constexpr bool is_noexcept = false;
    };

    // Specialization for noexcept function pointers
    template<typename R, typename... Args>
    struct function_traits<R(*)(Args...) noexcept> {
        using return_type = R;
        using argument_types = std::tuple<Args...>;
        static constexpr size_t arity = sizeof...(Args);
        static constexpr bool is_noexcept = true;
    };

    // Specialization for member function pointers
    template<typename C, typename R, typename... Args>
    struct function_traits<R(C::*)(Args...)> {
        using return_type = R;
        using class_type = C;
        using argument_types = std::tuple<Args...>;
        static constexpr size_t arity = sizeof...(Args);
        static constexpr bool is_noexcept = false;
    };

    // Specialization for const member function pointers
    template<typename C, typename R, typename... Args>
    struct function_traits<R(C::*)(Args...) const> {
        using return_type = R;
        using class_type = C;
        using argument_types = std::tuple<Args...>;
        static constexpr size_t arity = sizeof...(Args);
        static constexpr bool is_noexcept = false;
    };

    // Specialization for function objects and lambdas
    template<typename F>
    struct function_traits : function_traits<decltype(&F::operator())> {};

    /**
     * @brief Helper to get function traits
     */
    template<typename F>
    using function_return_t = typename function_traits<F>::return_type;

    template<typename F>
    using function_args_t = typename function_traits<F>::argument_types;

    template<typename F>
    constexpr size_t function_arity_v = function_traits<F>::arity;

    /**
     * @brief Compile-time validation for thread pool configuration
     */
    template<size_t ThreadCount>
    struct validate_thread_count {
        static_assert(ThreadCount > 0, "Thread count must be positive");
        static_assert(ThreadCount <= 1024, "Thread count is unreasonably high");
        static constexpr bool value = true;
    };

    /**
     * @brief Template helper for perfect forwarding with type constraints
     */
#ifdef USE_STD_CONCEPTS
    template<typename T>
        requires Callable<T>
    constexpr auto forward_if_callable(T&& t) -> T&& {
        return std::forward<T>(t);
    }
#else
    template<typename T>
    constexpr auto forward_if_callable(T&& t) -> std::enable_if_t<Callable<T>, T&&> {
        return std::forward<T>(t);
    }
#endif

    /**
     * @brief Type eraser for heterogeneous callable storage
     */
    class callable_eraser {
    public:
#ifdef USE_STD_CONCEPTS
        template<Callable F>
#else
        template<typename F, typename = std::enable_if_t<Callable<F>>>
#endif
        callable_eraser(F&& f)
            : vtable_(&vtable_for<std::decay_t<F>>)
        {
            static_assert(sizeof(std::decay_t<F>) <= sizeof(storage_),
                          "Callable is too large for inline storage");
            new (&storage_) std::decay_t<F>(std::forward<F>(f));
        }

        void operator()() {
            vtable_->invoke(&storage_);
        }

        ~callable_eraser() {
            if (vtable_) {
                vtable_->destroy(&storage_);
            }
        }

        callable_eraser(const callable_eraser&) = delete;
        callable_eraser& operator=(const callable_eraser&) = delete;

        callable_eraser(callable_eraser&& other) noexcept
            : vtable_(other.vtable_)
        {
            std::copy_n(other.storage_, sizeof(storage_), storage_);
            other.vtable_ = nullptr;
        }

        callable_eraser& operator=(callable_eraser&& other) noexcept {
            if (this != &other) {
                if (vtable_) {
                    vtable_->destroy(&storage_);
                }
                vtable_ = other.vtable_;
                std::copy_n(other.storage_, sizeof(storage_), storage_);
                other.vtable_ = nullptr;
            }
            return *this;
        }

    private:
        struct vtable_t {
            void (*invoke)(void*);
            void (*destroy)(void*);
        };

        template<typename F>
        static constexpr vtable_t vtable_for = {
            [](void* ptr) { (*static_cast<F*>(ptr))(); },
            [](void* ptr) { static_cast<F*>(ptr)->~F(); }
        };

        const vtable_t* vtable_;
        alignas(std::max_align_t) char storage_[64];
    };

    /**
     * @brief Compile-time string for template error messages
     */
    template<size_t N>
    struct compile_string {
        constexpr compile_string(const char (&str)[N]) {
            std::copy_n(str, N, value);
        }
        char value[N];
    };

    /**
     * @brief Template for generating descriptive error messages
     */
    template<typename T>
    constexpr auto get_type_name() {
        // This would ideally use std::source_location or compiler intrinsics
        // For now, return a generic message
        return "unknown_type";
    }

    /**
     * @brief SFINAE helper to detect if a type has a specific member
     */
    template<typename T, typename = void>
    struct has_get_method : std::false_type {};

    template<typename T>
    struct has_get_method<T, std::void_t<decltype(std::declval<T>().get())>>
        : std::true_type {};

    template<typename T>
    constexpr bool has_get_method_v = has_get_method<T>::value;

} // namespace kcenon::thread::detail
