##################################################
# ThreadBase Component CMakeLists.txt
# 
# Build configuration for the thread base library component.
# This component provides fundamental threading abstractions and primitives
# that serve as the foundation for higher-level thread management facilities.
##################################################

# Define the component name as a project
project(thread_base)

# Collect all header and source files in this directory
file(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

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

# Platform-specific threading library configuration
if(APPLE)
    # On macOS, pthread is automatically linked by the system
    # We only need to link our own utilities library
    target_link_libraries(${PROJECT_NAME} PUBLIC utilities)
else()
    # On other platforms, we need to explicitly find and link the threading library
    set(THREADS_PREFER_PTHREAD_FLAG ON)  # Prefer -pthread flag if available
    find_package(Threads REQUIRED)       # Find the threading library
    
    # Link against both our utilities and the system's threading library
    target_link_libraries(${PROJECT_NAME} PUBLIC 
        utilities           # Our utility functions
        Threads::Threads    # System threading library (usually pthread)
    )
endif()