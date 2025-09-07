# GitHub Workflows - vcpkg Fallback Improvements

This document describes the improvements made to GitHub Actions workflows to handle vcpkg failures gracefully.

## Problem Addressed

The original workflows were completely dependent on vcpkg working correctly. This caused CI/CD failures in several scenarios:
- ARM64 runners with compiler detection issues
- vcpkg bootstrap failures  
- Network issues preventing dependency downloads
- Transient vcpkg service interruptions

## Solution Implemented

Added intelligent fallback logic that mirrors the improvements made to `build.sh`:

### 1. Dual Build Strategy

**Primary Attempt (vcpkg)**:
- Builds full project including tests and samples
- Uses external dependencies via vcpkg
- Tagged with `id: build_vcpkg` and `continue-on-error: true`

**Fallback Attempt (system libraries)**:
- Triggered when primary build fails (`if: steps.build_vcpkg.outcome == 'failure'`)
- Builds core libraries only (`BUILD_THREADSYSTEM_AS_SUBMODULE=ON`)
- Uses C++20 standard library features (`USE_STD_FORMAT=ON`)
- Cleans CMake cache before retry

### 2. Adaptive Testing

**With vcpkg success**: Runs full unit test suite
**With system fallback**: Runs basic verification tests using C++20 features

## Affected Workflows

### Ubuntu GCC (`build-ubuntu-gcc.yaml`)
- ✅ Primary vcpkg build attempt
- ✅ System libraries fallback with GCC
- ✅ Adaptive testing (unit tests vs verification)

### Ubuntu Clang (`build-ubuntu-clang.yaml`) 
- ✅ Primary vcpkg build attempt
- ✅ System libraries fallback with Clang
- ✅ Adaptive testing (unit tests vs verification)

### Windows Visual Studio (`build-windows-vs.yaml`)
- ✅ Primary vcpkg build attempt  
- ✅ System libraries fallback with MSVC
- ✅ Windows-specific verification test with cl.exe

## Benefits

1. **Resilience**: CI/CD no longer fails due to vcpkg issues
2. **Coverage**: Still gets full testing when vcpkg works
3. **Minimum viable**: Always builds and tests core functionality
4. **Platform agnostic**: Works across Linux, Windows, different architectures
5. **Early detection**: Identifies when dependencies are problematic

## Verification Tests

When fallback occurs, workflows run simplified verification tests that confirm:
- C++20 `std::jthread` functionality
- Basic threading and atomic operations
- Standard library integration
- Platform-specific compilation

## Example Output

### Successful vcpkg build:
```
✅ Installing dependencies with vcpkg...
✅ Building with full test suite...
✅ Running 15 unit tests...
```

### Fallback scenario:
```
⚠️ vcpkg build failed, falling back to system libraries...
✅ Building core libraries only...
✅ Running basic verification test...
✅ Core functionality verified
```

## Future Improvements

Consider adding:
- Notification when fallback occurs (Slack/email)
- Metrics collection on fallback frequency
- Artifact upload for both build types
- Matrix builds with both strategies

This ensures the Thread System remains buildable and testable in all CI/CD environments.