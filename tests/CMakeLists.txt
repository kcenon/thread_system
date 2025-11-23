##################################################
# ThreadSystem Tests CMakeLists.txt
#
# Unified test directory structure:
#   tests/
#   ├── unit/          - Unit tests
#   ├── benchmarks/    - Performance benchmarks
#   └── stress/        - Stress tests (optional)
##################################################

# Add unit tests only when testing is enabled and GTest is available
if(BUILD_TESTING)
    find_package(GTest QUIET)
    if(GTest_FOUND OR TARGET GTest::gtest)
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/CMakeLists.txt)
            add_subdirectory(unit)
            message(STATUS "Added tests/unit subdirectory")
        endif()
    else()
        message(STATUS "GTest not found - skipping unit tests")
    endif()
else()
    message(STATUS "BUILD_TESTING is OFF - skipping unit tests")
endif()

# Add stress tests if enabled
if(BUILD_STRESS_TESTS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/stress AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/stress/CMakeLists.txt)
    add_subdirectory(stress)
    message(STATUS "Added tests/stress subdirectory")
endif()
