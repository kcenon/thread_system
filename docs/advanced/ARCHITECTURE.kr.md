---
doc_id: "THR-ARCH-004"
doc_title: "Threading Ecosystem 아키텍처"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "thread_system"
category: "ARCH"
---

# Threading Ecosystem 아키텍처

> **Language:** [English](ARCHITECTURE.md) | **한국어**

## 목차

- [🏗️ 생태계 개요](#-생태계-개요)
- [📋 프로젝트 역할 및 책임](#-프로젝트-역할-및-책임)
  - [1. thread_system (Foundation)](#1-thread_system-foundation)
  - [2. logger_system (Logging)](#2-logger_system-logging)
  - [3. monitoring_system (Metrics)](#3-monitoring_system-metrics)
  - [4. integrated_thread_system (Integration Hub)](#4-integrated_thread_system-integration-hub)
- [🔄 의존성 흐름 및 Interface Contract](#-의존성-흐름-및-interface-contract)
- [📁 디렉토리 구조 (개요)](#-디렉토리-구조-개요)
- [🚀 최근 아키텍처 하이라이트](#-최근-아키텍처-하이라이트)
  - [향상된 동기화 Primitive 🆕](#향상된-동기화-primitive-)
  - [개선된 Cancellation 지원 🆕](#개선된-cancellation-지원-)
  - [Service Registry 패턴 🆕](#service-registry-패턴-)
  - [Adaptive Job Queue](#adaptive-job-queue)
  - [Interface 기반 통합](#interface-기반-통합)
  - [탁월한 에러 처리](#탁월한-에러-처리)
  - [Typed Thread Pool](#typed-thread-pool)

modular threading 생태계와 프로젝트 간 관계에 대한 포괄적인 개요입니다.

## 🏗️ 생태계 개요

Threading 생태계는 완전하고 고성능의 동시 프로그래밍 솔루션을 제공하도록 설계된 4개의 상호 연결된 프로젝트로 구성됩니다:

```
                    ┌─────────────────────────────┐
                    │   Application Layer         │
                    │                             │
                    │   Your Production Apps      │
                    └─────────────┬───────────────┘
                                  │
                    ┌─────────────▼───────────────┐
                    │ integrated_thread_system    │
                    │ (Integration Hub)           │
                    │                             │
                    │ • Complete Examples         │
                    │ • Integration Tests         │
                    │ • Best Practices           │
                    │ • Migration Guides         │
                    └─────────────┬───────────────┘
                                  │ uses all
        ┌─────────────────────────┼─────────────────────────┐
        │                         │                         │
        ▼                         ▼                         ▼
┌───────────────┐     ┌───────────────┐     ┌─────────────────┐
│ thread_system │────▶│ logger_system │     │monitoring_system│
│   (Core)      │     │ (Logging)     │     │  (Metrics)      │
│               │     │               │     │                 │
│ Foundation    │     │ Implements    │     │ Implements      │
│ interfaces    │     │ logger_       │     │ monitoring_     │
│ and core      │     │ interface     │     │ interface       │
│ threading     │     │               │     │                 │
└───────────────┘     └───────────────┘     └─────────────────┘
```

## 📋 프로젝트 역할 및 책임

### 1. thread_system (Foundation)
**Repository**: https://github.com/kcenon/thread_system
**역할**: Core threading 프레임워크 및 interface 제공자
**코드 크기**: ~2,700줄 (coroutine 제거를 통해 8,700+에서 간소화됨)

책임:
- Interface 정의: `logger_interface`, `monitoring_interface`, `executor_interface`
- Core Threading: worker pool, job queue, thread 관리
- 동기화 Primitive: 향상된 wrapper 및 유틸리티
- Service 인프라: Dependency injection 및 service registry
- 크로스 플랫폼 지원: Windows, Linux, macOS

주요 Component:
```cpp
namespace thread_module {
    // Interfaces
    class logger_interface;           // logger_system에 의해 구현
    class monitoring_interface;       // monitoring_system에 의해 구현
    class executor_interface;         // Job 실행 contract

    // Core Threading
    class thread_pool;                // Main thread pool 구현
    class thread_worker;              // Worker thread 관리
    class job_queue;                  // Thread-safe job 분배
    class callback_job;               // Callback을 위한 job wrapper

    // Synchronization (NEW)
    class cancellation_token;         // 협력적 cancellation
    class scoped_lock_guard;          // timeout이 있는 RAII lock
    class condition_variable_wrapper; // 향상된 condition variable
    class service_registry;           // Dependency injection container

    // Adaptive Components
    class adaptive_job_queue;         // Dual-mode queue 최적화
    class hazard_pointer_manager;     // Lock-free 메모리 회수
}
```

의존성:
- 외부: 없음 (독립 실행형)
- 내부: Self-contained

---

### 2. logger_system (Logging)
**Repository**: https://github.com/kcenon/logger_system
**역할**: 고성능 비동기 로깅 구현

책임:
- `thread_module::logger_interface` 구현
- 높은 처리량의 비동기 로깅
- 여러 writer (console/file/custom)
- Thread-safe

---

### 3. monitoring_system (Metrics)
**Repository**: https://github.com/kcenon/monitoring_system
**역할**: 실시간 성능 모니터링 및 메트릭 수집

책임:
- `monitoring_interface::monitoring_interface` 구현
- System, thread pool, worker 메트릭
- 낮은 오버헤드 수집 및 ring buffer

---

### 4. integrated_thread_system (Integration Hub)
**Repository**: https://github.com/kcenon/integrated_thread_system
**역할**: 완전한 통합 예제 및 테스트 프레임워크

책임:
- 통합 예제 및 best practice
- 크로스 시스템 통합 테스트
- 마이그레이션 가이드

---

## 🔄 의존성 흐름 및 Interface Contract

Interface 계층:
```
thread_module::logger_interface
    ↑ implements
logger_module::logger

monitoring_interface::monitoring_interface
    ↑ implements
monitoring_module::monitoring
```

의존성 그래프:
```
┌─────────────────┐
│  thread_system  │ ← 외부 의존성 없음 (foundation)
└─────────┬───────┘
          │ provides interfaces
          ├─────────────────────┬─────────────────────┐
          ▼                     ▼                     ▼
┌─────────────────┐   ┌─────────────────┐   ┌─────────────────┐
│  logger_system  │   │monitoring_system│   │integrated_thread│
│                 │   │                 │   │    _system      │
│ depends on:     │   │ depends on:     │   │                 │
│ - thread_system │   │ - thread_system │   │ depends on:     │
│   (interfaces)  │   │   (interfaces)  │   │ - thread_system │
└─────────────────┘   └─────────────────┘   │ - logger_system │
                                            │ - monitoring_   │
                                            │   system        │
                                            └─────────────────┘
```

## 📁 디렉토리 구조 (개요)

모듈화 이후 프로젝트 레이아웃 (~2,700줄):

```
thread_system/
├── core/                          # Core threading foundation
│   ├── base/                      # Thread base, service registry
│   │   ├── include/
│   │   │   ├── thread_base.h
│   │   │   ├── service_registry.h  # 🆕 DI container
│   │   │   └── thread_conditions.h
│   │   └── src/
│   ├── jobs/                      # Job system
│   │   ├── include/
│   │   │   ├── job.h               # Cancellation 포함
│   │   │   ├── callback_job.h
│   │   │   └── job_queue.h
│   │   └── src/
│   └── sync/                      # Synchronization
│       ├── include/
│       │   ├── sync_primitives.h   # 🆕 Enhanced wrapper
│       │   ├── cancellation_token.h # 🆕 Cooperative cancellation
│       │   └── error_handling.h    # Result<T> pattern
│       └── src/
├── interfaces/                    # Public contract
│   ├── executor_interface.h
│   ├── scheduler_interface.h
│   ├── logger_interface.h
│   └── monitoring_interface.h
├── implementations/
│   ├── thread_pool/{include,src}
│   ├── typed_thread_pool/{include,src}
│   └── lockfree/{include,src}
├── utilities/{include,src}
├── tests/benchmarks/
├── samples/
├── docs/
└── cmake/
```

설계 규칙:
- core는 `include/` 아래의 public header와 `src/` 아래의 구현을 노출
- implementation은 core와 interface에 의존
- utilities는 독립 실행형; interface는 core/base에만 의존

---

## 🚀 최근 아키텍처 하이라이트

### 향상된 동기화 Primitive 🆕
- **`sync_primitives.h`**: 포괄적인 동기화 wrapper
  - `scoped_lock_guard`: timeout 지원이 있는 RAII
  - `condition_variable_wrapper`: Predicate 및 timeout
  - `atomic_flag_wrapper`: Wait/notify 작업
  - `shared_mutex_wrapper`: Reader-writer lock

### 개선된 Cancellation 지원 🆕
- **`cancellation_token`**: 협력적 cancellation 메커니즘
  - 계층적 cancellation을 위한 연결된 token 생성
  - Thread-safe callback 등록
  - 자동 signal 전파
  - cycle 방지를 위한 weak pointer 사용

### Service Registry 패턴 🆕
- **`service_registry`**: 경량 dependency injection
  - Type-safe service 등록/검색
  - shared_mutex로 thread-safe
  - 자동 lifetime 관리
  - Header-only 구현

### Adaptive Job Queue
- mutex 기반과 lock-free MPMC 전략 간 runtime 전환
- 경량 메트릭 사용 (지연 시간, 경합 비율, 작업 수)
- 워크로드 특성에 따른 자동 최적화
- 높은 경합에서 최대 7.7배 성능 향상

### Interface 기반 통합
- thread pool이 구현하는 `executor_interface` (`execute`, `shutdown`)
- job queue가 구현하는 `scheduler_interface` (enqueue/dequeue)
- pool/worker/system 메트릭을 제공하는 `monitoring_interface`
- 로깅을 pluggable하고 optional하게 유지하는 `logger_interface`

생태계 통합 참고 사항
- network_system은 `thread_integration_manager` 및 adapter를 통해 외부 thread pool과 통합; thread_system에 대한 하드 컴파일 시간 의존성 없음.

### 탁월한 에러 처리
- **`result<T>` pattern**: C++23 std::expected와 유사한 현대적 에러 처리
  - Type-safe 에러 코드
  - Monadic 작업 (map, and_then)
  - Zero-overhead 추상화
  - 명확한 에러 전파

### Typed Thread Pool
- lock-free/adaptive 변형이 있는 타입별 queue
- 이기종 워크로드를 위한 우선순위/타입 인식 스케줄링
- 모든 조건에서 99%+ 타입 정확도 유지

---

*Last Updated: 2025-10-20*
