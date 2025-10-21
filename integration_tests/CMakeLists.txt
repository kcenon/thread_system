##################################################
# Integration Tests CMakeLists.txt
#
# Layered Test Architecture:
# - Smoke: Quick sanity checks (< 30s)
# - Integration: Real-world scenarios (< 2min)
# - Performance: Benchmarks (< 10min, optional)
# - Stress: Long-running stability (> 30min, optional)
##################################################

cmake_minimum_required(VERSION 3.16)

##################################################
# Find GTest
##################################################

# Try to find GTest (vcpkg first, then system)
# Note: If not found, parent CMakeLists.txt will provide it via FetchContent
find_package(GTest CONFIG QUIET)
if(NOT GTest_FOUND)
    find_package(GTest QUIET)
endif()

# GTest targets (GTest::gtest, GTest::gtest_main) should be available
# either from find_package above or from parent CMakeLists via FetchContent

##################################################
# Test Source Collections
##################################################

# Smoke tests (fastest - always run)
file(GLOB SMOKE_SOURCES smoke/*.cpp)

# Integration tests (moderate - always run)
file(GLOB INTEGRATION_SOURCES integration/*.cpp)

# Performance benchmarks (slower - optional)
file(GLOB PERFORMANCE_SOURCES performance/*.cpp)

# Stress tests (slowest - manual only)
file(GLOB STRESS_SOURCES stress/*.cpp)

##################################################
# Common Settings for All Test Executables
##################################################

function(configure_test_executable TARGET_NAME)
    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/core/base/include
        ${CMAKE_SOURCE_DIR}/core/sync/include
    )

    target_link_libraries(${TARGET_NAME} PRIVATE
        GTest::gtest
        GTest::gtest_main
        thread_base
        utilities
    )

    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)

    install(TARGETS ${TARGET_NAME}
        RUNTIME DESTINATION bin/tests
    )
endfunction()

##################################################
# Layer 1: Smoke Tests (Always Built)
##################################################

if(SMOKE_SOURCES)
    add_executable(smoke_tests ${SMOKE_SOURCES})
    configure_test_executable(smoke_tests)

    add_test(NAME SmokeTests COMMAND smoke_tests)
    set_tests_properties(SmokeTests PROPERTIES
        TIMEOUT 30
        LABELS "smoke;fast;critical"
    )

    message(STATUS "✓ Smoke tests configured (${CMAKE_CURRENT_LIST_LINE} tests, < 30s)")
endif()

##################################################
# Layer 2: Integration Tests (Always Built)
##################################################

if(INTEGRATION_SOURCES)
    add_executable(integration_tests ${INTEGRATION_SOURCES})
    configure_test_executable(integration_tests)

    add_test(NAME IntegrationTests COMMAND integration_tests)
    set_tests_properties(IntegrationTests PROPERTIES
        TIMEOUT 120
        LABELS "integration;moderate"
    )

    message(STATUS "✓ Integration tests configured (real scenarios, < 2min)")
endif()

##################################################
# Layer 3: Performance Tests (Optional - Default ON)
##################################################

option(BUILD_PERFORMANCE_TESTS "Build performance benchmark tests" ON)

if(BUILD_PERFORMANCE_TESTS AND PERFORMANCE_SOURCES)
    add_executable(performance_tests ${PERFORMANCE_SOURCES})
    configure_test_executable(performance_tests)

    add_test(NAME PerformanceTests COMMAND performance_tests)
    set_tests_properties(PerformanceTests PROPERTIES
        TIMEOUT 600
        LABELS "performance;slow;benchmark"
    )

    message(STATUS "✓ Performance tests configured (benchmarks, < 10min)")
else()
    message(STATUS "⊘ Performance tests DISABLED (use -DBUILD_PERFORMANCE_TESTS=ON)")
endif()

##################################################
# Layer 4: Stress Tests (Optional - Default OFF)
##################################################

option(BUILD_STRESS_TESTS "Build long-running stress tests" OFF)

if(BUILD_STRESS_TESTS AND STRESS_SOURCES)
    add_executable(stress_tests ${STRESS_SOURCES})
    configure_test_executable(stress_tests)

    add_test(NAME StressTests COMMAND stress_tests)
    set_tests_properties(StressTests PROPERTIES
        TIMEOUT 3600
        LABELS "stress;very_slow;manual"
    )

    message(STATUS "✓ Stress tests configured (long-running, > 30min)")
else()
    message(STATUS "⊘ Stress tests DISABLED (use -DBUILD_STRESS_TESTS=ON)")
endif()

##################################################
# Summary
##################################################

message(STATUS "")
message(STATUS "========================================")
message(STATUS "Test Pyramid Summary:")
message(STATUS "  Smoke Tests:       ${SMOKE_SOURCES}")
message(STATUS "  Integration Tests: ${INTEGRATION_SOURCES}")
message(STATUS "  Performance Tests: ${BUILD_PERFORMANCE_TESTS}")
message(STATUS "  Stress Tests:      ${BUILD_STRESS_TESTS}")
message(STATUS "")
message(STATUS "Quick commands:")
message(STATUS "  All fast tests:     ctest -L 'smoke|integration'")
message(STATUS "  With performance:   ctest -L 'smoke|integration|performance'")
message(STATUS "  All tests:          ctest")
message(STATUS "========================================")
message(STATUS "")
