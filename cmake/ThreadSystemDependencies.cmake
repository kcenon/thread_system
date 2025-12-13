##################################################
# ThreadSystemDependencies.cmake
#
# Dependency finding module for ThreadSystem
# Handles common_system, fmt, and threading libraries
##################################################

##################################################
# Find common_system (required)
##################################################
function(find_common_system_dependency)
  if(NOT BUILD_WITH_COMMON_SYSTEM)
    message(FATAL_ERROR "ThreadSystem requires common_system. BUILD_WITH_COMMON_SYSTEM must remain ON.")
  endif()

  message(STATUS "Looking for common_system...")

  # Check if COMMON_SYSTEM_INCLUDE_DIR is already set (e.g., by parent project via FetchContent)
  if(COMMON_SYSTEM_INCLUDE_DIR AND EXISTS "${COMMON_SYSTEM_INCLUDE_DIR}/kcenon/common/patterns/result.h")
    message(STATUS "Found common_system via pre-set COMMON_SYSTEM_INCLUDE_DIR: ${COMMON_SYSTEM_INCLUDE_DIR}")
    add_compile_definitions(BUILD_WITH_COMMON_SYSTEM)
    add_compile_definitions(THREAD_HAS_COMMON_RESULT=1)
    add_compile_definitions(THREAD_HAS_COMMON_EXECUTOR=1)
    include_directories(${COMMON_SYSTEM_INCLUDE_DIR})
    set(COMMON_SYSTEM_FOUND TRUE PARENT_SCOPE)
    return()
  endif()

  # Try CMake config mode first
  find_package(common_system CONFIG QUIET)

  if(common_system_FOUND)
    message(STATUS "Found common_system via CMake config")
    add_compile_definitions(BUILD_WITH_COMMON_SYSTEM)
    add_compile_definitions(THREAD_HAS_COMMON_RESULT=1)
    add_compile_definitions(THREAD_HAS_COMMON_EXECUTOR=1)
    set(COMMON_SYSTEM_FOUND TRUE PARENT_SCOPE)
    return()
  endif()

  # Fallback to path-based search
  set(_COMMON_PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/../common_system/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../common_system/include"
  )

  foreach(_path ${_COMMON_PATHS})
    if(EXISTS "${_path}/kcenon/common/patterns/result.h")
      set(COMMON_SYSTEM_INCLUDE_DIR "${_path}" CACHE PATH "Path to common_system include directory")
      add_compile_definitions(BUILD_WITH_COMMON_SYSTEM)
      add_compile_definitions(THREAD_HAS_COMMON_RESULT=1)
      add_compile_definitions(THREAD_HAS_COMMON_EXECUTOR=1)
      include_directories(${COMMON_SYSTEM_INCLUDE_DIR})
      message(STATUS "Found common_system at: ${COMMON_SYSTEM_INCLUDE_DIR}")
      set(COMMON_SYSTEM_FOUND TRUE PARENT_SCOPE)
      return()
    endif()
  endforeach()

  message(FATAL_ERROR "common_system not found - set COMMON_SYSTEM_INCLUDE_DIR or add as dependency.")
endfunction()

##################################################
# fmt library is no longer used
# This project now uses C++20 std::format exclusively
##################################################
# Note: The find_fmt_library() function has been removed as part of the
# migration to C++20 std::format. See GitHub issue #219 for details.

##################################################
# Find iconv library (optional, improves conversions)
##################################################
function(find_iconv_library)
  message(STATUS "Looking for iconv library...")

  find_package(Iconv QUIET)

  if(Iconv_FOUND)
    message(STATUS "Found iconv: ${Iconv_LIBRARIES}")
    set(THREAD_SYSTEM_ICONV_FOUND TRUE PARENT_SCOPE)

    if(TARGET Iconv::Iconv)
      set(THREAD_SYSTEM_ICONV_TARGET Iconv::Iconv PARENT_SCOPE)
    else()
      set(THREAD_SYSTEM_ICONV_TARGET "" PARENT_SCOPE)
      set(THREAD_SYSTEM_ICONV_INCLUDE_DIRS "${Iconv_INCLUDE_DIRS}" PARENT_SCOPE)
      set(THREAD_SYSTEM_ICONV_LIBRARIES "${Iconv_LIBRARIES}" PARENT_SCOPE)
    endif()

    return()
  endif()

  message(WARNING "⚠️ Iconv not found - wide/narrow string conversion will use limited fallback")
  set(THREAD_SYSTEM_ICONV_FOUND FALSE PARENT_SCOPE)
  set(THREAD_SYSTEM_ICONV_TARGET "" PARENT_SCOPE)
  set(THREAD_SYSTEM_ICONV_INCLUDE_DIRS "" PARENT_SCOPE)
  set(THREAD_SYSTEM_ICONV_LIBRARIES "" PARENT_SCOPE)
endfunction()

##################################################
# Find threading library
##################################################
function(find_threading_library)
  if(APPLE)
    # macOS handles pthread differently
    add_definitions(-DHAVE_PTHREAD)
    add_definitions(-DAPPLE_PLATFORM)
    set(THREADS_FOUND TRUE)
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    message(STATUS "Using macOS native pthread support")
  else()
    # Other platforms use CMake's thread package finder
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    if(Threads_FOUND)
      message(STATUS "Found threading library: ${CMAKE_THREAD_LIBS_INIT}")
    endif()
  endif()

  set(THREADS_FOUND ${THREADS_FOUND} PARENT_SCOPE)
endfunction()

##################################################
# Find or fetch GTest for testing
##################################################
function(find_or_fetch_gtest)
  # Skip if not building tests
  if(NOT THREAD_BUILD_INTEGRATION_TESTS AND NOT BUILD_TESTING)
    return()
  endif()

  message(STATUS "Setting up GoogleTest...")

  # Try to find system GTest first (for faster local builds)
  find_package(GTest QUIET)

  if(GTest_FOUND)
    message(STATUS "✅ Using system GTest: ${GTest_VERSION}")
    set(GTEST_FOUND TRUE PARENT_SCOPE)
    return()
  endif()

  # Fallback: Use FetchContent to download and build GTest
  message(STATUS "System GTest not found - fetching from source...")

  include(FetchContent)

  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
  )

  # Configure GTest options
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

  # Make GTest available
  FetchContent_MakeAvailable(googletest)

  message(STATUS "✅ GTest fetched and configured (v1.14.0)")
  set(GTEST_FOUND TRUE PARENT_SCOPE)
  set(GTEST_FROM_SOURCE TRUE PARENT_SCOPE)
endfunction()

##################################################
# Main function to find all dependencies
##################################################
function(find_thread_system_dependencies)
  message(STATUS "Finding ThreadSystem dependencies...")

  find_common_system_dependency()
  # Note: fmt library is no longer used - using C++20 std::format exclusively
  find_iconv_library()
  find_threading_library()
  find_or_fetch_gtest()

  if(DEFINED THREAD_SYSTEM_ICONV_FOUND)
    set(THREAD_SYSTEM_ICONV_FOUND ${THREAD_SYSTEM_ICONV_FOUND} PARENT_SCOPE)
  endif()
  if(DEFINED THREAD_SYSTEM_ICONV_TARGET)
    set(THREAD_SYSTEM_ICONV_TARGET ${THREAD_SYSTEM_ICONV_TARGET} PARENT_SCOPE)
  endif()
  if(DEFINED THREAD_SYSTEM_ICONV_INCLUDE_DIRS)
    set(THREAD_SYSTEM_ICONV_INCLUDE_DIRS ${THREAD_SYSTEM_ICONV_INCLUDE_DIRS} PARENT_SCOPE)
  endif()
  if(DEFINED THREAD_SYSTEM_ICONV_LIBRARIES)
    set(THREAD_SYSTEM_ICONV_LIBRARIES ${THREAD_SYSTEM_ICONV_LIBRARIES} PARENT_SCOPE)
  endif()

  message(STATUS "Dependency detection complete")
endfunction()
