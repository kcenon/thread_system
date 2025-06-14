##################################################
# ThreadPool Component CMakeLists.txt
# 
# Build configuration for the thread pool library component.
# This component provides an efficient worker thread pool implementation
# for executing tasks in parallel with automatic load balancing.
##################################################

# Define the component name as a project
project(thread_pool)

# Collect all header and source files recursively
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# Create a static library from the collected source files
add_library(${PROJECT_NAME} STATIC ${HEADER_FILES} ${SOURCE_FILES})

# Configure include directory so client code can find headers
target_include_directories(${PROJECT_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/sources
)

# Apply compiler warnings if the function is available
if(COMMAND set_compiler_warnings)
    set_compiler_warnings(${PROJECT_NAME})
endif()

# Setup precompiled headers if supported
if(COMMAND target_precompile_headers_if_supported)
    target_precompile_headers_if_supported(${PROJECT_NAME})
endif()

# Link against dependencies: thread_base for core functionality and logger for logging
target_link_libraries(${PROJECT_NAME} PUBLIC thread_base logger)