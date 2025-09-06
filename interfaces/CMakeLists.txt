##################################################
# Interfaces Component (new layout)
##################################################

project(interfaces)

file(GLOB IF_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
set(IF_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/logger_interface.cpp)

add_library(${PROJECT_NAME} STATIC ${IF_HEADERS} ${IF_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/thread_system/interfaces>
)

if(COMMAND set_compiler_warnings)
    set_compiler_warnings(${PROJECT_NAME})
endif()

