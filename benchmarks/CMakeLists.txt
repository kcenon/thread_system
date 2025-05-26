##################################################
# Thread System Benchmarks CMakeLists.txt
# 
# Build configuration for all benchmark applications.
# Benchmarks are organized by component for better structure and maintainability.
##################################################

# Add subdirectories for each component's benchmarks
add_subdirectory(logger_benchmarks)
add_subdirectory(thread_pool_benchmarks)
add_subdirectory(priority_thread_pool_benchmarks)

# Create a master target that builds all benchmarks
add_custom_target(benchmarks
    COMMENT "Building all thread system benchmarks..."
)

# Add dependencies to the master benchmarks target
add_dependencies(benchmarks
    logger_benchmarks_target
    thread_pool_benchmarks_target
    priority_thread_pool_benchmarks_target
)

# Add convenience target to run all benchmarks
add_custom_target(run_all_benchmarks
    COMMENT "Running all thread system benchmarks..."
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Add dependencies for running benchmarks
add_dependencies(run_all_benchmarks benchmarks)