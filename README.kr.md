[![CI](https://github.com/kcenon/thread_system/actions/workflows/ci.yml/badge.svg)](https://github.com/kcenon/thread_system/actions/workflows/ci.yml)
[![Code Coverage](https://github.com/kcenon/thread_system/actions/workflows/coverage.yml/badge.svg)](https://github.com/kcenon/thread_system/actions/workflows/coverage.yml)
[![Static Analysis](https://github.com/kcenon/thread_system/actions/workflows/static-analysis.yml/badge.svg)](https://github.com/kcenon/thread_system/actions/workflows/static-analysis.yml)
[![Doxygen](https://github.com/kcenon/thread_system/actions/workflows/build-Doxygen.yaml/badge.svg)](https://github.com/kcenon/thread_system/actions/workflows/build-Doxygen.yaml)
[![codecov](https://codecov.io/gh/kcenon/thread_system/branch/main/graph/badge.svg)](https://codecov.io/gh/kcenon/thread_system)
[![License](https://img.shields.io/github/license/kcenon/thread_system)](https://github.com/kcenon/thread_system/blob/main/LICENSE)

# Thread System

> **Language:** [English](README.md) | **한국어**

동시성 프로그래밍의 민주화를 위해 설계된 현대적인 C++20 멀티스레딩 프레임워크입니다.

## 목차

- [개요](#개요)
- [주요 기능](#주요-기능)
- [요구사항](#요구사항)
- [빠른 시작](#빠른-시작)
- [설치](#설치)
- [아키텍처](#아키텍처)
- [핵심 개념](#핵심-개념)
- [API 개요](#api-개요)
- [예제](#예제)
- [성능](#성능)
- [생태계 통합](#생태계-통합)
- [기여하기](#기여하기)
- [라이선스](#라이선스)

---

## 개요

Thread System은 고성능 thread-safe 애플리케이션 구축을 위한 직관적인 추상화와 견고한 구현을 제공하는 포괄적인 멀티스레딩 프레임워크입니다.

**핵심 가치**:
- **검증된 품질**: 95%+ CI/CD 성공률, ThreadSanitizer 경고 제로, 72% 코드 커버리지
- **고성능**: 초당 1.16M 작업 처리, lock-free 큐 4배 성능 향상, 적응형 최적화
- **개발자 친화적**: 직관적 API, 포괄적 문서, 풍부한 예제
- **유연한 아키텍처**: 선택적 logger/monitoring 통합이 가능한 모듈식 설계
- **크로스 플랫폼**: Linux, macOS, Windows 지원 (다중 컴파일러)

**최신 업데이트**:
- Queue API 간소화: 8개 구현 → 2개 공개 타입 (adaptive_job_queue, job_queue)
- Hazard Pointer 구현 완료 - 프로덕션 안전한 lock-free 큐
- Lock-free 큐로 4배 성능 향상 (71 us vs 291 us)
- 향상된 동기화 프리미티브 및 취소 토큰
- 모든 CI/CD 파이프라인 정상 (ThreadSanitizer 및 AddressSanitizer 클린)

---

## 주요 기능

| 기능 | 설명 | 상태 |
|------|------|------|
| **표준 스레드 풀** | 적응형 큐를 지원하는 다중 워커 풀 | 안정 |
| **타입드 스레드 풀** | 타입 인식 라우팅을 갖춘 우선순위 기반 스케줄링 | 안정 |
| **적응형 큐** | mutex/lock-free 모드 자동 전환 | 안정 |
| **Hazard Pointer** | Lock-free 구조를 위한 안전한 메모리 회수 | 안정 |
| **취소 토큰** | 계층적 지원을 갖춘 협력적 취소 | 안정 |
| **서비스 레지스트리** | 경량 의존성 주입 컨테이너 | 안정 |
| **동기화 프리미티브** | 타임아웃 및 술어를 갖춘 향상된 래퍼 | 안정 |
| **워커 정책** | 세밀한 제어 (스케줄링, 유휴 동작, CPU 친화성) | 안정 |
| **DAG 스케줄러** | 의존성 그래프 기반 작업 실행 | 실험적 |
| **C++20 모듈** | 헤더 기반 인터페이스 대안 | 실험적 |

---

## 요구사항

### 컴파일러 매트릭스

| 컴파일러 | 최소 버전 | 권장 버전 | 비고 |
|----------|----------|----------|------|
| GCC | 13+ | 14+ | `std::format` 필수 |
| Clang | 17+ | 18+ | `std::format` 필수 |
| MSVC | 2022+ | 2022 17.4+ | C++20 모듈 지원 |

### 빌드 도구

- **CMake** 3.20+ (모듈 사용 시 3.28+)
- **[common_system](https://github.com/kcenon/common_system)**: 필수 의존성 (thread_system과 나란히 클론 필요)

> **하위 영향**: thread_system에 의존하는 시스템 (monitoring_system, database_system, network_system)은 이 컴파일러 요구사항을 상속합니다. 전체 생태계를 빌드할 때 컴파일러가 GCC 13+/Clang 17+를 충족하는지 확인하세요.

---

## 빠른 시작

```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/jobs/callback_job.h>

using namespace kcenon::thread;

int main() {
    // 스레드 풀 생성
    auto pool = std::make_shared<thread_pool>("MyPool");

    // 워커 추가
    std::vector<std::unique_ptr<thread_worker>> workers;
    for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
        workers.push_back(std::make_unique<thread_worker>());
    }
    pool->enqueue_batch(std::move(workers));

    // 처리 시작
    pool->start();

    // 작업 제출 (편의 API)
    for (int i = 0; i < 1000; ++i) {
        pool->submit_task([i]() {
            std::cout << "Processing job " << i << "\n";
        });
    }

    // 정상 종료
    pool->shutdown_pool(false);  // 완료 대기
    return 0;
}
```

**[전체 시작 가이드 ->](docs/guides/QUICK_START.md)**

---

## 설치

### 소스에서 빌드

```bash
# 리포지토리 클론 (common_system 필수)
git clone https://github.com/kcenon/common_system.git
git clone https://github.com/kcenon/thread_system.git
cd thread_system

# 의존성 설치
./scripts/dependency.sh  # Linux/macOS
./scripts/dependency.bat # Windows

# 빌드
./scripts/build.sh       # Linux/macOS
./scripts/build.bat      # Windows

# 예제 실행
./build/bin/thread_pool_sample
```

### vcpkg를 통한 설치

```bash
vcpkg install kcenon-thread-system
```

`CMakeLists.txt`에서:
```cmake
find_package(thread_system CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE kcenon::thread_system)
```

### FetchContent를 통한 설치

```cmake
include(FetchContent)
FetchContent_Declare(
    thread_system
    GIT_REPOSITORY https://github.com/kcenon/thread_system.git
    GIT_TAG v1.0.0  # 특정 릴리스 태그에 고정; main 사용 금지
)
FetchContent_MakeAvailable(thread_system)

target_link_libraries(your_target PRIVATE thread_system)
```

### 서브디렉토리 연동

```cmake
add_subdirectory(thread_system)

target_link_libraries(your_target PRIVATE
    thread_base
    thread_pool
    utilities
)
```

### C++20 모듈 지원

```bash
cmake -B build -DTHREAD_BUILD_MODULES=ON
cmake --build build
```

```cpp
import kcenon.thread;

int main() {
    using namespace kcenon::thread;
    auto pool = std::make_shared<thread_pool>("MyPool");
    pool->start();
    // ...
}
```

| 모듈 | 내용 |
|------|------|
| `kcenon.thread` | 기본 모듈 (모든 파티션 임포트) |
| `kcenon.thread:core` | 스레드 풀, 워커, 작업, 취소 |
| `kcenon.thread:queue` | 큐 구현 (job_queue, adaptive_job_queue) |

> **참고**: C++20 모듈은 실험적입니다. 헤더 기반 인터페이스가 기본 API로 유지됩니다.

---

## 아키텍처

### 모듈식 설계

```
+-----------------------------------------+
|         Thread System Core              |
|  +-----------------------------------+  |
|  |  Thread Pool & Workers            |  |
|  |  - Standard Pool                  |  |
|  |  - Typed Pool (Priority)          |  |
|  |  - Dynamic Worker Management      |  |
|  +-----------------------------------+  |
|  +-----------------------------------+  |
|  |  Queue Implementations            |  |
|  |  - Adaptive Queue (recommended)   |  |
|  |  - Standard Queue (blocking wait) |  |
|  |  - Internal: lock-free MPMC       |  |
|  +-----------------------------------+  |
|  +-----------------------------------+  |
|  |  Advanced Features                |  |
|  |  - Hazard Pointers                |  |
|  |  - Cancellation Tokens            |  |
|  |  - Service Registry               |  |
|  |  - Worker Policies                |  |
|  +-----------------------------------+  |
+-----------------------------------------+

선택적 통합 프로젝트 (별도 리포지토리):
+------------------+  +------------------+
|  Logger System   |  | Monitoring System|
|  - Async logging |  | - Real-time      |
|  - Multi-target  |  |   metrics        |
|  - High-perf     |  | - Observability  |
+------------------+  +------------------+
```

### 주요 컴포넌트

| 컴포넌트 | 설명 |
|----------|------|
| `thread_base` | 라이프사이클 관리를 갖춘 추상 스레드 클래스 |
| `thread_pool` | 적응형 큐를 갖춘 다중 워커 풀 |
| `typed_thread_pool` | 타입 인식 라우팅을 갖춘 우선순위 스케줄링 |
| `adaptive_job_queue` | 자동 최적화 큐 (권장 기본값) |
| `job_queue` | 블로킹 대기 지원을 갖춘 Mutex 기반 큐 |
| `hazard_pointer` | Lock-free 구조를 위한 안전한 메모리 회수 |
| `cancellation_token` | 협력적 취소 메커니즘 |

**[아키텍처 가이드 ->](docs/advanced/ARCHITECTURE.md)**

---

## 핵심 개념

### 스레드 풀 라이프사이클

스레드 풀은 명확한 라이프사이클을 따릅니다:

```
생성 -> 워커 등록 (enqueue_batch) -> 시작 (start) -> 작업 제출 (submit_task) -> 종료 (shutdown_pool)
```

`shutdown_pool(false)`는 진행 중인 작업이 완료될 때까지 대기하며, `shutdown_pool(true)`는 즉시 종료합니다.

### 큐 전략

Kent Beck의 Simple Design 원칙에 따라 2개의 공개 큐 타입만 제공합니다:

- **adaptive_job_queue** (권장): 경합 수준에 따라 mutex와 lock-free 모드를 자동 전환합니다.
- **job_queue**: 블로킹 대기와 정확한 크기 추적을 지원하는 Mutex 기반 FIFO입니다. 선택적 `max_size` 파라미터로 경계를 설정할 수 있습니다.

> `bounded_job_queue`는 선택적 `max_size` 파라미터를 갖춘 `job_queue`로 통합되었습니다.
> 내부 구현 (`lockfree_job_queue`, `concurrent_queue`)은 `detail::` 네임스페이스에 있습니다.

### Hazard Pointer

Lock-free 데이터 구조에서 안전한 메모리 회수를 위한 메커니즘입니다. 스레드별 hazard 배열(4슬롯)을 사용하여 다른 스레드가 참조 중인 메모리가 해제되지 않도록 보장합니다.

### 취소 토큰

계층적 취소를 지원하는 협력적 취소 메커니즘입니다. 부모 토큰이 취소되면 모든 자식 토큰도 함께 취소됩니다.

### 오류 처리

- `thread::result<T>` 및 `thread::result_void`는 `common::Result`를 래핑하되 thread 전용 헬퍼를 유지합니다.
- thread_system 내부에서는 `result.has_error()` / `result.get_error()`를 사용합니다.
- 모듈 경계를 넘을 때는 `detail::to_common_error(...)`로 `common::error_info`로 변환합니다.

---

## API 개요

| API | 헤더 | 설명 |
|-----|------|------|
| `thread_pool` | `core/thread_pool.h` | 다중 워커 스레드 풀 |
| `typed_thread_pool` | `impl/typed_pool/typed_thread_pool.h` | 우선순위 기반 스케줄링 |
| `adaptive_job_queue` | `queue/adaptive_job_queue.h` | 자동 최적화 큐 |
| `job_queue` | `core/job_queue.h` | Mutex 기반 FIFO 큐 |
| `queue_factory` | `queue/queue_factory.h` | 요구사항 기반 큐 생성 |
| `hazard_pointer` | `core/hazard_pointer.h` | Lock-free 메모리 회수 |
| `cancellation_token` | `core/cancellation_token.h` | 협력적 취소 |
| `service_registry` | `core/service_registry.h` | 의존성 주입 |
| `thread_pool_builder` | `core/thread_pool_builder.h` | 빌더 패턴 풀 생성 |
| `dag_scheduler` | `dag/dag_scheduler.h` | DAG 기반 작업 스케줄링 |

**[API 레퍼런스 ->](docs/advanced/API_REFERENCE.md)**

---

## 예제

| 예제 | 난이도 | 설명 |
|------|--------|------|
| [thread_pool_sample](examples/thread_pool_sample) | 초급 | 적응형 큐를 사용한 기본 스레드 풀 |
| [typed_thread_pool_sample](examples/typed_thread_pool_sample) | 중급 | 우선순위 기반 작업 스케줄링 |
| [adaptive_queue_sample](examples/adaptive_queue_sample) | 중급 | 큐 성능 비교 |
| [queue_factory_sample](examples/queue_factory_sample) | 중급 | 요구사항 기반 큐 생성 |
| [queue_capabilities_sample](examples/queue_capabilities_sample) | 중급 | 런타임 기능 인트로스펙션 |
| [hazard_pointer_sample](examples/hazard_pointer_sample) | 고급 | Lock-free 메모리 회수 |
| [integration_example](examples/integration_example) | 고급 | logger/monitoring 전체 통합 |

### 예제 실행

```bash
# 모든 예제 빌드
cmake -B build
cmake --build build

# 특정 예제 실행
./build/bin/thread_pool_sample
./build/bin/typed_thread_pool_sample
```

---

## 성능

**플랫폼**: Apple M1 @ 3.2GHz, 16GB RAM, macOS Sonoma

### 처리량 메트릭

| 메트릭 | 값 | 구성 |
|--------|------|------|
| **프로덕션 처리량** | 1.16M jobs/s | 10 워커, 실제 워크로드 |
| **타입드 풀** | 1.24M jobs/s | 6 워커, 6.9% 향상 |
| **Lock-free 큐** | 71 us/op | Mutex 대비 4배 향상 |
| **작업 지연 (P50)** | 77 ns | 마이크로초 이하 |
| **메모리 기준선** | <1 MB | 8 워커 |
| **스케일링 효율** | 96% | 8 워커까지 |

### 큐 성능 비교

| 큐 타입 | 지연 | 적합 대상 |
|---------|------|----------|
| Mutex 큐 | 96 ns | 낮은 경합 (1-2 스레드) |
| 적응형 (자동) | 96-320 ns | 가변 워크로드 |
| Lock-free | 320 ns | 높은 경합 (8+ 스레드), 37% 향상 |

### 워커 스케일링

| 워커 수 | 속도 향상 | 효율 | 등급 |
|---------|----------|------|------|
| 2 | 2.0x | 99% | 우수 |
| 4 | 3.9x | 97.5% | 우수 |
| 8 | 7.7x | 96% | 매우 좋음 |
| 16 | 15.0x | 94% | 매우 좋음 |

### 품질 메트릭

- 95%+ CI/CD 성공률 (모든 플랫폼)
- 72% 코드 커버리지 (포괄적 테스트 스위트)
- ThreadSanitizer 경고 제로 (프로덕션 코드)
- AddressSanitizer 누수 제로 (100% RAII 준수)
- 70+ 스레드 안전성 테스트

**[전체 벤치마크 ->](docs/BENCHMARKS.md)**

---

## 생태계 통합

### 프로젝트 생태계

이 프로젝트는 모듈식 생태계의 일부입니다:

```
thread_system (핵심 인터페이스)
    ^                    ^
logger_system    monitoring_system
    ^                    ^
    +-- integrated_thread_system --+
```

### 선택적 컴포넌트

| 프로젝트 | 설명 | 의존성 |
|----------|------|--------|
| [common_system](https://github.com/kcenon/common_system) | C++20 유틸리티 기반 | 필수 |
| [logger_system](https://github.com/kcenon/logger_system) | 고성능 비동기 로깅 | 선택 |
| [monitoring_system](https://github.com/kcenon/monitoring_system) | 실시간 메트릭 및 모니터링 | 선택 |

### 통합 이점

- **플러그 앤 플레이**: 필요한 컴포넌트만 사용
- **인터페이스 기반**: 쉬운 교체를 위한 깔끔한 추상화
- **성능 최적화**: 각 시스템이 고유 도메인에 최적화
- **통합 생태계**: 일관된 API 설계

### 플랫폼 지원

| 플랫폼 | 컴파일러 | 상태 |
|--------|----------|------|
| **Linux** | GCC 11+, Clang 14+ | 완전 지원 |
| **macOS** | Apple Clang 14+, GCC 11+ | 완전 지원 |
| **Windows** | MSVC 2022+ | 완전 지원 |

---

## 기여하기

기여를 환영합니다! 자세한 내용은 [기여 가이드](docs/contributing/CONTRIBUTING.md)를 참조하세요.

### 개발 워크플로우

1. 리포지토리 포크
2. 기능 브랜치 생성 (`git checkout -b feature/amazing-feature`)
3. 테스트와 함께 변경 사항 작성
4. 로컬에서 테스트 실행 (`ctest --verbose`)
5. 변경 사항 커밋 (`git commit -m 'Add amazing feature'`)
6. 브랜치 푸시 (`git push origin feature/amazing-feature`)
7. Pull Request 열기

### 코드 표준

- 현대 C++ 모범 사례 준수
- RAII 및 스마트 포인터 사용
- 포괄적 단위 테스트 작성
- 일관된 포맷팅 유지 (clang-format)
- 문서 업데이트

---

## 라이선스

이 프로젝트는 BSD 3-Clause 라이선스에 따라 배포됩니다 - 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.

---

<p align="center">
  Made with ❤️ by 🍀☀🌕🌥 🌊
</p>
