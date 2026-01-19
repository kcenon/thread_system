[![CI](https://github.com/kcenon/thread_system/actions/workflows/ci.yml/badge.svg?branch=phase-0-foundation)](https://github.com/kcenon/thread_system/actions/workflows/ci.yml)
[![Code Coverage](https://github.com/kcenon/thread_system/actions/workflows/coverage.yml/badge.svg?branch=phase-0-foundation)](https://github.com/kcenon/thread_system/actions/workflows/coverage.yml)
[![Static Analysis](https://github.com/kcenon/thread_system/actions/workflows/static-analysis.yml/badge.svg?branch=phase-0-foundation)](https://github.com/kcenon/thread_system/actions/workflows/static-analysis.yml)
[![Doxygen](https://github.com/kcenon/thread_system/actions/workflows/build-Doxygen.yaml/badge.svg)](https://github.com/kcenon/thread_system/actions/workflows/build-Doxygen.yaml)
[![codecov](https://codecov.io/gh/kcenon/thread_system/branch/main/graph/badge.svg)](https://codecov.io/gh/kcenon/thread_system)

# Thread System Project

> **Language:** [English](README.md) | **한국어**

## 개요

Thread System Project는 동시성 프로그래밍의 민주화를 목표로 설계된 포괄적인 C++20 멀티스레딩 프레임워크입니다. 모듈식 인터페이스 기반 아키텍처로 구축되어 직관적인 추상화와 견고한 구현을 제공하며, 모든 수준의 개발자가 일반적인 수동 thread 관리의 복잡성과 함정 없이 고성능의 thread-safe 애플리케이션을 구축할 수 있도록 지원합니다.

> **🏗️ Modular Architecture**: 공격적인 리팩토링과 coroutine 제거를 통해 약 2,700줄의 고도로 최적화된 코드로 간소화되었습니다. Logger와 monitoring system은 최대한의 유연성을 위해 별도의 선택적 프로젝트로 제공됩니다.

> **✅ 최신 업데이트 (2026-01)**:
> - Queue 통합 완료: 10개 구현 → 2개 공개 타입 (adaptive_job_queue, job_queue)
> - Deprecated queue 제거: typed_job_queue_t, typed_lockfree_job_queue_t, adaptive_typed_job_queue_t
> - Policy-based queue 템플릿 시스템 도입 (Kent Beck Simple Design 원칙)
> - 모든 플랫폼에서 CI/CD pipeline이 정상 작동합니다.

## 🔗 프로젝트 생태계 및 상호 의존성

이 프로젝트는 고성능 동시 실행 애플리케이션을 위해 설계된 모듈식 생태계의 일부입니다:

### Core Threading Framework
- **[thread_system](https://github.com/kcenon/thread_system)** (이 프로젝트): worker pool, job queue, thread 관리를 포함하는 핵심 threading framework
  - 제공: 통합을 위한 `kcenon::thread::interfaces::logger_interface`, `kcenon::thread::interfaces::monitoring_interface`
  - 의존성: **[common_system](https://github.com/kcenon/common_system)** (필수 - C++20 Concepts 및 공통 유틸리티)
  - 용도: 핵심 threading 기능, 다른 시스템을 위한 interface

### Optional Integration Components
- **[logger_system](https://github.com/kcenon/logger_system)**: 고성능 비동기 logging
  - 구현: `kcenon::thread::interfaces::logger_interface`
  - 의존성: `thread_system` (interface용)
  - 통합: thread 작업 및 디버깅을 위한 원활한 logging

- **[monitoring_system](https://github.com/kcenon/monitoring_system)**: 실시간 metric 수집 및 성능 monitoring
  - 구현: `kcenon::thread::interfaces::monitoring_interface`
  - 의존성: `thread_system` (interface용)
  - 통합: Thread pool metric, 시스템 성능 추적

- **[integrated_thread_system](https://github.com/kcenon/integrated_thread_system)**: 완전한 솔루션 예제
  - 의존성: `thread_system`, `logger_system`, `monitoring_system`
  - 목적: 통합 예제, 완전한 애플리케이션 템플릿
  - 용도: 전체 스택 통합을 위한 참조 구현

### Dependency Flow
```
common_system (C++20 Concepts, utilities)
    ↑
thread_system (core interfaces)
    ↑                    ↑
logger_system    monitoring_system
    ↑                    ↑
    └── integrated_thread_system ──┘
```

### 통합의 이점
- **Plug-and-play**: 필요한 구성 요소만 사용
- **Interface-driven**: 깔끔한 추상화로 쉬운 교체 가능
- **Performance-optimized**: 각 시스템이 해당 도메인에 최적화됨
- **Unified ecosystem**: 모든 프로젝트에서 일관된 API 디자인

> 📖 **[Complete Architecture Guide](docs/ARCHITECTURE.md)**: 전체 생태계 아키텍처, 의존성 관계 및 통합 패턴에 대한 종합 문서.

## 프로젝트 목적 및 미션

이 프로젝트는 전 세계 개발자들이 직면한 근본적인 과제를 해결합니다: **동시성 프로그래밍을 접근 가능하고, 안전하며, 효율적으로 만드는 것**. 전통적인 threading 접근 방식은 종종 복잡한 코드, 디버그하기 어려운 race condition, 성능 병목 현상으로 이어집니다. 우리의 미션은 다음을 제공하는 것입니다:

- **Threading 복잡성 제거** - 직관적이고 고수준의 추상화를 통해
- **Thread safety 보장** - 설계 단계에서 일반적인 동시성 버그 방지
- **성능 극대화** - 최적화된 알고리즘과 최신 C++ 기능을 통해
- **코드 재사용성 촉진** - 다양한 플랫폼과 사용 사례에서
- **개발 가속화** - 바로 사용 가능한 threading 구성 요소 제공

## 핵심 장점 및 이점

### 🚀 **탁월한 성능**
- **Zero-overhead abstraction**: 최신 C++ 디자인으로 최소한의 런타임 비용 보장
- **최적화된 자료 구조**: 적응형 알고리즘과 cache-friendly 디자인
- **Adaptive scheduling**: type 기반 job 처리로 최적의 리소스 활용
- **확장 가능한 아키텍처**: 하드웨어 thread 수에 따른 선형 성능 확장

### 🛡️ **안정적인 신뢰성**
- **Thread-safe by design**: 모든 구성 요소가 안전한 동시 액세스 보장
- **포괄적인 오류 처리**: 견고한 오류 보고 및 복구 메커니즘
- **메모리 안전성**: RAII 원칙과 smart pointer로 메모리 누수 및 손상 방지
- **광범위한 테스트**: 여러 플랫폼과 컴파일러에서 95% 이상의 CI/CD 성공률

### 🔧 **개발자 생산성**
- **직관적인 API 디자인**: 깔끔하고 자체 문서화된 interface로 학습 곡선 감소
- **풍부한 문서**: 예제가 포함된 포괄적인 Doxygen 문서
- **유연한 구성**: 자동 최적화를 지원하는 adaptive queue
- **모듈식 구성 요소**: 필요한 것만 사용 - logging과 monitoring은 선택 사항

### 🌐 **크로스 플랫폼 호환성**
- **범용 지원**: Windows, Linux, macOS에서 작동
- **컴파일러 유연성**: GCC, Clang, MSVC와 호환
- **C++ 표준 적응**: C++20에서 이전 표준으로의 우아한 fallback
- **아키텍처 독립성**: x86 및 ARM 프로세서 모두에 최적화

### 📈 **엔터프라이즈 준비 기능**
- **Type-based scheduling**: 실시간 시스템을 위한 정교한 job type 전문화
- **Interface-based design**: 잘 정의된 interface로 깔끔한 관심사 분리
- **Optional integration**: 별도의 프로젝트로 제공되는 logger 및 monitoring
- **Modular architecture**: 개별 구성 요소 또는 완전한 framework 사용

## 실제 영향 및 사용 사례

### 🎯 **이상적인 애플리케이션**
- **고빈도 거래 시스템**: 마이크로초 수준의 지연 시간 요구사항
- **게임 엔진**: 실시간 렌더링 및 물리 시뮬레이션
- **웹 서버**: type 처리를 통한 동시 요청 처리
- **과학 컴퓨팅**: 병렬 알고리즘 실행 및 데이터 처리
- **미디어 처리**: 비디오 인코딩, 이미지 처리 및 오디오 스트리밍
- **IoT 시스템**: 센서 데이터 수집 및 실시간 응답 시스템

### 📊 **성능 벤치마크**

*Apple M1 (8-core) @ 3.2GHz, 16GB, macOS Sonoma에서 벤치마킹*

> **🚀 아키텍처 업데이트**: 최신 modular architecture는 깔끔한 interface 기반 디자인을 통해 약 8,700줄 이상의 코드를 제거했습니다. Logger와 monitoring system은 이제 별도의 선택적 프로젝트입니다. Adaptive queue는 모든 워크로드 시나리오에 대해 계속해서 자동 최적화를 제공합니다.

#### 핵심 성능 지표 (최신 벤치마크)
- **최대 처리량**: 최대 13.0M job/초 (1 worker, empty job - 이론적)
- **실제 처리량**:
  - Standard thread pool: 1.16M job/s (10 worker, 프로덕션에서 검증됨)
  - Typed thread pool: 1.24M job/s (6 worker, 3 type)
  - **Adaptive queue**: 모든 시나리오에 대한 자동 최적화
- **Job 스케줄링 지연 시간**:
  - Standard pool: ~77 나노초 (신뢰할 수 있는 기준선)
  - **Adaptive queue**: 자동 전략 선택으로 96-580ns
- **Queue 작업**: Adaptive 전략은 필요할 때 **최대 7.7배 더 빠름**
- **높은 경합**: Adaptive 모드는 유익할 때 **최대 3.46배 개선** 제공
- **Priority scheduling**: 모든 조건에서 **높은 정확도**의 type 기반 라우팅
- **메모리 효율성**: <1MB baseline, 약 8,700줄 이상의 코드베이스 감소
- **확장성**: Adaptive 아키텍처는 모든 경합 수준에서 성능 유지

#### Thread Safety 수정의 영향
- **Wake interval 액세스**: mutex 보호로 5% 성능 영향
- **Cancellation token**: 적절한 double-check 패턴으로 3% 오버헤드
- **Job queue 작업**: 중복 atomic counter 제거 후 4% 성능 *향상*

#### 상세 성능 데이터

**실제 성능** (실제 워크로드로 측정):

*측정된 성능 (실제 워크로드):*
| 구성 | 처리량 | 100만 job당 시간 | Worker | 참고 |
|--------------|------------|--------------|---------|-------|
| Basic Pool   | 1.16M/s    | 862 ms       | 10      | 🏆 실제 기준 성능 |
| Adaptive Pool | Dynamic    | Optimized    | Variable| 🚀 부하 기반 자동 최적화 |
| Type Pool    | 1.24M/s    | 806 ms       | 6       | ✅ 더 적은 worker로 6.9% 더 빠름 |
| **Adaptive Queue** | **Dynamic** | **Optimized** | **Auto** | 🚀 **자동 최적화** |
| Peak (empty) | 13.0M/s    | -            | 1       | 📊 이론적 최대값 |

*Adaptive Queue 성능 (자동 최적화):*
| 경합 수준 | 선택된 전략 | 지연 시간 | vs Mutex 전용 | 이점 |
|-----------------|-------------------|---------|---------------|---------|
| Low (1-2 thread) | Mutex | 96 ns | Baseline | 낮은 부하에 최적 |
| Medium (4 thread) | Adaptive | 142 ns | +8.2% 더 빠름 | 균형 잡힌 성능 |
| High (8+ thread) | Lock-free | 320 ns | +37% 더 빠름 | 경합 시 확장 |
| Variable Load | **Auto-switching** | **Dynamic** | **Optimized** | **자동** |

## 문서

### 시작하기
- 📖 [빠른 시작 가이드](docs/guides/QUICK_START_KO.md) - 5분 안에 시작하기
- 🔧 [빌드 가이드](docs/guides/BUILD_GUIDE_KO.md) - 상세한 빌드 지침
- 🚀 [사용자 가이드](docs/advanced/USER_GUIDE_KO.md) - 포괄적인 사용 가이드

### 핵심 문서
- 📚 [기능](docs/FEATURES.md) - 상세한 기능 설명
- ⚡ [벤치마크](docs/BENCHMARKS.md) - 포괄적인 성능 데이터
- 📋 [API 레퍼런스](docs/advanced/02-API_REFERENCE_KO.md) - 완전한 API 문서
- 🏛️ [아키텍처](docs/advanced/01-ARCHITECTURE_KO.md) - 시스템 설계 및 내부

### 고급 주제
- 🔬 [성능 기준](docs/advanced/BASELINE.md) - 기준 지표 및 회귀 감지
- 🛡️ [프로덕션 품질](docs/PRODUCTION_QUALITY.md) - CI/CD, 테스트, 품질 지표
- 🧩 [C++20 Concepts](docs/advanced/CPP20_CONCEPTS_KO.md) - 스레드 작업을 위한 타입 안전 제약
- 📁 [프로젝트 구조](docs/PROJECT_STRUCTURE.md) - 상세한 코드베이스 구성
- ⚠️ [알려진 문제](docs/advanced/KNOWN_ISSUES.md) - 현재 제한 사항 및 해결 방법
- 📗 [큐 선택 가이드](docs/advanced/QUEUE_SELECTION_GUIDE.md) - 올바른 큐 선택

### 개발
- 🤝 [기여 가이드](docs/contributing/CONTRIBUTING.md) - 기여 방법
- 🔍 [문제 해결](docs/guides/TROUBLESHOOTING_KO.md) - 일반적인 문제 및 해결책
- ❓ [FAQ](docs/guides/FAQ.md) - 자주 묻는 질문
- 🔄 [마이그레이션 가이드](docs/advanced/MIGRATION_KO.md) - 이전 버전에서 업그레이드

Doxygen으로 API 문서 빌드 (선택 사항):

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target docs
# documents/html/index.html 열기
```

*실제 워크로드 성능 (8-worker 구성):*
| Job 복잡도 | 처리량 | 사용 사례 | 확장 효율성 |
|----------------|------------|----------|-------------------|
| **Empty job**     | 8.2M/s     | 📏 Framework 오버헤드 측정 | 95% |
| **1 μs 작업**     | 1.5M/s     | ⚡ 매우 가벼운 연산 | 94% |
| **10 μs 작업**    | 540K/s     | 🔧 일반적인 작은 작업 | 92% |
| **100 μs 작업**   | 70K/s      | 💻 중간 연산 | 90% |
| **1 ms 작업**     | 7.6K/s     | 🔥 무거운 연산 | 88% |
| **10 ms 작업**    | 760/s      | 🏗️ 매우 무거운 연산 | 85% |

**Worker Thread 확장 분석**:
| Worker | 가속 | 효율성 | 성능 등급 | 권장 사용 |
|---------|---------|------------|-------------------|-----------------|
| 1       | 1.0x    | 💯 **100%** | 🥇 우수 | 단일 스레드 워크로드 |
| 2       | 2.0x    | 💚 **99%**  | 🥇 우수 | 듀얼 코어 시스템 |
| 4       | 3.9x    | 💚 **97.5%**  | 🥇 우수 | 쿼드 코어 최적 |
| 8       | 7.7x    | 💚 **96%**  | 🥈 매우 좋음 | 표준 멀티 코어 |
| 16      | 15.0x   | 💙 **94%**  | 🥈 매우 좋음 | 고급 워크스테이션 |
| 32      | 28.3x   | 💛 **88%**  | 🥉 좋음 | 서버 환경 |

**라이브러리 성능 비교** (실제 측정):
| 라이브러리 | 처리량 | 성능 | 평가 | 주요 기능 |
|---------|------------|-------------|---------|--------------|
| 🥇 **Thread System (Typed)** | **1.24M/s** | 🟢 **107%** | ✅ **우수** | Priority scheduling, adaptive queue, C++20 |
| 🥈 **Intel TBB** | ~1.24M/s | 🟢 **107%** | ✅ **우수** | 업계 표준, work stealing |
| 🏆 **Thread System (Standard)** | **1.16M/s** | 🟢 **100%** | ✅ **기준** | Adaptive queue, 검증된 성능 |
| 📦 Boost.Thread Pool | ~1.09M/s | 🟡 **94%** | ✅ 좋음 | Header-only, 이식 가능 |
| 📦 OpenMP | ~1.06M/s | 🟡 **92%** | ✅ 좋음 | 컴파일러 지시문, 사용하기 쉬움 |
| 📦 Microsoft PPL | ~1.02M/s | 🟡 **88%** | ✅ 좋음 | Windows 전용 |
| 📚 std::async | ~267K/s | 🔴 **23%** | ⚠️ 제한적 | 표준 라이브러리, 기본 기능 |

**Logger 성능 비교** (높은 경합 시나리오):
| Logger Type | Single Thread | 4 Thread | 8 Thread | 16 Thread | 최적 사용 사례 |
|-------------|---------------|-----------|-----------|------------|---------------|
| 🏆 **Thread System Logger** | 4.41M/s | **1.07M/s** | **0.41M/s** | **0.39M/s** | 모든 시나리오 (adaptive) |
| 🥈 **Standard Mode** | 4.41M/s | 0.86M/s | 0.23M/s | 0.18M/s | 낮은 동시성 |
| 📊 **Adaptive 이점** | 0% | **+24%** | **+78%** | **+117%** | 자동 최적화 |

**Logger vs 업계 표준** (spdlog 비교 포함):
| System | Single-thread | 4 Thread | 8 Thread | 지연 시간 | vs Console |
|--------|---------------|-----------|-----------|---------|------------|
| 🐌 **Console** | 583K/s | - | - | 1,716 ns | Baseline |
| 🏆 **TS Logger** | **4.34M/s** | **1.07M/s** | **412K/s** | **148 ns** | 🚀 **7.4x** |
| 📦 **spdlog** | 515K/s | 210K/s | 52K/s | 2,333 ns | 🔴 **0.88x** |
| ⚡ **spdlog async** | **5.35M/s** | 785K/s | 240K/s | - | 🚀 **9.2x** |

**주요 인사이트**:
- 🏃 **Single-thread**: spdlog async 승리 (5.35M/s) 하지만 TS Logger도 근접 (4.34M/s)
- 🏋️ **Multi-thread**: Adaptive queue를 사용하는 TS Logger는 일관된 성능 표시
- ⏱️ **지연 시간**: TS Logger가 148ns로 승리 (spdlog보다 **15.7배 낮음**)
- 📈 **확장성**: Adaptive 모드는 자동 최적화 제공

**Type-based Thread Pool 성능 비교**:

*Mutex 기반 구현:*
| 복잡도 | vs Basic Pool | Type 정확도 | 성능 | 최적 용도 |
|------------|--------------|---------------|-------------|----------|
| **Single Type** | 💚 **-3%** | 💯 **100%** | 525K/s | 특수 워크로드 |
| **3 Type** | 💛 **-9%** | 💯 **99.6%** | 495K/s | 표준 우선순위화 |
| **실제 워크로드** | 💚 **+6.9%** | 💯 **100%** | **1.24M/s** | **실제 측정** |

*Adaptive Queue 사용:*
| 시나리오 | 성능 | vs Standard | Type 정확도 | 참고 |
|----------|-------------|-------------|---------------|-------|
| **낮은 경합** | 1.24M/s | 동일 | 💯 **100%** | Mutex 전략 선택됨 |
| **높은 경합** | Dynamic | **최대 +71%** | 💯 **99%+** | Lock-free 모드 활성화 |
| **혼합 워크로드** | Optimized | **자동** | 💯 **99.5%** | 필요에 따라 전략 전환 |
| **실제 측정** | **1.24M/s** | **+6.9%** | 💯 **100%** | **프로덕션 워크로드** |

**메모리 사용량 및 생성 성능**:
| Worker | 생성 시간 | 메모리 사용량 | 효율성 | 리소스 등급 |
|---------|---------------|--------------|------------|-----------------|
| 1       | 🟢 **162 ns** | 💚 **1.2 MB** | 💯 **100%** | ⚡ 초경량 |
| 4       | 🟢 **347 ns** | 💚 **1.8 MB** | 💚 **98%** | ⚡ 매우 가벼움 |
| 8       | 🟡 **578 ns** | 💛 **2.6 MB** | 💚 **96%** | 🔋 가벼움 |
| 16      | 🟡 **1.0 μs** | 🟡 **4.2 MB** | 💛 **94%** | 🔋 중간 |
| 32      | 🟠 **2.0 μs** | 🟠 **7.4 MB** | 🟡 **88%** | 📊 무거움 |

포괄적인 성능 분석 및 최적화 기술은 [Performance Guide](docs/PERFORMANCE.md)를 참조하세요.

## 기술 스택 및 아키텍처

### 🏗️ **최신 C++ 기반**
- **C++20 기능**: `std::jthread`, `std::format`, concept, range
- **Template metaprogramming**: Type-safe, 컴파일 타임 최적화
- **메모리 관리**: Smart pointer 및 RAII를 통한 자동 리소스 정리
- **Exception safety**: 전체적으로 강력한 exception safety 보장
- **Adaptive 알고리즘**: MPMC queue, 자동 전략 선택, atomic 연산
- **Interface 기반 디자인**: interface와 구현 간의 깔끔한 분리
- **Modular architecture**: 선택적 logger/monitoring 통합이 가능한 핵심 threading 기능

### 🔄 **디자인 패턴 구현**
- **Command Pattern**: 유연한 작업 실행을 위한 job 캡슐화
- **Observer Pattern**: 이벤트 기반 logging 및 monitoring
- **Factory Pattern**: 구성 가능한 thread pool 생성
- **Singleton Pattern**: Thread safety를 갖춘 전역 logger 액세스
- **Template Method Pattern**: 사용자 정의 가능한 thread 동작
- **Strategy Pattern**: 구성 가능한 backoff 전략 및 scheduling 정책

## 프로젝트 구조

### 📁 **디렉토리 구성**

```
thread_system/
├── 📁 include/kcenon/thread/       # Public header
│   ├── 📁 core/                    # 핵심 구성 요소
│   │   ├── thread_base.h           # 추상 thread 클래스
│   │   ├── thread_pool.h           # Thread pool interface
│   │   ├── thread_worker.h         # Worker thread
│   │   ├── job.h                   # Job interface
│   │   ├── callback_job.h          # 함수 기반 job
│   │   ├── job_queue.h             # Thread-safe queue
│   │   ├── service_registry.h      # Dependency injection
│   │   ├── cancellation_token.h    # Cancellation 지원
│   │   ├── sync_primitives.h       # Synchronization wrapper
│   │   └── error_handling.h        # Result<T> 패턴
│   ├── 📁 interfaces/              # 통합 interface
│   │   ├── logger_interface.h      # Logger 추상화
│   │   ├── monitoring_interface.h  # Monitoring 추상화
│   │   ├── thread_context.h        # Thread context
│   │   └── service_container.h     # Service 관리
│   ├── 📁 utils/                   # Utility
│   │   ├── formatter.h             # 문자열 포맷팅
│   │   ├── convert_string.h        # 문자열 변환
│   │   └── span.h                  # Span utility
│   └── compatibility.h             # 이전 버전 호환성
├── 📁 src/                         # 구현 파일
│   ├── 📁 core/                    # 핵심 구현
│   │   ├── thread_base.cpp         # Thread base 구현
│   │   ├── job.cpp                 # Job 구현
│   │   ├── callback_job.cpp        # Callback job 구현
│   │   └── job_queue.cpp           # Queue 구현
│   ├── 📁 impl/                    # 구체적 구현
│   │   ├── 📁 thread_pool/         # Thread pool 구현
│   │   │   ├── thread_pool.cpp     # Pool 구현
│   │   │   └── thread_worker.cpp   # Worker 구현
│   │   └── 📁 typed_pool/          # Typed thread pool
│   │       ├── typed_thread_pool.h # Typed pool header
│   │       ├── aging_typed_job_queue.h # Priority aging queue
│   │       └── typed_job.h        # Typed job base
│   └── 📁 utils/                   # Utility 구현
│       └── convert_string.cpp      # 문자열 변환 구현
├── 📁 examples/                    # 예제 애플리케이션
│   ├── thread_pool_sample/         # 기본 thread pool 사용
│   ├── typed_thread_pool_sample/   # Priority scheduling
│   ├── adaptive_queue_sample/      # Adaptive queue 사용
│   ├── queue_factory_sample/       # 요구사항 기반 queue 생성
│   ├── queue_capabilities_sample/  # Runtime capability 조회
│   ├── hazard_pointer_sample/      # 메모리 회수
│   └── integration_example/        # 통합 예제
├── 📁 tests/                       # 모든 테스트
│   ├── 📁 unit/                    # 단위 테스트
│   │   ├── thread_base_test/       # 기본 기능
│   │   ├── thread_pool_test/       # Pool 테스트
│   │   ├── interfaces_test/        # Interface 테스트
│   │   └── utilities_test/         # Utility 테스트
│   └── 📁 benchmarks/              # 성능 테스트
│       ├── thread_base_benchmarks/ # 핵심 벤치마크
│       ├── thread_pool_benchmarks/ # Pool 벤치마크
│       └── typed_thread_pool_benchmarks/ # Typed pool 벤치마크
├── 📁 docs/                        # 문서
├── 📁 cmake/                       # CMake module
├── 📄 CMakeLists.txt               # 빌드 구성
├── 📄 STRUCTURE.md                 # 프로젝트 구조 가이드
└── 📄 vcpkg.json                   # 의존성
```

### 📖 **주요 파일 및 목적**

#### Core Module 파일
- **`thread_base.h/cpp`**: 모든 worker thread의 추상 기본 클래스
- **`job.h/cpp`**: 작업 단위를 위한 추상 interface
- **`job_queue.h/cpp`**: Thread-safe FIFO queue 구현
- **`callback_job.h/cpp`**: Lambda 기반 job 구현

#### Thread Pool 파일
- **`thread_pool.h/cpp`**: Worker를 관리하는 주요 thread pool 클래스
- **`thread_worker.h/cpp`**: Job을 처리하는 worker thread
- **`future_extensions.h`**: 비동기 결과를 위한 future 기반 작업 확장

#### Typed Thread Pool 파일
- **`typed_thread_pool.h`**: Template 기반 priority thread pool
- **`typed_thread_worker.h`**: Type 책임 목록이 있는 worker
- **`job_types.h`**: 기본 priority 열거형 (RealTime, Batch, Background)
- **`aging_typed_job_queue.h`**: Priority aging 지원 queue (policy_queue 기반)

#### Logger 파일
- **`logger.h`**: 자유 함수가 있는 공용 API
- **`log_collector.h/cpp`**: 중앙 log 메시지 라우터
- **`console_writer.h/cpp`**: 색상이 있는 콘솔 출력
- **`file_writer.h/cpp`**: 회전 파일 logger

### 🔗 **Module 의존성**

```
utilities (의존성 없음)
    │
    └──> thread_base
             │
             ├──> thread_pool
             │
             └──> typed_thread_pool
                        │
                        └── typed_thread_pool (adaptive)

선택적 외부 프로젝트:
- logger (logging 기능을 위한 별도 프로젝트)
- monitoring (metric 수집을 위한 별도 프로젝트)
```

### 🛠️ **빌드 출력 구조**

```
build/
├── bin/                    # 실행 파일
│   ├── thread_pool_sample
│   ├── typed_thread_pool_sample          # Mutex 기반
│   ├── typed_thread_pool_sample_2        # 고급 사용
│   ├── logger_sample
│   ├── monitoring_sample
│   ├── adaptive_benchmark               # 🆕 성능 비교
│   ├── queue_comparison_benchmark        # 🆕 Queue 벤치마크
│   └── ...
├── lib/                    # 정적 라이브러리
│   ├── libthread_base.a
│   ├── libthread_pool.a
│   ├── libtyped_thread_pool.a  # Mutex 및 lock-free 모두 포함
│   └── libutilities.a
└── include/                # Public header (설치용)
```

## 주요 구성 요소

### 1. [Core Threading Foundation (thread_module)](https://github.com/kcenon/thread_system/tree/main/core)

#### Base Components
- **`thread_base` 클래스**: 모든 thread 작업의 기본 추상 클래스
  - 조건부 컴파일을 통해 `std::jthread` (C++20) 및 `std::thread` 모두 지원
  - 생명 주기 관리 (start/stop) 및 사용자 정의 가능한 hook 제공
  - Thread 상태 monitoring 및 상태 관리

#### Job System
- **`job` 클래스**: Cancellation 지원이 있는 작업 단위의 추상 기본 클래스
- **`callback_job` 클래스**: `std::function`을 사용하는 구체적인 job 구현
- **`job_queue` 클래스**: Job 관리를 위한 thread-safe queue
- **`backpressure_job_queue`**: Backpressure 지원이 있는 고품질 queue
  - 최대 queue 크기 강제 (메모리 고갈 방지)
  - 다양한 backpressure 정책 (block, drop_oldest, drop_newest, callback, adaptive)
  - Token bucket 기반 rate limiting
  - Watermark 기반 pressure detection
  - 리소스 제약이 있는 시스템에 이상적

> **참고**: 간단한 용량 제한의 경우 `job_queue`의 `max_size` 파라미터를 사용하세요.
- **`cancellation_token`** 🆕: 향상된 협력적 cancellation 메커니즘
  - 계층적 cancellation을 위한 연결된 token 생성
  - Thread-safe callback 등록
  - Cancellation signal의 자동 전파

#### Synchronization Primitives 🆕
- **`sync_primitives.h`**: 향상된 synchronization wrapper
  - `scoped_lock_guard`: 시간 초과 지원이 있는 RAII lock
  - `condition_variable_wrapper`: Predicate가 있는 향상된 condition variable
  - `atomic_flag_wrapper`: Wait/notify가 있는 확장 atomic 연산
  - `shared_mutex_wrapper`: Reader-writer lock 구현

#### Service Infrastructure 🆕
- **`service_registry`**: 경량 dependency injection container
  - Type-safe service 등록 및 검색
  - Shared_mutex를 사용한 thread-safe 액세스
  - Shared_ptr를 통한 자동 수명 관리

#### Adaptive Components
- **`adaptive_job_queue`**: Mutex 및 lock-free 전략을 모두 지원하는 이중 모드 queue
- **`lockfree_job_queue`**: Lock-free MPMC queue (adaptive 모드에서 사용)
- **`hazard_pointer`**: Lock-free 자료 구조를 위한 안전한 메모리 회수
- **`node_pool`**: 효율적인 node 할당을 위한 메모리 pool

### 2. [Logging System (별도 프로젝트)](https://github.com/kcenon/logger)

> **참고**: Logging system은 이제 최대한의 유연성과 최소한의 의존성을 위해 별도의 선택적 프로젝트로 제공됩니다.

- **Namespace 수준 logging 함수**: `write_information()`, `write_error()`, `write_debug()` 등
- **`log_types` 열거형**: Bitwise 활성화된 log 수준 (Exception, Error, Information, Debug, Sequence, Parameter)
- **여러 출력 대상**:
  - `console_writer`: 색상 지원이 있는 비동기 콘솔 출력
  - `file_writer`: 백업 지원이 있는 회전 파일 출력
  - `callback_writer`: Log 처리를 위한 사용자 정의 callback
- **`log_collector` 클래스**: Log 메시지 라우팅 및 처리를 위한 중앙 hub
- **구성 함수**: `set_title()`, `console_target()`, `file_target()` 등

### 3. [Thread Pool System (thread_pool_module)](https://github.com/kcenon/thread_system/tree/main/sources/thread_pool)

#### Standard Thread Pool
- **`thread_pool` 클래스**: Adaptive queue 지원이 있는 thread pool
  - 동적 worker 추가/제거
  - 이중 모드 job queue 아키텍처 (mutex 및 lock-free)
  - 일반 워크로드에 대한 검증된 신뢰성
- **`thread_worker` 클래스**: Adaptive queue를 지원하는 worker thread 구현

#### Adaptive Queue 기능
- **Adaptive job queue**: 자동 최적화를 지원하는 이중 모드 queue 구현
  - Mutex 및 lock-free 모드 간 **동적 전략 선택**
  - 필요할 때 hazard pointer가 있는 MPMC queue
  - 경합 처리를 위한 지능형 backoff
  - 향상된 처리량을 위한 배치 처리 지원
  - Worker별 통계 추적
  - 선택적 배치 처리 모드
  - 구성 가능한 backoff 전략

#### 공통 기능
- **`task<T>` template**: 비동기 결과를 위한 future 기반 작업 wrapper
- **Builder 패턴 지원**: Pool 구성을 위한 fluent API
- **Drop-in 호환성**: 쉬운 마이그레이션을 위한 동일한 API

### 4. [Real-time Monitoring System (별도 프로젝트)](https://github.com/kcenon/monitoring)

> **참고**: Monitoring system은 이제 깔끔한 관심사 분리를 위해 별도의 선택적 프로젝트로 제공됩니다.

- **`metrics_collector` 클래스**: 실시간 성능 metric 수집 엔진
- **크로스 플랫폼 시스템 metric**: 메모리 사용량, CPU 사용률, 활성 thread
- **Thread pool monitoring**: Job 완료율, queue 깊이, worker 활용률
- **Lock-free storage**: 시계열 데이터를 위한 메모리 효율적인 ring buffer
- **쉬운 통합**: 간단한 API를 갖춘 전역 singleton collector
- **주요 기능**:
  - 실시간 데이터 수집 (100ms-1s 간격)
  - Thread-safe metric 등록 및 업데이트
  - 구성 가능한 버퍼 크기 및 수집 간격
  - 비활성화 시 오버헤드 없음

### 5. [Typed Thread Pool System (typed_thread_pool_module)](https://github.com/kcenon/thread_system/tree/main/sources/typed_thread_pool)

Framework는 다양한 시나리오에 최적화된 두 가지 별개의 typed thread pool 구현을 제공합니다:

#### Typed Thread Pool 구현
- **`typed_thread_pool` 클래스**: Adaptive queue 지원이 있는 priority thread pool
- **최적**: 자동 최적화를 지원하는 type 기반 job scheduling
- **성능**: Adaptive queue는 다양한 워크로드에 최적의 성능 제공
- **기능**:
  - **Type별 adaptive queue**: 각 job type은 최적화된 queue 전략을 사용할 수 있음
  - **Priority 기반 라우팅**: RealTime > Batch > Background 순서
  - **Adaptive queue 지원**: 최적의 성능을 위해 이중 모드 queue 사용
  - **동적 queue 생성**: 자동 type queue 생명 주기 관리
  - **고급 통계**: Type별 metric 및 성능 monitoring

#### 공통 구성 요소
- **`job_types` 열거형**: 기본 priority 수준 (RealTime, Batch, Background)
- **Type 인식 구성 요소**:
  - `typed_job_t<T>`: 관련 type/priority가 있는 job
  - `aging_typed_job_queue_t<T>`: Priority aging 지원 queue (policy_queue 기반)
  - `typed_thread_worker_t<T>`: Queue 처리를 지원하는 worker
- **`callback_typed_job<T>`**: Lambda 기반 typed job 구현
- **사용자 정의 type 지원**: Job prioritization을 위해 자체 열거형 또는 type 사용

#### 사용 가이드라인
- **Adaptive 구현 사용**: 모든 시나리오에 대한 자동 최적화
- **이점**: 자동 성능 튜닝을 통한 간소화된 배포

## 고급 기능 및 능력

### 🎛️ **지능형 작업 스케줄링**
- **Adaptive 구현 전략**: 런타임 조건에 따른 자동 최적화
- **Type 인식 job 분배**: Worker는 구성 가능한 책임 목록으로 여러 type 수준을 처리할 수 있음
- **Priority 기반 scheduling**: Adaptive 구현은 최적의 priority 순서 제공 (RealTime > Batch > Background)
- **동적 type 적응**: 워크로드 패턴에 따른 worker 책임의 런타임 조정
- **FIFO 보장**: 동일한 type 수준 내에서 엄격한 선입선출 순서
- **Type별 queue 최적화**: Adaptive 구현은 각 job type에 최적화된 queue 사용
- **고급 경합 처리**: 안전한 메모리 회수를 위한 hazard pointer를 사용한 자동 전략 선택
- **확장 가능한 아키텍처**: 경합 패턴에 따른 동적 확장 최적화

### 🔬 **고급 Threading 기능**
- **계층적 디자인**: 특수화된 파생 클래스가 있는 깔끔한 `thread_base` 기반
- **C++20 호환성**: `std::jthread`에 대한 완전한 지원과 `std::thread`로의 우아한 fallback
- **Cancellation 지원**: `std::stop_token`을 사용한 협력적 작업 cancellation
- **사용자 정의 thread 이름 지정**: 의미 있는 thread 식별로 향상된 디버깅
- **Wake interval 지원**: Busy waiting 없는 주기적 작업 실행
- **Result<T> type**: Monadic 연산을 사용한 최신 오류 처리

### 📊 **프로덕션 Monitoring 및 진단**
- **선택적 monitoring 통합**: 필요할 때 별도의 monitoring 프로젝트와 연결
- **성능 프로파일링**: 내장된 타이밍 및 병목 현상 식별
- **상태 확인**: Thread 실패 및 복구의 자동 감지
- **선택적 logging 통합**: 포괄적인 logging을 위해 별도의 logger 프로젝트와 연결

### ⚙️ **구성 및 사용자 정의**
- **Template 기반 유연성**: 사용자 정의 type type 및 job 구현
- **런타임 구성**: 배포 유연성을 위한 JSON 기반 구성
- **컴파일 타임 최적화**: 최소 오버헤드를 위한 조건부 기능 컴파일
- **Builder 패턴**: 쉬운 thread pool 구성을 위한 fluent API
- **Worker policy 시스템** 🆕: Worker 동작에 대한 세밀한 제어
  - **Scheduling policy**: FIFO, LIFO, Priority, Work-stealing
  - **Idle 동작**: 구성 가능한 timeout, yield 또는 sleep 전략
  - **성능 튜닝**: CPU pinning, batch 크기 구성
  - **사전 정의된 policy**: `default_policy()`, `high_performance()`, `low_latency()`, `power_efficient()`
  - **사용자 정의 policy**: 애플리케이션별 worker 동작 정의

### 🔒 **안전성 및 신뢰성**
- **Exception safety**: Framework 전체에 걸쳐 강력한 exception safety 보장
- **리소스 누수 방지**: RAII 원칙을 사용한 자동 정리
- **Deadlock 방지**: 신중한 lock 순서 지정 및 시간 초과 메커니즘
- **메모리 손상 보호**: Smart pointer 사용 및 경계 검사

## 📖 API 참조

### Thread Pool API

`thread_pool` 클래스는 동시 작업 실행을 위한 포괄적인 API를 제공합니다:

#### 생명 주기 관리
```cpp
auto pool = std::make_shared<thread_pool>("PoolName");
auto result = pool->start();           // 처리 시작
result = pool->stop(false);            // 우아한 종료 (현재 작업 대기)
result = pool->stop(true);             // 즉시 종료
bool running = pool->is_running();     // Pool 활성 상태 확인
```

#### Job 제출
```cpp
// 편의 API - 간단한 작업 제출
bool success = pool->submit_task([]() {
    // 작업 코드
});

// Job 기반 API - 고급 기능용 (cancellation, 결과 처리)
auto job = std::make_unique<callback_job>([]() -> result_void {
    // Job 코드
    return {};
});
pool->enqueue(std::move(job));

// Batch 제출
std::vector<std::unique_ptr<job>> jobs;
// ... jobs vector 채우기
pool->enqueue_batch(std::move(jobs));
```

#### Worker 관리
```cpp
// 개별 worker 추가
auto worker = std::make_unique<thread_worker>();
pool->enqueue(std::move(worker));

// 여러 worker 추가
std::vector<std::unique_ptr<thread_worker>> workers;
for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
    workers.push_back(std::make_unique<thread_worker>());
}
pool->enqueue_batch(std::move(workers));
```

#### Monitoring 및 상태
```cpp
size_t workers = pool->get_thread_count();        // Worker thread 수
size_t pending = pool->get_pending_task_count();  // 대기 중인 작업
size_t idle = pool->get_idle_worker_count();      // Job 처리 중이지 않은 worker
pool->report_metrics();                            // Monitoring 시스템에 보고
```

#### 종료
```cpp
// 우아한 종료 (작업 완료 대기)
bool success = pool->shutdown_pool(false);

// 즉시 종료 (작업 중단 가능)
success = pool->shutdown_pool(true);
```

### Backpressure Job Queue API

`backpressure_job_queue` 클래스는 다양한 backpressure 정책을 지원하는 고품질 queue를 제공합니다:

```cpp
#include <kcenon/thread/core/backpressure_job_queue.h>

// Backpressure 설정으로 queue 생성
backpressure_config config;
config.max_size = 1000;
config.policy = backpressure_policy::adaptive;
config.low_watermark = 100;
config.high_watermark = 800;

auto queue = std::make_shared<backpressure_job_queue>(config);

// Job enqueue
auto result = queue->enqueue(std::move(job));
if (!result) {
    // Backpressure로 인한 거부 처리
}

// Pressure 수준 확인
auto level = queue->get_pressure_level();
if (level == pressure_level::high) {
    // Producer 속도 조절
}
```

> **참고**: 간단한 크기 제한의 경우 `job_queue`의 `max_size` 파라미터를 사용하세요.

### Worker Policy API

사전 정의된 또는 사용자 정의 policy로 worker 동작 구성:

```cpp
#include <kcenon/thread/core/worker_policy.h>

// 사전 정의된 policy 사용
auto policy = worker_policy::high_performance();  // 최소 지연 시간
policy = worker_policy::power_efficient();        // 낮은 CPU 사용량
policy = worker_policy::low_latency();            // 가장 빠른 응답
policy = worker_policy::default_policy();         // 균형잡힌

// 사용자 정의 policy
worker_policy custom;
custom.scheduling = scheduling_policy::priority;
custom.idle_strategy = idle_strategy::yield;
custom.max_batch_size = 64;

// Worker에 적용 (policy 지원이 있는 typed_thread_pool 사용 시)
auto worker = std::make_unique<thread_worker>();
// 참고: 표준 thread_worker는 policy 구성을 노출하지 않음
```

### Cancellation Token API

장시간 실행되는 job을 위한 협력적 cancellation:

```cpp
#include <kcenon/thread/core/cancellation_token.h>

auto token = std::make_shared<cancellation_token>();

// Producer thread에서
pool->submit_task([token]() {
    for (int i = 0; i < 1000000; ++i) {
        if (token->is_cancelled()) {
            return;  // 조기 종료
        }
        // 작업 수행
    }
});

// 다른 thread에서
token->cancel();  // Cancellation 요청
```

## 빠른 시작 및 사용 예제

### 🚀 **5분 안에 시작하기**

#### Adaptive 고성능 예제

```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/jobs/callback_job.h>
// Optional: #include "logger/core/logger.h" // 별도 logger 프로젝트 사용 시

using namespace kcenon::thread;


int main() {
    // 1. Logger 시작 (별도 logger 프로젝트 사용 시)
    // log_module::start();

    // 2. 고성능 adaptive thread pool 생성
    auto pool = std::make_shared<thread_pool>();

    // 3. Adaptive queue 최적화로 worker 추가
    std::vector<std::unique_ptr<thread_worker>> workers;
    for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
        auto worker = std::make_unique<thread_worker>();
        
        workers.push_back(std::move(worker));
    }
    pool->enqueue_batch(std::move(workers));

    // 4. 처리 시작
    pool->start();

    // 5. Job 제출 - adaptive pool은 다양한 경합을 효율적으로 처리
    std::atomic<int> counter{0};
    const int total_jobs = 100000;

    for (int i = 0; i < total_jobs; ++i) {
        pool->enqueue(std::make_unique<callback_job>(
            [&counter, i]() -> result_void {
                counter.fetch_add(1);
                if (i % 10000 == 0) {
                    // Optional logging: log_module::write_information("Processed {} jobs", i);
                    std::cout << "Processed " << i << " jobs\n";
                }
                return {};
            }
        ));
    }

    // 6. 진행 상황 monitoring으로 완료 대기
    auto start_time = std::chrono::high_resolution_clock::now();
    while (counter.load() < total_jobs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    auto end_time = std::chrono::high_resolution_clock::now();

    // 7. 포괄적인 성능 통계 가져오기
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    auto throughput = static_cast<double>(total_jobs) / duration.count() * 1000.0;

    // Optional logging (또는 std::cout 사용)
    std::cout << "Performance Results:\n";
    std::cout << "- Total jobs: " << total_jobs << "\n";
    std::cout << "- Execution time: " << duration.count() << " ms\n";
    std::cout << "- Throughput: " << std::fixed << std::setprecision(2) << throughput << " jobs/second\n";

    auto workers_list = pool->get_workers();
    for (size_t i = 0; i < workers_list.size(); ++i) {
        auto stats = static_cast<thread_worker*>(workers_list[i].get())->get_statistics();
        std::cout << "Worker " << i << ": " << stats.jobs_processed << " jobs, avg time: "
                  << stats.avg_processing_time_ns << " ns, " << stats.batch_operations << " batch ops\n";
    }

    // 8. 깔끔한 종료
    pool->stop();
    // log_module::stop(); // Logger 사용 시

    return 0;
}
```

> **성능 팁**: Adaptive queue는 워크로드에 맞게 자동으로 최적화됩니다. Mutex 기반 신뢰성과 유익할 때 lock-free 성능을 모두 제공합니다.

### 🔄 **추가 사용 예제**

#### Standard Thread Pool (낮은 경합)
```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/jobs/callback_job.h>

using namespace kcenon::thread;


// 낮은 경합 워크로드를 위한 간단한 thread pool 생성
auto pool = std::make_shared<thread_pool>("StandardPool");

// Worker 추가
std::vector<std::unique_ptr<thread_worker>> workers;
for (int i = 0; i < 4; ++i) {  // 간단한 작업을 위한 적은 수의 worker
    workers.push_back(std::make_unique<thread_worker>());
}
pool->enqueue_batch(std::move(workers));
pool->start();

// Job 제출
for (int i = 0; i < 100; ++i) {
    pool->enqueue(std::make_unique<callback_job>(
        [i]() -> result_void {
            // 데이터 처리
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            log_module::write_debug("Processed item {}", i);
            return {};
        }
    ));
}
```

#### Adaptive Thread Pool (높은 경합)
```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/jobs/callback_job.h>

using namespace kcenon::thread;


// 높은 경합 시나리오를 위한 adaptive pool 생성
auto pool = std::make_shared<thread_pool>("AdaptivePool");

// 최대 처리량을 위한 worker 구성
std::vector<std::unique_ptr<thread_worker>> workers;
for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
    auto worker = std::make_unique<thread_worker>();

    // 더 나은 처리량을 위한 배치 처리 활성화
    

    workers.push_back(std::move(worker));
}
pool->enqueue_batch(std::move(workers));
pool->start();

// 여러 thread에서 job 제출 (높은 경합)
// Adaptive queue는 유익할 때 자동으로 lock-free 모드로 전환
std::vector<std::thread> producers;
for (int t = 0; t < 8; ++t) {
    producers.emplace_back([&pool, t]() {
        for (int i = 0; i < 10000; ++i) {
            pool->enqueue(std::make_unique<callback_job>(
                [t, i]() -> result_void {
                    // 빠른 job 실행
                    std::atomic<int> sum{0};
                    for (int j = 0; j < 100; ++j) {
                        sum.fetch_add(j);
                    }
                    return {};
                }
            ));
        }
    });
}

// 모든 producer 대기
for (auto& t : producers) {
    t.join();
}

// 상세 통계 가져오기
auto workers_vec = pool->get_workers();
for (size_t i = 0; i < workers_vec.size(); ++i) {
    auto stats = static_cast<thread_worker*>(
        workers_vec[i].get())->get_statistics();
    log_module::write_information(
        "Worker {}: {} jobs, {} μs avg, {} batch ops",
        i, stats.jobs_processed,
        stats.avg_processing_time_ns / 1000,
        stats.batch_operations
    );
}
```

#### 비동기 Logging
```cpp
#include "logger/core/logger.h"

// Logger 구성
log_module::set_title("MyApplication");
log_module::console_target(log_module::log_types::Information |
                          log_module::log_types::Error);
log_module::file_target(log_module::log_types::All);

// Logger 시작
log_module::start();

// 다양한 log 수준 사용
log_module::write_information("Application started");
log_module::write_debug("Debug mode enabled");
log_module::write_error("Example error: {}", error_code);
log_module::write_sequence("Processing step {}/{}", current, total);

// 중요 오류를 위한 사용자 정의 callback
log_module::callback_target(log_module::log_types::Exception);
log_module::message_callback(
    [](const log_module::log_types& type,
       const std::string& datetime,
       const std::string& message) {
        if (type == log_module::log_types::Exception) {
            send_alert_email(message);
        }
    }
);
```

#### 고성능 Adaptive Logging
```cpp
#include "logger/core/logger.h"

using namespace log_module;

// 고성능 시나리오를 위한 logger 구성
log_module::set_title("HighPerformanceApp");
log_module::console_target(log_types::Information);
log_module::file_target(log_types::Information);

// Logger 시작
log_module::start();

// 여러 thread에서 고빈도 logging
// Logger는 경합 패턴에 자동으로 적응
std::vector<std::thread> log_threads;
for (int t = 0; t < 16; ++t) {
    log_threads.emplace_back([t]() {
        for (int i = 0; i < 10000; ++i) {
            log_module::write_information(
                "Thread {} - High-frequency log message {}", t, i);
        }
    });
}

// 모든 thread 대기
for (auto& t : log_threads) {
    t.join();
}

// Adaptive logger는 뛰어난 성능을 제공합니다:
// - 경합을 기반으로 한 자동 최적화
// - 효율적인 다중 스레드 작업
// - 16 thread에서 최대 238% 더 나은 처리량
// - 높은 동시성 logging 시나리오에 이상적

log_module::stop();
```

#### 실시간 성능 Monitoring
```cpp
#include "monitoring/core/metrics_collector.h"
#include <kcenon/thread/core/thread_pool.h>

using namespace monitoring_module;
using namespace kcenon::thread;

// Monitoring system 시작
monitoring_config config;
config.collection_interval = std::chrono::milliseconds(100);  // 100ms 간격
metrics::start_global_monitoring(config);

// Thread pool 생성 및 monitoring
auto pool = std::make_shared<thread_pool>();
pool->start();

// Thread pool metric 등록
auto collector = global_metrics_collector::instance().get_collector();
auto pool_metrics = std::make_shared<thread_pool_metrics>();
collector->register_thread_pool_metrics(pool_metrics);

// Job 제출 및 실시간 monitoring
for (int i = 0; i < 1000; ++i) {
    pool->enqueue(std::make_unique<callback_job>([&pool_metrics]() -> result_void {
        // Metric 업데이트
        pool_metrics->jobs_completed.fetch_add(1);
        return {};
    }));
}

// 실시간 metric 가져오기
auto snapshot = metrics::get_current_metrics();
std::cout << "Jobs completed: " << snapshot.thread_pool.jobs_completed.load() << "\n";
std::cout << "Memory usage: " << snapshot.system.memory_usage_bytes.load() << " bytes\n";

// Monitoring 중지
metrics::stop_global_monitoring();
```

### 📚 **포괄적인 샘플 모음**

샘플은 실제 사용 패턴과 모범 사례를 보여줍니다:

#### **성능 및 동시성**
- **[Adaptive Thread Pool](https://github.com/kcenon/thread_system/tree/main/samples/thread_pool_sample)**: Adaptive queue 최적화를 지원하는 thread pool
- **[Typed Thread Pool](https://github.com/kcenon/thread_system/tree/main/samples/typed_thread_pool_sample)**: Adaptive type별 queue를 사용한 priority scheduling
- **[Adaptive MPMC Queue](https://github.com/kcenon/thread_system/tree/main/samples/mpmc_queue_sample)**: 핵심 adaptive 자료 구조 기본
- **[Hazard Pointers](https://github.com/kcenon/thread_system/tree/main/samples/hazard_pointer_sample)**: Lock-free 프로그래밍을 위한 안전한 메모리 회수
- **[Node Pool](https://github.com/kcenon/thread_system/tree/main/samples/node_pool_sample)**: Adaptive queue를 위한 메모리 pool 작업

#### **Thread Pool 기본**
- **[Basic Thread Pool](https://github.com/kcenon/thread_system/tree/main/samples/thread_pool_sample)**: Adaptive queue 최적화를 사용한 간단한 job 처리
- **[Typed Thread Pool](https://github.com/kcenon/thread_system/tree/main/samples/typed_thread_pool_sample)**: Adaptive queue를 사용한 priority 기반 작업 scheduling
- **[Custom Job Types](https://github.com/kcenon/thread_system/tree/main/samples/typed_thread_pool_sample_2)**: 도메인별 type으로 framework 확장

#### **Monitoring 및 진단**
- **[Real-time Monitoring](https://github.com/kcenon/thread_system/tree/main/samples/monitoring_sample)**: 라이브 성능 metric 및 시스템 monitoring
- **[Asynchronous Logging](https://github.com/kcenon/thread_system/tree/main/samples/logger_sample)**: 고성능 다중 대상 logging system

### 🛠️ **빌드 및 통합**

#### 전제 조건
- CMake 3.20 이상
- C++20 지원 컴파일러 (GCC 13+, Clang 17+, MSVC 2022+)
- **[common_system](https://github.com/kcenon/common_system)**: 필수 의존성 (thread_system과 같은 위치에 복제 필요)
- vcpkg 패키지 관리자 (의존성 스크립트에 의해 자동 설치)

#### 빌드 단계

```bash
# 저장소 복제 (common_system 필수)
git clone https://github.com/kcenon/common_system.git
git clone https://github.com/kcenon/thread_system.git
cd thread_system

# vcpkg를 통해 의존성 설치
./scripts/dependency.sh  # Linux/macOS
./scripts/dependency.bat # Windows

# 프로젝트 빌드
./scripts/build.sh       # Linux/macOS
./scripts/build.bat      # Windows

# 샘플 실행
./build/bin/thread_pool_sample
./build/bin/typed_thread_pool_sample
./build/bin/logger_sample

# 테스트 실행 (Linux/Windows만, macOS에서는 비활성화)
cd build && ctest --verbose
```

#### CMake 통합

```cmake
# 하위 디렉토리로 사용
add_subdirectory(thread_system)
target_link_libraries(your_target PRIVATE
    thread_base
    thread_pool
    typed_thread_pool
    utilities
)

# Optional: 필요한 경우 logger 및 monitoring 추가
# add_subdirectory(logger)      # 별도 프로젝트
# add_subdirectory(monitoring)  # 별도 프로젝트
# target_link_libraries(your_target PRIVATE logger monitoring)

# FetchContent 사용
include(FetchContent)
FetchContent_Declare(
    thread_system
    GIT_REPOSITORY https://github.com/kcenon/thread_system.git
    GIT_TAG main
)
FetchContent_MakeAvailable(thread_system)
```

## API 문서

### Core API Reference

- **[API Reference](./docs/API_REFERENCE.md)**: Interface를 포함한 완전한 API 문서
- **[Architecture Guide](./docs/ARCHITECTURE.md)**: 시스템 디자인 및 내부
- **[Performance Guide](./docs/PERFORMANCE.md)**: 최적화 팁 및 벤치마크
- **[User Guide](./docs/USER_GUIDE.md)**: 사용 가이드 및 예제
- **[FAQ](./docs/FAQ.md)**: 자주 묻는 질문

### 빠른 API 개요

```cpp
// Thread Pool API
namespace thread_pool_module {
    // Adaptive queue 지원이 있는 thread pool
    class thread_pool {
        auto start() -> result_void;
        auto stop(bool immediately = false) -> result_void;
        auto enqueue(std::unique_ptr<job>&& job) -> result_void;
        auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> result_void;
        auto get_workers() const -> const std::vector<std::shared_ptr<thread_worker>>&;
        auto get_queue_statistics() const -> queue_statistics;
    };

    // Adaptive 기능이 있는 thread worker
    class thread_worker : public thread_base {
        struct worker_statistics {
            uint64_t jobs_processed;
            uint64_t total_processing_time_ns;
            uint64_t batch_operations;
            uint64_t avg_processing_time_ns;
        };

        auto set_batch_processing(bool enabled, size_t batch_size = 32) -> void;
        auto get_statistics() const -> worker_statistics;
    };
}

// Typed Thread Pool API (Mutex 기반)
namespace typed_thread_pool_module {
    template<typename T>
    class typed_thread_pool_t {
        auto start() -> result_void;
        auto stop(bool clear_queue = false) -> result_void;
        auto enqueue(std::unique_ptr<typed_job_t<T>>&& job) -> result_void;
        auto enqueue_batch(std::vector<std::unique_ptr<typed_job_t<T>>>&& jobs) -> result_void;
    };

    // Aging Priority Queue (policy_queue 기반)
    template<typename T>
    class aging_typed_job_queue_t {
        auto enqueue(std::unique_ptr<typed_job_t<T>>&& job) -> result_void;
        auto dequeue() -> result<std::unique_ptr<job>>;
        auto dequeue(const T& type) -> result<std::unique_ptr<typed_job_t<T>>>;
        auto size() const -> std::size_t;
        auto empty() const -> bool;
        auto get_typed_statistics() const -> typed_queue_statistics_t<T>;
    };
}

// 선택적 외부 API (별도 프로젝트로 제공):
// - Logger API: https://github.com/kcenon/logger 참조
// - Monitoring API: https://github.com/kcenon/monitoring 참조
```

## 기여

기여를 환영합니다! 자세한 내용은 [Contributing Guide](./docs/CONTRIBUTING.md)를 참조하세요.

### 개발 설정

1. 저장소 Fork
2. Feature branch 생성 (`git checkout -b feature/amazing-feature`)
3. 변경 사항 커밋 (`git commit -m 'Add some amazing feature'`)
4. Branch에 Push (`git push origin feature/amazing-feature`)
5. Pull Request 열기

### 코드 스타일

- 최신 C++ 모범 사례 준수
- RAII 및 smart pointer 사용
- 일관된 포맷 유지 (clang-format 구성 제공)
- 새로운 기능에 대한 포괄적인 단위 테스트 작성

## 지원

- **이슈**: [GitHub Issues](https://github.com/kcenon/thread_system/issues)
- **토론**: [GitHub Discussions](https://github.com/kcenon/thread_system/discussions)
- **이메일**: kcenon@naver.com

## 품질 및 아키텍처

thread_system은 포괄적인 품질 보증 및 성능 최적화를 통해 고품질의 동시 프로그래밍 기능을 제공합니다.

### 빌드 및 테스트 인프라

**다중 플랫폼 Continuous Integration**
- 자동화된 sanitizer 빌드 (ThreadSanitizer, AddressSanitizer, UBSanitizer)
- 크로스 플랫폼 테스트: Ubuntu (GCC/Clang), Windows (MSYS2/VS), macOS
- Baseline metric 추적을 통한 성능 회귀 임계값
- Codecov 통합을 통한 코드 적용 범위 추적 (~70% 적용 범위)
- Clang-tidy 및 cppcheck를 사용한 정적 분석

**성능 Baseline**
- Standard Pool: 1.16M job/초 (프로덕션에서 검증됨)
- Typed Pool: 1.24M job/초 (priority scheduling으로 6% 더 빠름)
- P50 지연 시간: 0.8 μs (마이크로초 이하 job scheduling)
- 메모리 baseline: 2 MB (최소 오버헤드)
- 회귀 감지가 포함된 포괄적인 [BASELINE.md](BASELINE.md)

### Thread Safety 및 동시성

**프로덕션에서 검증된 Thread Safety**
- 모든 동시 시나리오를 다루는 70개 이상의 thread safety 테스트
- 모든 구성 요소에서 검증된 ThreadSanitizer 규정 준수
- 프로덕션 사용에서 데이터 경쟁 경고 없음
- 자동 경합 최적화를 지원하는 adaptive queue 전략
- Service registry 및 cancellation token edge case 검증

**동시성 기능**
- Lock-free 및 mutex 기반 adaptive queue (자동 선택)
- Hazard pointer를 통한 안전한 메모리 회수
- 경합 처리를 위한 지능형 backoff 전략
- 구성 가능한 크기의 worker별 배치 처리

### 리소스 관리 (RAII - Grade A)

**완벽한 RAII 준수**
- 100% smart pointer 사용 (std::unique_ptr, std::shared_ptr)
- 프로덕션 코드에서 수동 메모리 관리 없음
- Adaptive queue에 최적화된 메모리 pool 패턴
- 전체에 걸쳐 강력한 exception safety 보장

**검증**
- AddressSanitizer 검증: 모든 테스트가 메모리 누수 없이 통과
- 모든 오류 경로에서 검증된 리소스 정리
- 자동 worker 생명 주기 관리
- Exception-safe job queue 작업

### 오류 처리 (개발 중 - 95% 완료)

thread_system은 Rust의 Result 또는 C++23의 expected와 유사한 Result<T> 패턴을 사용하여 모든 핵심 API에서 type-safe 오류 처리를 제공합니다.

**Core API 표준화**
모든 핵심 API는 포괄적인 오류 보고를 위해 `result_void`를 반환합니다:
- `start()`, `stop()`, `enqueue()`, `enqueue_batch()` → `result_void`
- `execute()`, `shutdown()` → `result_void`

**오류 코드 통합**
- Thread system 오류 코드: -100~-199 (common_system에 할당됨)
  - 시스템 통합: -100~-109
  - Pool 생명 주기: -110~-119
  - Job 제출: -120~-129
  - Worker 관리: -130~-139
- common_system을 통한 중앙 집중식 오류 코드 registry
- 잘못된 인수, 상태 전환 및 리소스 고갈 적용 범위가 포함된 포괄적인 오류 테스트 suite

**이중 API 디자인**
```cpp
// 프로덕션 시스템을 위한 상세 오류 처리
auto result = pool->start();
if (result.has_error()) {
    const auto& err = result.get_error();
    std::cerr << "Failed to start pool: " << err.message()
              << " (code: " << static_cast<int>(err.code()) << ")\n";
    return;
}

// 간단한 사용 사례를 위한 편의 wrapper
if (!pool->submit_task([]() { do_work(); })) {
    std::cerr << "Failed to submit task\n";
}
```

**이점**
- Exception 오버헤드 없는 명시적 오류 처리
- 계층화된 API는 상세 검사와 간단한 성공/실패 확인을 모두 허용
- 신속한 개발을 위한 편의 wrapper (`submit_task`, `shutdown_pool`)
- 포괄적인 테스트 적용 범위

자세한 구현 참고 사항은 [PHASE_3_PREPARATION.md](docs/PHASE_3_PREPARATION.md)를 참조하세요.

## 라이선스

이 프로젝트는 BSD 3-Clause License에 따라 라이선스가 부여됩니다. 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.

## 감사의 말

- 최신 동시 프로그래밍 패턴과 모범 사례에서 영감을 받았습니다
- 관리자: kcenon@naver.com

---

<p align="center">
  Made with ❤️ by 🍀☀🌕🌥 🌊
</p>
