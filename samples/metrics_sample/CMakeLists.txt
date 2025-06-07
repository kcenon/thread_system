# Metrics Sample CMakeLists.txt
#
# Demonstrates comprehensive metrics collection for thread pools

# Define the project name
project(metrics_sample)

# Create the executable from the source file
add_executable(${PROJECT_NAME} metrics_sample.cpp)

# Link against the required libraries
target_link_libraries(${PROJECT_NAME} 
    PRIVATE
        thread_pool      # For monitored_thread_pool
        typed_thread_pool # For monitored_typed_thread_pool
        metrics          # For metrics functionality
        logger           # For logging support
        utilities        # For utilities
)

# Add a custom target to run the sample
add_custom_target(run_metrics_sample
    COMMAND ${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Running metrics sample..."
)