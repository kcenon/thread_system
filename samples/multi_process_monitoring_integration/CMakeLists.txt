cmake_minimum_required(VERSION 3.16)

# Multi-process monitoring integration sample
add_executable(multi_process_monitoring_integration multi_process_monitoring_integration.cpp)
target_link_libraries(multi_process_monitoring_integration 
    PRIVATE 
        thread_pool
        thread_base
        interfaces
        utilities
)

# Add include directories
target_include_directories(multi_process_monitoring_integration 
    PRIVATE 
        ${PROJECT_SOURCE_DIR}/sources
)

# Set C++ standard
set_target_properties(multi_process_monitoring_integration PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)