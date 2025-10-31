##################################################
# Core Component (thread_base + jobs + sync)
##################################################

project(thread_base)

# Find headers in include directory
file(GLOB_RECURSE CORE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/*.h)

# Find sources in src directory
file(GLOB_RECURSE CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/thread_pool/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/typed_pool/typed_job.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/typed_pool/callback_typed_job.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/typed_pool/typed_job_queue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/typed_pool/typed_thread_worker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/typed_pool/typed_thread_pool.cpp)

add_library(${PROJECT_NAME} STATIC
    ${CORE_HEADERS}
    ${CORE_SOURCES}
)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../interfaces>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../src>
    $<INSTALL_INTERFACE:include>
)

# Add common_system support if available
if(BUILD_WITH_COMMON_SYSTEM)
    # Priority: Local Sources/ directory first, then relative paths
    set(_THREAD_COMMON_SEARCH_PATHS
        "/Users/dongcheolshin/Sources/common_system/include"      # macOS development (HIGHEST PRIORITY)
        "/home/${USER}/Sources/common_system/include"              # Linux development
        "${CMAKE_CURRENT_SOURCE_DIR}/../../common_system/include"  # Sibling directory (fallback)
    )

    set(_THREAD_COMMON_SYSTEM_PATH "")
    foreach(_path ${_THREAD_COMMON_SEARCH_PATHS})
        if(EXISTS "${_path}")
            message(STATUS "Found common_system at: ${_path}")
            set(_THREAD_COMMON_SYSTEM_PATH "${_path}")
            break()
        endif()
    endforeach()

    if(_THREAD_COMMON_SYSTEM_PATH)
        target_include_directories(${PROJECT_NAME} PUBLIC
            $<BUILD_INTERFACE:${_THREAD_COMMON_SYSTEM_PATH}>
        )
        target_compile_definitions(${PROJECT_NAME} PUBLIC BUILD_WITH_COMMON_SYSTEM)
        message(STATUS "ThreadSystem: common_system support enabled from ${_THREAD_COMMON_SYSTEM_PATH}")
    else()
        message(WARNING "ThreadSystem: BUILD_WITH_COMMON_SYSTEM is ON but common_system not found in any search path")
    endif()
endif()

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
