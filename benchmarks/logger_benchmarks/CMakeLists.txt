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

# Set common properties for all logger benchmarks
set(LOGGER_BENCHMARK_TARGETS
    logger_benchmark
    logging_throughput_benchmark
)

foreach(target ${LOGGER_BENCHMARK_TARGETS})
    # Link against logger and utilities
    if(APPLE)
        target_link_libraries(${target} PRIVATE logger utilities "-framework CoreFoundation")
    else()
        target_link_libraries(${target} PRIVATE logger utilities)
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
    DEPENDS logger_benchmarks_target
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running logger benchmarks..."
)