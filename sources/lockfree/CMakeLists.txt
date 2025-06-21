##################################################
# Lock-Free Components CMakeLists.txt
# 
# This script builds the lock-free components of the ThreadSystem library.
# Includes high-performance lock-free data structures and algorithms.
##################################################

cmake_minimum_required(VERSION 3.10)
project(lockfree VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)

# Source files
set(HEADERS
    queues/lockfree_job_queue.h
    queues/lockfree_job_queue.tpp
    memory/hazard_pointer.h
    memory/hazard_pointer.tpp
    memory/node_pool.h
    memory/node_pool.tpp
)

set(SOURCES
    # Template instantiations if needed
)

# Create library (header-only for templates)
add_library(lockfree INTERFACE)

target_include_directories(lockfree INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(lockfree INTERFACE
    thread_base
    utilities
)

# Export library
install(TARGETS lockfree
    EXPORT ThreadSystemTargets
    INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY queues memory
    DESTINATION include/lockfree
    FILES_MATCHING PATTERN "*.h" PATTERN "*.tpp"
)