# THR-009: Refactor CMake Build System

**ID**: THR-009
**Category**: BUILD
**Priority**: MEDIUM
**Status**: TODO
**Estimated Duration**: 6-8 days
**Dependencies**: THR-008 (recommended)
**Completed**: 2025-11-23

---

## Summary

Refactor the CMake build system to reduce complexity, improve maintainability, and follow modern CMake best practices. Current system has 51+ CMakeLists.txt files.

---

## Background

### Current State
- **CMakeLists.txt count**: 51 files (including build artifacts)
- **Structure**: Deeply nested, some duplication
- **Issues**:
  - Inconsistent target naming
  - Manual dependency tracking
  - Platform-specific code scattered
  - Limited use of modern CMake features

### Technical Debt
From PRIORITY_DIRECTIVE.md:
> CMake complexity increase (51 CMakeLists.txt files)

---

## Acceptance Criteria

- [ ] Reduced CMakeLists.txt count (target: 15-20)
- [ ] Modern CMake 3.20+ features used
- [ ] Consistent target naming convention
- [ ] Proper INTERFACE/PUBLIC/PRIVATE usage
- [ ] Platform-specific code centralized
- [ ] Presets for common configurations
- [ ] FetchContent for dependencies
- [ ] Improved build time
- [ ] Clear documentation

---

## Target Structure

```
thread_system/
├── CMakeLists.txt              # Root: project setup, options
├── cmake/
│   ├── ThreadSystemConfig.cmake
│   ├── Dependencies.cmake      # FetchContent
│   ├── CompilerFlags.cmake     # Platform/compiler settings
│   ├── Targets.cmake           # Target definitions
│   └── Install.cmake           # Installation rules
├── src/
│   └── CMakeLists.txt          # Single file for all sources
├── tests/
│   └── CMakeLists.txt          # Single file for all tests
├── benchmarks/
│   └── CMakeLists.txt          # Single file
├── examples/
│   └── CMakeLists.txt          # Single file
└── CMakePresets.json           # Build presets
```

---

## Implementation Tasks

### Phase 1: Modern CMake Setup

```cmake
# CMakeLists.txt (root)
cmake_minimum_required(VERSION 3.20)

project(thread_system
    VERSION 2.0.0
    LANGUAGES CXX
    DESCRIPTION "High-performance thread pool library"
)

# Options
option(THREAD_SYSTEM_BUILD_TESTS "Build tests" ON)
option(THREAD_SYSTEM_BUILD_BENCHMARKS "Build benchmarks" ON)
option(THREAD_SYSTEM_BUILD_EXAMPLES "Build examples" ON)
option(THREAD_SYSTEM_ENABLE_SANITIZERS "Enable sanitizers" OFF)

# Include modules
include(cmake/CompilerFlags.cmake)
include(cmake/Dependencies.cmake)

# Main library
add_subdirectory(src)

# Optional components
if(THREAD_SYSTEM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(THREAD_SYSTEM_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(THREAD_SYSTEM_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Installation
include(cmake/Install.cmake)
```

### Phase 2: Target-based Dependencies

```cmake
# cmake/Targets.cmake
# Main library target
add_library(thread_system)
add_library(thread_system::thread_system ALIAS thread_system)

target_sources(thread_system
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/impl/thread_pool/thread_pool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/impl/thread_pool/thread_worker.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/thread_base.cpp
)

target_include_directories(thread_system
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(thread_system PUBLIC cxx_std_20)

# Platform-specific
if(WIN32)
    target_compile_definitions(thread_system PRIVATE WIN32_LEAN_AND_MEAN)
elseif(APPLE)
    target_compile_definitions(thread_system PRIVATE THREAD_SYSTEM_MACOS)
endif()
```

### Phase 3: Dependency Management

```cmake
# cmake/Dependencies.cmake
include(FetchContent)

# Google Test
if(THREAD_SYSTEM_BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
endif()

# Google Benchmark
if(THREAD_SYSTEM_BUILD_BENCHMARKS)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.8.3
    )
    set(BENCHMARK_ENABLE_TESTING OFF)
    FetchContent_MakeAvailable(benchmark)
endif()
```

### Phase 4: Build Presets

```json
// CMakePresets.json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "default",
      "binaryDir": "${sourceDir}/build",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release"
      }
    },
    {
      "name": "debug",
      "inherits": "default",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "THREAD_SYSTEM_ENABLE_SANITIZERS": "ON"
      }
    },
    {
      "name": "ci",
      "inherits": "default",
      "cacheVariables": {
        "THREAD_SYSTEM_BUILD_TESTS": "ON",
        "THREAD_SYSTEM_BUILD_BENCHMARKS": "ON"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "default",
      "configurePreset": "default"
    },
    {
      "name": "debug",
      "configurePreset": "debug"
    }
  ]
}
```

### Phase 5: Consolidate Test CMake

```cmake
# tests/CMakeLists.txt (single file)
include(GoogleTest)

# Collect all test sources
file(GLOB_RECURSE TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/integration/*.cpp"
)

# Single test executable
add_executable(thread_system_tests ${TEST_SOURCES})

target_link_libraries(thread_system_tests
    PRIVATE
        thread_system::thread_system
        GTest::gtest_main
        GTest::gmock
)

gtest_discover_tests(thread_system_tests)
```

---

## Migration Plan

1. **Week 1**: Create cmake/ modules, test on feature branch
2. **Week 2**: Migrate src/ to target-based CMake
3. **Week 3**: Consolidate tests/, benchmarks/, examples/
4. **Week 4**: Documentation, CI updates, validation

---

## Success Metrics

| Metric | Current | Target |
|--------|---------|--------|
| CMakeLists.txt count | 51 | 15-20 |
| Configure time | ~15s | ~5s |
| Full build time | - | -10% |
| Lines of CMake | ~2000 | ~800 |
| Modern CMake usage | Partial | 100% |

---

## Files to Create/Modify

### Create
- `cmake/CompilerFlags.cmake`
- `cmake/Dependencies.cmake`
- `cmake/Targets.cmake`
- `cmake/Install.cmake`

### Modify
- `CMakeLists.txt` (root)
- `CMakePresets.json`
- `src/CMakeLists.txt`
- `tests/CMakeLists.txt`

### Delete
- Multiple nested CMakeLists.txt files
