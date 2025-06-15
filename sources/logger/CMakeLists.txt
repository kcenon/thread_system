##################################################
# Logger Component CMakeLists.txt
# 
# Build configuration for the thread-safe logging component.
# This component provides formatted, categorized logging functionality
# with configurable output targets and severity levels.
##################################################

# Define the component name as a project
project(logger)

# Collect all header and source files recursively
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# Create a static library from the collected source files
add_library(${PROJECT_NAME} STATIC ${HEADER_FILES} ${SOURCE_FILES})

# Configure include directories with proper path structure
# PUBLIC: For both building this library and consuming it
# Generator expressions ensure correct paths for build and install trees
target_include_directories(${PROJECT_NAME} PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/sources>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/thread_system>
    $<INSTALL_INTERFACE:include/thread_system/logger>
)

# Apply compiler warnings if the function is available
if(COMMAND set_compiler_warnings)
    set_compiler_warnings(${PROJECT_NAME})
endif()

# Setup precompiled headers if supported
if(COMMAND target_precompile_headers_if_supported)
    target_precompile_headers_if_supported(${PROJECT_NAME})
endif()

# Link against dependencies:
# - thread_base: For thread-safe logging operations
# - utilities: For string formatting and other helper functions
target_link_libraries(${PROJECT_NAME} PUBLIC thread_base utilities)