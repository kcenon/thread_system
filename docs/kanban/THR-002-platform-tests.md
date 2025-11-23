# THR-002: Platform-specific Edge Case Tests

**ID**: THR-002
**Category**: CORE
**Priority**: HIGH
**Status**: DONE
**Estimated Duration**: 3-4 days
**Dependencies**: None
**Completed**: 2025-11-23

---

## Summary

Create comprehensive platform-specific edge case tests for macOS, Linux, and Windows to ensure consistent behavior across all supported platforms.

---

## Background

- **Current Platforms**: macOS (primary), Linux, Windows
- **Gap**: Platform-specific edge cases not systematically tested
- **ARM64**: Windows ARM64 support needs verification

---

## Acceptance Criteria

- [ ] Platform-specific test matrix defined
- [ ] Edge cases for each platform documented
- [ ] Tests for thread affinity (Linux-specific)
- [ ] Tests for processor group support (Windows)
- [ ] Tests for Grand Central Dispatch interaction (macOS)
- [ ] CI matrix covers all platform combinations
- [ ] ARM64 variants tested (Apple Silicon, Windows ARM64)

---

## Platform-Specific Edge Cases

### macOS
| Edge Case | Test |
|-----------|------|
| M1/M2 efficiency cores | Thread priority handling |
| Grand Central Dispatch | Interaction with thread pool |
| pthread_setname_np | Thread naming limits |
| Memory pressure | Low memory behavior |

### Linux
| Edge Case | Test |
|-----------|------|
| cgroups CPU limits | Respect container limits |
| NUMA topology | Memory affinity |
| pthread_setaffinity_np | CPU pinning |
| Real-time scheduling | Priority inversion |

### Windows
| Edge Case | Test |
|-----------|------|
| Processor groups (>64 cores) | Large system support |
| SetThreadDescription | Thread naming |
| Windows on ARM64 | Native performance |
| Thread pool API interaction | OS thread pool |

---

## Implementation Tasks

### 1. Test Infrastructure
```cpp
// tests/platform/platform_test_base.h
class PlatformTestBase : public ::testing::Test {
protected:
    static bool is_arm64();
    static bool is_container_environment();
    static int get_physical_core_count();
    static int get_efficiency_core_count();  // macOS M-series
};
```

### 2. Platform Detection
```cpp
// Add to utilities
namespace platform {
    struct system_info {
        cpu_architecture arch;     // x86_64, arm64
        os_type os;                // macos, linux, windows
        int physical_cores;
        int logical_cores;
        bool has_efficiency_cores;
    };

    system_info get_system_info();
}
```

### 3. CI Matrix Update
```yaml
strategy:
  matrix:
    include:
      - os: ubuntu-latest
        arch: x86_64
      - os: macos-latest
        arch: arm64
      - os: windows-latest
        arch: x86_64
      - os: windows-11-arm64
        arch: arm64
```

---

## Files to Create/Modify

- `tests/platform/` - New platform test directory
- `unittest/platform_test/` - Existing tests enhancement
- `.github/workflows/ci.yml` - Matrix expansion
- `cmake/PlatformDetection.cmake` - Detection logic

---

## Success Metrics

| Metric | Target |
|--------|--------|
| Platform coverage | 100% (macOS, Linux, Windows) |
| Architecture coverage | x86_64, arm64 |
| Edge case coverage | All documented cases |
| CI pass rate | 100% on all platforms |

---

## Notes

- Windows ARM64 runner may have limited availability in GitHub Actions
- Consider self-hosted runners for special platforms
