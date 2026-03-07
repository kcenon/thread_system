##################################################
# Utilities Component (new layout)
##################################################

project(utilities)

file(GLOB_RECURSE UTIL_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
# Source files are now in the main src/utils directory
file(GLOB_RECURSE UTIL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/*.cpp)

# Only create library if we have sources
if(UTIL_SOURCES)
    add_library(${PROJECT_NAME} STATIC ${UTIL_HEADERS} ${UTIL_SOURCES})
else()
    # Create interface library if no sources
    add_library(${PROJECT_NAME} INTERFACE)
endif()

if(UTIL_SOURCES)
    # Add both utilities/include and root include (for kcenon/thread/utils/formatter.h)
    target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    # Also add parent include directory for accessing formatter.h via <kcenon/thread/utils/formatter.h>
    get_filename_component(THREAD_SYSTEM_ROOT_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../include" ABSOLUTE)
    target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${THREAD_SYSTEM_ROOT_INCLUDE}>
        $<INSTALL_INTERFACE:include>
    )

    if(COMMAND set_compiler_warnings)
        set_compiler_warnings(${PROJECT_NAME})
    endif()

    # Note: fmt library is no longer used - using C++20 std::format exclusively
    # The fmt fallback logic has been removed as part of issue #219

    # simdutf for Unicode string conversions (provided by find_simdutf_library)
    if(TARGET simdutf::simdutf)
        target_link_libraries(${PROJECT_NAME} PUBLIC simdutf::simdutf)
    else()
        find_package(simdutf CONFIG REQUIRED)
        target_link_libraries(${PROJECT_NAME} PUBLIC simdutf::simdutf)
    endif()
else()
    # Interface library case - add both utilities/include and root include
    target_include_directories(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    # Also add parent include directory
    get_filename_component(THREAD_SYSTEM_ROOT_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../include" ABSOLUTE)
    target_include_directories(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${THREAD_SYSTEM_ROOT_INCLUDE}>
        $<INSTALL_INTERFACE:include>
    )

    # Note: fmt library is no longer used - using C++20 std::format exclusively
    # The fmt fallback logic has been removed as part of issue #219

    # simdutf for Unicode string conversions (provided by find_simdutf_library)
    if(TARGET simdutf::simdutf)
        target_link_libraries(${PROJECT_NAME} INTERFACE simdutf::simdutf)
    else()
        find_package(simdutf CONFIG REQUIRED)
        target_link_libraries(${PROJECT_NAME} INTERFACE simdutf::simdutf)
    endif()
endif()

