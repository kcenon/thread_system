# Thread System - 새로운 디렉토리 구조

## 변경 일자
2025-09-06

## 개요
프로젝트의 모듈화와 유지보수성 향상을 위한 디렉토리 구조 재정리 계획입니다.

## 현재 구조 vs 목표 구조

### 현재 구조
```
thread_system/
├── sources/                    # 실제 소스 코드
│   ├── thread_base/            # 기본 스레드 기능
│   ├── thread_pool/            # 스레드 풀
│   ├── typed_thread_pool/      # 타입 기반 스레드 풀
│   ├── interfaces/             # 인터페이스 (문서화되지 않음)
│   └── utilities/              # 유틸리티
├── modular_structure/          # 별도 구조 (사용되지 않음)
├── samples/                    # 예제
├── unittest/                   # 단위 테스트
├── benchmarks/                 # 벤치마크
└── docs/                       # 문서
```

### 목표 구조
```
thread_system/
├── core/                       # 핵심 모듈
│   ├── base/                   # thread_base 관련
│   │   ├── include/            # 공개 헤더
│   │   │   ├── thread_base.h
│   │   │   ├── thread_conditions.h
│   │   │   └── service_registry.h
│   │   └── src/                # 구현 파일
│   │       ├── thread_base.cpp
│   │       └── service_registry.cpp
│   ├── jobs/                   # job 시스템
│   │   ├── include/
│   │   │   ├── job.h
│   │   │   ├── job_queue.h
│   │   │   └── callback_job.h
│   │   └── src/
│   │       ├── job.cpp
│   │       ├── job_queue.cpp
│   │       └── callback_job.cpp
│   └── sync/                   # 동기화 프리미티브
│       ├── include/
│       │   ├── sync_primitives.h
│       │   ├── cancellation_token.h
│       │   └── error_handling.h
│       └── src/
│           └── sync_primitives.cpp
│
├── interfaces/                 # 공개 인터페이스
│   ├── executor_interface.h
│   ├── scheduler_interface.h
│   ├── monitorable_interface.h
│   ├── logger_interface.h
│   ├── monitoring_interface.h
│   └── thread_context.h
│
├── implementations/            # 구현체
│   ├── thread_pool/
│   │   ├── include/
│   │   │   ├── thread_pool.h
│   │   │   └── thread_worker.h
│   │   └── src/
│   │       ├── thread_pool.cpp
│   │       └── thread_worker.cpp
│   ├── typed_thread_pool/
│   │   ├── include/
│   │   │   ├── typed_thread_pool.h
│   │   │   ├── typed_job_queue.h
│   │   │   └── typed_thread_worker.h
│   │   └── src/
│   │       └── typed_thread_pool.cpp
│   └── lockfree/
│       ├── include/
│       │   ├── lockfree_job_queue.h
│       │   ├── adaptive_job_queue.h
│       │   └── hazard_pointer.h
│       └── src/
│           ├── lockfree_job_queue.cpp
│           └── hazard_pointer.cpp
│
├── utilities/                  # 유틸리티
│   ├── include/
│   │   ├── formatter.h
│   │   ├── convert_string.h
│   │   └── span.h
│   └── src/
│       └── convert_string.cpp
│
├── tests/                      # 테스트 (이름 변경)
│   ├── unit/                   # 단위 테스트
│   ├── integration/            # 통합 테스트
│   └── performance/            # 성능 테스트
│
├── benchmarks/                 # 벤치마크
├── samples/                    # 예제
├── docs/                       # 문서
└── cmake/                      # CMake 모듈
```

## 이동 매핑 테이블

| 현재 위치 | 목표 위치 | 작업 |
|-----------|-----------|------|
| sources/thread_base/core/thread_base.h | core/base/include/thread_base.h | 이동 |
| sources/thread_base/core/thread_base.cpp | core/base/src/thread_base.cpp | 이동 |
| sources/thread_base/jobs/*.h | core/jobs/include/ | 이동 |
| sources/thread_base/jobs/*.cpp | core/jobs/src/ | 이동 |
| sources/thread_base/sync/*.h | core/sync/include/ | 이동 |
| sources/thread_base/sync/*.cpp | core/sync/src/ | 이동 |
| sources/thread_pool/* | implementations/thread_pool/ | 이동 |
| sources/typed_thread_pool/* | implementations/typed_thread_pool/ | 이동 |
| sources/thread_base/lockfree/* | implementations/lockfree/ | 이동 |
| sources/interfaces/* | interfaces/ | 이동 및 정리 |
| sources/utilities/* | utilities/ | 이동 |
| unittest/* | tests/unit/ | 이동 |
| modular_structure/* | - | 삭제 |

## 주요 변경 사항

### 1. 명확한 계층 구조
- **core**: 핵심 기능만 포함
- **interfaces**: 모든 공개 인터페이스 중앙 집중
- **implementations**: 구체적인 구현체 분리
- **utilities**: 독립적인 유틸리티 함수

### 2. include/src 분리
- 각 모듈은 include와 src 디렉토리로 분리
- 공개 API와 구현 세부사항의 명확한 구분
- 컴파일 시간 최적화

### 3. 테스트 구조 개선
- unittest → tests로 이름 변경
- unit, integration, performance로 세분화
- 각 테스트 유형별 명확한 구분

### 4. 불필요한 디렉토리 제거
- modular_structure 디렉토리 제거
- 중복된 구조 정리

## 의존성 규칙

1. **utilities**: 의존성 없음
2. **core/base**: utilities만 의존
3. **core/jobs**: core/base, utilities 의존
4. **core/sync**: core/base, utilities 의존
5. **implementations**: core, interfaces 의존
6. **interfaces**: core/base만 의존

## 빌드 시스템 영향

### CMake 수정 필요 항목
1. 메인 CMakeLists.txt의 subdirectory 경로
2. 각 모듈별 CMakeLists.txt 생성
3. include 경로 업데이트
4. 타겟 이름 표준화

### 예상 빌드 구조
```cmake
# 각 모듈을 별도 라이브러리로 빌드
add_library(thread_system_core STATIC)
add_library(thread_system_thread_pool STATIC)
add_library(thread_system_typed_thread_pool STATIC)
add_library(thread_system_lockfree STATIC)
add_library(thread_system_utilities STATIC)
```

## 마이그레이션 체크리스트

- [ ] 백업 생성
- [ ] 새 디렉토리 구조 생성
- [ ] 파일 이동
- [ ] include 경로 수정
- [ ] CMakeLists.txt 업데이트
- [ ] 빌드 테스트
- [ ] 테스트 실행
- [ ] 샘플 실행 확인
- [ ] 문서 업데이트

## 리스크 및 대응

### 리스크 1: include 경로 깨짐
**대응**: 모든 include 문을 체계적으로 검색하고 수정

### 리스크 2: 빌드 실패
**대응**: 단계별 이동 및 각 단계마다 빌드 테스트

### 리스크 3: 외부 프로젝트 호환성
**대응**: 심볼릭 링크 또는 호환성 헤더 제공

## 예상 효과

1. **개선된 모듈성**: 각 컴포넌트의 명확한 경계
2. **빠른 컴파일**: 의존성 최소화로 인한 컴파일 시간 단축
3. **쉬운 유지보수**: 직관적인 구조로 코드 탐색 용이
4. **확장성**: 새로운 구현체 추가 용이
5. **테스트 가능성**: 모듈별 독립적 테스트 가능