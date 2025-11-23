/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, kcenon
All rights reserved.
*****************************************************************************/

#pragma once

#include <cstdint>
#include <string>
#include <thread>

// Platform detection macros
#if defined(_WIN32) || defined(_WIN64)
    #define THREAD_SYSTEM_WINDOWS 1
    #define THREAD_SYSTEM_PLATFORM_NAME "windows"
#elif defined(__APPLE__)
    #define THREAD_SYSTEM_MACOS 1
    #define THREAD_SYSTEM_PLATFORM_NAME "macos"
    #include <TargetConditionals.h>
#elif defined(__linux__)
    #define THREAD_SYSTEM_LINUX 1
    #define THREAD_SYSTEM_PLATFORM_NAME "linux"
#else
    #define THREAD_SYSTEM_UNKNOWN_PLATFORM 1
    #define THREAD_SYSTEM_PLATFORM_NAME "unknown"
#endif

// Architecture detection
#if defined(__x86_64__) || defined(_M_X64)
    #define THREAD_SYSTEM_ARCH_X64 1
    #define THREAD_SYSTEM_ARCH_NAME "x86_64"
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define THREAD_SYSTEM_ARCH_ARM64 1
    #define THREAD_SYSTEM_ARCH_NAME "arm64"
#elif defined(__i386__) || defined(_M_IX86)
    #define THREAD_SYSTEM_ARCH_X86 1
    #define THREAD_SYSTEM_ARCH_NAME "x86"
#else
    #define THREAD_SYSTEM_ARCH_UNKNOWN 1
    #define THREAD_SYSTEM_ARCH_NAME "unknown"
#endif

// Feature detection
#if defined(THREAD_SYSTEM_MACOS) && defined(THREAD_SYSTEM_ARCH_ARM64)
    #define THREAD_SYSTEM_HAS_EFFICIENCY_CORES 1
#endif

namespace kcenon::thread::platform {

/**
 * @brief CPU architecture enumeration
 */
enum class cpu_architecture {
    x86,
    x86_64,
    arm64,
    unknown
};

/**
 * @brief Operating system enumeration
 */
enum class os_type {
    windows,
    macos,
    linux_os,
    unknown
};

/**
 * @brief System information structure
 */
struct system_info {
    os_type os;
    cpu_architecture arch;
    uint32_t physical_cores;
    uint32_t logical_cores;
    uint32_t efficiency_cores;  // 0 if not applicable (Apple Silicon only)
    bool is_container;
    bool is_arm64;
    bool has_efficiency_cores;
};

/**
 * @brief Get current CPU architecture
 */
inline cpu_architecture get_architecture() noexcept {
#if defined(THREAD_SYSTEM_ARCH_X64)
    return cpu_architecture::x86_64;
#elif defined(THREAD_SYSTEM_ARCH_ARM64)
    return cpu_architecture::arm64;
#elif defined(THREAD_SYSTEM_ARCH_X86)
    return cpu_architecture::x86;
#else
    return cpu_architecture::unknown;
#endif
}

/**
 * @brief Get current operating system
 */
inline os_type get_os() noexcept {
#if defined(THREAD_SYSTEM_WINDOWS)
    return os_type::windows;
#elif defined(THREAD_SYSTEM_MACOS)
    return os_type::macos;
#elif defined(THREAD_SYSTEM_LINUX)
    return os_type::linux_os;
#else
    return os_type::unknown;
#endif
}

/**
 * @brief Check if running on ARM64 architecture
 */
inline bool is_arm64() noexcept {
#if defined(THREAD_SYSTEM_ARCH_ARM64)
    return true;
#else
    return false;
#endif
}

/**
 * @brief Get physical core count
 */
inline uint32_t get_physical_core_count() noexcept {
    // std::thread::hardware_concurrency returns logical cores
    // For physical cores, platform-specific implementation needed
    return static_cast<uint32_t>(std::thread::hardware_concurrency());
}

/**
 * @brief Get logical core count
 */
inline uint32_t get_logical_core_count() noexcept {
    return static_cast<uint32_t>(std::thread::hardware_concurrency());
}

/**
 * @brief Check if running in container environment
 */
inline bool is_container_environment() noexcept {
#if defined(THREAD_SYSTEM_LINUX)
    // Check for common container indicators
    // This is a simplified check; production code should be more thorough
    return false;  // Would check /proc/1/cgroup in real implementation
#else
    return false;
#endif
}

/**
 * @brief Check if system has efficiency cores (Apple Silicon)
 */
inline bool has_efficiency_cores() noexcept {
#if defined(THREAD_SYSTEM_HAS_EFFICIENCY_CORES)
    return true;
#else
    return false;
#endif
}

/**
 * @brief Get comprehensive system information
 */
inline system_info get_system_info() noexcept {
    system_info info{};
    info.os = get_os();
    info.arch = get_architecture();
    info.physical_cores = get_physical_core_count();
    info.logical_cores = get_logical_core_count();
    info.efficiency_cores = 0;  // Platform-specific
    info.is_container = is_container_environment();
    info.is_arm64 = is_arm64();
    info.has_efficiency_cores = has_efficiency_cores();
    return info;
}

/**
 * @brief Get platform name string
 */
inline const char* get_platform_name() noexcept {
    return THREAD_SYSTEM_PLATFORM_NAME;
}

/**
 * @brief Get architecture name string
 */
inline const char* get_arch_name() noexcept {
    return THREAD_SYSTEM_ARCH_NAME;
}

} // namespace kcenon::thread::platform
