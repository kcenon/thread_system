##################################################
# Priority Thread Pool Benchmarks CMakeLists.txt
# 
# Build configuration for priority thread pool benchmark applications.
##################################################

# Define the benchmark group name
project(priority_thread_pool_benchmarks)

# Priority scheduling benchmark
add_executable(priority_scheduling_benchmark
    priority_scheduling_benchmark.cpp
)

# Set common properties for all priority benchmarks
set(PRIORITY_BENCHMARK_TARGETS
    priority_scheduling_benchmark
)

foreach(target ${PRIORITY_BENCHMARK_TARGETS})
    # Link against priority thread pool and dependencies
    if(APPLE)
        target_link_libraries(${target} PRIVATE priority_thread_pool thread_pool logger utilities "-framework CoreFoundation")
    else()
        target_link_libraries(${target} PRIVATE priority_thread_pool thread_pool logger utilities)
    endif()
    
    # Set C++20 standard
    target_compile_features(${target} PRIVATE cxx_std_20)
    
    # Set output directory
    set_target_properties(${target}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endforeach()

# Create a group target for all priority thread pool benchmarks
add_custom_target(priority_thread_pool_benchmarks_target
    DEPENDS ${PRIORITY_BENCHMARK_TARGETS}
    COMMENT "Building priority thread pool benchmarks..."
)

# Add run target for priority thread pool benchmarks
add_custom_target(run_priority_thread_pool_benchmarks
    COMMAND priority_scheduling_benchmark
    DEPENDS priority_thread_pool_benchmarks_target
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running priority thread pool benchmarks..."
)