##################################################
# Logger Benchmarks CMakeLists.txt
# 
# Build configuration for logger benchmark applications.
##################################################

# Define the benchmark group name
project(logger_benchmarks)

# Logger performance benchmark
add_executable(logger_benchmark
    logger_benchmark.cpp
)

# NEW: Comprehensive logging throughput benchmark
add_executable(logging_throughput_benchmark
    logging_throughput_benchmark.cpp
)

# NEW: Lock-free logger performance benchmark
add_executable(lockfree_logger_benchmark
    lockfree_logger_benchmark.cpp
)

# NEW: Logger comparison benchmark (Thread System vs spdlog)
add_executable(logger_comparison_benchmark
    logger_comparison_benchmark.cpp
)

# Set common properties for all logger benchmarks
set(LOGGER_BENCHMARK_TARGETS
    logger_benchmark
    logging_throughput_benchmark
    lockfree_logger_benchmark
    logger_comparison_benchmark
)

# Check for spdlog availability
find_package(spdlog CONFIG QUIET)
if(spdlog_FOUND)
    message(STATUS "spdlog found - enabling spdlog comparison benchmarks")
    add_compile_definitions(HAS_SPDLOG)
else()
    message(STATUS "spdlog not found - spdlog comparison benchmarks will be disabled")
    message(STATUS "To enable spdlog benchmarks, install spdlog via vcpkg: vcpkg install spdlog")
endif()

foreach(target ${LOGGER_BENCHMARK_TARGETS})
    # Link against logger and utilities
    if(APPLE)
        target_link_libraries(${target} PRIVATE logger utilities benchmark::benchmark "-framework CoreFoundation")
    else()
        target_link_libraries(${target} PRIVATE logger utilities benchmark::benchmark)
    endif()
    
    # Link spdlog if available and this is the comparison benchmark
    if(spdlog_FOUND AND ${target} STREQUAL "logger_comparison_benchmark")
        target_link_libraries(${target} PRIVATE spdlog::spdlog)
    endif()
    
    # Set C++20 standard
    target_compile_features(${target} PRIVATE cxx_std_20)
    
    # Set output directory
    set_target_properties(${target}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endforeach()

# Create a group target for all logger benchmarks
add_custom_target(logger_benchmarks_target
    DEPENDS ${LOGGER_BENCHMARK_TARGETS}
    COMMENT "Building logger benchmarks..."
)

# Add run target for logger benchmarks
add_custom_target(run_logger_benchmarks
    COMMAND logger_benchmark
    COMMAND logging_throughput_benchmark
    COMMAND lockfree_logger_benchmark
    DEPENDS logger_benchmarks_target
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running logger benchmarks..."
)