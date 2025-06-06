##################################################
# Typed Thread Pool Benchmarks CMakeLists.txt
# 
# Build configuration for typed thread pool benchmark applications.
##################################################

# Define the benchmark group name
project(typed_thread_pool_benchmarks)

# Typed scheduling benchmark
add_executable(typed_scheduling_benchmark
    typed_scheduling_benchmark.cpp
)

# Set common properties for all priority benchmarks
set(TYPED_BENCHMARK_TARGETS
    typed_scheduling_benchmark
)

foreach(target ${TYPED_BENCHMARK_TARGETS})
    # Link against priority thread pool and dependencies
    if(APPLE)
        target_link_libraries(${target} PRIVATE typed_thread_pool thread_pool logger utilities "-framework CoreFoundation")
    else()
        target_link_libraries(${target} PRIVATE typed_thread_pool thread_pool logger utilities)
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
add_custom_target(typed_thread_pool_benchmarks_target
    DEPENDS ${TYPED_BENCHMARK_TARGETS}
    COMMENT "Building typed thread pool benchmarks..."
)

# Add run target for priority thread pool benchmarks
add_custom_target(run_typed_thread_pool_benchmarks
    COMMAND typed_scheduling_benchmark
    DEPENDS typed_thread_pool_benchmarks_target
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running typed thread pool benchmarks..."
)