# THR-010: Separate & Clean Platform-specific Code

**ID**: THR-010
**Category**: BUILD
**Priority**: MEDIUM
**Status**: TODO
**Estimated Duration**: 4-5 days
**Dependencies**: None
**Completed**: 2025-11-23

---

## Summary

Centralize and clean up platform-specific code into well-defined modules. Improve maintainability and make porting to new platforms easier.

---

## Background

### Current State
- Platform detection scattered across files
- `#ifdef` blocks mixed with core logic
- Inconsistent platform handling
- Some platform code duplicated

### Supported Platforms
- **macOS**: Apple Silicon (M1/M2/M3), Intel
- **Linux**: x86_64, potentially ARM64
- **Windows**: x86_64, ARM64 (experimental)

---

## Acceptance Criteria

- [ ] Platform abstraction layer defined
- [ ] All `#ifdef` blocks centralized
- [ ] Platform-specific implementations separated
- [ ] Consistent API across platforms
- [ ] Easy to add new platform support
- [ ] Platform detection at configure time (CMake)
- [ ] Runtime platform info available

---

## Target Architecture

```
include/kcenon/thread/
├── platform/
│   ├── platform.h              # Unified interface
│   ├── platform_detection.h    # Compile-time detection
│   ├── platform_info.h         # Runtime info
│   ├── macos/
│   │   ├── thread_affinity.h
│   │   └── system_info.h
│   ├── linux/
│   │   ├── thread_affinity.h
│   │   └── system_info.h
│   └── windows/
│       ├── thread_affinity.h
│       └── system_info.h
```

---

## Implementation Tasks

### Phase 1: Platform Detection

```cpp
// include/kcenon/thread/platform/platform_detection.h
#pragma once

// OS Detection
#if defined(_WIN32) || defined(_WIN64)
    #define THREAD_SYSTEM_WINDOWS 1
    #define THREAD_SYSTEM_PLATFORM_NAME "windows"
#elif defined(__APPLE__)
    #define THREAD_SYSTEM_MACOS 1
    #define THREAD_SYSTEM_PLATFORM_NAME "macos"
#elif defined(__linux__)
    #define THREAD_SYSTEM_LINUX 1
    #define THREAD_SYSTEM_PLATFORM_NAME "linux"
#else
    #error "Unsupported platform"
#endif

// Architecture Detection
#if defined(__x86_64__) || defined(_M_X64)
    #define THREAD_SYSTEM_ARCH_X64 1
    #define THREAD_SYSTEM_ARCH_NAME "x86_64"
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define THREAD_SYSTEM_ARCH_ARM64 1
    #define THREAD_SYSTEM_ARCH_NAME "arm64"
#else
    #error "Unsupported architecture"
#endif

// Feature Detection
#if THREAD_SYSTEM_MACOS && THREAD_SYSTEM_ARCH_ARM64
    #define THREAD_SYSTEM_HAS_EFFICIENCY_CORES 1
#endif
```

### Phase 2: Platform Interface

```cpp
// include/kcenon/thread/platform/platform.h
#pragma once

#include "platform_detection.h"
#include <cstdint>
#include <string_view>

namespace kcenon::thread::platform {

// System information
struct system_info {
    std::string_view os_name;
    std::string_view arch_name;
    uint32_t physical_cores;
    uint32_t logical_cores;
    uint32_t efficiency_cores;  // 0 if not applicable
    uint64_t total_memory_bytes;
};

// Thread operations
struct thread_ops {
    static void set_name(std::string_view name);
    static void set_affinity(uint32_t core_id);
    static void set_priority(int priority);
    static uint32_t get_current_core();
};

// Query functions
system_info get_system_info();
bool is_running_in_container();
bool is_running_as_root();

} // namespace
```

### Phase 3: Platform Implementations

```cpp
// src/platform/macos/thread_ops.cpp
#include "platform/platform.h"

#if THREAD_SYSTEM_MACOS

#include <pthread.h>
#include <sys/sysctl.h>

namespace kcenon::thread::platform {

void thread_ops::set_name(std::string_view name) {
    pthread_setname_np(std::string(name).c_str());
}

void thread_ops::set_affinity(uint32_t /*core_id*/) {
    // macOS doesn't support explicit affinity
    // Thread scheduling is handled by kernel
}

system_info get_system_info() {
    system_info info;
    info.os_name = "macos";
    info.arch_name = THREAD_SYSTEM_ARCH_NAME;

    int cores;
    size_t size = sizeof(cores);
    sysctlbyname("hw.physicalcpu", &cores, &size, nullptr, 0);
    info.physical_cores = cores;

    sysctlbyname("hw.logicalcpu", &cores, &size, nullptr, 0);
    info.logical_cores = cores;

#if THREAD_SYSTEM_HAS_EFFICIENCY_CORES
    sysctlbyname("hw.perflevel1.physicalcpu", &cores, &size, nullptr, 0);
    info.efficiency_cores = cores;
#else
    info.efficiency_cores = 0;
#endif

    return info;
}

} // namespace

#endif // THREAD_SYSTEM_MACOS
```

```cpp
// src/platform/linux/thread_ops.cpp
#include "platform/platform.h"

#if THREAD_SYSTEM_LINUX

#include <pthread.h>
#include <sched.h>
#include <fstream>

namespace kcenon::thread::platform {

void thread_ops::set_name(std::string_view name) {
    pthread_setname_np(pthread_self(), std::string(name).c_str());
}

void thread_ops::set_affinity(uint32_t core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

bool is_running_in_container() {
    std::ifstream cgroup("/proc/1/cgroup");
    std::string line;
    while (std::getline(cgroup, line)) {
        if (line.find("docker") != std::string::npos ||
            line.find("kubepods") != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // namespace

#endif // THREAD_SYSTEM_LINUX
```

### Phase 4: Refactor Existing Code

Replace scattered `#ifdef` blocks:

**Before:**
```cpp
void ThreadPool::set_thread_name(int id) {
#ifdef __APPLE__
    pthread_setname_np(("worker-" + std::to_string(id)).c_str());
#elif defined(__linux__)
    pthread_setname_np(pthread_self(), ("worker-" + std::to_string(id)).c_str());
#elif defined(_WIN32)
    SetThreadDescription(GetCurrentThread(), L"worker");
#endif
}
```

**After:**
```cpp
void ThreadPool::set_thread_name(int id) {
    platform::thread_ops::set_name("worker-" + std::to_string(id));
}
```

---

## Files to Create

- `include/kcenon/thread/platform/platform_detection.h`
- `include/kcenon/thread/platform/platform.h`
- `include/kcenon/thread/platform/platform_info.h`
- `src/platform/macos/thread_ops.cpp`
- `src/platform/linux/thread_ops.cpp`
- `src/platform/windows/thread_ops.cpp`

## Files to Modify

- `src/impl/thread_pool/thread_pool.cpp`
- `src/impl/thread_pool/thread_worker.cpp`
- `src/core/thread_base.cpp`
- `CMakeLists.txt` - Platform-conditional compilation

---

## Success Metrics

| Metric | Current | Target |
|--------|---------|--------|
| #ifdef blocks in core code | ~20 | 0 |
| Platform modules | 0 | 3 |
| Lines of platform code | Scattered | Centralized |
| New platform effort | High | Low |
