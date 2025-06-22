##################################################
# TypedLockfreeJobQueueSample CMakeLists.txt
# 
# Demonstrates the usage of typed_lockfree_job_queue_t with performance comparison
##################################################

project(typed_lockfree_job_queue_sample)

# Create executable
add_executable(${PROJECT_NAME} typed_lockfree_job_queue_sample.cpp)

# Link required libraries
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        utilities
        thread_base
        thread_pool
        typed_thread_pool
        logger
)

# Add include directories
target_include_directories(${PROJECT_NAME} 
    PRIVATE 
        ${CMAKE_SOURCE_DIR}/sources
)

# Set C++ standard
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Platform-specific settings
if(APPLE)
    target_compile_options(${PROJECT_NAME} PRIVATE -pthread)
    target_link_options(${PROJECT_NAME} PRIVATE -pthread)
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
endif()

# Install the sample executable
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/samples
)