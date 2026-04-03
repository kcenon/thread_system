# CMake Config Validation Test
# Test standardized CMake config files

cmake_minimum_required(VERSION 3.16)

# Set CMAKE_PREFIX_PATH to find our config files
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build" ${CMAKE_PREFIX_PATH})

# Test 1: Find package with standardized naming
message(STATUS "Testing find_package(thread_system CONFIG REQUIRED)...")
find_package(thread_system CONFIG REQUIRED)

if(thread_system_FOUND)
    message(STATUS "PASS: thread_system found with config files")
    message(STATUS "   Libraries: ${thread_system_LIBRARIES}")

    # Test component targets
    foreach(target ${thread_system_COMPONENT_TARGETS})
        if(TARGET ${target})
            message(STATUS "   Target available: ${target}")
        else()
            message(STATUS "   Target missing: ${target}")
        endif()
    endforeach()
else()
    message(FATAL_ERROR "FAIL: thread_system not found")
endif()

# Test 2: Component-based find
message(STATUS "Testing component-based find...")
find_package(thread_system CONFIG REQUIRED COMPONENTS thread_pool service_container)

if(thread_system_FOUND)
    message(STATUS "PASS: Component-based find successful")
else()
    message(STATUS "FAIL: Component-based find failed")
endif()

message(STATUS "")
message(STATUS "thread_system CMake Config Validation Complete")
message(STATUS "==========================================")
