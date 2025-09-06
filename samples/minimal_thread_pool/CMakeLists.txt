# Minimal thread pool sample without logger dependency
project(minimal_thread_pool)

add_executable(${PROJECT_NAME} minimal_thread_pool.cpp)

# Link only with thread_pool (which includes thread_base and interfaces)
target_link_libraries(${PROJECT_NAME} PRIVATE thread_pool)

# Include new headers for core and thread_pool
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../implementations/thread_pool/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/base/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/jobs/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/sync/include
)

# Set as a sample application
set_target_properties(${PROJECT_NAME} PROPERTIES
    FOLDER "samples"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
