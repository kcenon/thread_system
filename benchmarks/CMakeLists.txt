# Benchmarks for Thread System

# Core performance benchmark
add_executable(thread_pool_benchmark
    thread_pool_benchmark.cpp
)

target_link_libraries(thread_pool_benchmark
    PRIVATE
    thread_base
    thread_pool
    priority_thread_pool
    logger
    utilities
)

# Memory usage benchmark
add_executable(memory_benchmark
    memory_benchmark.cpp
)

target_link_libraries(memory_benchmark
    PRIVATE
    thread_base
    thread_pool
    priority_thread_pool
    logger
    utilities
)

# Logger performance benchmark
add_executable(logger_benchmark
    logger_benchmark.cpp
)

target_link_libraries(logger_benchmark
    PRIVATE
    logger
    utilities
)

# Real-world scenarios benchmark
add_executable(real_world_benchmark
    real_world_benchmark.cpp
)

target_link_libraries(real_world_benchmark
    PRIVATE
    thread_base
    thread_pool
    priority_thread_pool
    logger
    utilities
)

# Stress test benchmark
add_executable(stress_test_benchmark
    stress_test_benchmark.cpp
)

target_link_libraries(stress_test_benchmark
    PRIVATE
    thread_base
    thread_pool
    priority_thread_pool
    logger
    utilities
)

# Comparison with other libraries
add_executable(comparison_benchmark
    comparison_benchmark.cpp
)

target_link_libraries(comparison_benchmark
    PRIVATE
    thread_base
    thread_pool
    priority_thread_pool
    logger
    utilities
)

# Check for OpenMP
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(comparison_benchmark PRIVATE OpenMP::OpenMP_CXX)
    target_compile_definitions(comparison_benchmark PRIVATE _OPENMP)
endif()

# Detailed throughput analysis
add_executable(throughput_detailed_benchmark
    throughput_detailed_benchmark.cpp
)

target_link_libraries(throughput_detailed_benchmark
    PRIVATE
    thread_base
    thread_pool
    priority_thread_pool
    logger
    utilities
)

# Set output directory for all benchmarks
set_target_properties(
    thread_pool_benchmark 
    memory_benchmark 
    logger_benchmark
    real_world_benchmark
    stress_test_benchmark
    comparison_benchmark
    throughput_detailed_benchmark
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Add benchmark target that builds all benchmarks
add_custom_target(benchmarks
    DEPENDS 
        thread_pool_benchmark 
        memory_benchmark 
        logger_benchmark
        real_world_benchmark
        stress_test_benchmark
        comparison_benchmark
        throughput_detailed_benchmark
)

# Add individual run targets
add_custom_target(run_core_benchmarks
    COMMAND thread_pool_benchmark
    COMMAND memory_benchmark
    COMMAND logger_benchmark
    DEPENDS thread_pool_benchmark memory_benchmark logger_benchmark
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running core benchmarks..."
)

add_custom_target(run_advanced_benchmarks
    COMMAND real_world_benchmark
    COMMAND stress_test_benchmark
    COMMAND comparison_benchmark
    DEPENDS real_world_benchmark stress_test_benchmark comparison_benchmark
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running advanced benchmarks..."
)

# Add run_benchmarks target that runs all benchmarks
add_custom_target(run_benchmarks
    COMMAND thread_pool_benchmark
    COMMAND memory_benchmark
    COMMAND logger_benchmark
    COMMAND real_world_benchmark
    COMMAND stress_test_benchmark
    COMMAND comparison_benchmark
    DEPENDS benchmarks
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running all benchmarks..."
)