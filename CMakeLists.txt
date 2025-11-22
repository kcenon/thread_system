##################################################
# ThreadSystem CMakeLists.txt
#
# Main build configuration for the ThreadSystem library.
# High-performance C++20 thread management system.
##################################################

cmake_minimum_required(VERSION 3.16)

# Define the project
project(ThreadSystem
    VERSION 1.0.0
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

# Lock-free queue option (UNSAFE - see KNOWN_ISSUES.md)
option(THREAD_ENABLE_LOCKFREE_QUEUE "Enable lock-free queue (UNSAFE - TLS bug exists)" OFF)

if(NOT BUILD_WITH_COMMON_SYSTEM)
    message(FATAL_ERROR "ThreadSystem now requires common_system. Set BUILD_WITH_COMMON_SYSTEM=ON.")
endif()

if(THREAD_ENABLE_LOCKFREE_QUEUE)
    if(NOT THREAD_ALLOW_UNSAFE_LOCKFREE_QUEUE)
        message(FATAL_ERROR "THREAD_ENABLE_LOCKFREE_QUEUE is enabled but UNSAFE. The lock-free queue has a CRITICAL TLS bug. Set THREAD_ALLOW_UNSAFE_LOCKFREE_QUEUE=ON to force enable it for testing.")
    else()
        message(WARNING "")
        message(WARNING "========================================")
        message(WARNING "THREAD_ENABLE_LOCKFREE_QUEUE is ENABLED (Forced)")
        message(WARNING "")
        message(WARNING "The lock-free queue has a CRITICAL TLS bug that causes segmentation faults.")
        message(WARNING "DO NOT USE IN PRODUCTION. This option is for TESTING ONLY.")
        message(WARNING "See KNOWN_ISSUES.md for details.")
        message(WARNING "========================================")
        message(WARNING "")
    endif()
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
include(FindOrFetch)

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
    message(STATUS "ThreadSystem building as submodule - samples and tests disabled")
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
    message(STATUS "    ⚠️  WARNING: Lock-free queue is UNSAFE for production!")
endif()
message(STATUS "========================================")
