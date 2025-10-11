##################################################
# ThreadSystemDependencies.cmake
#
# Dependency finding module for ThreadSystem
# Handles common_system, fmt, and threading libraries
##################################################

##################################################
# Find common_system (optional)
##################################################
function(find_common_system_dependency)
  if(NOT BUILD_WITH_COMMON_SYSTEM)
    return()
  endif()

  message(STATUS "Looking for common_system...")

  # Try CMake config mode first
  find_package(common_system CONFIG QUIET)

  if(common_system_FOUND)
    message(STATUS "Found common_system via CMake config")
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
      include_directories(${COMMON_SYSTEM_INCLUDE_DIR})
      message(STATUS "Found common_system at: ${COMMON_SYSTEM_INCLUDE_DIR}")
      set(COMMON_SYSTEM_FOUND TRUE PARENT_SCOPE)
      return()
    endif()
  endforeach()

  message(WARNING "common_system not found - integration disabled")
  set(BUILD_WITH_COMMON_SYSTEM OFF PARENT_SCOPE)
  set(COMMON_SYSTEM_FOUND FALSE PARENT_SCOPE)
endfunction()

##################################################
# Find fmt library (optional, fallback to std::format)
##################################################
function(find_fmt_library)
  # Skip if using std::format
  if(USE_STD_FORMAT)
    message(STATUS "Using std::format - skipping fmt library search")
    return()
  endif()

  message(STATUS "Looking for fmt library...")

  # Try CMake config mode first
  find_package(fmt CONFIG QUIET)

  if(fmt_FOUND)
    message(STATUS "Found fmt via CMake config")
    add_definitions(-DHAS_FMT_LIBRARY)
    set(FMT_FOUND TRUE PARENT_SCOPE)
    set(FMT_TARGET fmt::fmt PARENT_SCOPE)
    return()
  endif()

  # Try pkgconfig
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(FMT QUIET IMPORTED_TARGET fmt)
    if(FMT_FOUND)
      message(STATUS "Found fmt via pkgconfig: ${FMT_VERSION}")
      add_definitions(-DHAS_FMT_LIBRARY)
      set(FMT_FOUND TRUE PARENT_SCOPE)
      set(FMT_TARGET PkgConfig::FMT PARENT_SCOPE)
      return()
    endif()
  endif()

  # Manual search
  find_path(FMT_INCLUDE_DIR
    NAMES fmt/format.h
    PATHS
      /opt/homebrew/include
      /usr/local/include
      /usr/include
  )

  find_library(FMT_LIBRARY
    NAMES fmt
    PATHS
      /opt/homebrew/lib
      /usr/local/lib
      /usr/lib
  )

  if(FMT_INCLUDE_DIR)
    message(STATUS "Found fmt include dir: ${FMT_INCLUDE_DIR}")
    add_definitions(-DHAS_FMT_LIBRARY)
    if(FMT_LIBRARY)
      message(STATUS "Found fmt library: ${FMT_LIBRARY}")
      set(FMT_FOUND TRUE PARENT_SCOPE)
      set(FMT_INCLUDE_DIR ${FMT_INCLUDE_DIR} PARENT_SCOPE)
      set(FMT_LIBRARY ${FMT_LIBRARY} PARENT_SCOPE)
    else()
      message(STATUS "Using fmt as header-only")
      set(FMT_FOUND TRUE PARENT_SCOPE)
      set(FMT_INCLUDE_DIR ${FMT_INCLUDE_DIR} PARENT_SCOPE)
      set(FMT_HEADER_ONLY TRUE PARENT_SCOPE)
    endif()
  else()
    message(WARNING "⚠️ fmt library not found - using basic string operations fallback")
    set(FMT_FOUND FALSE PARENT_SCOPE)
  endif()
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
  if(NOT BUILD_INTEGRATION_TESTS AND NOT BUILD_TESTING)
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
  find_fmt_library()
  find_threading_library()
  find_or_fetch_gtest()

  message(STATUS "Dependency detection complete")
endfunction()
