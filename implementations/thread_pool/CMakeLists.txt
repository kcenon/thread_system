##################################################
# thread_pool Implementation
##################################################

project(thread_pool)

file(GLOB TP_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h ${CMAKE_CURRENT_SOURCE_DIR}/include/detail/*.h)
set(TP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/thread_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/thread_worker.cpp
)

add_library(${PROJECT_NAME} STATIC ${TP_HEADERS} ${TP_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/core/base/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/core/jobs/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/core/sync/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/interfaces>
    $<INSTALL_INTERFACE:include/thread_system/implementations/thread_pool>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    thread_base
    utilities
    interfaces
    lockfree
)

if(NOT APPLE)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
endif()

if(COMMAND set_compiler_warnings)
    set_compiler_warnings(${PROJECT_NAME})
endif()
