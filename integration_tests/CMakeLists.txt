##################################################
# Integration Tests CMakeLists.txt
#
# Configuration for thread_system integration tests
##################################################

cmake_minimum_required(VERSION 3.16)

# GTest is now provided by parent CMakeLists via FetchContent
# No need to find_package here - targets are already available

# Collect all integration test source files
file(GLOB SCENARIO_TESTS scenarios/*.cpp)
file(GLOB FAILURE_TESTS failures/*.cpp)
file(GLOB PERFORMANCE_TESTS performance/*.cpp)

# Create integration test executable
add_executable(integration_tests
    ${SCENARIO_TESTS}
    ${FAILURE_TESTS}
    ${PERFORMANCE_TESTS}
)

# Include directories
target_include_directories(integration_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/core/base/include
    ${CMAKE_SOURCE_DIR}/core/sync/include
)

# Link libraries
target_link_libraries(integration_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    thread_base
    utilities
)

# Compiler settings
target_compile_features(integration_tests PRIVATE cxx_std_20)

# Add test to CTest
add_test(NAME IntegrationTests COMMAND integration_tests)

# Set test properties
set_tests_properties(IntegrationTests PROPERTIES
    TIMEOUT 300
    LABELS "integration"
)

# Install integration tests (optional)
install(TARGETS integration_tests
    RUNTIME DESTINATION bin/tests
)

message(STATUS "Integration tests configured successfully")
