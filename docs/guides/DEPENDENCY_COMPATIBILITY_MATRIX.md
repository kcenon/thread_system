# Dependency Version Compatibility Matrix

> **Language:** **English** | [한국어](DEPENDENCY_COMPATIBILITY_MATRIX.kr.md)

## Core Dependencies

| Package | Minimum Version | Tested Versions | Compatibility Notes |
|---------|----------------|-----------------|-------------------|
| libiconv | latest | system default | Platform dependency (excluded on Windows) |

> **Note**: As of this release, thread_system uses C++20 `std::format` exclusively. The fmt library is no longer required. Minimum compiler requirements: GCC 13+, Clang 14+, MSVC 19.29+.

## Testing Dependencies (Feature: testing)

| Package | Minimum Version | Tested Versions | Compatibility Notes |
|---------|----------------|-----------------|-------------------|
| gtest | 1.14.0 | 1.14.0, 1.15.0 | Google Test framework with GMock. Version 1.14+ required for C++20 |
| benchmark | 1.8.0 | 1.8.0, 1.8.3 | Google Benchmark library. Version 1.8+ required for modern CMake support |

## Logging Dependencies (Feature: logging)

| Package | Minimum Version | Tested Versions | Compatibility Notes |
|---------|----------------|-----------------|-------------------|
| spdlog | 1.12.0 | 1.12.0, 1.13.0, 1.14.0 | Fast C++ logging library (optional external logging). Version 1.12+ recommended |

## Version Compatibility Rules

### C++20 std::format (Built-in)
- **Compiler Requirements**: GCC 13+, Clang 14+, MSVC 19.29+
- **Note**: The project now uses C++20 `std::format` exclusively. No external formatting library is required.

### Testing Framework
- **gtest 1.14.0+**: Required for C++20 compatibility and modern CMake integration
- **benchmark 1.8.0+**: Required for performance testing with minimal overhead
- **Compatibility**: Both packages work together without conflicts

### Logging Framework
- **spdlog 1.12.0+**: Recommended for external logging (optional)
- **Header-only Mode**: Preferred for minimal dependency footprint
- **Performance**: Asynchronous logging support verified

## Platform-Specific Considerations

### Windows (MSVC, MinGW)
- `libiconv` excluded (not required)
- All other dependencies supported
- Visual Studio 2019+ recommended

### Linux (GCC, Clang)
- All dependencies supported
- `libiconv` included for character encoding support
- GCC 10+ or Clang 12+ recommended

### macOS (Clang)
- All dependencies supported
- `libiconv` may use system version
- Xcode 12+ recommended

## Conflict Resolution

### Common Issues
1. **C++20 compiler support**: Ensure your compiler supports `std::format` (GCC 13+, Clang 14+, MSVC 19.29+)
2. **gtest/benchmark conflicts**: Ensure both use same C++ standard

### Resolution Strategy
1. Check version compatibility matrix
2. Use vcpkg overrides for critical dependencies
3. Verify through integration testing
4. Document any custom patches or workarounds

## Security Considerations

### Dependency Scanning
- Regular vulnerability scanning scheduled
- Critical updates applied within 48 hours
- Non-critical updates evaluated monthly

### Supply Chain Security
- All dependencies from official vcpkg registry
- Signature verification enabled
- Source code auditing for critical dependencies

## Update Policy

### Automatic Updates
- Patch versions: Automatic (within same minor version)
- Minor versions: Manual review required
- Major versions: Full compatibility testing required

### Testing Requirements
- Unit tests: 100% pass rate required
- Integration tests: Full suite execution
- Performance tests: No regression beyond 5%
- Security tests: Vulnerability scan clear

## Last Updated
**Date**: 2025-09-13  
**Reviewer**: Backend Developer  
**Next Review**: 2025-10-13