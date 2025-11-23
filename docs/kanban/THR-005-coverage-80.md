# THR-005: Achieve 80% Integration Test Coverage

**ID**: THR-005
**Category**: TEST
**Priority**: HIGH
**Status**: TODO
**Estimated Duration**: 5-6 days
**Dependencies**: None

---

## Summary

Increase test coverage from current 72% to target 80%. Focus on untested code paths, edge cases, and integration scenarios.

---

## Background

- **Current Coverage**: 72%
- **Target Coverage**: 80%
- **Gap**: 8% (approximately 500-800 lines)
- **Reference**: `codecov.yml` configuration

---

## Coverage Analysis

### Current Coverage by Component (Estimated)

| Component | Current | Target | Gap |
|-----------|---------|--------|-----|
| thread_pool | 78% | 85% | 7% |
| typed_thread_pool | 75% | 82% | 7% |
| job_queue | 80% | 85% | 5% |
| lockfree_queue | 70% | 80% | 10% |
| hazard_pointer | 65% | 80% | 15% |
| utilities | 82% | 85% | 3% |
| interfaces | 60% | 75% | 15% |

---

## Acceptance Criteria

- [ ] Overall coverage >= 80%
- [ ] No component below 70%
- [ ] All public APIs have at least one test
- [ ] Error paths tested
- [ ] Boundary conditions tested
- [ ] Concurrent scenarios tested
- [ ] Coverage report generated in CI

---

## Implementation Tasks

### Phase 1: Coverage Gap Analysis
- [ ] Run lcov/gcov to identify uncovered lines
- [ ] Categorize uncovered code by component
- [ ] Prioritize by risk (public API > internal)

### Phase 2: High-Impact Tests

#### Thread Pool Coverage
```cpp
// Missing coverage areas
TEST(ThreadPool, GracefulShutdownWithPendingTasks) {
    ThreadPool pool(4);
    std::atomic<int> completed{0};

    // Submit many tasks
    for (int i = 0; i < 1000; ++i) {
        pool.submit([&] { completed++; });
    }

    // Shutdown before all complete
    pool.shutdown();

    // Verify graceful handling
    EXPECT_GE(completed.load(), 0);
}

TEST(ThreadPool, SubmitAfterShutdown) {
    ThreadPool pool(2);
    pool.shutdown();

    auto result = pool.try_submit([] {});
    EXPECT_FALSE(result);  // Should fail
}
```

#### Hazard Pointer Coverage
```cpp
TEST(HazardPointer, MaxHazardsExceeded) {
    // Test behavior when all hazard slots used
}

TEST(HazardPointer, ReclamationUnderMemoryPressure) {
    // Test with many deferred objects
}
```

#### Interface Coverage
```cpp
TEST(LoggerInterface, NullLoggerBehavior) {
    NullLogger logger;
    // Should not crash
    logger.log(log_level::info, "test");
}

TEST(ErrorHandler, DefaultErrorHandling) {
    // Test default error handler behavior
}
```

### Phase 3: Edge Case Tests

| Category | Tests to Add |
|----------|--------------|
| Boundary | Zero workers, max queue size |
| Error | Invalid arguments, resource exhaustion |
| Concurrent | Race conditions, deadlock scenarios |
| Lifecycle | Create/destroy cycles, reuse |

### Phase 4: Integration Tests

```cpp
// integration_tests/full_lifecycle_test.cpp
TEST(Integration, CompleteLifecycle) {
    // Create pool
    // Submit various task types
    // Wait for completion
    // Shutdown gracefully
    // Verify all resources released
}

TEST(Integration, MultiPoolInteraction) {
    // Multiple pools sharing work
    // Cross-pool task submission
    // Coordinated shutdown
}
```

---

## Files to Create/Modify

### New Test Files
- `unittest/thread_pool_test/shutdown_test.cpp`
- `unittest/thread_base_test/hazard_pointer_edge_test.cpp`
- `unittest/interfaces_test/logger_coverage_test.cpp`
- `integration_tests/lifecycle_test.cpp`

### Modified Files
- `unittest/CMakeLists.txt` - Add new tests
- `integration_tests/CMakeLists.txt` - Add new tests
- `.github/workflows/coverage.yml` - Enforce 80% threshold

---

## Coverage Enforcement

```yaml
# codecov.yml update
coverage:
  status:
    project:
      default:
        target: 80%
        threshold: 1%
    patch:
      default:
        target: 80%
```

---

## Success Metrics

| Metric | Current | Target |
|--------|---------|--------|
| Overall coverage | 72% | 80% |
| Minimum component | ~60% | 70% |
| Lines tested | ~2800 | ~3100 |
| Branch coverage | ~65% | 75% |

---

## Notes

- Focus on meaningful tests, not just coverage numbers
- Prioritize testing error paths and edge cases
- Use parameterized tests to increase coverage efficiently
- Consider mutation testing for quality verification
