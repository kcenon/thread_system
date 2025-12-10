# 문제 해결 가이드

> **Language:** [English](TROUBLESHOOTING.md) | **한국어**

Thread System 작업 시 자주 발생하는 문제와 해결 방법을 다룹니다.

---

## 목차

1. [빌드 문제](#빌드-문제)
2. [런타임 문제](#런타임-문제)
3. [성능 문제](#성능-문제)
4. [플랫폼별 문제](#플랫폼별-문제)
5. [도움 받기](#도움-받기)

---

## 빌드 문제

### C++20 컴파일 오류

**증상:** `std::format`, `std::jthread` 또는 concepts 관련 C++20 기능 오류로 빌드 실패.

**해결 방법:**

1. 컴파일러 버전 확인:
```bash
g++ --version      # 11 이상이어야 함
clang++ --version  # 14 이상이어야 함
```

2. C++20이 활성화되어 있는지 확인:
```bash
cmake -S . -B build -DCMAKE_CXX_STANDARD=20
```

3. 이전 컴파일러를 사용하는 경우 지원되지 않는 기능 비활성화:
```bash
cmake -S . -B build \
    -DDISABLE_STD_FORMAT=ON \
    -DDISABLE_STD_JTHREAD=ON
```

---

### vcpkg 설치 실패

**증상:** `dependency.sh` 또는 `dependency.bat`이 vcpkg 또는 패키지 설치에 실패.

**해결 방법:**

1. 정리 후 재설치:
```bash
rm -rf vcpkg
./scripts/dependency.sh
```

2. 수동 설치:
```bash
git clone https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT=$PWD/vcpkg
```

3. 네트워크 연결 및 프록시 설정 확인.

---

### CMake가 컴파일러를 찾지 못함

**증상:** CMake가 "No CMAKE_CXX_COMPILER could be found" 오류 발생.

**해결 방법:**

1. 컴파일러 명시적 설정:
```bash
export CC=/usr/bin/gcc
export CXX=/usr/bin/g++
cmake -S . -B build
```

2. 또는 CMake 변수 사용:
```bash
cmake -S . -B build \
    -DCMAKE_C_COMPILER=/usr/bin/gcc \
    -DCMAKE_CXX_COMPILER=/usr/bin/g++
```

---

### Windows 링크 오류

**증상:** Windows에서 미해결 외부 심볼 또는 LNK 오류.

**해결 방법:**

1. 일관된 런타임 라이브러리 사용:
```batch
cmake -S . -B build -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL
```

2. 일치하는 Debug/Release 구성 사용.

3. 모든 종속성이 동일한 설정으로 빌드되었는지 확인.

---

### 테스트 빌드 실패

**증상:** Google Test 관련 빌드 오류.

**해결 방법:**

1. vcpkg를 통해 Google Test 설치:
```bash
./vcpkg/vcpkg install gtest
```

2. 또는 테스트 비활성화:
```bash
cmake -S . -B build -DBUILD_TESTING=OFF
```

---

## 런타임 문제

### 스레드 풀이 시작되지 않음

**증상:** 작업이 처리되지 않음.

**해결 방법:**

1. 시작 전에 워커가 추가되었는지 확인:
```cpp
auto pool = std::make_shared<thread_pool>("MyPool");

// 먼저 워커 추가
std::vector<std::unique_ptr<thread_worker>> workers;
workers.push_back(std::make_unique<thread_worker>());
pool->enqueue_batch(std::move(workers));

// 그 다음 시작
pool->start();
```

2. `start()`가 호출되었는지 확인.

---

### 데드락 또는 멈춤

**증상:** 작업 실행 중 애플리케이션이 멈춤.

**해결 방법:**

1. 동일한 풀에서 다른 작업을 기다리는 작업 제출 피하기:
```cpp
// 나쁨: 잠재적 데드락
pool->submit_task([&pool]() {
    auto future = pool->submit_task([]() { /* 내부 작업 */ });
    future.get();  // 모든 워커가 바쁘면 데드락
});
```

2. 중첩된 작업 제출에는 별도의 풀 사용.

3. ThreadSanitizer로 문제 감지:
```bash
cmake -S . -B build -DENABLE_TSAN=ON
```

---

### 메모리 누수

**증상:** 시간이 지남에 따라 메모리 사용량 증가.

**해결 방법:**

1. 적절한 종료 확인:
```cpp
pool->shutdown_pool(false);  // 완료 대기
```

2. AddressSanitizer로 실행:
```bash
cmake -S . -B build -DENABLE_ASAN=ON
./build/bin/your_app
```

3. 모든 작업이 완료되고 멈추지 않았는지 확인.

---

### 예기치 않은 작업 실패

**증상:** 작업이 조용히 실패하거나 예상치 못한 오류 반환.

**해결 방법:**

1. 작업 반환 값 확인:
```cpp
pool->execute(std::make_unique<callback_job>([]() -> result_void {
    // 작업 내용
    return result_void();
}));
```

2. 적절한 오류 처리 사용:
```cpp
auto result = pool->execute(std::move(job));
if (result.has_error()) {
    std::cerr << "작업 실패: " << result.get_error().message() << "\n";
}
```

---

## 성능 문제

### 낮은 처리량

**증상:** 작업이 예상보다 느리게 처리됨.

**해결 방법:**

1. 작업량에 맞는 워커 수 설정:
```cpp
// CPU 바운드 작업의 경우
size_t workers = std::thread::hardware_concurrency();

// I/O 바운드 작업의 경우
size_t workers = std::thread::hardware_concurrency() * 2;
```

2. 높은 경쟁에는 lock-free 큐 사용:
```cpp
thread_module::adaptive_job_queue q{
    thread_module::adaptive_job_queue::queue_strategy::LOCKFREE
};
```

3. 애플리케이션을 프로파일링하여 병목 현상 식별.

---

### 유휴 시 높은 CPU 사용량

**증상:** 작업이 없어도 CPU 사용량이 높음.

**해결 방법:**

1. 바쁜 폴링 대신 블로킹 대기 사용:
```cpp
// 적절한 유휴 동작으로 워커 구성
// 워커는 대기를 위해 조건 변수를 사용해야 함
```

2. 풀이 더 이상 필요하지 않을 때 shutdown 호출 확인.

---

### 락 경쟁

**증상:** 여러 스레드에서 확장성 저하.

**해결 방법:**

1. adaptive 또는 lock-free 큐 사용:
```cpp
thread_module::adaptive_job_queue q{
    thread_module::adaptive_job_queue::queue_strategy::ADAPTIVE
};
```

2. 작업에서 공유 상태 줄이기.

3. 관련 작업을 배치하여 큐 작업 줄이기.

---

## 플랫폼별 문제

### macOS: CI에서 테스트 비활성화

**증상:** macOS CI 환경에서 테스트가 건너뛰어짐.

**설명:** CI 환경 제한으로 인해 macOS에서는 기본적으로 테스트가 비활성화됨. 로컬에서 테스트 실행:
```bash
cd build && ctest --verbose
```

---

### Windows: 문자 인코딩 문제

**증상:** 콘솔 출력에 깨진 텍스트 표시.

**해결 방법:**

1. UTF-8 인코딩 사용:
```cpp
#ifdef _WIN32
SetConsoleOutputCP(CP_UTF8);
#endif
```

2. Windows에서는 기본적으로 libiconv가 제외됨.

---

### Linux: 권한 문제

**증상:** 스크립트 또는 바이너리를 실행할 수 없음.

**해결 방법:**

1. 스크립트를 실행 가능하게 만들기:
```bash
chmod +x scripts/*.sh
chmod +x build/bin/*
```

---

## 도움 받기

### 디버그 정보

문제 보고 시 다음을 포함하세요:

1. **시스템 정보:**
```bash
uname -a           # OS 정보
g++ --version      # 컴파일러 버전
cmake --version    # CMake 버전
```

2. **빌드 구성:**
```bash
cmake -S . -B build -L  # CMake 변수 목록
```

3. **오류 메시지:** 빌드 또는 런타임의 전체 오류 출력.

### 리소스

- **[GitHub Issues](https://github.com/kcenon/thread_system/issues)** - 버그 보고 및 기능 요청
- **[FAQ](FAQ.md)** - 자주 묻는 질문
- **[빌드 가이드](BUILD_GUIDE_KO.md)** - 상세한 빌드 지침
- **이메일:** kcenon@naver.com

---

*최종 업데이트: 2025-12-10*
