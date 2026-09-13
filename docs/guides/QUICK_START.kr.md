---
doc_id: "THR-GUID-021"
doc_title: "빠른 시작 가이드"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "thread_system"
category: "GUID"
---

# 빠른 시작 가이드

> **SSOT**: This document is the single source of truth for **빠른 시작 가이드**.

> **Language:** [English](QUICK_START.md) | **한국어**

Thread System을 5분 안에 시작해보세요.

---

## 사전 요구사항

- CMake 3.16 이상
- C++20 지원 컴파일러 (GCC 11+, Clang 14+, MSVC 2022+)
- Git
- **[common_system](https://github.com/kcenon/common_system)** - 필수 종속성 (thread_system과 같은 위치에 클론 필요)

## 설치

### 1. 저장소 복제

```bash
# common_system 먼저 클론 (필수 종속성)
git clone https://github.com/kcenon/common_system.git

# thread_system을 common_system과 같은 위치에 클론
git clone https://github.com/kcenon/thread_system.git
cd thread_system
```

> **참고:** 빌드가 올바르게 작동하려면 두 저장소가 같은 부모 디렉토리에 있어야 합니다.

### 2. 의존성 설치

```bash
# Linux/macOS
./scripts/dependency.sh

# Windows
./scripts/dependency.bat
```

### 3. 빌드

```bash
# Linux/macOS
./scripts/build.sh

# Windows
./scripts/build.bat

# 또는 CMake 직접 사용
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### 4. 설치 확인

```bash
# 샘플 애플리케이션 실행
./build/bin/minimal_thread_pool
```

---

## 첫 번째 스레드 풀

간단한 스레드 풀 애플리케이션을 만들어 보세요:

```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/core/thread_worker.h>

#include <algorithm>
#include <future>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

using namespace kcenon::thread;

int main() {
    // 1. 스레드 풀 생성
    auto pool = std::make_shared<thread_pool>("MyFirstPool");

    // 2. 워커 추가 (CPU 코어 수만큼)
    std::vector<std::unique_ptr<thread_worker>> workers;
    const unsigned worker_count = std::max(1u, std::thread::hardware_concurrency());
    for (unsigned i = 0; i < worker_count; ++i) {
        workers.push_back(std::make_unique<thread_worker>());
    }
    pool->enqueue_batch(std::move(workers));

    // 3. 풀 시작
    pool->start();

    // 4. 태스크 제출 (submit()은 태스크마다 std::future를 반환)
    std::vector<std::future<void>> tasks;
    for (int i = 0; i < 10; ++i) {
        tasks.push_back(pool->submit([i]() {
            std::cout << "태스크 " << i << " 처리 중\n";
        }));
    }
    for (auto& task : tasks) {
        task.get();  // 각 태스크 완료 대기
    }

    // 5. 정상 종료
    pool->stop(false);

    std::cout << "모든 태스크 완료!\n";
    return 0;
}
```

### 애플리케이션 빌드

`CMakeLists.txt`에 추가:

```cmake
# FetchContent 사용: thread_system은 common_system을 직접 가져오지 않음
include(FetchContent)
FetchContent_Declare(
    common_system
    GIT_REPOSITORY https://github.com/kcenon/common_system.git
    GIT_TAG v0.2.0
)
FetchContent_MakeAvailable(common_system)
set(COMMON_SYSTEM_INCLUDE_DIR "${common_system_SOURCE_DIR}/include")

FetchContent_Declare(
    thread_system
    GIT_REPOSITORY https://github.com/kcenon/thread_system.git
    GIT_TAG v1.0.0  # Pin to a specific release tag; do NOT use main
)
FetchContent_MakeAvailable(thread_system)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE thread_system)
```

---

## 핵심 개념

### 스레드 풀
워커 스레드를 관리하고 작업을 실행하는 핵심 컴포넌트입니다.

```cpp
auto pool = std::make_shared<thread_pool>("PoolName");
pool->start();
// ... 태스크 제출 ...
pool->stop(false);  // false = 워커가 현재 작업을 마친 뒤 종료
```

### 워커
큐에서 작업을 처리하는 스레드입니다.

```cpp
// 워커 생성
std::vector<std::unique_ptr<thread_worker>> workers;
workers.push_back(std::make_unique<thread_worker>());
pool->enqueue_batch(std::move(workers));
```

### 작업(Jobs)
실행할 작업 단위입니다.

```cpp
// submit() 사용: std::future를 반환
auto future = pool->submit([]() {
    // 작업 내용
});

// callback_job을 사용하여 더 세밀한 제어
pool->enqueue(std::make_unique<callback_job>([]() -> kcenon::common::VoidResult {
    // 작업 내용
    return kcenon::common::ok();
}));
```

---

## 일반적인 패턴

### 병렬 처리

```cpp
std::atomic<int> counter{0};
std::vector<std::future<void>> futures;
for (int i = 0; i < 1000; ++i) {
    futures.push_back(pool->submit([&counter]() {
        counter++;
    }));
}
for (auto& f : futures) {
    f.get();  // 모든 태스크가 실행될 때까지 대기
}
```

### 오류 처리

```cpp
pool->enqueue(std::make_unique<callback_job>([]() -> kcenon::common::VoidResult {
    if (some_error_condition) {
        return kcenon::thread::make_error_result(kcenon::thread::error_code::job_execution_failed, "태스크 실패");
    }
    return kcenon::common::ok();
}));
```

### 정상 종료

```cpp
// 제출한 태스크의 future를 먼저 기다린 뒤 종료; stop(false)는
// 각 워커가 현재 작업을 마치게 함 (대기 중인 작업은 비우지 않음)
pool->stop(false);

// 또는 즉시 종료 (진행 중인 작업이 중단될 수 있음)
pool->stop(true);
```

---

## 다음 단계

- **[빌드 가이드](BUILD_GUIDE.kr.md)** - 모든 플랫폼에 대한 상세한 빌드 지침
- **[사용자 가이드](../advanced/USER_GUIDE.kr.md)** - 포괄적인 사용 가이드
- **[API 레퍼런스](../advanced/API_REFERENCE.kr.md)** - 완전한 API 문서
- **[예제](../../examples/)** - 더 많은 샘플 애플리케이션

---

## 문제 해결

### 일반적인 문제

**C++20 오류로 빌드 실패:**
```bash
# 호환되는 컴파일러가 있는지 확인
g++ --version  # 11 이상이어야 함
clang++ --version  # 14 이상이어야 함
```

**vcpkg 설치 실패:**
```bash
rm -rf vcpkg
./scripts/dependency.sh
```

**테스트 실행 실패:**
```bash
cd build && ctest --verbose
```

더 많은 문제 해결 도움이 필요하면 [FAQ](FAQ.md)를 참조하세요.

---

*최종 업데이트: 2025-12-10*
