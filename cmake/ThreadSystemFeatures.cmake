##################################################
# ThreadSystemFeatures.cmake
#
# Feature detection module for ThreadSystem
# Detects C++17/C++20 standard library features
# Strategy: C++17 minimum, C++20 optional
##################################################

include(CheckCXXSourceCompiles)

# Function to test for C++20 features at configure time
# Note: This will fail gracefully in C++17 mode and use fallback implementations
function(check_cxx20_feature FEATURE_NAME TEST_CODE RESULT_VAR)
  # Use CMAKE_CXX_STANDARD for standard selection (more reliable across compilers)
  set(CMAKE_REQUIRED_FLAGS "")
  set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_EXE_LINKER_FLAGS}")

  # Set C++20 standard requirement
  if(MSVC)
    # MSVC requires explicit C++20 flags:
    # - /std:c++20: Enable C++20 standard
    # - /EHsc: Enable C++ exception handling
    # - /Zc:__cplusplus: Make __cplusplus report correct value (needed for feature detection)
    # - /permissive-: Enable strict standards conformance mode
    set(CMAKE_REQUIRED_FLAGS "/std:c++20 /EHsc /Zc:__cplusplus /permissive-")
  else()
    # GCC/Clang use -std=c++20
    set(CMAKE_REQUIRED_FLAGS "-std=c++20 ${CMAKE_CXX_FLAGS}")
  endif()

  check_cxx_source_compiles("
    #include <cstddef>
    #include <cstdint>
    #include <utility>
    ${TEST_CODE}
  " ${RESULT_VAR})
endfunction()

##################################################
# Check std::format support
##################################################
function(check_std_format_support)
  # C++20 std::format is required - no fallback to fmt library
  # This project requires C++20 compliant compilers (GCC 13+, Clang 14+, MSVC 19.29+)

  # MSVC: Use version-based detection instead of compile tests
  # Reason: CMake's check_cxx_source_compiles has issues with Ninja generator
  # because MSVC flags like /Zc:__cplusplus aren't properly propagated
  # MSVC 19.29+ (VS 2019 16.10+) has full std::format support
  if(MSVC)
    if(MSVC_VERSION GREATER_EQUAL 1929)
      message(STATUS "MSVC ${MSVC_VERSION} detected - using version-based std::format detection")
      add_definitions(-DUSE_STD_FORMAT)
      set(USE_STD_FORMAT TRUE CACHE BOOL "Using std::format (C++20)" FORCE)
      message(STATUS "✅ Using std::format (MSVC ${MSVC_VERSION} - version check)")
      set(USE_STD_FORMAT TRUE PARENT_SCOPE)
      return()
    else()
      message(FATAL_ERROR "❌ MSVC ${MSVC_VERSION} does not support std::format.\n"
        "Please use MSVC 19.29 or later (Visual Studio 2019 16.10+)")
    endif()
  endif()

  # GCC/Clang: Use compile tests for feature detection
  # First check basic std::format availability
  check_cxx20_feature(std_format_basic "
    #include <format>
    #include <string>

    int main() {
      std::string s = std::format(\"{} {}\", \"Hello\", \"World\");
      return 0;
    }
  " HAS_STD_FORMAT_BASIC)

  # Then check formatter specialization support
  check_cxx20_feature(std_format_specialization "
    #include <format>
    #include <string>
    #include <string_view>

    struct test_type {
      std::string to_string() const { return \"test\"; }
    };

    template<>
    struct std::formatter<test_type> : std::formatter<std::string_view> {
      template<typename FormatContext>
      auto format(const test_type& t, FormatContext& ctx) const {
        return std::formatter<std::string_view>::format(t.to_string(), ctx);
      }
    };

    int main() {
      std::string basic = std::format(\"{} {}\", \"Hello\", \"World\");
      test_type t;
      std::string custom = std::format(\"{}\", t);
      return 0;
    }
  " HAS_STD_FORMAT_SPECIALIZATION)

  if(HAS_STD_FORMAT_BASIC AND HAS_STD_FORMAT_SPECIALIZATION)
    # Final verification: compile test
    try_compile(STD_FORMAT_COMPILE_TEST
      ${CMAKE_BINARY_DIR}/format_test
      SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_std_format.cpp
      CXX_STANDARD 20
      CXX_STANDARD_REQUIRED ON
      OUTPUT_VARIABLE COMPILE_OUTPUT
    )

    if(STD_FORMAT_COMPILE_TEST)
      add_definitions(-DUSE_STD_FORMAT)
      set(USE_STD_FORMAT TRUE CACHE BOOL "Using std::format (C++20)" FORCE)
      message(STATUS "✅ Using std::format (C++20 standard)")
    else()
      message(FATAL_ERROR "❌ std::format compile test failed. This project requires C++20 std::format support.\n"
        "Please use a compatible compiler:\n"
        "  - GCC 13 or later\n"
        "  - Clang 14 or later\n"
        "  - MSVC 19.29 or later (Visual Studio 2019 16.10+)\n"
        "Compile output: ${COMPILE_OUTPUT}")
    endif()
  else()
    message(FATAL_ERROR "❌ std::format is not available. This project requires C++20 std::format support.\n"
      "Please use a compatible compiler:\n"
      "  - GCC 13 or later\n"
      "  - Clang 14 or later\n"
      "  - MSVC 19.29 or later (Visual Studio 2019 16.10+)")
  endif()

  # Export result to parent scope
  set(USE_STD_FORMAT TRUE PARENT_SCOPE)
endfunction()

##################################################
# Check std::jthread support
##################################################
function(check_std_jthread_support)
  option(SET_STD_JTHREAD "Use std::jthread if available" ON)

  check_cxx20_feature(std_jthread "
    #include <thread>
    #include <stop_token>
    int main() {
      std::jthread t([](std::stop_token st) {});
      return 0;
    }
  " HAS_STD_JTHREAD)

  if(HAS_STD_JTHREAD AND SET_STD_JTHREAD)
    add_definitions(-DUSE_STD_JTHREAD)
    message(STATUS "✅ Using std::jthread")
  else()
    message(STATUS "Using std::thread fallback")
  endif()
endfunction()

##################################################
# Check std::latch and std::barrier support
##################################################
function(check_std_latch_support)
  option(SET_STD_LATCH "Use std::latch and std::barrier if available" ON)

  check_cxx20_feature(std_latch "
    #include <latch>
    #include <barrier>
    int main() {
      std::latch l(1);
      std::barrier b(1);
      return 0;
    }
  " HAS_STD_LATCH)

  if(HAS_STD_LATCH AND SET_STD_LATCH)
    add_definitions(-DHAS_STD_LATCH)
    message(STATUS "✅ Using std::latch and std::barrier")
  else()
    message(STATUS "Using custom latch/barrier implementation")
  endif()
endfunction()

##################################################
# Check std::atomic::wait support
##################################################
function(check_std_atomic_wait_support)
  option(SET_STD_ATOMIC_WAIT "Use std::atomic::wait if available" ON)

  check_cxx20_feature(std_atomic_wait "
    #include <atomic>
    int main() {
      std::atomic<int> a{0};
      a.wait(0);
      a.notify_one();
      return 0;
    }
  " HAS_STD_ATOMIC_WAIT)

  if(HAS_STD_ATOMIC_WAIT AND SET_STD_ATOMIC_WAIT)
    add_definitions(-DHAS_STD_ATOMIC_WAIT)
    message(STATUS "✅ Using std::atomic::wait/notify")
  else()
    message(STATUS "Using custom atomic wait/notify implementation")
  endif()
endfunction()

##################################################
# Check std::chrono::current_zone support
##################################################
function(check_std_chrono_current_zone_support)
  option(SET_STD_CHRONO_CURRENT_ZONE "Use std::chrono::current_zone if available" ON)

  check_cxx20_feature(std_chrono_current_zone "
    #include <chrono>
    int main() {
      const auto now = std::chrono::system_clock::now();
      const auto local_time = std::chrono::current_zone()->to_local(now);
      return 0;
    }
  " HAS_STD_CHRONO_CURRENT_ZONE)

  if(HAS_STD_CHRONO_CURRENT_ZONE AND SET_STD_CHRONO_CURRENT_ZONE)
    add_definitions(-DUSE_STD_CHRONO_CURRENT_ZONE)
    message(STATUS "✅ Using std::chrono::current_zone")
  else()
    message(STATUS "Using time_t fallback")
  endif()
endfunction()

##################################################
# Check std::span support
##################################################
function(check_std_span_support)
  option(SET_STD_SPAN "Use std::span if available" ON)

  check_cxx20_feature(std_span_basic "
    #include <span>
    #include <vector>

    int main() {
      std::vector<int> vec = {1, 2, 3};
      std::span<int> s(vec);
      return s.size();
    }
  " HAS_STD_SPAN_BASIC)

  if(HAS_STD_SPAN_BASIC)
    try_compile(STD_SPAN_COMPILE_TEST
      ${CMAKE_BINARY_DIR}/span_test
      SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_std_span.cpp
      CXX_STANDARD 20
      CXX_STANDARD_REQUIRED ON
    )

    if(STD_SPAN_COMPILE_TEST AND SET_STD_SPAN)
      add_definitions(-DUSE_STD_SPAN)
      message(STATUS "✅ Using std::span")
    endif()
  else()
    message(STATUS "Using fallback span implementation")
  endif()
endfunction()

##################################################
# Check std::filesystem support
##################################################
function(check_std_filesystem_support)
  option(SET_STD_FILESYSTEM "Use std::filesystem if available" ON)

  check_cxx_source_compiles("
    #include <filesystem>
    int main() {
      std::filesystem::path p(\"/tmp\");
      return std::filesystem::exists(p) ? 0 : 1;
    }
  " HAS_STD_FILESYSTEM)

  if(SET_STD_FILESYSTEM AND HAS_STD_FILESYSTEM)
    try_compile(STD_FILESYSTEM_COMPILE_TEST
      ${CMAKE_BINARY_DIR}/filesystem_test
      SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_std_filesystem.cpp
      CXX_STANDARD 20
      CXX_STANDARD_REQUIRED ON
    )

    if(STD_FILESYSTEM_COMPILE_TEST)
      add_definitions(-DUSE_STD_FILESYSTEM)
      message(STATUS "✅ Using std::filesystem")
    endif()
  endif()
endfunction()

##################################################
# Check std::ranges support
##################################################
function(check_std_ranges_support)
  option(SET_STD_RANGES "Use std::ranges if available" ON)

  check_cxx_source_compiles("
    #include <ranges>
    #include <vector>
    int main() {
      std::vector<int> v = {1, 2, 3, 4, 5};
      auto even = v | std::views::filter([](int i) { return i % 2 == 0; });
      return 0;
    }
  " HAS_STD_RANGES)

  if(SET_STD_RANGES AND HAS_STD_RANGES)
    try_compile(STD_RANGES_COMPILE_TEST
      ${CMAKE_BINARY_DIR}/ranges_test
      SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_std_ranges.cpp
      CXX_STANDARD 20
      CXX_STANDARD_REQUIRED ON
    )

    if(STD_RANGES_COMPILE_TEST)
      add_definitions(-DUSE_STD_RANGES)
      message(STATUS "✅ Using std::ranges")
    endif()
  endif()
endfunction()

##################################################
# Check std::concepts support
##################################################
function(check_std_concepts_support)
  option(SET_STD_CONCEPTS "Use std::concepts if available" ON)

  if(NOT SET_STD_CONCEPTS)
    message(STATUS "std::concepts support disabled by user")
    return()
  endif()

  # Try compile with a comprehensive test file
  try_compile(STD_CONCEPTS_COMPILE_TEST
    ${CMAKE_BINARY_DIR}/concepts_test
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_std_concepts.cpp
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    OUTPUT_VARIABLE CONCEPTS_COMPILE_OUTPUT
  )

  if(STD_CONCEPTS_COMPILE_TEST)
    add_definitions(-DUSE_STD_CONCEPTS)
    message(STATUS "✅ Using std::concepts")
  else()
    # Only show detailed error in verbose mode
    if(CMAKE_VERBOSE_MAKEFILE)
      message(STATUS "std::concepts compile output: ${CONCEPTS_COMPILE_OUTPUT}")
    endif()
    message(STATUS "std::concepts not fully supported - continuing without concepts")
  endif()
endfunction()

##################################################
# Check common_system C++20 Concepts support
##################################################
function(check_common_concepts_support)
  option(SET_COMMON_CONCEPTS "Use common_system C++20 concepts if available" ON)

  if(NOT SET_COMMON_CONCEPTS)
    message(STATUS "common_system concepts support disabled by user")
    return()
  endif()

  if(NOT BUILD_WITH_COMMON_SYSTEM)
    message(STATUS "common_system not enabled - skipping concepts check")
    return()
  endif()

  # Check compiler version requirements for C++20 concepts:
  # - GCC 10+
  # - Clang 10+
  # - Apple Clang 12+
  # - MSVC 2019 16.3+ (19.23+)
  set(_COMPILER_SUPPORTS_CONCEPTS FALSE)

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "10.0")
      set(_COMPILER_SUPPORTS_CONCEPTS TRUE)
    endif()
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "10.0")
      set(_COMPILER_SUPPORTS_CONCEPTS TRUE)
    endif()
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "12.0")
      set(_COMPILER_SUPPORTS_CONCEPTS TRUE)
    endif()
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if(MSVC_VERSION GREATER_EQUAL 1923)
      set(_COMPILER_SUPPORTS_CONCEPTS TRUE)
    endif()
  endif()

  if(NOT _COMPILER_SUPPORTS_CONCEPTS)
    message(STATUS "Compiler does not meet minimum version for C++20 concepts")
    message(STATUS "  Required: GCC 10+, Clang 10+, Apple Clang 12+, MSVC 19.23+")
    return()
  endif()

  # Check if common_system concepts header exists
  set(_COMMON_CONCEPTS_PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/../common_system/include/kcenon/common/concepts/concepts.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../common_system/include/kcenon/common/concepts/concepts.h"
    "${COMMON_SYSTEM_INCLUDE_DIR}/kcenon/common/concepts/concepts.h"
  )

  set(_COMMON_CONCEPTS_FOUND FALSE)
  foreach(_path ${_COMMON_CONCEPTS_PATHS})
    if(EXISTS "${_path}")
      set(_COMMON_CONCEPTS_FOUND TRUE)
      message(STATUS "Found common_system concepts at: ${_path}")
      break()
    endif()
  endforeach()

  if(_COMMON_CONCEPTS_FOUND)
    add_definitions(-DTHREAD_HAS_COMMON_CONCEPTS=1)
    set(THREAD_HAS_COMMON_CONCEPTS TRUE CACHE BOOL "common_system C++20 concepts available" FORCE)
    message(STATUS "✅ Using common_system C++20 concepts")
    message(STATUS "   Available concept categories:")
    message(STATUS "   - Core: Resultable, Unwrappable, Mappable, Chainable")
    message(STATUS "   - Callable: VoidCallable, Predicate, JobLike, ExecutorLike")
    message(STATUS "   - Event: EventType, EventHandler, EventFilter")
    message(STATUS "   - Service: ServiceInterface, ServiceImplementation")
    message(STATUS "   - Container: SequenceContainer, AssociativeContainer")
  else()
    message(STATUS "common_system concepts header not found - concepts not available")
    set(THREAD_HAS_COMMON_CONCEPTS FALSE CACHE BOOL "common_system C++20 concepts available" FORCE)
  endif()
endfunction()

##################################################
# Main function to check all features
##################################################
function(check_thread_system_features)
  message(STATUS "Checking C++20 feature support...")

  check_std_format_support()
  check_std_jthread_support()
  check_std_latch_support()
  check_std_atomic_wait_support()
  check_std_chrono_current_zone_support()
  check_std_span_support()
  check_std_filesystem_support()
  check_std_ranges_support()
  check_std_concepts_support()
  check_common_concepts_support()

  message(STATUS "Feature detection complete")
endfunction()
