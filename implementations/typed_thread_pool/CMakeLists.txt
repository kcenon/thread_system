##################################################
# typed_thread_pool Implementation
##################################################

project(typed_thread_pool)

file(GLOB TYPED_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h ${CMAKE_CURRENT_SOURCE_DIR}/include/*.tpp)
file(GLOB TYPED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(${PROJECT_NAME} STATIC ${TYPED_HEADERS} ${TYPED_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/core/base/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/core/jobs/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/core/sync/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/interfaces>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/implementations/lockfree/include>
    $<INSTALL_INTERFACE:include/thread_system/implementations/typed_thread_pool>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    thread_pool
    lockfree
    thread_base
    utilities
    interfaces
)

if(COMMAND set_compiler_warnings)
    set_compiler_warnings(${PROJECT_NAME})
endif()

