##################################################
# TypedThreadPool Component CMakeLists.txt
# 
# Build configuration for the type-based thread pool library component.
# This component extends the base thread pool with task type specialization capabilities,
# allowing different types of tasks to be processed by specialized worker threads.
##################################################

# Define the component name as a project
project(typed_thread_pool)

# Collect all header and template implementation files in this directory
file(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.tpp)

# Create an interface library (header-only implementation)
# Interface libraries don't produce any build artifacts themselves
# but propagate compile definitions, include directories, and dependencies
add_library(${PROJECT_NAME} INTERFACE)

# Add the source files to the interface target for IDE integration
target_sources(${PROJECT_NAME} INTERFACE ${HEADER_FILES} ${SOURCE_FILES})

# Configure include directory so client code can find headers
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Link against required dependencies:
# - thread_base: For core threading primitives
# - thread_pool: This component extends the thread pool implementation
# - logger: For diagnostic and error logging
target_link_libraries(${PROJECT_NAME} INTERFACE thread_base thread_pool logger)