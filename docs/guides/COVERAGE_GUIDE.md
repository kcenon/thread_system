# Test Coverage Guide

## Current Status

- **Overall Coverage**: 72% (Target: 80%)
- **Unit Tests**: `unittest/` directory
- **Integration Tests**: `integration_tests/` directory
- **Benchmarks**: `benchmarks/` directory

## Running Coverage Locally

```bash
# Build with coverage enabled
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
cmake --build build

# Run tests with coverage
./scripts/run_coverage.sh

# Generate HTML report
lcov --capture --directory build --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

## Coverage Targets by Component

| Component | Current | Target | Priority |
|-----------|---------|--------|----------|
| thread_pool | ~78% | 85% | HIGH |
| typed_thread_pool | ~75% | 82% | HIGH |
| job_queue | ~80% | 85% | MEDIUM |
| lockfree_queue | ~70% | 80% | HIGH |
| hazard_pointer | ~65% | 80% | HIGH |
| utilities | ~82% | 85% | LOW |
| interfaces | ~60% | 75% | MEDIUM |

## Areas Needing Additional Tests

### High Priority

1. **Error paths in thread_pool**
   - Shutdown with pending tasks
   - Submit after shutdown
   - Worker failure recovery

2. **Edge cases in lockfree_queue**
   - Memory pressure scenarios
   - Concurrent push/pop stress

3. **Hazard pointer reclamation**
   - High contention scenarios
   - Memory exhaustion

### Medium Priority

1. **Interface implementations**
   - All logger interface methods
   - Error handler callbacks

2. **Platform-specific code**
   - Thread affinity (Linux)
   - Thread naming (all platforms)

## CI Coverage Enforcement

Coverage is checked in CI via `codecov.yml`:

```yaml
coverage:
  status:
    project:
      default:
        target: 80%
        threshold: 1%
```

## Adding New Tests

1. Create test file in appropriate directory
2. Follow naming convention: `*_test.cpp`
3. Use Google Test framework
4. Include both success and failure paths
5. Test edge cases and boundary conditions
