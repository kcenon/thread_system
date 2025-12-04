# Thread System Integration Tests

Comprehensive integration tests for the thread_system library following the common_system PR #33 pattern.

## Overview

This test suite provides extensive integration testing coverage for thread_system, focusing on:
- Real-world usage scenarios
- Cross-component interactions
- Performance benchmarks
- Error handling and recovery
- Resource management

## Test Structure

```
integration_tests/
├── framework/              # Test infrastructure
│   ├── system_fixture.h    # Base test fixtures
│   └── test_helpers.h      # Testing utilities
├── integration/            # Integration test scenarios
│   ├── adaptive_queue_integration_test.cpp
│   ├── error_recovery_test.cpp
│   ├── job_queue_concurrency_test.cpp
│   └── thread_pool_lifecycle_test.cpp
├── scenarios/              # Additional scenarios
│   ├── thread_pool_lifecycle_test.cpp
│   └── job_queue_integration_test.cpp
├── performance/            # Performance benchmarks
│   └── thread_pool_performance_test.cpp
└── failures/               # Failure scenario tests
    └── error_handling_test.cpp
```

## Test Coverage

### Thread Pool Lifecycle Tests (15 tests)
- Pool creation and destruction
- Start/stop operations
- Worker management
- Job submission (single and batch)
- Shutdown scenarios (graceful and immediate)
- Multiple start/stop cycles
- Resource cleanup

### Job Queue Integration Tests (11 tests)
- Basic enqueue/dequeue operations
- FIFO ordering verification
- Concurrent enqueue/dequeue
- Batch operations
- Queue state consistency
- High throughput scenarios
- Stop waiting mechanisms

### Adaptive Queue Integration Tests (13 tests)
- Balanced policy under variable load
- Data integrity during mode transitions
- Mode switching with concurrent operations
- Accuracy guard under concurrent load
- Policy enforcement (accuracy_first, performance_first, manual, balanced)
- High concurrency stress test with mode switching
- Statistics accuracy verification

### Error Handling Tests (10 tests)
- Result<T> pattern success/failure
- Exception handling in jobs
- Partial job failures
- Error propagation
- Resource cleanup on errors
- Recovery after failures
- Error code validation

### Performance Benchmark Tests (8 tests)
- Job submission latency
- Throughput measurements (empty jobs and with work)
- Scalability testing (worker count scaling)
- High contention scenarios
- Batch submission performance
- Memory overhead
- Sustained load testing

## Building and Running

### Build Integration Tests

```bash
# Configure with integration tests enabled
cmake -B build -DBUILD_INTEGRATION_TESTS=ON

# Build
cmake --build build

# Run integration tests
cd build && ctest -L integration --output-on-failure
```

### Run Specific Test Suites

```bash
# Run only lifecycle tests
./build/integration_tests --gtest_filter=ThreadPoolLifecycleTest.*

# Run only adaptive queue tests
./build/integration_tests --gtest_filter=AdaptiveQueueIntegrationTest.*

# Run only performance tests
./build/integration_tests --gtest_filter=*Performance*

# Run only error handling tests
./build/integration_tests --gtest_filter=ErrorHandlingTest.*
```

### Run with Verbose Output

```bash
# Show detailed test output
./build/integration_tests --gtest_print_time=1

# Show even more details
./build/integration_tests --gtest_print_time=1 --gtest_brief=0
```

## Performance Baselines

The integration tests include performance benchmarks with the following target baselines:

| Metric | Baseline | Test |
|--------|----------|------|
| Job submission latency (P50) | < 1 µs | JobSubmissionLatency |
| Throughput (empty jobs) | > 100k jobs/sec | ThroughputEmptyJobs |
| Throughput (10µs work) | > 10k jobs/sec | ThroughputWithWork |
| High contention throughput | > 50k jobs/sec | HighContentionPerformance |

## CI/CD Integration

Integration tests run automatically on:
- Push to main branch
- Pull requests to main
- Manual workflow dispatch

### GitHub Actions Workflow

The `.github/workflows/integration-tests.yml` workflow runs tests on:
- **Ubuntu**: GCC 11, Clang 14 (Debug/Release)
- **macOS**: Apple Clang (Debug/Release)
- **Windows**: MSVC (Debug/Release)

Additional jobs:
- Code coverage reporting
- Performance baseline verification

## Test Fixtures

### SystemFixture
Base fixture providing:
- Thread pool creation with configurable workers
- Job submission helpers
- Condition waiting with timeout
- Automatic cleanup

Example usage:
```cpp
TEST_F(SystemFixture, MyTest) {
    CreateThreadPool(4);  // 4 workers
    auto result = pool_->start();
    ASSERT_TRUE(result);

    SubmitCountingJob();  // Submit a job
    EXPECT_TRUE(WaitForJobCompletion(1));
}
```

### MultiSystemFixture
For tests requiring multiple thread pools:
```cpp
TEST_F(MultiSystemFixture, MultiPoolTest) {
    CreateMultiplePools(3, 4);  // 3 pools, 4 workers each
    StartAllPools();

    // Test cross-pool interactions
}
```

## Test Helpers

### Performance Measurement
```cpp
PerformanceMetrics metrics;
for (auto i = 0; i < iterations; ++i) {
    ScopedTimer timer([&metrics](auto duration) {
        metrics.add_sample(duration);
    });

    // Operation to measure
}

std::cout << "P50: " << metrics.p50() << " ns\n";
std::cout << "P95: " << metrics.p95() << " ns\n";
```

### Work Simulation
```cpp
// Simulate 10µs of CPU work
WorkSimulator::simulate_work(std::chrono::microseconds(10));

// Variable work duration
WorkSimulator::simulate_variable_work(
    std::chrono::microseconds(5),
    std::chrono::microseconds(15)
);
```

### Synchronization
```cpp
BarrierSync barrier(num_threads);

// In each thread:
barrier.arrive_and_wait();  // Synchronizes all threads
```

## Coverage Goals

- **Overall coverage**: 80%+ of thread_system codebase
- **Integration scenarios**: 46+ tests
- **Performance benchmarks**: 8+ tests
- **Failure scenarios**: 10+ tests

## Writing New Tests

### Test Naming Convention
- Use descriptive test names: `TEST_F(FixtureName, DescriptiveTestName)`
- Group related tests in test suites
- Use consistent naming patterns

### Best Practices
1. Use fixtures for common setup/teardown
2. Make tests hermetic (no external dependencies)
3. Include performance baselines where appropriate
4. Test both success and failure paths
5. Verify resource cleanup
6. Use meaningful assertions with error messages

### Example Test Template
```cpp
TEST_F(SystemFixture, MyIntegrationTest) {
    // Setup
    CreateThreadPool(4);
    auto result = pool_->start();
    ASSERT_TRUE(result) << "Failed to start pool";

    // Execute
    const size_t job_count = 100;
    for (size_t i = 0; i < job_count; ++i) {
        SubmitCountingJob();
    }

    // Verify
    EXPECT_TRUE(WaitForJobCompletion(job_count));
    EXPECT_EQ(completed_jobs_.load(), job_count);

    // Cleanup (automatic via fixture)
}
```

## Troubleshooting

### Tests Timing Out
- Increase timeout in `WaitForCondition` calls
- Check for deadlocks or missing notifications
- Verify job completion counters

### Flaky Tests
- Check for race conditions
- Ensure proper synchronization
- Use barriers for coordinated thread operations
- Increase sleep/wait durations if needed

### Performance Test Failures
- Baselines may vary by hardware
- Run on dedicated hardware for consistent results
- Adjust baselines if running in CI/CD environments
- Check for system load interference

## References

- [Thread System Documentation](../docs/)
- [common_system PR #33](https://github.com/kcenon/common_system/pull/33) - Reference implementation
- [Google Test Documentation](https://google.github.io/googletest/)

## Contributing

When adding new integration tests:
1. Follow the existing test structure
2. Add documentation for new test scenarios
3. Update this README with new test counts
4. Ensure tests pass in CI/CD
5. Include performance baselines for benchmark tests

## License

BSD 3-Clause License - See [LICENSE](../LICENSE) for details.
