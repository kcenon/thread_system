###################################################
# metrics module CMakeLists.txt
#
# This module provides comprehensive metrics collection
# for the thread system, including counters, gauges,
# histograms, and time-series storage.
###################################################

# Find required packages
find_package(Threads REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Collect source files
file(GLOB_RECURSE SOURCES 
    "*.cpp"
    "*.cc"
)

file(GLOB_RECURSE HEADERS 
    "*.h"
    "*.hpp"
)

# Create metrics library
add_library(metrics STATIC ${SOURCES} ${HEADERS})

# Set include directories
target_include_directories(metrics PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/sources
)

# Link dependencies
target_link_libraries(metrics
    PUBLIC
        utilities
        nlohmann_json::nlohmann_json
        Threads::Threads
)

# Set compile features
target_compile_features(metrics PUBLIC cxx_std_20)

# Platform-specific definitions
if(WIN32)
    target_compile_definitions(metrics PUBLIC WIN32_LEAN_AND_MEAN)
elseif(APPLE)
    target_compile_definitions(metrics PUBLIC APPLE_PLATFORM)
endif()

# Set source groups for IDE
source_group("Source Files" FILES ${SOURCES})
source_group("Header Files" FILES ${HEADERS})