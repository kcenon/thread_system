##################################################
# ThreadSystem Tests CMakeLists.txt
#
# Unified test directory structure:
#   tests/
#   ├── unit/          - Unit tests
#   ├── benchmarks/    - Performance benchmarks
#   └── stress/        - Stress tests (optional)
##################################################

# Add unit tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/CMakeLists.txt)
    add_subdirectory(unit)
    message(STATUS "Added tests/unit subdirectory")
endif()

# Add stress tests if enabled
if(BUILD_STRESS_TESTS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/stress AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/stress/CMakeLists.txt)
    add_subdirectory(stress)
    message(STATUS "Added tests/stress subdirectory")
endif()
