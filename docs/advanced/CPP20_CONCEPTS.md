# C++20 Concepts Integration

> **Language:** **English** | [한국어](CPP20_CONCEPTS.kr.md)

This document describes the C++20 Concepts integration in Thread System, providing type-safe constraints for thread pool operations.

---

## Overview

Thread System leverages C++20 Concepts to provide:
- **Compile-time type safety** for callable validation
- **Clear error messages** when type constraints are violated
- **Backward compatibility** with C++17 via constexpr bool fallbacks

When `USE_STD_CONCEPTS` is defined (automatically detected by CMake), true C++20 concepts are used. Otherwise, equivalent constexpr bool expressions provide the same functionality for C++17 compilers.

---

## Available Concepts

All concepts are defined in `kcenon::thread::concepts` namespace and re-exported to `kcenon::thread::detail` for backward compatibility.

### Callable Concepts

| Concept | Description | Example |
|---------|-------------|---------|
| `Callable<F>` | Type can be invoked with no arguments | `[]() {}` |
| `VoidCallable<F>` | Callable that returns void | `[]() { return; }` |
| `ReturningCallable<F>` | Callable that returns non-void | `[]() { return 42; }` |
| `CallableWith<F, Args...>` | Callable with specific arguments | `[](int x) {}` |

### Job-Related Concepts

| Concept | Description | Example |
|---------|-------------|---------|
| `JobType<T>` | Valid job type (enum or integral, not bool) | `enum class Priority { High, Low }` |
| `JobCallable<F>` | Callable returning void, bool, or string-convertible | `[]() { return true; }` |
| `PoolJob<Job>` | Valid thread pool job type | `[]() { do_work(); }` |

### Utility Concepts

| Concept | Description | Example |
|---------|-------------|---------|
| `Duration<T>` | std::chrono::duration type | `std::chrono::milliseconds` |
| `FutureLike<T>` | Type with get() and wait() methods | `std::future<int>` |

---

## Usage Examples

### Basic Callable Validation

```cpp
#include <kcenon/thread/concepts/thread_concepts.h>

using namespace kcenon::thread::concepts;

// Using concepts in function templates
template<Callable F>
void submit_job(F&& func) {
    // F is guaranteed to be callable with no arguments
    func();
}

// More specific constraint
template<VoidCallable F>
void submit_void_job(F&& func) {
    // F is guaranteed to return void
    func();
}

// With return value
template<ReturningCallable F>
auto submit_returning_job(F&& func) {
    return func();
}
```

### Job Type Constraints

```cpp
#include <kcenon/thread/concepts/thread_concepts.h>

using namespace kcenon::thread::concepts;

// Define job priority as enum
enum class JobPriority { High, Normal, Low };

// Constrained by JobType concept
template<JobType T>
class PriorityQueue {
    // T must be enum or integral (not bool)
};

// Valid usage
PriorityQueue<JobPriority> pq1;    // OK: enum type
PriorityQueue<int> pq2;             // OK: integral type

// Invalid usage (compile error)
// PriorityQueue<bool> pq3;         // Error: bool is not a valid JobType
// PriorityQueue<std::string> pq4;  // Error: string is not a valid JobType
```

### Pool Job Validation

```cpp
#include <kcenon/thread/concepts/thread_concepts.h>

using namespace kcenon::thread::concepts;

template<PoolJob Job>
void execute_pool_job(Job&& job) {
    // Job must be callable and return void or bool-convertible
    if constexpr (VoidCallable<Job>) {
        job();
    } else {
        auto result = job();
        // Handle result as bool
    }
}

// Valid jobs
execute_pool_job([]() { /* void return */ });
execute_pool_job([]() { return true; });
execute_pool_job([]() { return 0; });  // int converts to bool

// Invalid (compile error)
// execute_pool_job([]() { return "string"; });  // Not PoolJob
```

---

## Type Traits

In addition to concepts, several type traits are provided:

### Duration Detection

```cpp
#include <kcenon/thread/concepts/thread_concepts.h>

using namespace kcenon::thread::concepts;

static_assert(is_duration_v<std::chrono::milliseconds>);
static_assert(!is_duration_v<int>);
```

### Future-Like Detection

```cpp
static_assert(is_future_like_v<std::future<int>>);
static_assert(!is_future_like_v<int>);
```

### Callable Return Type

```cpp
using ReturnType = callable_return_type_t<decltype([]() { return 42; })>;
static_assert(std::is_same_v<ReturnType, int>);
```

### Job Type Validation

```cpp
static_assert(is_valid_job_type_v<JobPriority>);  // enum
static_assert(is_valid_job_type_v<int>);          // integral
static_assert(!is_valid_job_type_v<bool>);        // explicitly excluded
```

---

## Compiler Requirements

| Compiler | Minimum Version | C++20 Concepts Support |
|----------|-----------------|------------------------|
| GCC | 10.0+ | Full support |
| Clang | 10.0+ | Full support |
| Apple Clang | 12.0+ | Full support |
| MSVC | 19.23+ (VS 2019 16.3+) | Full support |

### CMake Configuration

CMake automatically detects C++20 concepts support:

```cmake
# Automatic detection (recommended)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Manual control
cmake -S . -B build -DSET_COMMON_CONCEPTS=OFF  # Disable concepts
```

When concepts are available, `THREAD_HAS_COMMON_CONCEPTS` macro is defined.

---

## C++17 Fallback

For compilers without C++20 concepts support, constexpr bool fallbacks are automatically used:

```cpp
// C++20 version (true concepts)
template<Callable F>
void submit(F&& f);

// C++17 fallback (constexpr bool + SFINAE)
template<typename F, std::enable_if_t<Callable<F>, int> = 0>
void submit(F&& f);
```

Both versions provide the same compile-time validation; the C++20 version offers clearer error messages.

---

## Integration with common_system

When building with `common_system`, additional concepts are available:

- **Core**: `Resultable`, `Unwrappable`, `Mappable`, `Chainable`
- **Callable**: Extended callable concepts
- **Event**: `EventType`, `EventHandler`, `EventFilter`
- **Service**: `ServiceInterface`, `ServiceImplementation`
- **Container**: `SequenceContainer`, `AssociativeContainer`

These are enabled when `BUILD_WITH_COMMON_SYSTEM=ON` (default) and the compiler meets the version requirements.

---

## Best Practices

### 1. Use Concepts for Public APIs

```cpp
// Good: Clear constraint in public interface
template<Callable F>
void submit_task(F&& func);

// Avoid: Unconstrained template (poor error messages)
template<typename F>
void submit_task(F&& func);
```

### 2. Combine Concepts for Complex Constraints

```cpp
template<typename F>
    requires Callable<F> && std::is_nothrow_invocable_v<F>
void submit_noexcept_task(F&& func);
```

### 3. Use Type Traits for Implementation Details

```cpp
template<typename F>
void execute(F&& func) {
    if constexpr (is_nothrow_callable_v<F>) {
        // Optimized path for noexcept callables
    } else {
        // Standard path with exception handling
    }
}
```

---

## Related Documentation

- [Architecture Guide](01-ARCHITECTURE.md) - System design overview
- [API Reference](02-API_REFERENCE.md) - Complete API documentation
- [Build Guide](../guides/BUILD_GUIDE.md) - Build instructions and options

---

*Last Updated: 2025-12-10*
