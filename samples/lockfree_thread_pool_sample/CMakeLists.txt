##################################################
# Lockfree Thread Pool Sample (New) CMakeLists.txt
# 
# Build configuration for the new lockfree thread pool sample application.
# This sample demonstrates the usage of the high-performance lock-free
# thread pool implementation with performance comparisons.
##################################################

# Define the sample name as a project
project(lockfree_thread_pool_sample)

# Create an executable from the sample source file
add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/lockfree_thread_pool_sample.cpp)

# Link against required libraries
# thread_pool: Contains both standard and lock-free thread pool implementations
# thread_base: Core threading components including lock-free queue
# logger: For logging functionality
# utilities: Helper functions and formatters
target_link_libraries(${PROJECT_NAME} PRIVATE 
    thread_pool 
    thread_base 
    logger 
    utilities
)

# Set the output directory for the executable
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Apply compiler warnings if the function is available
if(COMMAND set_compiler_warnings)
    set_compiler_warnings(${PROJECT_NAME})
endif()

# Add a custom target to run the sample
add_custom_target(run_${PROJECT_NAME}
    COMMAND $<TARGET_FILE:${PROJECT_NAME}>
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running lock-free thread pool sample..."
)