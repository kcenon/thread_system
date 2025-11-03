# Job cancellation system demonstration
project(job_cancellation_example)

add_executable(${PROJECT_NAME} job_cancellation_example.cpp)

# Link with ThreadSystem
target_link_libraries(${PROJECT_NAME} PRIVATE ThreadSystem)

# Include headers
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# Set as a sample application
set_target_properties(${PROJECT_NAME} PROPERTIES
    FOLDER "examples"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
