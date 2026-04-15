---
doc_id: "THR-PROJ-003"
doc_title: "Thread System 프로젝트 구조"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "thread_system"
category: "PROJ"
---

# Thread System 프로젝트 구조

> **SSOT**: This document is the single source of truth for **Thread System 프로젝트 구조**.

**버전**: 1.0.0
**최종 업데이트**: 2026-01-11
**언어**: [English](PROJECT_STRUCTURE.md) | **한국어**

---

## 개요

이 문서는 thread_system 프로젝트 구조에 대한 포괄적인 가이드를 제공하며, 모든 디렉토리, 파일 및 그 목적에 대한 상세 설명을 포함합니다.

---

## 목차

1. [디렉토리 구성](#디렉토리-구성)
2. [소스 코드 구조](#소스-코드-구조)
3. [공개 헤더](#공개-헤더)
4. [구현 파일](#구현-파일)
5. [예제](#예제)
6. [테스트](#테스트)
7. [문서](#문서)
8. [빌드 아티팩트](#빌드-아티팩트)
9. [모듈 의존성](#모듈-의존성)

---

## 디렉토리 구성

### 최상위 구조

```
thread_system/
├── 📁 include/kcenon/thread/       # 공개 API 헤더
├── 📁 src/                         # 구현 파일
├── 📁 examples/                    # 예제 애플리케이션
├── 📁 tests/                       # 테스트 스위트
├── 📁 docs/                        # 문서
├── 📁 cmake/                       # CMake 모듈
├── 📁 scripts/                     # 빌드 및 유틸리티 스크립트
├── 📄 CMakeLists.txt               # 루트 빌드 설정
├── 📄 vcpkg.json                   # 의존성 매니페스트
├── 📄 LICENSE                      # BSD 3-Clause 라이선스
├── 📄 README.md                    # 프로젝트 개요
└── 📄 README.kr.md                 # 한국어 문서
```

---

## 소스 코드 구조

### 전체 디렉토리 트리

```
thread_system/
├── 📁 include/kcenon/thread/       # 공개 헤더
│   ├── 📁 core/                    # 코어 컴포넌트
│   │   ├── thread_base.h           # 추상 스레드 클래스
│   │   ├── thread_pool.h           # 스레드 풀 인터페이스 (umbrella 헤더)
│   │   ├── thread_pool_impl.h      # 스레드 풀 템플릿 구현
│   │   ├── thread_pool_fmt.h       # 스레드 풀 std::formatter 특수화
│   │   ├── thread_worker.h         # 워커 스레드
│   │   ├── job.h                   # 작업 인터페이스
│   │   ├── callback_job.h          # 함수 기반 작업
│   │   ├── job_queue.h             # 스레드 안전 큐
│   │   ├── backpressure_job_queue.h # 백프레셔 지원 큐
│   │   ├── lockfree_job_queue.h    # 락프리 MPMC 큐
│   │   ├── adaptive_job_queue.h    # 적응형 듀얼 모드 큐
│   │   ├── hazard_pointer.h        # 메모리 회수
│   │   ├── node_pool.h             # 메모리 풀
│   │   ├── service_registry.h      # 의존성 주입
│   │   ├── cancellation_token.h    # 취소 지원
│   │   ├── sync_primitives.h       # 동기화 래퍼
│   │   ├── error_handling.h        # Result<T> 패턴
│   │   └── worker_policy.h         # 워커 설정
│   ├── 📁 interfaces/              # 통합 인터페이스
│   │   ├── logger_interface.h      # 로거 추상화
│   │   ├── monitoring_interface.h  # 모니터링 추상화
│   │   ├── thread_context.h        # 스레드 컨텍스트
│   │   └── service_container.h     # 서비스 관리
│   ├── 📁 utils/                   # 유틸리티
│   │   ├── formatter.h             # 문자열 포매팅
│   │   ├── convert_string.h        # 문자열 변환
│   │   └── span.h                  # Span 유틸리티
│   └── compatibility.h             # 하위 호환성
│
├── 📁 src/                         # 구현 파일
│   ├── 📁 core/                    # 코어 구현
│   │   ├── thread_base.cpp
│   │   ├── job.cpp
│   │   ├── callback_job.cpp
│   │   ├── job_queue.cpp
│   │   ├── backpressure_job_queue.cpp
│   │   ├── lockfree_job_queue.cpp
│   │   ├── adaptive_job_queue.cpp
│   │   ├── hazard_pointer.cpp
│   │   ├── node_pool.cpp
│   │   └── cancellation_token.cpp
│   ├── 📁 impl/                    # 구체적 구현
│   │   ├── 📁 thread_pool/
│   │   │   ├── thread_pool.cpp
│   │   │   └── thread_worker.cpp
│   │   └── 📁 typed_pool/
│   │       ├── typed_thread_pool.h
│   │       └── aging_typed_job_queue.h
│   ├── 📁 modules/                 # C++20 모듈 파일 (실험적)
│   │   ├── thread.cppm             # 주 모듈 인터페이스 (kcenon.thread)
│   │   ├── core.cppm               # 코어 파티션 (thread_pool, jobs)
│   │   └── queue.cppm              # 큐 파티션 (job_queue, adaptive)
│   └── 📁 utils/
│       └── convert_string.cpp
│
├── 📁 examples/                    # 예제 애플리케이션
│   ├── 📁 thread_pool_sample/
│   ├── 📁 typed_thread_pool_sample/
│   ├── 📁 adaptive_queue_sample/
│   ├── 📁 hazard_pointer_sample/
│   └── 📁 integration_example/
│
├── 📁 tests/                       # 모든 테스트
│   ├── 📁 unit/                    # 유닛 테스트
│   │   ├── 📁 thread_base_test/
│   │   ├── 📁 thread_pool_test/
│   │   ├── 📁 interfaces_test/
│   │   └── 📁 utilities_test/
│   └── 📁 benchmarks/              # 성능 테스트
│       ├── 📁 thread_base_benchmarks/
│       ├── 📁 thread_pool_benchmarks/
│       └── 📁 typed_thread_pool_benchmarks/
│
├── 📁 docs/                        # 문서
│   ├── 📁 advanced/
│   ├── 📁 guides/
│   ├── 📁 contributing/
│   ├── FEATURES.md
│   ├── BENCHMARKS.md
│   ├── PROJECT_STRUCTURE.md
│   └── PRODUCTION_QUALITY.md
│
├── 📁 cmake/                       # CMake 모듈
│   ├── CompilerWarnings.cmake
│   ├── Sanitizers.cmake
│   └── CodeCoverage.cmake
│
└── 📁 scripts/                     # 빌드 및 유틸리티 스크립트
    ├── build.sh
    ├── build.bat
    ├── dependency.sh
    ├── dependency.bat
    ├── format.sh
    └── test.sh
```

---

## 공개 헤더

### 코어 모듈 (`include/kcenon/thread/core/`)

#### thread_base.h

**목적**: 모든 스레드 연산을 위한 추상 베이스 클래스

**주요 컴포넌트**:
- `thread_base` 클래스: 기본 스레드 추상화
- 라이프사이클 관리 (시작/중지)
- 커스터마이즈 가능한 훅 (on_initialize, on_execute, on_destroy)
- 스레드 모니터링 및 상태 관리

**사용처**: 모든 워커 스레드, 스레드 풀

---

#### thread_pool.h

**목적**: 메인 스레드 풀 인터페이스

**주요 컴포넌트**:
- `thread_pool` 클래스: 멀티 워커 스레드 풀
- 작업 제출 API
- 워커 관리
- 적응형 큐 지원

**의존성**: thread_base, job_queue, thread_worker

---

#### job_queue.h

**목적**: 스레드 안전 FIFO 큐

**주요 컴포넌트**:
- `job_queue` 클래스: 뮤텍스 기반 큐
- `enqueue()` 및 `dequeue()` 메서드
- 블로킹을 위한 조건 변수

**사용처**: thread_pool, thread_worker

---

#### lockfree_job_queue.h

**목적**: 고성능 락프리 MPMC 큐

**주요 컴포넌트**:
- `lockfree_job_queue` 클래스: 락프리 큐
- 해저드 포인터 통합
- CAS 연산
- 노드 풀링

**성능**: 뮤텍스 기반 큐보다 4배 빠름

---

#### adaptive_job_queue.h

**목적**: 지능형 듀얼 모드 큐

**주요 컴포넌트**:
- `adaptive_job_queue` 클래스: 적응형 큐
- 자동 전략 선택
- 런타임 성능 모니터링
- 원활한 모드 전환

**최적 용도**: 가변 워크로드 패턴

---

#### hazard_pointer.h

**목적**: 락프리 구조체를 위한 안전한 메모리 회수

**주요 컴포넌트**:
- `hazard_pointer` 클래스: 메모리 회수
- `hazard_pointer_guard` RAII 래퍼
- 리타이어 리스트 관리
- 주기적 스캐닝

**사용처**: lockfree_job_queue, 락프리 데이터 구조

---

#### error_handling.h

**목적**: Result<T> 패턴을 이용한 현대적 오류 처리

**주요 컴포넌트**:
- `result<T>` 클래스: 성공 또는 오류
- `result_void`: Void 결과 타입
- `error` 클래스: 오류 정보
- 모나딕 연산 (and_then, or_else 등)

**사용처**: 모든 공개 API

---

### 인터페이스 모듈 (`include/kcenon/thread/interfaces/`)

#### logger_interface.h

**목적**: 통합을 위한 로거 추상화

**구현자**: 외부 logger_system 프로젝트

---

#### monitoring_interface.h

**목적**: 메트릭을 위한 모니터링 추상화

**구현자**: 외부 monitoring_system 프로젝트

---

## 구현 파일

### 코어 구현 (`src/core/`)

| 파일 | 목적 | 라인 수 | 복잡도 |
|------|---------|-------|------------|
| thread_base.cpp | 스레드 베이스 구현 | ~200 | 중간 |
| job.cpp | 작업 인터페이스 구현 | ~50 | 낮음 |
| callback_job.cpp | 콜백 작업 구현 | ~80 | 낮음 |
| job_queue.cpp | 뮤텍스 큐 구현 | ~150 | 중간 |
| backpressure_job_queue.cpp | 백프레셔 큐 구현 | ~350 | 중간 |
| lockfree_job_queue.cpp | 락프리 큐 구현 | ~400 | 높음 |
| adaptive_job_queue.cpp | 적응형 큐 구현 | ~300 | 높음 |
| hazard_pointer.cpp | 메모리 회수 구현 | ~350 | 높음 |
| node_pool.cpp | 메모리 풀 구현 | ~120 | 중간 |
| cancellation_token.cpp | 취소 구현 | ~100 | 낮음 |

**총 코어 라인 수**: ~2,000 라인

---

### 코드 통계

**총 프로덕션 코드**: ~3,600 라인

**모듈별 분석**:
- 코어: ~2,000 라인 (56%)
- 스레드 풀: ~550 라인 (15%)
- 타입드 풀: ~900 라인 (25%)
- 유틸리티: ~150 라인 (4%)

**코드 감소** (이전 버전 대비):
- 이전: ~11,400 라인
- 현재: ~2,700 라인 (인터페이스 기반 설계)
- **감소**: ~8,700 라인 (76%)

---

## 예제

### 예제 애플리케이션

#### thread_pool_sample

**목적**: 기본 스레드 풀 사용법 시연

**기능**:
- 워커 생성
- 작업 제출
- 적응형 큐 사용
- 성능 모니터링

**파일**: `examples/thread_pool_sample/main.cpp` (~150 라인)

---

#### typed_thread_pool_sample

**목적**: 우선순위 기반 작업 스케줄링

**기능**:
- 타입 기반 작업 라우팅
- 우선순위 스케줄링 (RealTime, Batch, Background)
- 타입별 큐 최적화
- 통계 추적

**파일**: `examples/typed_thread_pool_sample/main.cpp` (~200 라인)

---

#### hazard_pointer_sample

**목적**: 안전한 메모리 회수 시연

**기능**:
- 해저드 포인터 사용
- 락프리 데이터 구조 통합
- 메모리 안전성 검증

**파일**: `examples/hazard_pointer_sample/main.cpp` (~220 라인)

---

## 테스트

### 유닛 테스트 (`tests/unit/`)

#### thread_pool_test

**테스트**:
- 워커 관리
- 작업 제출
- 적응형 큐 동작
- 락프리 큐 정확성
- 배치 처리
- 종료 시나리오

**파일**: ~800 라인의 테스트

---

### 벤치마크 (`tests/benchmarks/`)

#### thread_pool_benchmarks

**측정**:
- 작업 처리량
- 큐 성능 (뮤텍스 vs 락프리 vs 적응형)
- 워커 스케일링
- 지연시간 분포

**파일**: ~500 라인

---

### 테스트 통계

**총 테스트 코드**: ~3,000 라인

**커버리지**: ~52% (codecov로 추적)

---

## 문서

### 설계 문서 (`docs/design/`)

| 파일 | 목적 | 라인 수 |
|------|---------|-------|
| QUEUE_POLICY_DESIGN.md | 큐 정책 인터페이스 설계 | ~450 |

---

### 고급 문서 (`docs/advanced/`)

| 파일 | 목적 | 라인 수 |
|------|---------|-------|
| ARCHITECTURE.md | 시스템 아키텍처 | ~800 |
| KNOWN_ISSUES.md | 알려진 이슈 | ~300 |

### 성능 문서 (`docs/performance/`)

| 파일 | 목적 | 라인 수 |
|------|---------|-------|
| BASELINE.md | 성능 기준선 | ~500 |

---

### 사용자 가이드 (`docs/guides/`)

| 파일 | 목적 | 라인 수 |
|------|---------|-------|
| API_REFERENCE.md | 완전한 API 문서 | ~1,200 |
| USER_GUIDE.md | 사용자 가이드 | ~600 |
| BUILD_GUIDE.md | 빌드 지침 | ~400 |
| QUICK_START.md | 빠른 시작 | ~300 |
| INTEGRATION.md | 통합 가이드 | ~500 |
| BEST_PRACTICES.md | 모범 사례 | ~400 |
| TROUBLESHOOTING.md | 문제 해결 | ~350 |
| FAQ.md | FAQ | ~250 |
| MIGRATION_GUIDE.md | 마이그레이션 가이드 | ~300 |

---

**총 문서**: ~9,050 라인

---

## 빌드 아티팩트

### 빌드 디렉토리 구조

```
build/
├── 📁 bin/                    # 실행 파일
│   ├── thread_pool_sample
│   ├── typed_thread_pool_sample
│   ├── adaptive_queue_sample
│   ├── hazard_pointer_sample
│   ├── integration_example
│   ├── thread_base_test
│   ├── thread_pool_test
│   ├── interfaces_test
│   ├── utilities_test
│   ├── thread_base_benchmark
│   ├── thread_pool_benchmark
│   └── typed_pool_benchmark
│
├── 📁 lib/                    # 정적 라이브러리
│   ├── libthread_base.a
│   ├── libthread_pool.a
│   ├── libtyped_thread_pool.a
│   └── libutilities.a
│
├── 📁 include/                # 공개 헤더 (설치용)
│   └── kcenon/
│       └── thread/
│
└── 📁 CMakeFiles/             # CMake 빌드 메타데이터
```

---

### 빌드 출력 크기

**바이너리 크기** (Release 빌드, stripped):

| 바이너리 | 크기 | 설명 |
|--------|------|-------------|
| thread_pool_sample | ~80 KB | 기본 예제 |
| typed_thread_pool_sample | ~95 KB | 타입드 풀 예제 |
| thread_base_test | ~120 KB | 유닛 테스트 |
| thread_pool_test | ~150 KB | 풀 테스트 |
| libthread_base.a | ~40 KB | 코어 라이브러리 |
| libthread_pool.a | ~50 KB | 풀 라이브러리 |
| libtyped_thread_pool.a | ~60 KB | 타입드 풀 라이브러리 |

**총 바이너리 크기**: ~600 KB

---

## 모듈 의존성

### 의존성 그래프

```
utilities (의존성 없음)
    │
    └──> thread_base
             │
             ├──> thread_pool
             │       │
             │       └──> thread_worker
             │
             └──> typed_thread_pool
                        │
                        └── aging_typed_job_queue (policy_queue 기반)
```

---

### 외부 의존성

**필수**:
- C++20 컴파일러 (GCC 13+, Clang 17+, MSVC 2022+)
- CMake 3.20+

**선택적** (vcpkg 통해):
- Google Test (유닛 테스트용)
- Google Benchmark (벤치마크용)

**선택적 통합 프로젝트**:
- logger_system (별도 저장소)
- monitoring_system (별도 저장소)
- integrated_thread_system (완전한 예제)

---

### 헤더 의존성

**thread_pool.h 의존성**:
- thread_base.h
- job_queue.h (또는 adaptive_job_queue.h)
- thread_worker.h
- error_handling.h

**lockfree_job_queue.h 의존성**:
- hazard_pointer.h
- node_pool.h
- error_handling.h

---

## 파일 명명 규칙

### 헤더 파일

- **인터페이스**: `*_interface.h`
- **템플릿**: `*.h` 및 구현용 `*.tpp`
- **코어 클래스**: `snake_case.h`
- **유틸리티**: 설명적 이름 (예: `formatter.h`)

### 구현 파일

- **클래스 구현**: 헤더 이름과 `.cpp` 확장자 일치
- **템플릿 구현**: `*.tpp` (헤더에 포함됨)

### 테스트

- **유닛 테스트**: `*_test.cpp`
- **벤치마크**: `*_benchmark.cpp`

### 예제

- **샘플**: `*_sample/main.cpp`

---

## 코드 구성 원칙

### 1. 인터페이스 기반 설계

모든 주요 컴포넌트는 추상 인터페이스를 제공합니다:
- `logger_interface.h`
- `monitoring_interface.h`
- `job` (추상 베이스)

**이점**:
- 깔끔한 관심사 분리
- 테스트를 위한 쉬운 모킹
- 플러거블 구현

---

### 2. 헤더 전용 vs 컴파일됨

**헤더 전용**:
- 템플릿 (typed_thread_pool.h)
- 유틸리티 (formatter.h)

**컴파일됨**:
- 코어 클래스 (thread_base.cpp)
- 무거운 구현 (lockfree_job_queue.cpp)

**근거**: 컴파일 시간과 유연성의 균형

---

### 3. 모듈형 아키텍처

각 모듈은 자체 완결적입니다:
- 명확한 공개 API (`include/`)
- 숨겨진 구현 세부사항 (`src/`)
- 최소한의 모듈 간 결합

---

## 요약

### 프로젝트 통계

| 카테고리 | 개수 | 라인 수 |
|----------|-------|-------|
| **소스 파일** | ~30 | ~3,600 |
| **헤더 파일** | ~35 | ~2,500 |
| **테스트 파일** | ~15 | ~3,000 |
| **예제 파일** | ~5 | ~1,050 |
| **문서** | ~20 | ~9,050 |
| **합계** | ~105 | ~19,200 |

---

### 주요 디렉토리

1. **`include/kcenon/thread/`**: 공개 API (이것 사용)
2. **`src/`**: 구현 세부사항 (직접 의존하지 말 것)
3. **`examples/`**: 예제로 학습
4. **`tests/`**: 정확성 보장
5. **`docs/`**: 포괄적인 문서

---

**참고 문서**:
- [기능 문서](FEATURES.md) / [기능 문서 (한국어)](FEATURES.kr.md)
- [성능 벤치마크](BENCHMARKS.md) / [성능 벤치마크 (한국어)](BENCHMARKS.kr.md)
- [아키텍처 가이드](advanced/ARCHITECTURE.md)
- [빌드 가이드](guides/BUILD_GUIDE.md)

---

**최종 업데이트**: 2026-01-11
**관리자**: kcenon@naver.com

---

Made with ❤️ by 🍀☀🌕🌥 🌊
