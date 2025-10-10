##################################################
# ThreadSystemTargets.cmake
#
# Target creation and configuration for ThreadSystem
# Handles library targets and subdirectory builds
##################################################

##################################################
# Configure build directories
##################################################
function(setup_build_directories)
  # Set output directories for binaries, libraries, and archives
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin PARENT_SCOPE)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib PARENT_SCOPE)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib PARENT_SCOPE)

  message(STATUS "Build directories configured")
endfunction()

##################################################
# Create ThreadSystem library targets
##################################################
function(create_thread_system_targets)
  # Set up include directories
  set(THREAD_SYSTEM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
  set(THREAD_SYSTEM_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

  # Add include directories
  include_directories(${THREAD_SYSTEM_INCLUDE_DIR})
  include_directories(${CMAKE_CURRENT_SOURCE_DIR})  # For legacy structure

  # Check for new structure
  if(EXISTS ${THREAD_SYSTEM_INCLUDE_DIR}/kcenon/thread AND EXISTS ${THREAD_SYSTEM_SOURCE_DIR})
    message(STATUS "Using new directory structure")

    # Collect source files
    file(GLOB_RECURSE THREAD_SYSTEM_HEADERS
      ${THREAD_SYSTEM_INCLUDE_DIR}/kcenon/thread/*.h
    )

    file(GLOB_RECURSE THREAD_SYSTEM_SOURCES
      ${THREAD_SYSTEM_SOURCE_DIR}/*.cpp
    )

    # Check if we have enough sources
    list(LENGTH THREAD_SYSTEM_SOURCES SOURCE_COUNT)
    if(SOURCE_COUNT LESS 10)
      message(STATUS "Limited sources in new structure, using hybrid approach")
      set(USE_LEGACY_BUILD TRUE PARENT_SCOPE)
    else()
      # Create the main library
      add_library(ThreadSystem STATIC
        ${THREAD_SYSTEM_SOURCES}
        ${THREAD_SYSTEM_HEADERS}
      )

      target_include_directories(ThreadSystem
        PUBLIC
          $<BUILD_INTERFACE:${THREAD_SYSTEM_INCLUDE_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<INSTALL_INTERFACE:include>
      )
      set(USE_LEGACY_BUILD FALSE PARENT_SCOPE)
      message(STATUS "Created ThreadSystem library target")
    endif()
  else()
    message(STATUS "New structure not complete, using legacy build")
    set(USE_LEGACY_BUILD TRUE PARENT_SCOPE)
  endif()
endfunction()

##################################################
# Add legacy subdirectories
##################################################
function(add_legacy_subdirectories)
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/utilities)
    add_subdirectory(utilities)
    message(STATUS "Added utilities subdirectory")
  endif()

  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/interfaces)
    add_subdirectory(interfaces)
    message(STATUS "Added interfaces subdirectory")
  endif()

  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/core)
    add_subdirectory(core)
    message(STATUS "Added core subdirectory")
  endif()

  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/implementations)
    add_subdirectory(implementations)
    message(STATUS "Added implementations subdirectory")
  endif()
endfunction()

##################################################
# Add examples and samples
##################################################
function(add_examples_and_samples)
  if(NOT DEFINED BUILD_SAMPLES OR BUILD_SAMPLES)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples)
      add_subdirectory(examples)
      message(STATUS "Added examples subdirectory")
    elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/samples)
      add_subdirectory(samples)
      message(STATUS "Added samples subdirectory")
    endif()
  endif()
endfunction()

##################################################
# Add tests
##################################################
function(add_tests_subdirectory)
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
    message(STATUS "Added tests subdirectory (new structure)")
  elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unittest)
    add_subdirectory(unittest)
    message(STATUS "Added unittest subdirectory (legacy)")
  endif()

  # Add integration tests if they exist
  # Note: BUILD_INTEGRATION_TESTS is defined in top-level CMakeLists.txt
  if(BUILD_INTEGRATION_TESTS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration_tests AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration_tests/CMakeLists.txt)
    add_subdirectory(integration_tests)
    message(STATUS "Added integration_tests subdirectory")
  endif()
endfunction()

##################################################
# Add benchmarks (optional)
##################################################
function(add_benchmarks_subdirectory)
  option(BUILD_BENCHMARKS "Build performance benchmarks" OFF)

  if(BUILD_BENCHMARKS)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/benchmarks)
      add_subdirectory(tests/benchmarks)
      message(STATUS "Added benchmarks subdirectory")
    elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks)
      add_subdirectory(benchmarks)
      message(STATUS "Added legacy benchmarks subdirectory")
    endif()
  endif()
endfunction()

##################################################
# Setup documentation (Doxygen)
##################################################
function(setup_documentation)
  option(BUILD_DOCUMENTATION "Build Doxygen documentation" ON)

  if(BUILD_DOCUMENTATION)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
      add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
      )
      message(STATUS "Doxygen documentation target added")
    else()
      message(WARNING "Doxygen not found; 'docs' target will not be available")
    endif()
  endif()
endfunction()
