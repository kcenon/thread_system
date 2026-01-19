# Troubleshooting Guide

> **Language:** **English** | [한국어](TROUBLESHOOTING.kr.md)

This guide covers common issues and their solutions when working with Thread System.

---

## Table of Contents

1. [Build Issues](#build-issues)
2. [Runtime Issues](#runtime-issues)
3. [Performance Issues](#performance-issues)
4. [Platform-Specific Issues](#platform-specific-issues)
5. [Getting Help](#getting-help)

---

## Build Issues

### C++20 Compilation Errors

**Symptom:** Build fails with errors related to C++20 features like `std::format`, `std::jthread`, or concepts.

**Solution:**

1. Check your compiler version:
```bash
g++ --version      # Should be 11+
clang++ --version  # Should be 14+
```

2. Ensure C++20 is enabled:
```bash
cmake -S . -B build -DCMAKE_CXX_STANDARD=20
```

3. If using older compilers, disable unsupported features:
```bash
cmake -S . -B build \
    -DDISABLE_STD_FORMAT=ON \
    -DDISABLE_STD_JTHREAD=ON
```

---

### vcpkg Installation Fails

**Symptom:** `dependency.sh` or `dependency.bat` fails to install vcpkg or packages.

**Solution:**

1. Clean and reinstall:
```bash
rm -rf vcpkg
./scripts/dependency.sh
```

2. Manual installation:
```bash
git clone https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT=$PWD/vcpkg
```

3. Check network connectivity and proxy settings.

---

### CMake Cannot Find Compiler

**Symptom:** CMake fails with "No CMAKE_CXX_COMPILER could be found."

**Solution:**

1. Set compiler explicitly:
```bash
export CC=/usr/bin/gcc
export CXX=/usr/bin/g++
cmake -S . -B build
```

2. Or use CMake variables:
```bash
cmake -S . -B build \
    -DCMAKE_C_COMPILER=/usr/bin/gcc \
    -DCMAKE_CXX_COMPILER=/usr/bin/g++
```

---

### Link Errors on Windows

**Symptom:** Unresolved external symbols or LNK errors on Windows.

**Solution:**

1. Ensure consistent runtime library:
```batch
cmake -S . -B build -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL
```

2. Use matching Debug/Release configurations.

3. Check that all dependencies are built with the same settings.

---

### Tests Fail to Build

**Symptom:** Google Test related build errors.

**Solution:**

1. Install Google Test via vcpkg:
```bash
./vcpkg/vcpkg install gtest
```

2. Or disable tests:
```bash
cmake -S . -B build -DBUILD_TESTING=OFF
```

---

## Runtime Issues

### Thread Pool Doesn't Start

**Symptom:** Jobs are not being processed.

**Solution:**

1. Ensure workers are added before starting:
```cpp
auto pool = std::make_shared<thread_pool>("MyPool");

// Add workers first
std::vector<std::unique_ptr<thread_worker>> workers;
workers.push_back(std::make_unique<thread_worker>());
pool->enqueue_batch(std::move(workers));

// Then start
pool->start();
```

2. Check that `start()` is called.

---

### Deadlock or Hang

**Symptom:** Application hangs during job execution.

**Solution:**

1. Avoid submitting jobs that wait for other jobs from the same pool:
```cpp
// BAD: Potential deadlock
pool->submit_task([&pool]() {
    auto future = pool->submit_task([]() { /* inner job */ });
    future.get();  // Deadlock if all workers are busy
});
```

2. Use separate pools for nested job submissions.

3. Run with ThreadSanitizer to detect issues:
```bash
cmake -S . -B build -DENABLE_TSAN=ON
```

---

### Memory Leaks

**Symptom:** Memory usage grows over time.

**Solution:**

1. Ensure proper shutdown:
```cpp
pool->shutdown_pool(false);  // Wait for completion
```

2. Run with AddressSanitizer:
```bash
cmake -S . -B build -DENABLE_ASAN=ON
./build/bin/your_app
```

3. Check that all jobs are completing and not stuck.

---

### Unexpected Job Failures

**Symptom:** Jobs fail silently or return unexpected errors.

**Solution:**

1. Check job return values:
```cpp
pool->execute(std::make_unique<callback_job>([]() -> kcenon::common::VoidResult {
    // Your work
    return kcenon::common::ok();
}));
```

2. Use proper error handling:
```cpp
auto result = pool->execute(std::move(job));
if (result.has_error()) {
    std::cerr << "Job failed: " << result.get_error().message() << "\n";
}
```

---

## Performance Issues

### Low Throughput

**Symptom:** Jobs are processed slower than expected.

**Solution:**

1. Match worker count to workload:
```cpp
// For CPU-bound work
size_t workers = std::thread::hardware_concurrency();

// For I/O-bound work
size_t workers = std::thread::hardware_concurrency() * 2;
```

2. Use lock-free queue for high contention:
```cpp
thread_module::adaptive_job_queue q{
    thread_module::adaptive_job_queue::queue_strategy::LOCKFREE
};
```

3. Profile your application to identify bottlenecks.

---

### High CPU Usage When Idle

**Symptom:** CPU usage is high even with no jobs.

**Solution:**

1. Use blocking wait instead of busy polling:
```cpp
// Configure worker with proper idle behavior
// Workers should use condition variables for waiting
```

2. Check that shutdown is called when pool is no longer needed.

---

### Lock Contention

**Symptom:** Poor scaling with multiple threads.

**Solution:**

1. Use adaptive or lock-free queues:
```cpp
thread_module::adaptive_job_queue q{
    thread_module::adaptive_job_queue::queue_strategy::ADAPTIVE
};
```

2. Reduce shared state in jobs.

3. Batch related operations to reduce queue operations.

---

## Platform-Specific Issues

### macOS: Tests Disabled in CI

**Symptom:** Tests are skipped on macOS CI environments.

**Explanation:** Tests are disabled by default on macOS due to CI environment limitations. Run tests locally:
```bash
cd build && ctest --verbose
```

---

### Windows: Character Encoding Issues

**Symptom:** Console output shows garbled text.

**Solution:**

1. Use UTF-8 encoding:
```cpp
#ifdef _WIN32
SetConsoleOutputCP(CP_UTF8);
#endif
```

2. libiconv is excluded on Windows by default.

---

### Linux: Permission Issues

**Symptom:** Cannot execute scripts or binaries.

**Solution:**

1. Make scripts executable:
```bash
chmod +x scripts/*.sh
chmod +x build/bin/*
```

---

## Getting Help

### Debug Information

When reporting issues, include:

1. **System information:**
```bash
uname -a           # OS info
g++ --version      # Compiler version
cmake --version    # CMake version
```

2. **Build configuration:**
```bash
cmake -S . -B build -L  # List CMake variables
```

3. **Error messages:** Full error output from the build or runtime.

### Resources

- **[GitHub Issues](https://github.com/kcenon/thread_system/issues)** - Report bugs and request features
- **[FAQ](FAQ.md)** - Frequently asked questions
- **[Build Guide](BUILD_GUIDE.md)** - Detailed build instructions
- **Email:** kcenon@naver.com

---

*Last Updated: 2025-12-10*
