##################################################
# Utilities Component CMakeLists.txt
# 
# Build configuration for the utilities library component.
# This component provides common helper functions, data structures,
# and utility classes used throughout the ThreadSystem library.
##################################################

# Define the component name as a project
project(utilities)

# Collect all header and source files in this directory
file(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# Create a static library from the collected source files
add_library(${PROJECT_NAME} STATIC ${HEADER_FILES} ${SOURCE_FILES})

# Configure include directory so client code can find headers
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# String formatting library configuration
if(NOT HAS_STD_FORMAT OR NOT SET_STD_FORMAT)
    # If std::format is not available or disabled, use the fmt library instead
    find_package(fmt CONFIG REQUIRED)  # Find the fmt library (via vcpkg)
    target_link_libraries(${PROJECT_NAME} PUBLIC fmt::fmt-header-only)  # Link to the header-only version
endif()

# Character set conversion library
find_package(Iconv REQUIRED)  # Find the Iconv library for character encoding conversion
target_link_libraries(${PROJECT_NAME} PUBLIC Iconv::Iconv)  # Link against Iconv