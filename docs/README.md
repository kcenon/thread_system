# thread_system

> **Version**: 2.0
> **Status**: Production
> **Tier**: 1

## 개요

thread_system은 고성능 멀티스레드 프로그래밍을 위한 C++17/20 기반 스레드 관리 라이브러리입니다. thread_pool, typed_thread_pool, 다양한 동시성 큐 구현 및 서비스 인프라를 제공합니다.

## 주요 기능

- **Thread Pool**: 작업 기반 스레드 풀 (4.5x 성능 향상)
- **Typed Thread Pool**: 타입 안전 스레드 풀 (3.8x 성능 향상)
- **Lock-Free Queue**: MPMC 큐 구현 (5.2x 성능 향상)
- **Adaptive Queue**: 부하 기반 자동 크기 조정
- **Service Registry**: 서비스 라이프사이클 관리
- **Thread Safety**: 포괄적인 동기화 원시 타입

## 빠른 시작

### 설치

#### 통합 빌드
```bash
cd unified_system
cmake -B build -DBUILD_TIER1=ON
cmake --build build
```

#### 독립 빌드
```bash
cd thread_system
cmake -B build
cmake --build build
```

### 기본 사용법

#### Thread Pool 사용

```cpp
#include <kcenon/thread/thread_pool.h>

using namespace kcenon::thread;

int main() {
    // 4개 워커 스레드로 스레드 풀 생성
    thread_pool pool(4);

    // 작업 큐에 추가
    auto future = pool.enqueue([]() {
        return 42;
    });

    // 결과 대기
    int result = future.get();

    return 0;
}
```

#### Typed Thread Pool 사용

```cpp
#include <kcenon/thread/typed_thread_pool.h>

using namespace kcenon::thread;

int main() {
    // int 타입 작업을 처리하는 스레드 풀
    typed_thread_pool<int> pool(4);

    // 타입 안전 작업 추가
    pool.enqueue(10);
    pool.enqueue(20);
    pool.enqueue(30);

    // 작업 처리 함수 등록
    pool.set_process_function([](int value) {
        std::cout << "Processing: " << value << std::endl;
    });

    pool.start();

    return 0;
}
```

## 의존성

### 필수 의존성
- **common_system** (Tier 0) - Result<T> 패턴, 인터페이스

### 선택적 의존성
- 없음 (모든 의존성 필수)

## 성능

| 구현 | Throughput | Latency | Improvement |
|------|------------|---------|-------------|
| thread_pool | 1.2M ops/sec | 0.8 μs | 4.5x |
| typed_thread_pool | 980K ops/sec | 1.0 μs | 3.8x |
| mpmc_queue | 2.1M ops/sec | 0.5 μs | 5.2x |
| adaptive_queue | 1.5M ops/sec | 0.7 μs | 4.1x |

**측정 환경**: Apple M1 Max, 10 cores, macOS 14

자세한 벤치마크 결과는 [BENCHMARKS.md](BENCHMARKS.md)를 참조하세요.

## 문서

- [아키텍처](ARCHITECTURE.md) - 시스템 구조 및 설계
- [기능 목록](FEATURES.md) - 전체 기능 설명
- [벤치마크](BENCHMARKS.md) - 성능 측정 결과
- [프로덕션 품질](PRODUCTION_QUALITY.md) - 프로덕션 체크리스트
- [프로젝트 구조](PROJECT_STRUCTURE.md) - 프로젝트 구조
- [변경 이력](CHANGELOG.md) - 버전별 변경 사항

### 가이드
- [빠른 시작](guides/QUICK_START.md)
- [FAQ](guides/FAQ.md)
- [문제 해결](guides/TROUBLESHOOTING.md)
- [모범 사례](guides/BEST_PRACTICES.md)

### 고급 주제
- [아키텍처 결정 기록](advanced/ARCHITECTURE_DECISIONS.md)
- [성능 튜닝](advanced/PERFORMANCE_TUNING.md)
- [Lock-Free 알고리즘](advanced/LOCK_FREE_ALGORITHMS.md)

## 기여

기여 가이드는 [CONTRIBUTING.md](contributing/CONTRIBUTING.md)를 참조하세요.

## 라이선스

MIT License

## 연락처

문제 보고: https://github.com/kcenon/thread_system/issues
