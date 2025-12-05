# Queue capabilities sample without logger dependency
project(queue_capabilities_sample)

add_executable(${PROJECT_NAME} queue_capabilities_sample.cpp)

# Link with thread_base (which includes core functionality)
target_link_libraries(${PROJECT_NAME} PRIVATE thread_base)

# Include headers from the new structure
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# Set as a sample application
set_target_properties(${PROJECT_NAME} PROPERTIES
    FOLDER "samples"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
