##################################################
# lockfree Implementation
##################################################

project(lockfree)

file(GLOB LF_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
file(GLOB LF_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(${PROJECT_NAME} STATIC ${LF_HEADERS} ${LF_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/core/jobs/include>
    $<INSTALL_INTERFACE:include/thread_system/implementations/lockfree>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    thread_base
    utilities
)

if(COMMAND set_compiler_warnings)
    set_compiler_warnings(${PROJECT_NAME})
endif()

