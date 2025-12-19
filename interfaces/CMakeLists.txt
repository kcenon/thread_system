##################################################
# Interfaces Component (new layout)
##################################################

project(interfaces)

# Note: logger_interface.cpp was removed in v3.0.0 as part of Issue #311
# The deprecated logger_interface has been replaced by common::interfaces::ILogger
# This library is now header-only (INTERFACE)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:include>
)

