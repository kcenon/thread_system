# C++20 Concepts 통합

> **Language:** [English](CPP20_CONCEPTS.md) | **한국어**

이 문서는 Thread System의 C++20 Concepts 통합에 대해 설명하며, 스레드 풀 작업에 대한 타입 안전 제약 조건을 제공합니다.

---

## 개요

Thread System은 C++20 Concepts를 활용하여 다음을 제공합니다:
- **컴파일 타임 타입 안전성** - callable 유효성 검사
- **명확한 오류 메시지** - 타입 제약 조건 위반 시
- **하위 호환성** - C++17용 constexpr bool 폴백

`USE_STD_CONCEPTS`가 정의되면(CMake에서 자동 감지) 진정한 C++20 concepts가 사용됩니다. 그렇지 않으면 동등한 constexpr bool 표현식이 C++17 컴파일러에 동일한 기능을 제공합니다.

---

## 사용 가능한 Concepts

모든 concepts는 `kcenon::thread::concepts` 네임스페이스에 정의되어 있으며 하위 호환성을 위해 `kcenon::thread::detail`로 재내보내집니다.

### Callable Concepts

| Concept | 설명 | 예제 |
|---------|------|------|
| `Callable<F>` | 인수 없이 호출 가능한 타입 | `[]() {}` |
| `VoidCallable<F>` | void를 반환하는 Callable | `[]() { return; }` |
| `ReturningCallable<F>` | void가 아닌 값을 반환하는 Callable | `[]() { return 42; }` |
| `CallableWith<F, Args...>` | 특정 인수로 호출 가능한 Callable | `[](int x) {}` |

### 작업 관련 Concepts

| Concept | 설명 | 예제 |
|---------|------|------|
| `JobType<T>` | 유효한 작업 타입 (enum 또는 정수형, bool 제외) | `enum class Priority { High, Low }` |
| `JobCallable<F>` | void, bool 또는 문자열로 변환 가능한 값을 반환하는 Callable | `[]() { return true; }` |
| `PoolJob<Job>` | 유효한 스레드 풀 작업 타입 | `[]() { do_work(); }` |

### 유틸리티 Concepts

| Concept | 설명 | 예제 |
|---------|------|------|
| `Duration<T>` | std::chrono::duration 타입 | `std::chrono::milliseconds` |
| `FutureLike<T>` | get()과 wait() 메서드가 있는 타입 | `std::future<int>` |

---

## 사용 예제

### 기본 Callable 유효성 검사

```cpp
#include <kcenon/thread/concepts/thread_concepts.h>

using namespace kcenon::thread::concepts;

// 함수 템플릿에서 concepts 사용
template<Callable F>
void submit_job(F&& func) {
    // F는 인수 없이 호출 가능함이 보장됨
    func();
}

// 더 구체적인 제약
template<VoidCallable F>
void submit_void_job(F&& func) {
    // F는 void를 반환함이 보장됨
    func();
}

// 반환 값이 있는 경우
template<ReturningCallable F>
auto submit_returning_job(F&& func) {
    return func();
}
```

### 작업 타입 제약

```cpp
#include <kcenon/thread/concepts/thread_concepts.h>

using namespace kcenon::thread::concepts;

// 작업 우선순위를 enum으로 정의
enum class JobPriority { High, Normal, Low };

// JobType concept으로 제약
template<JobType T>
class PriorityQueue {
    // T는 enum 또는 정수형(bool 제외)이어야 함
};

// 유효한 사용
PriorityQueue<JobPriority> pq1;    // OK: enum 타입
PriorityQueue<int> pq2;             // OK: 정수형 타입

// 유효하지 않은 사용 (컴파일 오류)
// PriorityQueue<bool> pq3;         // 오류: bool은 유효한 JobType이 아님
// PriorityQueue<std::string> pq4;  // 오류: string은 유효한 JobType이 아님
```

### 풀 작업 유효성 검사

```cpp
#include <kcenon/thread/concepts/thread_concepts.h>

using namespace kcenon::thread::concepts;

template<PoolJob Job>
void execute_pool_job(Job&& job) {
    // Job은 호출 가능하고 void 또는 bool로 변환 가능한 값을 반환해야 함
    if constexpr (VoidCallable<Job>) {
        job();
    } else {
        auto result = job();
        // result를 bool로 처리
    }
}

// 유효한 작업
execute_pool_job([]() { /* void 반환 */ });
execute_pool_job([]() { return true; });
execute_pool_job([]() { return 0; });  // int는 bool로 변환됨

// 유효하지 않음 (컴파일 오류)
// execute_pool_job([]() { return "string"; });  // PoolJob이 아님
```

---

## 타입 트레잇

concepts 외에도 여러 타입 트레잇이 제공됩니다:

### Duration 감지

```cpp
#include <kcenon/thread/concepts/thread_concepts.h>

using namespace kcenon::thread::concepts;

static_assert(is_duration_v<std::chrono::milliseconds>);
static_assert(!is_duration_v<int>);
```

### Future-Like 감지

```cpp
static_assert(is_future_like_v<std::future<int>>);
static_assert(!is_future_like_v<int>);
```

### Callable 반환 타입

```cpp
using ReturnType = callable_return_type_t<decltype([]() { return 42; })>;
static_assert(std::is_same_v<ReturnType, int>);
```

### 작업 타입 유효성 검사

```cpp
static_assert(is_valid_job_type_v<JobPriority>);  // enum
static_assert(is_valid_job_type_v<int>);          // 정수형
static_assert(!is_valid_job_type_v<bool>);        // 명시적으로 제외됨
```

---

## 컴파일러 요구사항

| 컴파일러 | 최소 버전 | C++20 Concepts 지원 |
|----------|----------|---------------------|
| GCC | 10.0+ | 완전 지원 |
| Clang | 10.0+ | 완전 지원 |
| Apple Clang | 12.0+ | 완전 지원 |
| MSVC | 19.23+ (VS 2019 16.3+) | 완전 지원 |

### CMake 구성

CMake는 자동으로 C++20 concepts 지원을 감지합니다:

```cmake
# 자동 감지 (권장)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 수동 제어
cmake -S . -B build -DSET_COMMON_CONCEPTS=OFF  # concepts 비활성화
```

concepts를 사용할 수 있으면 `THREAD_HAS_COMMON_CONCEPTS` 매크로가 정의됩니다.

---

## C++17 폴백

C++20 concepts를 지원하지 않는 컴파일러의 경우 constexpr bool 폴백이 자동으로 사용됩니다:

```cpp
// C++20 버전 (진정한 concepts)
template<Callable F>
void submit(F&& f);

// C++17 폴백 (constexpr bool + SFINAE)
template<typename F, std::enable_if_t<Callable<F>, int> = 0>
void submit(F&& f);
```

두 버전 모두 동일한 컴파일 타임 유효성 검사를 제공합니다. C++20 버전은 더 명확한 오류 메시지를 제공합니다.

---

## common_system과의 통합

`common_system`과 함께 빌드할 때 추가 concepts를 사용할 수 있습니다:

- **Core**: `Resultable`, `Unwrappable`, `Mappable`, `Chainable`
- **Callable**: 확장된 callable concepts
- **Event**: `EventType`, `EventHandler`, `EventFilter`
- **Service**: `ServiceInterface`, `ServiceImplementation`
- **Container**: `SequenceContainer`, `AssociativeContainer`

이들은 `BUILD_WITH_COMMON_SYSTEM=ON`(기본값)이고 컴파일러가 버전 요구사항을 충족할 때 활성화됩니다.

---

## 모범 사례

### 1. 공개 API에 Concepts 사용

```cpp
// 좋음: 공개 인터페이스에 명확한 제약
template<Callable F>
void submit_task(F&& func);

// 피하기: 제약 없는 템플릿 (오류 메시지가 불명확함)
template<typename F>
void submit_task(F&& func);
```

### 2. 복잡한 제약을 위해 Concepts 결합

```cpp
template<typename F>
    requires Callable<F> && std::is_nothrow_invocable_v<F>
void submit_noexcept_task(F&& func);
```

### 3. 구현 세부사항에 타입 트레잇 사용

```cpp
template<typename F>
void execute(F&& func) {
    if constexpr (is_nothrow_callable_v<F>) {
        // noexcept callable을 위한 최적화된 경로
    } else {
        // 예외 처리가 있는 표준 경로
    }
}
```

---

## 관련 문서

- [아키텍처 가이드](01-ARCHITECTURE_KO.md) - 시스템 설계 개요
- [API 레퍼런스](02-API_REFERENCE_KO.md) - 완전한 API 문서
- [빌드 가이드](../guides/BUILD_GUIDE_KO.md) - 빌드 지침 및 옵션

---

*최종 업데이트: 2025-12-10*
