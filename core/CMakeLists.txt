##################################################
# Core Component (thread_base + jobs + sync)
##################################################

project(thread_base)

file(GLOB_RECURSE CORE_BASE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/base/include/*.h)
file(GLOB_RECURSE CORE_BASE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/base/src/*.cpp)

file(GLOB_RECURSE CORE_JOBS_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/jobs/include/*.h)
file(GLOB_RECURSE CORE_JOBS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/jobs/src/*.cpp)

file(GLOB_RECURSE CORE_SYNC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/sync/include/*.h)

add_library(${PROJECT_NAME} STATIC
    ${CORE_BASE_HEADERS} ${CORE_BASE_SOURCES}
    ${CORE_JOBS_HEADERS} ${CORE_JOBS_SOURCES}
    ${CORE_SYNC_HEADERS}
)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/base/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/jobs/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sync/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/interfaces>
    $<INSTALL_INTERFACE:include/thread_system/core/base>
    $<INSTALL_INTERFACE:include/thread_system/core/jobs>
    $<INSTALL_INTERFACE:include/thread_system/core/sync>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    utilities
    interfaces
)

if(NOT APPLE)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
endif()

if(COMMAND set_compiler_warnings)
    set_compiler_warnings(${PROJECT_NAME})
endif()
