##################################################
# ThreadSystem CMakeLists.txt
#
# Main build configuration for the ThreadSystem library.
# High-performance C++20 thread management system.
##################################################

cmake_minimum_required(VERSION 3.16)

# Note: C++20 modules require CMake 3.28+.
# The THREAD_BUILD_MODULES option is only available when using CMake 3.28 or later.

# Define the project
project(ThreadSystem
    VERSION 0.3.0.0
    DESCRIPTION "High-performance C++20 multithreading framework"
    HOMEPAGE_URL "https://github.com/kcenon/thread_system"
    LANGUAGES CXX
)

##################################################
# Build Mode Detection
##################################################

# Detect if we're being built as a submodule
if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(BUILD_THREADSYSTEM_AS_SUBMODULE ON CACHE BOOL "Build ThreadSystem as submodule" FORCE)
else()
    option(BUILD_THREADSYSTEM_AS_SUBMODULE "Build ThreadSystem as submodule" OFF)
endif()

##################################################
# Options
##################################################

option(BUILD_WITH_COMMON_SYSTEM "Build with common_system for standard interfaces" ON)
option(THREAD_BUILD_INTEGRATION_TESTS "Build integration tests" ON)
option(BUILD_DOCUMENTATION "Build Doxygen documentation" ON)
option(ENABLE_COVERAGE "Enable code coverage" OFF)

# Lock-free queue option (TICKET-001 RESOLVED - Now safe with Hazard Pointers)
option(THREAD_ENABLE_LOCKFREE_QUEUE "Enable lock-free queue (safe with Hazard Pointers)" ON)

# Work-stealing option for improved load balancing
option(THREAD_ENABLE_WORK_STEALING "Enable work-stealing scheduler for load balancing" OFF)

if(NOT BUILD_WITH_COMMON_SYSTEM)
    message(FATAL_ERROR "ThreadSystem now requires common_system. Set BUILD_WITH_COMMON_SYSTEM=ON.")
endif()

if(THREAD_ENABLE_LOCKFREE_QUEUE)
    message(STATUS "Lock-free queue is ENABLED (TICKET-001 resolved with Hazard Pointers)")
endif()

if(THREAD_ENABLE_WORK_STEALING)
    message(STATUS "Work-stealing scheduler is ENABLED")
    add_compile_definitions(THREAD_WORK_STEALING_ENABLED=1)
endif()

# Respect global BUILD_INTEGRATION_TESTS flag if set
if(DEFINED BUILD_INTEGRATION_TESTS)
    if(BUILD_INTEGRATION_TESTS)
        set(_THREAD_BUILD_IT_VALUE ON)
    else()
        set(_THREAD_BUILD_IT_VALUE OFF)
    endif()
    set(THREAD_BUILD_INTEGRATION_TESTS ${_THREAD_BUILD_IT_VALUE} CACHE BOOL "Build integration tests" FORCE)
endif()

##################################################
# C++ Standard Configuration
##################################################

# C++20 minimum requirement
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Detect if compiler supports C++20 features
# Features will be auto-detected in ThreadSystemFeatures.cmake

##################################################
# Include CMake Modules
##################################################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Include existing modules (backward compatibility)
include(CompilerChecks)
include(Coverage)
include(dependency_checker)

# Include new modular CMake files
include(ThreadSystemCompiler)
include(ThreadSystemDependencies)
include(ThreadSystemFeatures)
include(ThreadSystemTargets)
include(ThreadSystemInstall)

##################################################
# Compiler Configuration
##################################################

print_compiler_info()
setup_thread_system_compiler_flags()
setup_static_analysis()

# Perform compiler checks (from CompilerChecks module)
check_compiler_version()
configure_platform_settings()
configure_build_types()
check_required_headers()
check_cpp_stdlib_features()

##################################################
# Feature Detection (must run before dependency detection)
##################################################

check_thread_system_features()

##################################################
# Dependency Detection (depends on feature detection results)
##################################################

find_thread_system_dependencies()

##################################################
# Build Configuration
##################################################

if(NOT BUILD_THREADSYSTEM_AS_SUBMODULE)
    # Standalone build configuration
    setup_build_directories()

    # vcpkg toolchain warning
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        message(WARNING "vcpkg toolchain file not provided. Some dependencies may not be found automatically.")
        message(STATUS "To use vcpkg, provide -DCMAKE_TOOLCHAIN_FILE=<path_to_vcpkg_toolchain>")
    else()
        message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
    endif()

    # Dependency conflict checks
    if(CHECK_DEPENDENCIES)
        check_dependency_conflicts()
    endif()

    # Create targets
    create_thread_system_targets()

    # Add subdirectories based on build structure
    if(USE_LEGACY_BUILD)
        add_legacy_subdirectories()
    endif()

    # Add examples/samples
    add_examples_and_samples()

    # Enable CTest
    enable_testing()

    # Add tests
    add_tests_subdirectory()

    # Add benchmarks
    add_benchmarks_subdirectory()

    # Setup documentation
    setup_documentation()

    # Installation configuration
    setup_thread_system_install()

    # Code coverage
    if(ENABLE_COVERAGE)
        create_coverage_report()
        message(STATUS "")
        message(STATUS "Code coverage is enabled.")
        message(STATUS "To generate coverage report:")
        message(STATUS "  1. Build: cmake -B build -DENABLE_COVERAGE=ON")
        message(STATUS "  2. Run tests: cmake --build build")
        message(STATUS "  3. Generate report: cmake --build build --target coverage")
        message(STATUS "")
    endif()
else()
    # Submodule build configuration
    # Only build core library
    add_subdirectory(interfaces)
    add_subdirectory(utilities)
    add_subdirectory(core)
    # Provide unified aliases for parent projects when building as a submodule.
    # This allows dependents to link against ThreadSystem/thread_system even
    # though only the legacy targets (thread_base/utilities/interfaces) exist.
    if(TARGET thread_base)
        if(NOT TARGET ThreadSystem)
            add_library(ThreadSystem ALIAS thread_base)
        endif()
        if(NOT TARGET thread_system)
            add_library(thread_system ALIAS thread_base)
        endif()
    endif()
    message(STATUS "ThreadSystem building as submodule - samples and tests disabled")
endif()

##################################################
# C++20 Module Support (Phase 2)
##################################################
# Enable C++20 modules with CMake 3.28+ when building with module-capable compilers.
# This provides an alternative to the header-based interface.
#
# Usage:
#   cmake -DTHREAD_BUILD_MODULES=ON ..
#
# Requirements:
#   - CMake 3.28 or later
#   - Clang 16+, GCC 14+, or MSVC 2022 17.4+
#   - common_system with module support enabled
##################################################

option(THREAD_BUILD_MODULES "Build C++20 module version of thread_system" OFF)

if(THREAD_BUILD_MODULES)
    # Check CMake version
    if(CMAKE_VERSION VERSION_LESS "3.28")
        message(WARNING "C++20 modules require CMake 3.28+. Current version: ${CMAKE_VERSION}. Disabling module build.")
        set(THREAD_BUILD_MODULES OFF)
    else()
        message(STATUS "C++20 module build enabled")

        # Create module library target
        add_library(thread_system_modules)
        add_library(kcenon::thread_modules ALIAS thread_system_modules)

        # Set C++20 standard for module target
        target_compile_features(thread_system_modules PUBLIC cxx_std_20)

        # Enable module scanning
        set_target_properties(thread_system_modules PROPERTIES
            CXX_SCAN_FOR_MODULES ON
        )

        # Add module source files
        target_sources(thread_system_modules
            PUBLIC FILE_SET CXX_MODULES
            FILES
                # Primary module
                src/modules/thread.cppm
                # Tier 1: Core partition
                src/modules/core.cppm
                # Tier 2: Queue partition
                src/modules/queue.cppm
        )

        # Include directories for module target
        target_include_directories(thread_system_modules PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<INSTALL_INTERFACE:include>
        )

        # Link to common_system modules if available
        if(TARGET kcenon::common_modules)
            target_link_libraries(thread_system_modules PUBLIC kcenon::common_modules)
            message(STATUS "Linking thread_system_modules to kcenon::common_modules")
        elseif(TARGET kcenon::common)
            target_link_libraries(thread_system_modules PUBLIC kcenon::common)
            message(STATUS "Linking thread_system_modules to kcenon::common (header-only)")
        endif()

        # Link to ThreadSystem library for implementation
        if(TARGET ThreadSystem)
            target_link_libraries(thread_system_modules PUBLIC ThreadSystem)
        elseif(TARGET thread_base)
            target_link_libraries(thread_system_modules PUBLIC thread_base)
        endif()

        # Compile definitions
        target_compile_definitions(thread_system_modules PUBLIC
            KCENON_USE_MODULES=1
        )

        message(STATUS "C++20 module target: thread_system_modules")
    endif()
endif()

##################################################
# Build Summary
##################################################

message(STATUS "========================================")
message(STATUS "ThreadSystem Build Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build mode: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Submodule: ${BUILD_THREADSYSTEM_AS_SUBMODULE}")
message(STATUS "  common_system: ${BUILD_WITH_COMMON_SYSTEM}")
message(STATUS "  std::format: ${USE_STD_FORMAT}")
message(STATUS "  Coverage: ${ENABLE_COVERAGE}")
message(STATUS "  Lock-free queue: ${THREAD_ENABLE_LOCKFREE_QUEUE}")
if(THREAD_ENABLE_LOCKFREE_QUEUE)
    message(STATUS "    TICKET-001 resolved: Safe with Hazard Pointers")
endif()
message(STATUS "  Work-stealing: ${THREAD_ENABLE_WORK_STEALING}")
message(STATUS "  C++20 Modules: ${THREAD_BUILD_MODULES}")
message(STATUS "========================================")
