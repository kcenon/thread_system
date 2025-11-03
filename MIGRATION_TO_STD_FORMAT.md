# Migration to std::format (C++20)

## Overview

thread_system has been updated to use C++20's `std::format` as the default formatting library, removing the dependency on the external `fmt` library. This change aligns with modern C++ standards and reduces external dependencies.

## Changes Made

### 1. vcpkg.json
- **Removed**: `fmt` from the required dependencies list
- **Rationale**: C++20's `std::format` provides equivalent functionality

### 2. CMakeLists.txt
- **Changed**: Feature detection now runs before dependency detection
- **Reason**: `USE_STD_FORMAT` flag must be set before attempting to find `fmt`

### 3. cmake/ThreadSystemFeatures.cmake
- **Changed**: `USE_STD_FORMAT` option default is now `ON`
- **Simplified**: Removed Windows-specific restrictions for `std::format`
- **Added**: `FORCE_FMT_FORMAT` option to explicitly use `fmt` if needed
- **Improved**: Better status messages during configuration

### 4. cmake/ThreadSystemDependencies.cmake
- **Changed**: `find_fmt_library()` now skips search when `USE_STD_FORMAT` is `TRUE`
- **Improved**: Clearer status messages about format library selection

## Build Configuration

### Default Behavior (C++20 std::format)
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Force fmt Library Usage
If you need to use the `fmt` library instead of `std::format`:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DFORCE_FMT_FORMAT=ON
cmake --build build
```

## Verification

All tests pass with `std::format`:
- thread_base_unit: 56 tests passed
- utilities_unit: 34 tests passed
- All other unit tests and integration tests: passed

## Compatibility

### Minimum Requirements
- **C++ Standard**: C++20 (already required)
- **Compiler Support**:
  - GCC 13+
  - Clang 15+
  - MSVC 2022+
  - Apple Clang 15+

### Benefits
1. **Reduced Dependencies**: No external formatting library required
2. **Better Integration**: Native compiler support and optimization
3. **Future-Proof**: Aligned with C++ standard evolution
4. **Consistent Behavior**: Same formatting across all platforms

### Backward Compatibility
- The `fmt` library can still be used by setting `FORCE_FMT_FORMAT=ON`
- Existing code using formatting remains unchanged
- The `formatter_macros.h` still supports both backends via `USE_STD_FORMAT` macro

## Migration Notes

If you're using thread_system as a dependency:

1. **No Code Changes Required**: All formatting APIs remain the same
2. **CMake Integration**: thread_system now exports `USE_STD_FORMAT` variable
3. **vcpkg Users**: Run `vcpkg update` to reflect the removed `fmt` dependency

## Testing

Verified configurations:
- ✅ macOS (Apple Clang 15.0.0) with std::format
- ✅ All unit tests passed
- ✅ All integration tests passed
- ✅ All smoke tests passed

---

**Date**: 2025-11-03
**Version**: 1.0.0
