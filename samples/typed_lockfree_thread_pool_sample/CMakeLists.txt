cmake_minimum_required(VERSION 3.10)

project(typed_lockfree_thread_pool_sample)

set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES typed_lockfree_thread_pool_sample.cpp)

add_executable(typed_lockfree_thread_pool_sample ${SOURCE_FILES})

target_include_directories(typed_lockfree_thread_pool_sample PRIVATE
    ${CMAKE_SOURCE_DIR}/sources
)

target_link_libraries(typed_lockfree_thread_pool_sample
    thread_base
    lockfree
    typed_thread_pool
    logger
)

install(TARGETS typed_lockfree_thread_pool_sample
    RUNTIME DESTINATION bin
)