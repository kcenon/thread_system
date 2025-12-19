# 빠른 시작 가이드

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
./build/bin/thread_pool_sample
```

---

## 첫 번째 스레드 풀

간단한 스레드 풀 애플리케이션을 만들어 보세요:

```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/jobs/callback_job.h>

#include <iostream>
#include <memory>
#include <vector>

using namespace kcenon::thread;

int main() {
    // 1. 스레드 풀 생성
    auto pool = std::make_shared<thread_pool>("MyFirstPool");

    // 2. 워커 추가 (CPU 코어 수만큼)
    std::vector<std::unique_ptr<thread_worker>> workers;
    for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
        workers.push_back(std::make_unique<thread_worker>());
    }
    pool->enqueue_batch(std::move(workers));

    // 3. 풀 시작
    pool->start();

    // 4. 태스크 제출
    for (int i = 0; i < 10; ++i) {
        pool->submit_task([i]() {
            std::cout << "태스크 " << i << " 처리 중\n";
        });
    }

    // 5. 정상 종료 (모든 태스크 완료 대기)
    pool->shutdown_pool(false);

    std::cout << "모든 태스크 완료!\n";
    return 0;
}
```

### 애플리케이션 빌드

`CMakeLists.txt`에 추가:

```cmake
# FetchContent 사용
include(FetchContent)
FetchContent_Declare(
    thread_system
    GIT_REPOSITORY https://github.com/kcenon/thread_system.git
    GIT_TAG main
)
FetchContent_MakeAvailable(thread_system)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE
    thread_base
    thread_pool
    utilities
)
```

---

## 핵심 개념

### 스레드 풀
워커 스레드를 관리하고 작업을 실행하는 핵심 컴포넌트입니다.

```cpp
auto pool = std::make_shared<thread_pool>("PoolName");
pool->start();
// ... 태스크 제출 ...
pool->shutdown_pool(false);  // false = 완료 대기
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
// 편의 API 사용
pool->submit_task([]() {
    // 작업 내용
});

// callback_job을 사용하여 더 세밀한 제어
pool->execute(std::make_unique<callback_job>([]() -> kcenon::common::VoidResult {
    // 작업 내용
    return kcenon::common::ok();
}));
```

---

## 일반적인 패턴

### 병렬 처리

```cpp
std::atomic<int> counter{0};
for (int i = 0; i < 1000; ++i) {
    pool->submit_task([&counter]() {
        counter++;
    });
}
```

### 오류 처리

```cpp
pool->execute(std::make_unique<callback_job>([]() -> kcenon::common::VoidResult {
    if (some_error_condition) {
        return kcenon::thread::make_error_result(kcenon::thread::error_code::operation_failed, "태스크 실패");
    }
    return kcenon::common::ok();
}));
```

### 정상 종료

```cpp
// 모든 태스크 완료 대기
pool->shutdown_pool(false);

// 또는 즉시 강제 종료
pool->shutdown_pool(true);
```

---

## 다음 단계

- **[빌드 가이드](BUILD_GUIDE_KO.md)** - 모든 플랫폼에 대한 상세한 빌드 지침
- **[사용자 가이드](../advanced/USER_GUIDE_KO.md)** - 포괄적인 사용 가이드
- **[API 레퍼런스](../advanced/02-API_REFERENCE_KO.md)** - 완전한 API 문서
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
