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
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
  set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_EXE_LINKER_FLAGS}")

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
  # Options to control feature usage
  # Changed default: Always use std::format (C++20 standard) unless explicitly disabled
  option(USE_STD_FORMAT "Use std::format (C++20 standard)" ON)
  option(FORCE_FMT_FORMAT "Force use of fmt library instead of std::format" OFF)

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

  # If user explicitly wants fmt, skip std::format checks
  if(FORCE_FMT_FORMAT)
    message(STATUS "⚙️  FORCE_FMT_FORMAT enabled - using fmt library")
    set(USE_STD_FORMAT FALSE CACHE BOOL "Not using std::format" FORCE)
    set(USE_STD_FORMAT ${USE_STD_FORMAT} PARENT_SCOPE)
    return()
  endif()

  # Determine if we should use std::format (default: ON)
  set(USE_STD_FORMAT_ENABLED ${USE_STD_FORMAT})

  if(USE_STD_FORMAT_ENABLED AND HAS_STD_FORMAT_BASIC AND HAS_STD_FORMAT_SPECIALIZATION)
    # Final verification: compile test
    try_compile(STD_FORMAT_COMPILE_TEST
      ${CMAKE_BINARY_DIR}/format_test
      SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_std_format.cpp
      CXX_STANDARD 20
      CXX_STANDARD_REQUIRED ON
      OUTPUT_VARIABLE COMPILE_OUTPUT
    )

    if(STD_FORMAT_COMPILE_TEST)
      set(USE_STD_FORMAT_ENABLED TRUE)
      message(STATUS "✅ Using std::format (C++20 standard)")
    else()
      message(STATUS "❌ std::format compile test failed - fallback to fmt library if available")
      set(USE_STD_FORMAT_ENABLED FALSE)
    endif()
  elseif(USE_STD_FORMAT_ENABLED)
    message(STATUS "⚠️  std::format requested but not fully supported - fallback to fmt library if available")
    set(USE_STD_FORMAT_ENABLED FALSE)
  endif()

  # Apply the decision
  if(USE_STD_FORMAT_ENABLED)
    add_definitions(-DUSE_STD_FORMAT)
    set(USE_STD_FORMAT TRUE CACHE BOOL "Using std::format (C++20)" FORCE)
    message(STATUS "std::format enabled - fmt library not required")
  else()
    set(USE_STD_FORMAT FALSE CACHE BOOL "Not using std::format" FORCE)
    message(STATUS "std::format disabled - fmt library will be searched if needed")
  endif()

  # Export result to parent scope
  set(USE_STD_FORMAT ${USE_STD_FORMAT} PARENT_SCOPE)
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
# Main function to check all features
##################################################
function(check_thread_system_features)
  message(STATUS "Checking C++20 feature support...")

  check_std_format_support()
  check_std_jthread_support()
  check_std_chrono_current_zone_support()
  check_std_span_support()
  check_std_filesystem_support()
  check_std_ranges_support()
  check_std_concepts_support()

  message(STATUS "Feature detection complete")
endfunction()
