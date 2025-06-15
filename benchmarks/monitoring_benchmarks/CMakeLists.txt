# Monitoring Benchmarks CMakeLists.txt

# Create the monitoring overhead benchmark executable
add_executable(monitoring_overhead_benchmark
    monitoring_overhead_benchmark.cpp
)

# Set target properties
set_target_properties(monitoring_overhead_benchmark PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Link required libraries
target_link_libraries(monitoring_overhead_benchmark PRIVATE
    utilities
    thread_base
    thread_pool
    logger
    monitoring
    ${CMAKE_THREAD_LIBS_INIT}
)

# Include directories
target_include_directories(monitoring_overhead_benchmark PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../sources
)

# Platform-specific linking
if(WIN32)
    target_link_libraries(monitoring_overhead_benchmark PRIVATE psapi)
elseif(UNIX)
    target_link_libraries(monitoring_overhead_benchmark PRIVATE pthread)
endif()

# Compiler-specific options for optimization
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(monitoring_overhead_benchmark PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic
        -O3                # Maximum optimization for accurate benchmarks
        -DNDEBUG          # Disable assertions
    )
elseif(MSVC)
    target_compile_options(monitoring_overhead_benchmark PRIVATE 
        /W4
        /O2               # Maximum optimization
        /DNDEBUG         # Disable assertions
    )
endif()

# Set output directory
set_target_properties(monitoring_overhead_benchmark PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

message(STATUS "✅ Monitoring benchmark configured")
message(STATUS "   Target: monitoring_overhead_benchmark")
message(STATUS "   Purpose: Measure monitoring system overhead")
message(STATUS "   Features: Multiple collection intervals, throughput comparison")