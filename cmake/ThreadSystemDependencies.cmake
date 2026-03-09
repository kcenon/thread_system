##################################################
# ThreadSystemDependencies.cmake
#
# Dependency finding module for ThreadSystem
# Handles common_system, fmt, and threading libraries
##################################################

##################################################
# DEPRECATED: logger_system integration (Issue #336)
# Use common_system ILogger interface instead
##################################################
option(BUILD_WITH_LOGGER_SYSTEM
    "DEPRECATED: Direct logger_system integration. Use common_system ILogger instead."
    OFF)

function(check_logger_system_deprecation)
    if(BUILD_WITH_LOGGER_SYSTEM)
        message(DEPRECATION
            "BUILD_WITH_LOGGER_SYSTEM is deprecated and will be removed in v0.5.0.0. "
            "Use common_system's ILogger interface instead. "
            "logger_system can provide ILogger implementation via ServiceContainer. "
            "See GitHub issue #336 for migration guide.")

        # Define the macro for code compilation
        add_compile_definitions(BUILD_WITH_LOGGER_SYSTEM)
    endif()
endfunction()

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
    # KCENON_* unified feature flags (primary)
    add_compile_definitions(KCENON_HAS_COMMON_RESULT=1)
    add_compile_definitions(KCENON_HAS_COMMON_EXECUTOR=1)
    # Legacy aliases for backward compatibility (deprecated, will be removed in v1.0.0)
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
    # KCENON_* unified feature flags (primary)
    add_compile_definitions(KCENON_HAS_COMMON_RESULT=1)
    add_compile_definitions(KCENON_HAS_COMMON_EXECUTOR=1)
    # Legacy aliases for backward compatibility (deprecated, will be removed in v1.0.0)
    add_compile_definitions(THREAD_HAS_COMMON_RESULT=1)
    add_compile_definitions(THREAD_HAS_COMMON_EXECUTOR=1)
    set(COMMON_SYSTEM_FOUND TRUE PARENT_SCOPE)
    return()
  endif()

  # Fallback to path-based search
  set(_COMMON_PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/common_system/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../common_system/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../common_system/include"
  )

  foreach(_path ${_COMMON_PATHS})
    if(EXISTS "${_path}/kcenon/common/patterns/result.h")
      set(COMMON_SYSTEM_INCLUDE_DIR "${_path}" CACHE PATH "Path to common_system include directory")
      add_compile_definitions(BUILD_WITH_COMMON_SYSTEM)
      # KCENON_* unified feature flags (primary)
      add_compile_definitions(KCENON_HAS_COMMON_RESULT=1)
      add_compile_definitions(KCENON_HAS_COMMON_EXECUTOR=1)
      # Legacy aliases for backward compatibility (deprecated, will be removed in v1.0.0)
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

set(THREAD_SYSTEM_SIMDUTF_FETCHCONTENT_REPOSITORY "https://github.com/simdutf/simdutf.git")
set(THREAD_SYSTEM_SIMDUTF_FETCHCONTENT_TAG "v5.2.5")
set(THREAD_SYSTEM_SIMDUTF_INTENDED_VERSION "5.2.5")

function(_thread_system_json_escape output_variable input_value)
  set(_escaped "${input_value}")
  string(REPLACE "\\" "\\\\" _escaped "${_escaped}")
  string(REPLACE "\"" "\\\"" _escaped "${_escaped}")
  string(REPLACE "\n" "\\n" _escaped "${_escaped}")
  string(REPLACE "\r" "\\r" _escaped "${_escaped}")
  set(${output_variable} "${_escaped}" PARENT_SCOPE)
endfunction()

function(_thread_system_get_simdutf_target_hint output_variable)
  set(_hint "")
  set(_location_properties
    IMPORTED_LOCATION
    IMPORTED_LOCATION_RELEASE
    IMPORTED_LOCATION_RELWITHDEBINFO
    IMPORTED_LOCATION_DEBUG
    IMPORTED_IMPLIB
    IMPORTED_IMPLIB_RELEASE
    IMPORTED_IMPLIB_RELWITHDEBINFO
    IMPORTED_IMPLIB_DEBUG
  )

  foreach(_property IN LISTS _location_properties)
    get_target_property(_value simdutf::simdutf ${_property})
    if(_value AND NOT _value MATCHES "-NOTFOUND$")
      set(_hint "${_value}")
      break()
    endif()
  endforeach()

  if(_hint STREQUAL "")
    get_target_property(_include_directories simdutf::simdutf INTERFACE_INCLUDE_DIRECTORIES)
    if(_include_directories)
      list(GET _include_directories 0 _hint)
    endif()
  endif()

  set(${output_variable} "${_hint}" PARENT_SCOPE)
endfunction()

function(_thread_system_determine_simdutf_source output_variable hint_value)
  set(_source "system")

  if(NOT "${hint_value}" STREQUAL "")
    string(FIND "${hint_value}" "vcpkg_installed" _contains_vcpkg_installed)
    string(FIND "${hint_value}" "/vcpkg/" _contains_vcpkg_root)
    string(FIND "${hint_value}" "\\vcpkg\\" _contains_vcpkg_root_windows)
    if(NOT _contains_vcpkg_installed EQUAL -1
        OR NOT _contains_vcpkg_root EQUAL -1
        OR NOT _contains_vcpkg_root_windows EQUAL -1)
      set(_source "vcpkg")
    endif()
  endif()

  if(_source STREQUAL "system" AND "${hint_value}" STREQUAL "" AND DEFINED CMAKE_TOOLCHAIN_FILE)
    string(FIND "${CMAKE_TOOLCHAIN_FILE}" "vcpkg.cmake" _uses_vcpkg_toolchain)
    if(NOT _uses_vcpkg_toolchain EQUAL -1)
      set(_source "vcpkg")
    endif()
  endif()

  set(${output_variable} "${_source}" PARENT_SCOPE)
endfunction()

function(write_simdutf_provenance_artifacts)
  if(NOT DEFINED THREAD_SYSTEM_SIMDUTF_SOURCE OR "${THREAD_SYSTEM_SIMDUTF_SOURCE}" STREQUAL "")
    set(THREAD_SYSTEM_SIMDUTF_SOURCE "unknown")
  endif()
  if(NOT DEFINED THREAD_SYSTEM_SIMDUTF_VERSION OR "${THREAD_SYSTEM_SIMDUTF_VERSION}" STREQUAL "")
    set(THREAD_SYSTEM_SIMDUTF_VERSION "unknown")
  endif()
  if(NOT DEFINED THREAD_SYSTEM_SIMDUTF_SOURCE_REFERENCE OR "${THREAD_SYSTEM_SIMDUTF_SOURCE_REFERENCE}" STREQUAL "")
    set(THREAD_SYSTEM_SIMDUTF_SOURCE_REFERENCE "unknown")
  endif()
  if(NOT DEFINED THREAD_SYSTEM_SIMDUTF_SOURCE_REPOSITORY)
    set(THREAD_SYSTEM_SIMDUTF_SOURCE_REPOSITORY "")
  endif()
  if(NOT DEFINED THREAD_SYSTEM_SIMDUTF_PACKAGE_CONFIG_PATH)
    set(THREAD_SYSTEM_SIMDUTF_PACKAGE_CONFIG_PATH "")
  endif()
  if(NOT DEFINED THREAD_SYSTEM_SIMDUTF_RESOLVED_PATH)
    set(THREAD_SYSTEM_SIMDUTF_RESOLVED_PATH "")
  endif()

  string(TIMESTAMP _generated_at "%Y-%m-%dT%H:%M:%SZ" UTC)
  set(_provenance_dir "${CMAKE_BINARY_DIR}/dependency-provenance")
  set(_json_output "${_provenance_dir}/simdutf-provenance.json")
  set(_markdown_output "${_provenance_dir}/simdutf-provenance.md")

  file(MAKE_DIRECTORY "${_provenance_dir}")

  _thread_system_json_escape(_json_source "${THREAD_SYSTEM_SIMDUTF_SOURCE}")
  _thread_system_json_escape(_json_version "${THREAD_SYSTEM_SIMDUTF_VERSION}")
  _thread_system_json_escape(_json_reference "${THREAD_SYSTEM_SIMDUTF_SOURCE_REFERENCE}")
  _thread_system_json_escape(_json_repository "${THREAD_SYSTEM_SIMDUTF_SOURCE_REPOSITORY}")
  _thread_system_json_escape(_json_package_config "${THREAD_SYSTEM_SIMDUTF_PACKAGE_CONFIG_PATH}")
  _thread_system_json_escape(_json_resolved_path "${THREAD_SYSTEM_SIMDUTF_RESOLVED_PATH}")
  _thread_system_json_escape(_json_generated_at "${_generated_at}")

  file(WRITE "${_json_output}" "{\n")
  file(APPEND "${_json_output}" "  \"dependency\": \"simdutf\",\n")
  file(APPEND "${_json_output}" "  \"intended_version\": \"${THREAD_SYSTEM_SIMDUTF_INTENDED_VERSION}\",\n")
  file(APPEND "${_json_output}" "  \"resolved_source\": \"${_json_source}\",\n")
  file(APPEND "${_json_output}" "  \"resolved_version\": \"${_json_version}\",\n")
  file(APPEND "${_json_output}" "  \"source_reference\": \"${_json_reference}\",\n")
  file(APPEND "${_json_output}" "  \"source_repository\": \"${_json_repository}\",\n")
  file(APPEND "${_json_output}" "  \"package_config_path\": \"${_json_package_config}\",\n")
  file(APPEND "${_json_output}" "  \"resolved_path\": \"${_json_resolved_path}\",\n")
  file(APPEND "${_json_output}" "  \"cmake_target\": \"simdutf::simdutf\",\n")
  file(APPEND "${_json_output}" "  \"generated_at_utc\": \"${_json_generated_at}\"\n")
  file(APPEND "${_json_output}" "}\n")

  file(WRITE "${_markdown_output}" "## simdutf Resolved Provenance\n\n")
  file(APPEND "${_markdown_output}" "| Field | Value |\n")
  file(APPEND "${_markdown_output}" "|-------|-------|\n")
  file(APPEND "${_markdown_output}" "| Dependency | simdutf |\n")
  file(APPEND "${_markdown_output}" "| Intended version | ${THREAD_SYSTEM_SIMDUTF_INTENDED_VERSION} |\n")
  file(APPEND "${_markdown_output}" "| Resolved source | ${THREAD_SYSTEM_SIMDUTF_SOURCE} |\n")
  file(APPEND "${_markdown_output}" "| Resolved version | ${THREAD_SYSTEM_SIMDUTF_VERSION} |\n")
  file(APPEND "${_markdown_output}" "| Source reference | ${THREAD_SYSTEM_SIMDUTF_SOURCE_REFERENCE} |\n")
  file(APPEND "${_markdown_output}" "| Source repository | ${THREAD_SYSTEM_SIMDUTF_SOURCE_REPOSITORY} |\n")
  file(APPEND "${_markdown_output}" "| Package config path | ${THREAD_SYSTEM_SIMDUTF_PACKAGE_CONFIG_PATH} |\n")
  file(APPEND "${_markdown_output}" "| Resolved path | ${THREAD_SYSTEM_SIMDUTF_RESOLVED_PATH} |\n")
  file(APPEND "${_markdown_output}" "| CMake target | simdutf::simdutf |\n")
  file(APPEND "${_markdown_output}" "| Generated at (UTC) | ${_generated_at} |\n")

  set(THREAD_SYSTEM_SIMDUTF_PROVENANCE_JSON "${_json_output}" PARENT_SCOPE)
  set(THREAD_SYSTEM_SIMDUTF_PROVENANCE_MARKDOWN "${_markdown_output}" PARENT_SCOPE)

  message(STATUS
    "Recorded simdutf provenance: ${THREAD_SYSTEM_SIMDUTF_SOURCE} "
    "(${THREAD_SYSTEM_SIMDUTF_VERSION}) -> ${_json_output}")
endfunction()

##################################################
# Find or fetch simdutf library (required for Unicode conversion)
##################################################
function(find_simdutf_library)
  message(STATUS "Looking for simdutf library...")

  # Try to find system or vcpkg-installed simdutf first
  find_package(simdutf CONFIG QUIET)

  if(TARGET simdutf::simdutf)
    _thread_system_get_simdutf_target_hint(_simdutf_hint)
    _thread_system_determine_simdutf_source(_simdutf_source "${_simdutf_hint}")

    set(_simdutf_version "${simdutf_VERSION}")
    if("${_simdutf_version}" STREQUAL "" AND DEFINED simdutf_VERSION_STRING)
      set(_simdutf_version "${simdutf_VERSION_STRING}")
    endif()
    if("${_simdutf_version}" STREQUAL "")
      set(_simdutf_version "unknown")
    endif()

    set(_package_config_path "")
    if(DEFINED simdutf_DIR)
      set(_package_config_path "${simdutf_DIR}")
    endif()

    message(STATUS "Found simdutf via ${_simdutf_source}: ${_simdutf_version}")
    set(THREAD_SYSTEM_SIMDUTF_FOUND TRUE PARENT_SCOPE)
    set(THREAD_SYSTEM_SIMDUTF_TARGET simdutf::simdutf PARENT_SCOPE)
    set(THREAD_SYSTEM_SIMDUTF_SOURCE "${_simdutf_source}" PARENT_SCOPE)
    set(THREAD_SYSTEM_SIMDUTF_VERSION "${_simdutf_version}" PARENT_SCOPE)
    set(THREAD_SYSTEM_SIMDUTF_SOURCE_REFERENCE "${_simdutf_version}" PARENT_SCOPE)
    set(THREAD_SYSTEM_SIMDUTF_SOURCE_REPOSITORY "" PARENT_SCOPE)
    set(THREAD_SYSTEM_SIMDUTF_PACKAGE_CONFIG_PATH "${_package_config_path}" PARENT_SCOPE)
    set(THREAD_SYSTEM_SIMDUTF_RESOLVED_PATH "${_simdutf_hint}" PARENT_SCOPE)
    return()
  endif()

  # Fallback: Use FetchContent to download and build simdutf
  message(STATUS "System simdutf not found - fetching from source...")

  include(FetchContent)

  FetchContent_Declare(
    simdutf
    GIT_REPOSITORY ${THREAD_SYSTEM_SIMDUTF_FETCHCONTENT_REPOSITORY}
    GIT_TAG ${THREAD_SYSTEM_SIMDUTF_FETCHCONTENT_TAG}
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
  )

  # Configure simdutf build options
  set(SIMDUTF_TESTS OFF CACHE BOOL "" FORCE)
  set(SIMDUTF_BENCHMARKS OFF CACHE BOOL "" FORCE)
  set(SIMDUTF_TOOLS OFF CACHE BOOL "" FORCE)

  # Save CMake C++ standard variables before FetchContent
  # simdutf-flags.cmake sets CMAKE_CXX_STANDARD=11 globally which would
  # downgrade the entire project from C++20 to C++11
  set(_SAVED_CXX_STANDARD ${CMAKE_CXX_STANDARD})
  set(_SAVED_CXX_STANDARD_REQUIRED ${CMAKE_CXX_STANDARD_REQUIRED})
  set(_SAVED_CXX_EXTENSIONS ${CMAKE_CXX_EXTENSIONS})

  FetchContent_MakeAvailable(simdutf)

  # Restore C++ standard variables after FetchContent
  set(CMAKE_CXX_STANDARD ${_SAVED_CXX_STANDARD})
  set(CMAKE_CXX_STANDARD_REQUIRED ${_SAVED_CXX_STANDARD_REQUIRED})
  set(CMAKE_CXX_EXTENSIONS ${_SAVED_CXX_EXTENSIONS})

  message(STATUS "simdutf fetched and configured (${THREAD_SYSTEM_SIMDUTF_FETCHCONTENT_TAG})")
  set(THREAD_SYSTEM_SIMDUTF_FOUND TRUE PARENT_SCOPE)
  set(THREAD_SYSTEM_SIMDUTF_TARGET simdutf::simdutf PARENT_SCOPE)
  set(THREAD_SYSTEM_SIMDUTF_SOURCE "FetchContent" PARENT_SCOPE)
  set(THREAD_SYSTEM_SIMDUTF_VERSION "${THREAD_SYSTEM_SIMDUTF_INTENDED_VERSION}" PARENT_SCOPE)
  set(THREAD_SYSTEM_SIMDUTF_SOURCE_REFERENCE "${THREAD_SYSTEM_SIMDUTF_FETCHCONTENT_TAG}" PARENT_SCOPE)
  set(THREAD_SYSTEM_SIMDUTF_SOURCE_REPOSITORY "${THREAD_SYSTEM_SIMDUTF_FETCHCONTENT_REPOSITORY}" PARENT_SCOPE)
  set(THREAD_SYSTEM_SIMDUTF_PACKAGE_CONFIG_PATH "" PARENT_SCOPE)
  if(DEFINED simdutf_SOURCE_DIR)
    set(THREAD_SYSTEM_SIMDUTF_RESOLVED_PATH "${simdutf_SOURCE_DIR}" PARENT_SCOPE)
  else()
    set(THREAD_SYSTEM_SIMDUTF_RESOLVED_PATH "" PARENT_SCOPE)
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
  set(gtest_disable_pthreads ON CACHE BOOL "Disable pthreads for Windows" FORCE)
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

  # Check for deprecated logger_system flag (Issue #336)
  check_logger_system_deprecation()

  find_common_system_dependency()
  # Note: fmt library is no longer used - using C++20 std::format exclusively
  find_simdutf_library()
  find_threading_library()
  find_or_fetch_gtest()

  if(DEFINED THREAD_SYSTEM_SIMDUTF_FOUND)
    set(THREAD_SYSTEM_SIMDUTF_FOUND ${THREAD_SYSTEM_SIMDUTF_FOUND} PARENT_SCOPE)
  endif()
  if(DEFINED THREAD_SYSTEM_SIMDUTF_TARGET)
    set(THREAD_SYSTEM_SIMDUTF_TARGET ${THREAD_SYSTEM_SIMDUTF_TARGET} PARENT_SCOPE)
  endif()
  if(DEFINED THREAD_SYSTEM_SIMDUTF_SOURCE)
    set(THREAD_SYSTEM_SIMDUTF_SOURCE ${THREAD_SYSTEM_SIMDUTF_SOURCE} PARENT_SCOPE)
  endif()
  if(DEFINED THREAD_SYSTEM_SIMDUTF_VERSION)
    set(THREAD_SYSTEM_SIMDUTF_VERSION ${THREAD_SYSTEM_SIMDUTF_VERSION} PARENT_SCOPE)
  endif()
  if(DEFINED THREAD_SYSTEM_SIMDUTF_SOURCE_REFERENCE)
    set(THREAD_SYSTEM_SIMDUTF_SOURCE_REFERENCE ${THREAD_SYSTEM_SIMDUTF_SOURCE_REFERENCE} PARENT_SCOPE)
  endif()
  if(DEFINED THREAD_SYSTEM_SIMDUTF_SOURCE_REPOSITORY)
    set(THREAD_SYSTEM_SIMDUTF_SOURCE_REPOSITORY ${THREAD_SYSTEM_SIMDUTF_SOURCE_REPOSITORY} PARENT_SCOPE)
  endif()
  if(DEFINED THREAD_SYSTEM_SIMDUTF_PACKAGE_CONFIG_PATH)
    set(THREAD_SYSTEM_SIMDUTF_PACKAGE_CONFIG_PATH ${THREAD_SYSTEM_SIMDUTF_PACKAGE_CONFIG_PATH} PARENT_SCOPE)
  endif()
  if(DEFINED THREAD_SYSTEM_SIMDUTF_RESOLVED_PATH)
    set(THREAD_SYSTEM_SIMDUTF_RESOLVED_PATH ${THREAD_SYSTEM_SIMDUTF_RESOLVED_PATH} PARENT_SCOPE)
  endif()

  write_simdutf_provenance_artifacts()
  if(DEFINED THREAD_SYSTEM_SIMDUTF_PROVENANCE_JSON)
    set(THREAD_SYSTEM_SIMDUTF_PROVENANCE_JSON ${THREAD_SYSTEM_SIMDUTF_PROVENANCE_JSON} PARENT_SCOPE)
  endif()
  if(DEFINED THREAD_SYSTEM_SIMDUTF_PROVENANCE_MARKDOWN)
    set(THREAD_SYSTEM_SIMDUTF_PROVENANCE_MARKDOWN ${THREAD_SYSTEM_SIMDUTF_PROVENANCE_MARKDOWN} PARENT_SCOPE)
  endif()

  message(STATUS "Dependency detection complete")
endfunction()
