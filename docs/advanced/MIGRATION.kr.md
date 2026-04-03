---
doc_id: "THR-MIGR-002"
doc_title: "Thread System 마이그레이션 가이드"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "thread_system"
category: "MIGR"
---

# Thread System 마이그레이션 가이드

> **Language:** [English](MIGRATION.md) | **한국어**

## 목차

- [v3.0.0 마이그레이션 (common_system)](#v300-마이그레이션-common_system)
- [개요](#개요)
- [마이그레이션 상태](#마이그레이션-상태)
  - [Phase 1: Interface 추출 및 정리 ✅ 완료](#phase-1-interface-추출-및-정리--완료)
  - [Phase 2: 새로운 Repository 구조 생성 ✅ 완료](#phase-2-새로운-repository-구조-생성--완료)
  - [Phase 3: Component 마이그레이션 ✅ 완료](#phase-3-component-마이그레이션--완료)
  - [Phase 4: 통합 테스트 ✅ 완료](#phase-4-통합-테스트--완료)
  - [2025-09 업데이트 (Phase 2–3)](#2025-09-업데이트-phase-23)
  - [Phase 5: 점진적 배포 🔄 대기 중](#phase-5-점진적-배포--대기-중)
- [Breaking Change](#breaking-change)
  - [API 변경사항](#api-변경사항)
  - [빌드 시스템 변경사항](#빌드-시스템-변경사항)
- [사용자를 위한 마이그레이션 지침](#사용자를-위한-마이그레이션-지침)
  - [현재 사용자 (Phase 1)](#현재-사용자-phase-1)
  - [향후 마이그레이션 (Phase 2-5)](#향후-마이그레이션-phase-2-5)
- [타임라인](#타임라인)
  - [현재 상태 (2025-09-13)](#현재-상태-2025-09-13)
  - [상세 상태 로그](#상세-상태-로그)

## v3.0.0 마이그레이션 (common_system)

**릴리스 날짜:** 2025-12-19

v3.0.0은 **common_system 전용** 공개 계약으로의 마이그레이션을 완료합니다. 이것은 **주요 변경사항(breaking change)** 릴리스입니다.

### 제거된 타입

| 레거시 타입 | 대체 타입 |
|-------------|-----------|
| `kcenon::thread::result<T>` | `kcenon::common::Result<T>` |
| `kcenon::thread::result_void` | `kcenon::common::VoidResult` |
| `kcenon::thread::error` | `kcenon::common::error_info` |
| `kcenon::thread::logger_interface` | `kcenon::common::interfaces::ILogger` |
| `kcenon::thread::log_level` | `kcenon::common::log_level` |
| `kcenon::thread::monitoring_interface` | `kcenon::common::interfaces::IMonitor` |
| `kcenon::thread::monitorable_interface` | `kcenon::common::interfaces::IMonitorable` |
| `kcenon::shared::*` | `kcenon::common::interfaces::IExecutor` |

### 빠른 마이그레이션

```cpp
// 에러 처리
// 이전:
#include <kcenon/thread/core/error_handling.h>
kcenon::thread::result<int> foo();

// 이후:
#include <kcenon/common/result.h>
kcenon::common::Result<int> foo();

// API 변경:
// .has_error() → .is_err()
// .get_error() → .error()
// .value() → .value() (변경 없음)
```

### 상세 마이그레이션 가이드

- [에러 시스템 마이그레이션 가이드](ERROR_SYSTEM_MIGRATION_GUIDE.md)
- [로거 인터페이스 마이그레이션 가이드](../guides/LOGGER_INTERFACE_MIGRATION_GUIDE.kr.md)

---

## 개요

이 문서는 thread_system의 monolithic 아키텍처에서 modular 생태계로의 마이그레이션을 추적합니다.

## 마이그레이션 상태

### Phase 1: Interface 추출 및 정리 ✅ 완료

**완료된 작업:**
- 기존 interface (`logger_interface.h`, `monitoring_interface.h`)가 적절히 격리되었는지 확인
- multi-pool monitoring을 지원하도록 `thread_context.h` 업데이트 (overloaded method 포함)
- `thread_pool.cpp` 및 `thread_worker.cpp`의 초기화 순서 경고 수정
- 올바른 API signature를 사용하도록 샘플 코드 업데이트
- `multi_process_monitoring_integration` 샘플의 namespace 충돌 수정
- 모든 테스트 성공적으로 통과

**주요 변경사항:**
1. `thread_context.h`에 overloaded `update_thread_pool_metrics` method 추가:
   ```cpp
   void update_thread_pool_metrics(const std::string& pool_name,
                                  std::uint32_t pool_instance_id,
                                  const monitoring_interface::thread_pool_metrics& metrics)
   ```

2. 다음에서 constructor 초기화 순서 수정:
   - `thread_pool.cpp`: member 선언 순서에 맞게 재정렬
   - `thread_worker.cpp`: member 선언 순서에 맞게 재정렬

3. 샘플 코드 업데이트:
   - `callback_job` constructor 매개변수 순서 수정 (callback 먼저, 그 다음 name)
   - 새로운 `thread_pool::start()` API 사용으로 업데이트 (worker count 매개변수 없음)
   - monitoring interface 타입에 대한 namespace 해결 수정

### Phase 2: 새로운 Repository 구조 생성 ✅ 완료

**완료된 작업:**
- `modular_structure/` 아래에 modular 디렉토리 구조 생성
- 적절한 export 구성으로 core module CMakeLists.txt 설정
- logger 및 monitoring module에 대한 통합 template 생성
- find_package 지원을 위한 CMake package 구성 준비
- optional module에 대한 통합 패턴 문서화

**새로운 구조:**
```
modular_structure/
├── core/                    # Core thread_system module
│   ├── CMakeLists.txt      # Main build configuration
│   ├── cmake/              # CMake config templates
│   ├── include/            # Public headers
│   └── src/                # Implementation files
└── optional/               # Integration templates
    ├── logger_integration/
    └── monitoring_integration/
```

**주요 기능:**
1. 외부 의존성이 없는 Core module (표준 라이브러리 제외)
2. 쉬운 통합을 위한 깨끗한 CMake export 구성
3. logger 및 monitoring에 대한 포괄적인 통합 가이드
4. target alias를 통한 backward compatibility 지원

### Phase 3: Component 마이그레이션 ✅ 완료

**완료된 작업:**
- ✅ 모든 core component를 modular 구조로 이동
- ✅ thread_system_core namespace를 사용하도록 모든 include path 업데이트
- ✅ 자동화된 스크립트로 모든 컴파일 에러 수정
- ✅ 독립 실행형 라이브러리로 core module 빌드 성공
- ✅ backward compatibility를 위한 호환성 header 생성

**주요 변경사항:**
1. 마이그레이션된 component:
   - `thread_base/` - Core threading 추상화
   - `thread_pool/` - 표준 thread pool 구현
   - `typed_thread_pool/` - 우선순위를 가진 type-safe thread pool
   - `utilities/` - 문자열 변환 및 형식 지정 유틸리티
   - `interfaces/` - Logger 및 monitoring interface

2. Include path 업데이트:
   - 모든 내부 include가 이제 `thread_system_core/` prefix 사용
   - include path 수정을 자동화하기 위한 Python 스크립트 생성
   - 잘못된 include path가 있는 60개 이상의 파일 수정

3. 빌드 시스템 개선:
   - Core module이 C++20 표준으로 빌드
   - 플랫폼별 지원 추가 (macOS용 iconv)
   - C++20 std::format 독점 사용 (fmt 라이브러리 제거됨)
   - 깨끗한 CMake export 구성

4. 호환성:
   - 원활한 마이그레이션을 위한 `.compat` header 생성
   - 원래 프로젝트는 변경 없이 여전히 빌드됨
   - 원본 및 modular 버전 모두에서 모든 테스트 통과

### Phase 4: 통합 테스트 ✅ 완료

**완료된 작업:**
- ✅ 포괄적인 통합 테스트 suite 생성
- ✅ 기본 thread pool, logger, monitoring, typed thread pool에 대한 테스트 구현
- ✅ 성능 벤치마크 생성
- ✅ core module이 독립적으로 컴파일 및 링크될 수 있는지 확인
- ✅ CMake config 생성과 관련된 통합 문제 식별

**주요 발견사항:**
1. Core module이 독립 실행형 라이브러리로 성공적으로 빌드됨
2. Job queue 및 job 실행이 격리 상태에서 올바르게 작동
3. CMake config 파일 생성에 문제 있음 (config 파일의 EOF)
4. Thread pool worker 초기화가 조정이 필요할 수 있음
5. API signature가 진화함 (callback_job이 result 타입 필요)

**생성된 테스트 파일:**
- `test_basic_thread_pool.cpp` - 기본 thread pool 기능
- `test_logger_integration.cpp` - 커스텀 logger 구현 테스트
- `test_monitoring_integration.cpp` - 커스텀 monitoring 구현 테스트
- `test_typed_thread_pool.cpp` - 우선순위 기반 thread pool 테스트
- `benchmark_thread_system.cpp` - 성능 벤치마크
- `simple_test.cpp` - 최소 통합 테스트
- `minimal_test.cpp` - 직접 job queue 테스트

---

### 2025-09 업데이트 (Phase 2–3)

프로젝트가 구조적 마이그레이션 및 문서화를 완료했습니다:

- core/, implementations/, interfaces/, utilities/ 아래의 새로운 소스 레이아웃
- module별 target 및 optional `docs` target (Doxygen)으로 CMake 업데이트
- public interface 추가: executor_interface, scheduler_interface, monitorable_interface
- job_queue가 scheduler_interface 구현; thread_pool 및 typed_thread_pool이 executor_interface 구현
- 추가된 문서:
  - docs/API_REFERENCE.md (interface를 포함한 완전한 API 문서)
  - docs/USER_GUIDE.md (빌드, 사용, 문서 생성)
  - core/, implementations/, interfaces/의 Module README

downstream 통합을 위한 작업 항목:
- 새로운 module header로 include path 업데이트
- 새로운 라이브러리 target에 링크 (thread_base, thread_pool, typed_thread_pool, lockfree, interfaces, utilities)
- `cmake --build build --target docs`를 통해 Doxygen 문서 생성 (Doxygen 필요)

**확인된 통합 패턴:**
- 커스텀 logger 구현이 thread_context와 작동
- 커스텀 monitoring 구현이 메트릭을 올바르게 캡처
- Job queue enqueue/dequeue 작업이 제대로 작동
- C++20 std::format 독점 사용 (외부 format 라이브러리 의존성 없음)

### Phase 5: 점진적 배포 🔄 대기 중

**계획된 작업:**
- 사용자를 위한 마이그레이션 가이드 생성
- alpha/beta 버전 릴리스
- 피드백 수집 및 반복
- 사용 중단 알림과 함께 최종 릴리스

## Breaking Change

### API 변경사항
1. `thread_pool::start()`가 더 이상 worker count 매개변수를 받지 않음
2. `callback_job` constructor가 이제 callback을 먼저, 그 다음 optional name을 받음
3. Namespace `monitoring_interface`가 동일한 이름의 namespace와 class를 모두 포함
4. API 일관성: `thread_pool` method가 이제 `std::optional<std::string>` 대신 `result_void` 반환
   - 업데이트된 signature:
     - `auto start() -> result_void`
     - `auto stop(bool immediately = false) -> result_void`
     - `auto enqueue(std::unique_ptr<job>&&) -> result_void`
     - `auto enqueue_batch(std::vector<std::unique_ptr<job>>&&) -> result_void`
   - `has_error()`를 통해 에러 확인 및 `get_error().to_string()`으로 검사

### 빌드 시스템 변경사항
- 향후 phase에서 별도의 module 의존성이 필요
- Include path가 내부에서 외부 module로 변경될 예정

## 사용자를 위한 마이그레이션 지침

### 현재 사용자 (Phase 1)
조치가 필요하지 않습니다. 모든 변경사항은 backward compatible합니다.

### 향후 마이그레이션 (Phase 2-5)
1. 별도의 module에 대해 find_package를 사용하도록 CMake 업데이트
2. logger 및 monitoring에 대한 include path 업데이트
3. monolithic thread_system 대신 별도의 라이브러리에 링크

## 타임라인

- Phase 1: ✅ 완료 (2025-01-27)
- Phase 2: ✅ 완료 (2025-01-27)
- Phase 3: ✅ 완료 (2025-01-27)
- Phase 4: ✅ 완료 (2025-01-27)
- Phase 5: 진행 중 - 예상 6주

총 예상 완료: 2025년 1분기
### 현재 상태 (2025-09-13)

마이그레이션이 완료되었으며 modular 구조와 pool 및 queue 전체에 통합된 interface가 갖춰졌습니다. 모든 문서가 현재 아키텍처를 반영하도록 업데이트되었습니다. 자세한 내용은 아래를 참조하세요.
### 상세 상태 로그

이전에 별도였던 상태 문서 (MIGRATION_STATUS.md)가 마이그레이션 지침과 현재 상태를 함께 유지하기 위해 이 섹션에 병합되었습니다.

---

*Last Updated: 2025-10-20*
