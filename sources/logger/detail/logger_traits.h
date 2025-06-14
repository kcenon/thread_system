#pragma once

/**
 * @file logger_traits.h
 * @brief Type traits and metaprogramming utilities for logger
 * 
 * This file contains type traits, concepts, and compile-time utilities
 * that help ensure type safety and provide better error messages.
 */

#include "../types/log_types.h"
#include <type_traits>
#include <concepts>
#include <string>
#include <string_view>
#include <chrono>

namespace log_module::detail {
    
    /**
     * @brief Concept for types that can be formatted as log messages
     */
    template<typename T>
    concept Formattable = requires(T t) {
        { std::to_string(t) } -> std::convertible_to<std::string>;
    } || requires(T t) {
        { t.to_string() } -> std::convertible_to<std::string>;
    } || std::convertible_to<T, std::string> || std::convertible_to<T, std::string_view>;
    
    /**
     * @brief Concept for log writer types
     */
    template<typename T>
    concept LogWriter = requires(T writer, log_types level, const std::string& message) {
        { writer.write(level, message) } -> std::same_as<thread_module::result_void>;
        { writer.flush() } -> std::same_as<thread_module::result_void>;
        { writer.close() } -> std::same_as<thread_module::result_void>;
        { writer.is_open() } -> std::convertible_to<bool>;
    };
    
    /**
     * @brief Concept for log job types
     */
    template<typename T>
    concept LogJob = requires(T job) {
        { job.do_work() } -> std::same_as<thread_module::result_void>;
    };
    
    /**
     * @brief Type trait to detect string-like types
     */
    template<typename T>
    struct is_string_like : std::false_type {};
    
    template<>
    struct is_string_like<std::string> : std::true_type {};
    
    template<>
    struct is_string_like<std::string_view> : std::true_type {};
    
    template<>
    struct is_string_like<const char*> : std::true_type {};
    
    template<size_t N>
    struct is_string_like<char[N]> : std::true_type {};
    
    template<size_t N>
    struct is_string_like<const char[N]> : std::true_type {};
    
    template<typename T>
    constexpr bool is_string_like_v = is_string_like<T>::value;
    
    /**
     * @brief Type trait to detect duration types
     */
    template<typename T>
    struct is_duration : std::false_type {};
    
    template<typename Rep, typename Period>
    struct is_duration<std::chrono::duration<Rep, Period>> : std::true_type {};
    
    template<typename T>
    constexpr bool is_duration_v = is_duration<T>::value;
    
    /**
     * @brief Type trait to detect time point types
     */
    template<typename T>
    struct is_time_point : std::false_type {};
    
    template<typename Clock, typename Duration>
    struct is_time_point<std::chrono::time_point<Clock, Duration>> : std::true_type {};
    
    template<typename T>
    constexpr bool is_time_point_v = is_time_point<T>::value;
    
    /**
     * @brief SFINAE helper to detect if a type has a specific member function
     */
    template<typename T, typename = void>
    struct has_to_string : std::false_type {};
    
    template<typename T>
    struct has_to_string<T, std::void_t<decltype(std::declval<T>().to_string())>>
        : std::true_type {};
    
    template<typename T>
    constexpr bool has_to_string_v = has_to_string<T>::value;
    
    /**
     * @brief SFINAE helper for stream insertion operator
     */
    template<typename T, typename = void>
    struct is_streamable : std::false_type {};
    
    template<typename T>
    struct is_streamable<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T>())>>
        : std::true_type {};
    
    template<typename T>
    constexpr bool is_streamable_v = is_streamable<T>::value;
    
    /**
     * @brief Helper to convert various types to string
     */
    template<typename T>
    std::string to_log_string(T&& value) {
        if constexpr (is_string_like_v<std::decay_t<T>>) {
            return std::string(std::forward<T>(value));
        } else if constexpr (has_to_string_v<std::decay_t<T>>) {
            return value.to_string();
        } else if constexpr (std::is_arithmetic_v<std::decay_t<T>>) {
            return std::to_string(value);
        } else if constexpr (is_streamable_v<std::decay_t<T>>) {
            std::ostringstream oss;
            oss << value;
            return oss.str();
        } else {
            static_assert(sizeof(T) == 0, "Type cannot be converted to string for logging");
        }
    }
    
    /**
     * @brief Compile-time validation for log levels
     */
    template<log_types Level>
    struct validate_log_level {
        static_assert(Level >= log_types::trace && Level <= log_types::fatal,
                     "Invalid log level");
        static constexpr bool value = true;
    };
    
    /**
     * @brief Template helper for perfect forwarding with type constraints
     */
    template<typename T>
    constexpr auto forward_if_formattable(T&& t) -> std::enable_if_t<Formattable<T>, T&&> {
        return std::forward<T>(t);
    }
    
    /**
     * @brief Type eraser for heterogeneous message storage
     */
    class message_eraser {
    public:
        template<typename T>
        message_eraser(T&& value) 
            requires Formattable<T>
            : vtable_(&vtable_for<std::decay_t<T>>)
            , storage_(std::forward<T>(value)) {}
        
        std::string to_string() const {
            return vtable_->to_string(storage_);
        }
        
    private:
        struct vtable_t {
            std::string (*to_string)(const void*);
            void (*destroy)(void*);
        };
        
        template<typename T>
        static constexpr vtable_t vtable_for = {
            [](const void* ptr) { return detail::to_log_string(*static_cast<const T*>(ptr)); },
            [](void* ptr) { static_cast<T*>(ptr)->~T(); }
        };
        
        const vtable_t* vtable_;
        alignas(std::max_align_t) char storage_[64];
    };
    
    /**
     * @brief Concept for valid log message arguments
     */
    template<typename... Args>
    concept LoggableArgs = (Formattable<Args> && ...);
    
    /**
     * @brief Helper for compile-time format string validation
     */
    template<typename... Args>
    struct format_validator {
        static_assert((Formattable<Args> && ...), 
                     "All log arguments must be formattable");
        static constexpr bool value = true;
    };
    
    /**
     * @brief Template for log level filtering at compile time
     */
    template<log_types MinLevel, log_types Level>
    struct should_log : std::bool_constant<Level >= MinLevel> {};
    
    template<log_types MinLevel, log_types Level>
    constexpr bool should_log_v = should_log<MinLevel, Level>::value;
    
    /**
     * @brief Compile-time string for error messages
     */
    template<size_t N>
    struct compile_string {
        constexpr compile_string(const char (&str)[N]) {
            std::copy_n(str, N, value);
        }
        char value[N];
    };
    
    /**
     * @brief Helper to get type name for error messages
     */
    template<typename T>
    constexpr auto get_type_name() {
        // This would ideally use std::source_location or compiler intrinsics
        return "unknown_type";
    }
    
    /**
     * @brief Optimization helpers for log message formatting
     */
    namespace optimization {
        
        /**
         * @brief Check if a log operation can be optimized away
         */
        template<log_types Level, log_types MinLevel>
        constexpr bool can_optimize_away() {
            return Level < MinLevel;
        }
        
        /**
         * @brief Stack-based string builder for small messages
         */
        template<size_t BufferSize = 256>
        class stack_string_builder {
            char buffer_[BufferSize];
            size_t pos_ = 0;
            
        public:
            template<typename T>
            void append(T&& value) {
                std::string str = to_log_string(std::forward<T>(value));
                size_t copy_size = std::min(str.size(), BufferSize - pos_ - 1);
                std::memcpy(buffer_ + pos_, str.data(), copy_size);
                pos_ += copy_size;
                buffer_[pos_] = '\0';
            }
            
            std::string to_string() const {
                return std::string(buffer_, pos_);
            }
        };
        
    } // namespace optimization
    
} // namespace log_module::detail