##################################################
# TypedThreadPool Component CMakeLists.txt
# 
# Build configuration for the type-based thread pool library component.
# This component extends the base thread pool with task type specialization capabilities,
# allowing different types of tasks to be processed by specialized worker threads.
##################################################

# Define the component name as a project
project(typed_thread_pool)

# Collect all header and template implementation files recursively
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.tpp)

# Create an interface library (header-only implementation)
# Interface libraries don't produce any build artifacts themselves
# but propagate compile definitions, include directories, and dependencies
add_library(${PROJECT_NAME} INTERFACE)

# INTERFACE libraries should not have source files added directly
# The headers and templates are included through include directories
# This avoids INTERFACE_SOURCES path issues on Windows/Visual Studio

# Configure include directories with proper path structure for header-only library
# INTERFACE: Only for consumers of this library (header-only)
# Generator expressions ensure correct paths for build and install trees
target_include_directories(${PROJECT_NAME} INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/sources>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/thread_system>
    $<INSTALL_INTERFACE:include/thread_system/typed_thread_pool>
)

# Link against required dependencies:
# - thread_base: For core threading primitives
# - thread_pool: This component extends the thread pool implementation
# - logger: For diagnostic and error logging
target_link_libraries(${PROJECT_NAME} INTERFACE thread_base thread_pool logger)