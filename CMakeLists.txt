##################################################
# ThreadSystem CMakeLists.txt
# 
# Main build configuration for the ThreadSystem library.
# This builds a high-performance, C++20 thread management system.
##################################################

# Specify minimum required CMake version for this project
cmake_minimum_required(VERSION 3.16)

# Define the project name and language
project(ThreadSystem)

# Option to control whether ThreadSystem is built as a standalone project or as a submodule
# - OFF (default): Builds as standalone project with samples and tests
# - ON: Builds only the core libraries for integration into other projects
option(BUILD_THREADSYSTEM_AS_SUBMODULE "Build ThreadSystem as submodule" OFF)

##################################################
# C++ Standard Configuration
##################################################

# Set C++20 as the default standard for all targets
set(CMAKE_CXX_STANDARD 20)           # Use C++20 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # Require C++20 support
set(CMAKE_CXX_EXTENSIONS OFF)        # Disable compiler-specific extensions
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Generate compile_commands.json for tooling

# Output compiler information for debugging purposes
message(STATUS "Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler version: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")

##################################################
# Compiler-Specific Configuration
##################################################

# Configure compiler flags based on the compiler being used
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  # For GCC and Clang compilers
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
  
  # Add additional flags for macOS
  if(APPLE)
    # Add linker flag to avoid duplicate library warnings on macOS
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-warn_duplicate_libraries")
  endif()
elseif(MSVC)
  # For Microsoft Visual C++ compiler
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest /Zc:__cplusplus")
endif()

##################################################
# Platform-Specific Threading Configuration
##################################################

# Handle threading differently for macOS vs other platforms
if(APPLE)
  # On macOS, we handle pthread differently
  add_definitions(-DHAVE_PTHREAD)           # Define that we have pthread support
  add_definitions(-DAPPLE_PLATFORM)         # Define that we're on Apple platform
  
  # Set thread-related variables without adding explicit link flags
  set(THREADS_FOUND TRUE)
  set(CMAKE_HAVE_THREADS_LIBRARY 1)
  set(Threads_FOUND TRUE)
else()
  # On other platforms, use CMake's thread package finder
  set(THREADS_PREFER_PTHREAD_FLAG ON)       # Prefer using -pthread flag when available
  find_package(Threads REQUIRED)            # Find threading library
endif()

##################################################
# Feature Detection
##################################################

# Include the module for checking C++ source compilation
include(CheckCXXSourceCompiles)

# Options to control whether specific C++20 features are used
option(SET_STD_FORMAT "Set use std::format if it can use" ON)
option(SET_STD_JTHREAD "Set use std::jthread if it can use" ON)
option(SET_STD_CHRONO_CURRENT_ZONE "Set use std::chrono::current_zone if it can use" ON)

# Function to test for C++20 features at configure time
# Parameters:
# - FEATURE_NAME: Name of the feature being checked (for logging)
# - TEST_CODE: C++ code snippet that tests the feature
# - RESULT_VAR: Variable name to store the result (TRUE/FALSE)
function(check_cxx20_feature FEATURE_NAME TEST_CODE RESULT_VAR)
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_EXE_LINKER_FLAGS}")
  
  check_cxx_source_compiles("
    #include <cstddef>
    #include <cstdint>
    #include <utility>
    ${TEST_CODE}
  " ${RESULT_VAR})
endfunction()

##################################################
# Test for std::format (C++20 feature)
##################################################

check_cxx20_feature(std_format "
  #include <format>
  #include <string>
  int main() {
    std::string s = std::format(\"{} {}\", \"Hello\", \"World\");
    return 0;
  }
" HAS_STD_FORMAT)

if(HAS_STD_FORMAT AND SET_STD_FORMAT)
  add_definitions(-DUSE_STD_FORMAT)  # Define macro to use std::format in code
  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has support for std::format.")
else()
  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} does not have support for std::format, so will use the fmt::format instead.")
endif()

##################################################
# Test for std::jthread (C++20 feature)
##################################################

check_cxx20_feature(std_jthread "
  #include <thread>
  #include <stop_token>
  int main() {
    std::jthread t([](std::stop_token st) {});
    return 0;
  }
" HAS_STD_JTHREAD)

if(HAS_STD_JTHREAD AND SET_STD_JTHREAD)
  add_definitions(-DUSE_STD_JTHREAD)  # Define macro to use std::jthread in code
  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has support for std::jthread.")
else()
  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} does not have support for std::jthread, so will use the std::thread instead.")
endif()

##################################################
# Test for std::chrono::current_zone (C++20 feature)
##################################################

check_cxx20_feature(std_chrono_current_zone "
  #include <chrono>
  int main() {
    const auto now = std::chrono::system_clock::now();
    const auto local_time = std::chrono::current_zone()->to_local(now);
    return 0;
  }
" HAS_STD_CHRONO_CURRENT_ZONE)

if(HAS_STD_CHRONO_CURRENT_ZONE AND SET_STD_CHRONO_CURRENT_ZONE)
  add_definitions(-DUSE_STD_CHRONO_CURRENT_ZONE)  # Define macro to use std::chrono::current_zone in code
  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has support for std::chrono::current_zone.")
else()
  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} does not have support for std::chrono::current_zone, so will use the time_t instead.")
endif()

##################################################
# Build Configuration
##################################################

if(NOT BUILD_THREADSYSTEM_AS_SUBMODULE)
  # When building as a standalone project:
  
  # Set output directories for binaries, libraries, and archives
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

  # Verify that vcpkg toolchain is provided
  if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(FATAL_ERROR "Please provide the path to vcpkg toolchain file using -DCMAKE_TOOLCHAIN_FILE=<path_to_vcpkg_toolchain>")
  endif()

  # Add subdirectories for source code, samples, and unit tests
  add_subdirectory(sources)  # Core library code
  add_subdirectory(samples)  # Example applications
  
  # Disable unit tests on macOS due to known pthread/gtest linkage issues
  if(UNIX AND NOT APPLE)
    add_subdirectory(unittest)  # Unit tests
  endif()
  
  # Add benchmarks directory if requested
  option(BUILD_BENCHMARKS "Build performance benchmarks" OFF)
  if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)  # Performance benchmarks
  endif()
else()
  # When building as a submodule of another project:
  # Only build the core library (no samples or tests)
  add_subdirectory(sources)
endif()