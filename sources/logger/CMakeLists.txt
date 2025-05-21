##################################################
# Logger Component CMakeLists.txt
# 
# Build configuration for the thread-safe logging component.
# This component provides formatted, categorized logging functionality
# with configurable output targets and severity levels.
##################################################

# Define the component name as a project
project(logger)

# Collect all header and source files in this directory
file(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# Create a static library from the collected source files
add_library(${PROJECT_NAME} STATIC ${HEADER_FILES} ${SOURCE_FILES})

# Configure include directory so client code can find headers
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link against dependencies:
# - thread_base: For thread-safe logging operations
# - utilities: For string formatting and other helper functions
target_link_libraries(${PROJECT_NAME} PUBLIC thread_base utilities)