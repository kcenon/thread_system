# Integration Tests CMakeLists.txt for Thread System
# Phase 4 T4.1 Task: Integration Test Enhancement

cmake_minimum_required(VERSION 3.20)

# Project for integration tests
project(integration_tests
    VERSION 1.0.0
    DESCRIPTION "Integration tests for thread system dependency improvements"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common_interfaces
    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/base/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../implementations/thread_pool/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../interfaces
)

# Common compile definitions
add_compile_definitions(
    GTEST_HAS_PTHREAD=1
)

# Test source files
set(INTEGRATION_TEST_SOURCES
    test_dependency_injection.cpp
    test_interface_compliance.cpp
)

# Create integration test executable
add_executable(integration_test_suite ${INTEGRATION_TEST_SOURCES})

# Link libraries
target_link_libraries(integration_test_suite
    PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        Threads::Threads
        thread_pool
        thread_base
        utilities
        interfaces
)

# Compiler-specific settings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(integration_test_suite PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -fno-omit-frame-pointer
    )
    
    # AddressSanitizer for memory safety testing
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(integration_test_suite PRIVATE
            -fsanitize=address
            -fsanitize=undefined
        )
        target_link_options(integration_test_suite PRIVATE
            -fsanitize=address
            -fsanitize=undefined
        )
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(integration_test_suite PRIVATE
        /W4
        /permissive-
    )
endif()

# Test discovery for CTest
include(GoogleTest)
gtest_discover_tests(integration_test_suite
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTIES
        TIMEOUT 300
        DISCOVERY_TIMEOUT 60
)

# Custom test targets for specific test categories
add_custom_target(run_dependency_injection_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "DependencyInjectionTest"
    DEPENDS integration_test_suite
    COMMENT "Running dependency injection tests"
)

add_custom_target(run_interface_compliance_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "InterfaceComplianceTest"
    DEPENDS integration_test_suite
    COMMENT "Running interface compliance tests"
)

add_custom_target(run_all_integration_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS integration_test_suite
    COMMENT "Running all integration tests"
)

# Memory leak detection target (with Valgrind if available)
find_program(VALGRIND_PROGRAM valgrind)
if(VALGRIND_PROGRAM AND NOT WIN32)
    add_custom_target(run_integration_tests_with_valgrind
        COMMAND ${VALGRIND_PROGRAM} 
            --tool=memcheck 
            --leak-check=full 
            --show-leak-kinds=all 
            --track-origins=yes 
            --verbose 
            --error-exitcode=1
            $<TARGET_FILE:integration_test_suite>
        DEPENDS integration_test_suite
        COMMENT "Running integration tests with Valgrind memory checking"
    )
endif()

# Performance test target
add_custom_target(run_integration_performance_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "PerformanceTest"
    DEPENDS integration_test_suite
    COMMENT "Running integration performance tests"
)

# Coverage target (if gcov is available)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_program(GCOV_PROGRAM gcov)
    find_program(LCOV_PROGRAM lcov)
    find_program(GENHTML_PROGRAM genhtml)
    
    if(GCOV_PROGRAM AND LCOV_PROGRAM AND GENHTML_PROGRAM)
        target_compile_options(integration_test_suite PRIVATE --coverage)
        target_link_options(integration_test_suite PRIVATE --coverage)
        
        add_custom_target(integration_test_coverage
            COMMAND ${LCOV_PROGRAM} --capture --directory . --output-file coverage.info
            COMMAND ${LCOV_PROGRAM} --remove coverage.info '/usr/*' --output-file coverage.info
            COMMAND ${LCOV_PROGRAM} --list coverage.info
            COMMAND ${GENHTML_PROGRAM} coverage.info --output-directory coverage_html
            DEPENDS integration_test_suite
            COMMENT "Generating integration test coverage report"
        )
    endif()
endif()

# Install integration test executable (optional)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    install(TARGETS integration_test_suite
        RUNTIME DESTINATION bin/tests
        COMPONENT Testing
    )
endif()

# Print configuration summary
message(STATUS "Integration Tests Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
if(VALGRIND_PROGRAM)
    message(STATUS "  Valgrind: Found at ${VALGRIND_PROGRAM}")
else()
    message(STATUS "  Valgrind: Not found")
endif()