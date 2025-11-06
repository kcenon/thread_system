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

    # Fallback to fmt when std::format is not used
    if(NOT USE_STD_FORMAT)
        find_package(fmt CONFIG QUIET)
        if(fmt_FOUND)
            target_link_libraries(${PROJECT_NAME} PUBLIC fmt::fmt-header-only)
            target_compile_definitions(${PROJECT_NAME} PUBLIC HAS_FMT_LIBRARY)
            message(STATUS "✅ Found fmt library - using fmt::format")
        else()
            message(STATUS "⚠️ fmt library not found - using basic string operations fallback")
        endif()
    endif()

    # Iconv for string conversions (optional)
    find_package(Iconv QUIET)
    if(Iconv_FOUND)
        target_link_libraries(${PROJECT_NAME} PUBLIC Iconv::Iconv)
        target_compile_definitions(${PROJECT_NAME} PUBLIC HAS_ICONV)
        message(STATUS "✅ Found Iconv library")
    else()
        message(STATUS "⚠️ Iconv not found - some string conversion features may be limited")
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

    # Fallback to fmt when std::format is not used
    if(NOT USE_STD_FORMAT)
        find_package(fmt CONFIG QUIET)
        if(fmt_FOUND)
            target_link_libraries(${PROJECT_NAME} INTERFACE fmt::fmt-header-only)
            target_compile_definitions(${PROJECT_NAME} INTERFACE HAS_FMT_LIBRARY)
            message(STATUS "✅ Found fmt library - using fmt::format (interface)")
        else()
            message(STATUS "⚠️ fmt library not found - using basic string operations fallback (interface)")
        endif()
    endif()

    # Iconv for string conversions (optional)
    find_package(Iconv QUIET)
    if(Iconv_FOUND)
        target_link_libraries(${PROJECT_NAME} INTERFACE Iconv::Iconv)
        target_compile_definitions(${PROJECT_NAME} INTERFACE HAS_ICONV)
        message(STATUS "✅ Found Iconv library (interface)")
    else()
        message(STATUS "⚠️ Iconv not found - some string conversion features may be limited (interface)")
    endif()
endif()

