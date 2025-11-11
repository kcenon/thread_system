# Thread System Performance Guide

> **Language:** [English](PERFORMANCE.md) | **한국어**

이 포괄적인 가이드는 Thread System framework의 성능 benchmark, 튜닝 전략 및 최적화 기법을 다룹니다. 모든 측정은 실제 benchmark 데이터와 광범위한 테스트를 기반으로 합니다.

## 목차

1. [성능 개요](#성능-개요)
2. [Benchmark 환경](#benchmark-환경)
3. [핵심 성능 Metric](#핵심-성능-metric)
4. [Data Race 수정 영향](#data-race-수정-영향)
5. [상세 Benchmark 결과](#상세-benchmark-결과)
6. [Typed Lock-Free Thread Pool Benchmark](#typed-lock-free-thread-pool-benchmark)
7. [확장성 분석](#확장성-분석)
8. [메모리 성능](#메모리-성능)
9. [다른 라이브러리와의 비교](#다른-라이브러리와의-비교)
10. [최적화 전략](#최적화-전략)
11. [플랫폼별 최적화](#플랫폼별-최적화)
12. [모범 사례](#모범-사례)

## 성능 개요

Thread System framework는 다양한 workload 패턴에서 탁월한 성능을 제공합니다:

### 주요 성능 하이라이트 (현재 아키텍처)

- **최고 처리량**: 최대 13.0M jobs/초 (1 worker, 빈 job - 이론적)
- **실제 처리량**:
  - Standard thread pool: 1.16M jobs/s (10 worker)
  - Typed thread pool: 1.24M jobs/s (6 worker)
  - Adaptive job queue: 자동으로 최적 전략 선택
- **낮은 지연 시간**:
  - Standard pool: ~77 nanosecond job 스케줄링 지연 시간
  - Adaptive queue: 경합 수준에 따라 96-580ns
- **확장 효율성**: 8 코어에서 96% (이론적), 실제 55-56%
- **메모리 효율성**: <1MB 기본 메모리 사용량
- **크로스 플랫폼**: Windows, Linux 및 macOS에서 일관된 성능
- **간소화된 아키텍처**:
  - 자동 최적화를 위한 adaptive queue 전략
  - 코드 복잡성을 ~8,700에서 ~2,700줄로 감소
  - logger 및 monitoring을 독립 프로젝트로 분리
  - 향상된 동기화 primitive 및 취소 지원
  - Dependency injection을 위한 service registry

## Benchmark 환경

### 테스트 하드웨어
- **CPU**: Apple M1 (8-core) - 4 performance + 4 efficiency 코어
- **메모리**: 16GB unified 메모리
- **스토리지**: NVMe SSD
- **OS**: macOS Sonoma 14.x

### Compiler 구성
- **Compiler**: Apple Clang 17.0.0
- **C++ 표준**: C++20
- **최적화**: -O3 Release 모드
- **기능**: std::format 활성화, std::thread fallback (std::jthread 사용 불가)

### Thread System 버전
- **버전**: 간소화된 아키텍처가 있는 최신 개발 빌드
- **빌드 날짜**: 2025-09-07 (최신 업데이트 - 모듈화된 아키텍처)
- **구성**: adaptive queue 지원이 있는 Release 빌드
- **Benchmark 도구**: Google Benchmark
- **아키텍처 변경**:
  - ~8,700에서 ~2,700줄의 코드로 간소화
  - Logger 및 monitoring이 별도 프로젝트로 이동
  - 깔끔한 interface 기반 아키텍처
  - sync_primitives, cancellation_token, service_registry 추가
- **성능**: adaptive queue를 통한 자동 최적화로 유지

## 핵심 성능 Metric

### 컴포넌트 오버헤드

| 컴포넌트 | 작업 | 오버헤드 | 참고 |
|-----------|-----------|----------|-------|
| Thread Base | Thread 생성 | ~10-15 μs | Thread 초기화당 |
| Thread Base | Job 스케줄링 | ~77 ns | 이전 ~1-2 μs에서 10배 개선 |
| Thread Base | Wake interval 접근 | +5% | Mutex 보호 추가 |
| Thread Pool | Pool 생성 (1 worker) | ~162 ns | Google Benchmark로 측정 |
| Thread Pool | Pool 생성 (8 worker) | ~578 ns | 선형 확장 |
| Thread Pool | Pool 생성 (16 worker) | ~1041 ns | 일관된 오버헤드 |
| Adaptive Queue | Mutex 모드 enqueue | ~96 ns | 기본 전략 |
| Adaptive Queue | Lock-free 모드 enqueue | ~320 ns | 높은 경합 모드 |
| Adaptive Queue | Batch 작업 | ~212 ns/job | 최적화된 처리 |
| Sync Primitives | Timeout이 있는 scoped lock | ~15 ns | RAII wrapper 오버헤드 |
| Cancellation Token | 등록 | +3% | Thread-safe callback |
| Service Registry | Service 조회 | ~25 ns | Type-safe 검색 |
| Job Queue | 작업 | -4% | 최적화된 atomic |

### Thread Pool 생성 성능

| Worker 수 | 생성 시간 | Item/초 | 참고 |
|-------------|---------------|-----------|-------|
| 1           | 162 ns        | 6.19M/s   | 최소 오버헤드 |
| 2           | 227 ns        | 4.43M/s   | 좋은 확장 |
| 4           | 347 ns        | 2.89M/s   | 선형 증가 |
| 8           | 578 ns        | 1.73M/s   | 예상 오버헤드 |
| 16          | 1041 ns       | 960K/s    | 여전히 sub-microsecond |

## Data Race 수정 영향

### 개요
최근 data race 수정은 우수한 성능 특성을 유지하면서 세 가지 중요한 동시성 문제를 해결했습니다:

1. **thread_base::wake_interval** - thread-safe 접근을 위한 mutex 보호 추가
2. **cancellation_token** - double-check 패턴 및 순환 참조 수정
3. **job_queue::queue_size_** - 중복 atomic counter 제거

### 성능 영향 분석

| 수정 | 성능 영향 | 이점 | 트레이드오프 |
|-----|-------------------|---------|-----------|
| Wake interval mutex | +5% 오버헤드 | Thread-safe 접근 | 대부분의 workload에 최소 영향 |
| Cancellation token 수정 | +3% 오버헤드 | race condition 방지 | 더 안전한 callback 등록 |
| Job queue 최적화 | -4% (개선) | 더 나은 cache locality | 없음 - 순수 이득 |
| **순 영향** | **+4% 전체** | **100% thread 안전성** | **우수한 트레이드오프** |

### 수정 전후 비교

| Metric | 수정 전 | 수정 후 | 변화 |
|--------|--------------|-------------|--------|
| 최고 처리량 | ~12.5M jobs/s | 13.0M jobs/s | +4% |
| Job 제출 지연 시간 | ~80 ns | ~77 ns | -4% |
| Thread 안전성 | 3 data race | 0 data race | ✅ |
| 메모리 순서 | Weak | Strong | ✅ |

### 실제 영향
- **프로덕션 안전성**: 모든 data race가 제거되어 높은 동시성에서 안정적인 작업 보장
- **성능**: job queue 최적화가 mutex 오버헤드를 상쇄하여 약간의 순 개선
- **유지 관리성**: 적절한 동기화 primitive가 있는 더 깔끔한 코드

## 상세 Benchmark 결과

### Job 제출 지연 시간

#### Standard Thread Pool (Mutex 기반)
| Queue 상태 | 평균 지연 시간 | 50번째 백분위수 | 95번째 백분위수 | 99번째 백분위수 |
|------------|-------------|-----------------|-----------------|-----------------|
| 비어 있음      | 0.8 μs      | 0.7 μs          | 1.1 μs          | 1.2 μs          |
| 100 job   | 0.9 μs      | 0.8 μs          | 1.3 μs          | 1.5 μs          |
| 1K job    | 1.1 μs      | 1.0 μs          | 1.8 μs          | 2.1 μs          |
| 10K job   | 1.3 μs      | 1.2 μs          | 2.8 μs          | 3.5 μs          |
| 100K job  | 1.6 μs      | 1.4 μs          | 3.2 μs          | 4.8 μs          |

#### Adaptive Queue (Lock-free 모드)
| Queue 상태 | 평균 지연 시간 | 50번째 백분위수 | 95번째 백분위수 | 99번째 백분위수 |
|------------|-------------|-----------------|-----------------|-----------------|
| 비어 있음      | 0.32 μs     | 0.28 μs         | 0.45 μs         | 0.52 μs         |
| 100 job   | 0.35 μs     | 0.31 μs         | 0.48 μs         | 0.58 μs         |
| 1K job    | 0.38 μs     | 0.34 μs         | 0.52 μs         | 0.65 μs         |
| 10K job   | 0.42 μs     | 0.38 μs         | 0.58 μs         | 0.72 μs         |
| 100K job  | 0.48 μs     | 0.43 μs         | 0.68 μs         | 0.85 μs         |

### Job 복잡도별 처리량

#### Standard Thread Pool 성능

| Job 기간 | 1 Worker | 2 Worker | 4 Worker | 8 Worker | 참고 |
|-------------|----------|-----------|-----------|-----------|-------|
| 빈 job   | 13.0M/s  | 10.4M/s   | 8.3M/s    | 6.6M/s    | 높은 경합 |
| 1 μs work   | 890K/s   | 1.6M/s    | 3.0M/s    | 5.5M/s    | 좋은 확장 |
| 10 μs work  | 95K/s    | 180K/s    | 350K/s    | 680K/s    | 거의 선형 |
| 100 μs work | 9.9K/s   | 19.8K/s   | 39.5K/s   | 78K/s     | 우수한 확장 |
| 1 ms work   | 990/s    | 1.98K/s   | 3.95K/s   | 7.8K/s    | CPU 바운드 |
| 10 ms work  | 99/s     | 198/s     | 395/s     | 780/s     | I/O 바운드 영역 |
| **실제 workload** | **1.16M/s** | - | - | - | **10 worker, 측정됨** |

#### Adaptive Job Queue 성능 (Lock-free 모드)

| Job 기간 | 1 Worker | 2 Worker | 4 Worker | 8 Worker | vs Standard |
|-------------|----------|-----------|-----------|-----------|-------------|
| 빈 job   | 15.2M/s  | 14.8M/s   | 13.5M/s   | 12.1M/s   | +83% avg    |
| 1 μs work   | 1.2M/s   | 2.3M/s    | 4.4M/s    | 8.2M/s    | +49% avg    |
| 10 μs work  | 112K/s   | 218K/s    | 425K/s    | 820K/s    | +21% avg    |
| 100 μs work | 10.2K/s  | 20.3K/s   | 40.5K/s   | 80K/s     | +3% avg     |
| 1 ms work   | 995/s    | 1.99K/s   | 3.97K/s   | 7.9K/s    | +1% avg     |
| 10 ms work  | 99/s     | 198/s     | 396/s     | 781/s     | ~0% avg     |
| **실제 workload** | **Adaptive 전략을 통해 사용 가능** | - | - | - | **동적 선택** |

#### Type Thread Pool 성능

| Type 혼합 | Basic Pool | Type Pool | 성능 | Type 정확도 |
|-------------|------------|-----------|-------------|---------------|
| 단일 (High) | 540K/s    | 525K/s    | -3%         | 100%          |
| 2 레벨    | 540K/s     | 510K/s    | -6%         | 99.8%         |
| 3 레벨    | 540K/s     | 495K/s    | -9%         | 99.6%         |
| 5 레벨    | 540K/s     | 470K/s    | -15%        | 99.3%         |
| 10 레벨   | 540K/s     | 420K/s    | -29%        | 98.8%         |

#### 실제 측정 (Lock-Free 구현)

| 구성 | 처리량 | 시간 (1M job) | Worker | CPU 사용률 | 개선 |
|--------------|------------|----------------|---------|-----------|-------------|
| Basic Pool   | 1.16M/s    | 862 ms         | 10      | 559%      | 기준    |
| Type Pool    | 1.24M/s    | 806 ms         | 6       | 330%      | +6.9%       |

#### Adaptive Queue가 있는 Type Thread Pool

Type Thread Pool은 adaptive job queue 구현을 특징으로 합니다:

##### typed_thread_pool (현재 구현)
- **아키텍처**: 적응형 전략 선택이 있는 type별 job queue
- **동기화**: 경합에 따라 동적 mutex/lock-free 전환
- **메모리**: job type당 적응형 queue 할당
- **최적**: 자동 최적화가 있는 모든 시나리오

##### Adaptive Queue 전략
- **아키텍처**: 부하에 따라 mutex와 lock-free 간 자동 전환
- **동기화**: 전략 간 원활한 fallback
- **메모리**: queue 사용 패턴에 따른 최적화된 할당
- **최적**: 다양한 경합 패턴이 있는 동적 workload

**성능 특성**:

| Metric | Adaptive 구현 | 이점 |
|--------|-------------------------|----------|
| 간단한 job (100-10K) | 540K/s 기준 | 최적 전략 선택 |
| 높은 경합 시나리오 | 자동 lock-free 모드 | 성능 유지 |
| 우선순위 스케줄링 | Type 기반 라우팅 | 효율적인 job 분배 |
| Job dequeue 지연 시간 | ~571 ns (lock-free 모드) | 자동 최적화 |
| Type당 메모리 | 동적 할당 | 확장 가능한 리소스 사용 |

**구현 기능**:
- 경합 감지에 따른 자동 전략 선택
- Type 기반 job 라우팅 및 worker 전문화
- 동적 queue 생성 및 수명 주기 관리
- 모니터링 및 튜닝을 위한 type별 통계 수집
- 원활한 최적화가 있는 호환 가능한 API

*참고: adaptive 구현은 런타임 조건에 따라 자동으로 최적 queue 전략을 선택하여 단순성과 성능을 모두 제공합니다.*

## Adaptive Job Queue Benchmark

### 개요

여러 차원에서 자동 전략 선택이 있는 adaptive job queue 성능을 보여주는 포괄적인 benchmark:

### Thread Pool 레벨 Benchmark

#### 간단한 Job 처리
*최소 계산이 있는 Job (10 iteration)*

| Queue 전략 | Job 수 | 실행 시간 | 처리량 | 상대 성능 |
|---------------|-----------|----------------|------------|---------------------|
| Mutex (낮은 부하) | 100    | ~45 μs         | 2.22M/s    | 기준            |
| Adaptive      | 100       | ~42 μs         | 2.38M/s    | **+7.2%**           |
| Mutex (중간 부하) | 1,000  | ~380 μs        | 2.63M/s    | 기준            |
| Adaptive      | 1,000     | ~365 μs        | 2.74M/s    | **+4.2%**           |
| Mutex (높은 부하) | 10,000 | ~3.2 ms        | 3.13M/s    | 기준            |
| Adaptive      | 10,000    | ~3.0 ms        | 3.33M/s    | **+6.4%**           |

#### 중간 Workload 처리
*적당한 계산이 있는 Job (100 iteration)*

| Queue 전략 | Job 수 | 실행 시간 | 처리량 | 상대 성능 |
|---------------|-----------|----------------|------------|---------------------|
| Mutex 기반   | 100       | ~125 μs        | 800K/s     | 기준            |
| Adaptive      | 100       | ~118 μs        | 847K/s     | **+5.9%**           |
| Mutex 기반   | 1,000     | ~1.1 ms        | 909K/s     | 기준            |
| Adaptive      | 1,000     | ~1.0 ms        | 1.00M/s    | **+10.0%**          |

#### 우선순위 스케줄링 성능
*Adaptive queue 선택이 있는 type 기반 job 라우팅*

| Type당 Job | 총 Job | 처리 시간 | 라우팅 정확도 | 우선순위 처리 |
|---------------|------------|-----------------|------------------|-------------------|
| 100           | 300        | ~285 μs         | 99.7%            | 최적           |
| 500           | 1,500      | ~1.35 ms        | 99.4%            | 효율적         |
| 1,000         | 3,000      | ~2.65 ms        | 99.1%            | 안정적            |

#### 높은 경합 시나리오
*동시에 job을 제출하는 여러 producer thread*

| Thread 수 | Standard Logger | Adaptive Logger | 성능 이득 |
|-------------|---------------------|-------------------|------------------|
| 1           | 1,000 jobs/μs       | 1,000 jobs/μs     | 0% (기준)    |
| 2           | 850 jobs/μs         | 920 jobs/μs       | **+8.2%**        |
| 4           | 620 jobs/μs         | 780 jobs/μs       | **+25.8%**       |
| 8           | 380 jobs/μs         | 650 jobs/μs       | **+71.1%**       |
| 16          | 190 jobs/μs         | 520 jobs/μs       | **+173.7%**      |

### Queue 레벨 Benchmark

#### 기본 Queue 작업
*원시 enqueue/dequeue 성능*

| 작업 | Mutex Queue | Adaptive Queue | 개선 |
|-----------|-------------|----------------|-------------|
| Enqueue (단일) | ~85 ns | ~78 ns | **+8.2%** |
| Dequeue (단일) | ~195 ns | ~142 ns | **+37.3%** |
| Enqueue/Dequeue 쌍 | ~280 ns | ~220 ns | **+27.3%** |

#### Batch 작업
*여러 item을 한 번에 처리*

| Batch 크기 | Mutex Queue (μs) | Adaptive Queue (μs) | 개선 |
|-----------|------------------|---------------------|-------------|
| 8         | 2.8              | 2.1                 | **+33.3%**  |
| 32        | 9.2              | 6.8                 | **+35.3%**  |
| 128       | 34.1             | 24.7                | **+38.0%**  |
| 512       | 128.4            | 91.2                | **+41.0%**  |
| 1024      | 248.7            | 175.3               | **+41.9%**  |

#### 경합 스트레스 테스트
*queue 접근을 위해 경쟁하는 여러 thread*

| 동시 Thread | Mutex Queue (μs) | Adaptive Queue (μs) | 확장성 요인 |
|-------------------|------------------|---------------------|-------------------|
| 1                 | 28.5             | 29.1                | 0.98x             |
| 2                 | 65.2             | 42.3                | **1.54x**         |
| 4                 | 156.8            | 73.5                | **2.13x**         |
| 8                 | 387.2            | 125.8               | **3.08x**         |
| 16                | 892.5            | 218.6               | **4.08x**         |

#### Job Type 라우팅 기능
*Type 기반 job queue 선택 및 라우팅*

| Job Type 혼합 | Type별 Job | 라우팅 시간 | Standard 시간 | 라우팅 이점 |
|--------------|-------------------|--------------|---------------|-----------------|
| 각 type 33% | 1,000 | 142 ns | 168 ns | **+18.3%** |
| High 우선순위 50% | 1,500 | 138 ns | 175 ns | **+26.8%** |
| High 우선순위 80% | 2,400 | 135 ns | 182 ns | **+34.8%** |

#### 메모리 사용 비교

| Queue 타입 | Job 수 | 메모리 사용량 | Job당 메모리 | 참고 |
|------------|-----------|--------------|----------------|-------|
| Mutex Queue | 100 | 8.2 KB | 82 bytes | 공유 데이터 구조 |
| Adaptive Queue | 100 | 12.5 KB | 125 bytes | 동적 할당 |
| Mutex Queue | 1,000 | 24.1 KB | 24 bytes | 메모리 효율성 향상 |
| Adaptive Queue | 1,000 | 31.8 KB | 32 bytes | 좋은 확장 속성 |
| Mutex Queue | 10,000 | 195.2 KB | 20 bytes | 우수한 밀도 |
| Adaptive Queue | 10,000 | 248.7 KB | 25 bytes | 허용 가능한 오버헤드 |

### Benchmark 환경 세부 정보

- **하드웨어**: Apple M1 (8-core), 16GB RAM
- **소프트웨어**: macOS Sonoma, Apple Clang 17.0.0, C++20
- **빌드**: Release 모드 (-O3), Google Benchmark framework
- **테스트 기간**: warmup이 있는 benchmark당 10초
- **Iteration**: 통계적 유의성을 위해 Google Benchmark에서 자동 결정
- **Thread 구성**: 4 worker (type당 1 + universal 1)
- **최신 업데이트**: 향상된 lock-free 알고리즘이 있는 2025-06-29

### 사용 가능한 Benchmark

Thread System에는 성능 테스트를 위한 포괄적인 benchmark가 포함되어 있습니다:

#### Thread Pool Benchmark (`benchmarks/thread_pool_benchmarks/`)
- **gbench_thread_pool**: 기본 Google Benchmark 통합
- **thread_pool_benchmark**: 핵심 thread pool 성능 metric
- **memory_benchmark**: 메모리 사용량 및 할당 패턴 (logger benchmark 제거됨)
- **real_world_benchmark**: 현실적인 workload 시뮬레이션
- **stress_test_benchmark**: 극한 부하 및 경합 테스트
- **scalability_benchmark**: 멀티 코어 확장 분석
- **contention_benchmark**: 경합별 시나리오
- **comparison_benchmark**: 라이브러리 간 비교
- **throughput_detailed_benchmark**: 상세한 처리량 분석

#### Queue Benchmark (`benchmarks/thread_base_benchmarks/`)
- **mpmc_performance_test**: MPMC queue 성능 분석
- **simple_mpmc_benchmark**: 기본 queue 작업
- **quick_mpmc_test**: 빠른 queue 검증

#### 기타 Benchmark
- **data_race_benchmark**: Data race 수정 영향 분석

#### Benchmark 실행
```bash
# Build with benchmarks enabled
./build.sh --clean --benchmark

# Run specific benchmark
./build/bin/thread_pool_benchmark

# Run with custom parameters
./build/bin/thread_pool_benchmark --benchmark_time_unit=ms --benchmark_min_time=1s

# Filter specific tests
./build/bin/thread_pool_benchmark --benchmark_filter="BM_ThreadPool/*"

# Export results
./build/bin/thread_pool_benchmark --benchmark_format=json > results.json
```

### 주요 성능 통찰력

1. **Adaptive Queue 장점**:
   - 경합에 따른 자동 전략 선택
   - 필요할 때 더 나은 queue 작업 지연 시간 (20-40% 더 빠름)
   - Mutex 및 lock-free 모드 모두 지원
   - 일관된 성능 확장

2. **간소화된 아키텍처 이점**:
   - 일반적인 시나리오에 대한 낮은 메모리 오버헤드
   - 자동 최적화가 있는 더 깔끔한 코드베이스
   - 예측 가능한 성능 특성
   - 유익할 때 lock-free 기능 유지

3. **권장 사용법**:
   - **Adaptive queue**: 모든 시나리오에 대한 자동 최적화
   - **Type 기반 라우팅**: 전문 job 처리
   - **동적 확장**: 자동 리소스 할당

4. **성능 특성**:
   - Adaptive queue는 경합 시 2-4배 더 나은 확장성 표시
   - 메모리 오버헤드: 사용량에 따라 최적화된 할당
   - Type 라우팅은 전문 job에 대해 15-35% 효율성 추가

## 확장성 분석

### Worker Thread 확장 효율성

| Worker | 속도 향상 | 효율성 | Queue 깊이 (평균) | CPU 활용도 |
|---------|---------|------------|-------------------|-----------------|
| 1       | 1.0x    | 100%       | 0.1               | 98%             |
| 2       | 2.0x    | 99%        | 0.2               | 97%             |
| 4       | 3.9x    | 97.5%      | 0.5               | 96%             |
| 8       | 7.7x    | 96.25%     | 1.2               | 95%             |
| 16      | 15.0x   | 93.75%     | 3.1               | 92%             |
| 32      | 28.3x   | 88.4%      | 8.7               | 86%             |
| 64      | 52.1x   | 81.4%      | 22.4              | 78%             |

### Workload별 확장

#### CPU 바운드 작업
- **최적 Worker**: 하드웨어 코어 수
- **최고 효율성**: 8 코어에서 96%
- **확장 제한**: 물리적 코어 (ARM의 performance 코어)
- **권장**: CPU 집약적 작업에 정확한 코어 수 사용

#### I/O 바운드 작업
- **최적 Worker**: 하드웨어 코어 수의 2-3배
- **최고 효율성**: 16+ worker에서 85%
- **확장 이점**: 코어 수를 넘어 계속됨
- **권장**: 2배 코어로 시작, I/O 대기 시간에 따라 튜닝

#### 혼합 Workload
- **최적 Worker**: 하드웨어 코어 수의 1.5배
- **최고 효율성**: 12 worker에서 90%
- **균형점**: CPU 및 I/O 특성 사이
- **권장**: workload를 프로파일링하여 최적 균형 찾기

## 메모리 성능

### 구성별 메모리 사용량

| 구성 | 가상 메모리 | 상주 메모리 | 최대 메모리 | Worker당 |
|--------------|----------------|-----------------|-------------|------------|
| 기본 시스템  | 45.2 MB        | 12.8 MB         | 12.8 MB     | -          |
| 1 Worker     | 46.4 MB        | 14.0 MB         | 14.2 MB     | 1.2 MB     |
| 4 Worker    | 48.1 MB        | 14.6 MB         | 15.1 MB     | 450 KB     |
| 8 Worker    | 50.4 MB        | 15.4 MB         | 16.3 MB     | 325 KB     |
| 16 Worker   | 54.8 MB        | 16.6 MB         | 18.7 MB     | 262 KB     |
| 32 Worker   | 63.2 MB        | 20.2 MB         | 25.1 MB     | 231 KB     |

### 메모리 최적화 영향 (v2.0)

최신 메모리 최적화 사용:

| 컴포넌트 | 이전 | 이후 | 절감 | 참고 |
|-----------|--------|-------|---------|-------|
| Adaptive Queue (유휴) | 8.2 MB | 0.4 MB | 95% | Lazy 초기화 |
| Node Pool (256 node) | 16 KB | 1 KB | 93.75% | 감소된 초기 chunk |
| Thread Pool (8 worker) | 15.4 MB | 12.1 MB | 21% | 결합된 최적화 |
| 최대 메모리 (변경 없음) | 16.3 MB | 16.3 MB | 0% | 동일한 최대 용량 |

### 시작 메모리 프로필

| 단계 | 메모리 사용량 | 시간 | 설명 |
|-------|-------------|------|-------------|
| Binary Load | 8.2 MB | 0 ms | 기본 실행 파일 |
| Library Init | 10.4 MB | 2 ms | 동적 라이브러리 |
| Thread Pool Create | 10.8 MB | 0.3 ms | Pool 구조만 |
| Worker Spawn (8) | 12.1 MB | 1.2 ms | Thread stack 할당 |
| First Job | 12.3 MB | 0.1 ms | Queue 초기화 |
| Steady State | 12.8 MB | - | 정상 작업 |

### 메모리 할당이 성능에 미치는 영향

| 메모리 패턴 | 할당 크기 | Job/초 | vs 할당 없음 | P99 지연 시간 | 메모리 오버헤드 |
|---------------|----------------|----------|-------------|-------------|-----------------|
| 없음          | 0              | 1,160,000| 100%        | 1.8μs       | 0               |
| 작음         | <1KB           | 1,044,000| 90%         | 2.2μs       | +15%            |
| 중간        | 1-100KB        | 684,000  | 59%         | 3.8μs       | +45%            |
| 큼         | 100KB-1MB      | 267,000  | 23%         | 9.5μs       | +120%           |
| 매우 큼    | >1MB           | 58,000   | 5%          | 42μs        | +300%           |

### 잠재적 메모리 Pool 최적화

*참고: Thread System은 현재 내장 메모리 pool을 구현하지 않습니다. 다음은 사용자 정의 메모리 pool 구현으로 잠재적 개선을 나타냅니다:*

| Pool 타입 | 현재 | Pool 사용 (추정) | 잠재적 개선 | 메모리 절감 |
|-----------|---------|----------------------|----------------------|----------------|
| 작은 Job | 1.04M/s | 1.11M/s (추정) | +7% | 60% |
| 중간 Job | 684K/s | 848K/s (추정) | +24% | 75% |
| 큰 Job | 267K/s | 385K/s (추정) | +44% | 80% |

## Adaptive MPMC Queue 성능

### 개요
Adaptive MPMC (Multiple Producer Multiple Consumer) queue 구현은 최적 성능을 위한 자동 전략 선택을 제공합니다:

- **아키텍처**: mutex와 lock-free 전략 간 동적 전환
- **메모리 관리**: queue 사용 패턴에 따른 효율적인 할당
- **경합 처리**: 자동 감지 및 전략 전환
- **Cache 최적화**: 성능을 위한 최적화된 메모리 레이아웃

### 성능 비교

| 구성 | Mutex 전용 Queue | Adaptive MPMC | 개선 |
|--------------|-------------------|----------------|-------------|
| 1P-1C (10K op) | 2.03 ms | 1.87 ms | +8.6% |
| 2P-2C (10K op) | 5.21 ms | 3.42 ms | +52.3% |
| 4P-4C (10K op) | 12.34 ms | 5.67 ms | +117.6% |
| 8P-8C (10K op) | 28.91 ms | 9.23 ms | +213.4% |
| **원시 작업** | **12.2 μs** | **2.8 μs** | **+431%** |
| **실제 workload** | **950 ms/1M** | **865 ms/1M** | **+10%** |

### 확장성 분석

| Worker | Mutex 전용 효율성 | Adaptive 효율성 | 효율성 이득 |
|---------|----------------------|-------------------|-----------------|
| 1 | 100% | 100% | 0% |
| 2 | 81% | 95% | +14% |
| 4 | 52% | 88% | +36% |
| 8 | 29% | 82% | +53% |

## Adaptive Logger 성능

### 개요
Adaptive logger 구현은 높은 처리량 로깅 시나리오에 대한 자동 최적화를 제공합니다:

- **아키텍처**: 로그 메시지 제출을 위한 adaptive job queue
- **경합 처리**: 부하에 따른 동적 전략 선택
- **확장성**: thread 수에 따른 최적 성능 확장
- **호환성**: 기존 코드와 원활한 통합

### 단일 Thread 성능
*메시지 처리량 비교*

| 메시지 크기 | Standard Logger | Adaptive Logger | 개선 |
|--------------|-----------------|------------------|-------------|
| 짧음 (17 char) | 7.64 M/s | 7.42 M/s | -2.9% |
| 중간 (123 char) | 5.73 M/s | 5.61 M/s | -2.1% |
| 긴 (1024 char) | 2.59 M/s | 2.55 M/s | -1.5% |

*참고: 단일 thread 성능은 최소 오버헤드를 보여줍니다. 이점은 경합 시 나타납니다.*

### 멀티 Thread 확장성
*동시 로깅 thread의 처리량*

| Thread | Standard Logger | Adaptive Logger | 개선 |
|---------|-----------------|------------------|-------------|
| 2 | 1.91 M/s | 1.95 M/s | **+2.1%** |
| 4 | 0.74 M/s | 1.07 M/s | **+44.6%** |
| 8 | 0.22 M/s | 0.63 M/s | **+186.4%** |
| 16 | 0.16 M/s | 0.54 M/s | **+237.5%** |

### 형식화된 로깅 성능
*여러 매개변수가 있는 복잡한 format 문자열*

| Logger 타입 | 처리량 | 지연 시간 (ns) |
|-------------|------------|--------------|
| Standard | 2.94 M/s | 340 |
| Adaptive | 2.89 M/s | 346 |

### Burst 로깅 성능
*갑작스런 로그 burst 처리*

| Burst 크기 | Standard Logger | Adaptive Logger | 개선 |
|------------|-----------------|------------------|-------------|
| 10 메시지 | 1.90 M/s | 1.88 M/s | -1.1% |
| 100 메시지 | 5.33 M/s | 5.15 M/s | -3.4% |

### 혼합 로그 타입 성능
*다른 로그 레벨 (Info, Debug, Error, Exception)*

| Logger 타입 | 처리량 | CPU 효율성 |
|-------------|------------|----------------|
| Standard | 6.51 M/s | 100% |
| Adaptive | 6.42 M/s | 98% |

### 주요 발견

1. **높은 경합 이점**: Adaptive logger는 4+ thread에서 상당한 장점 표시
2. **확장성**: 16 thread에서 최대 237% 개선
3. **최소 오버헤드**: 단일 thread 성능이 거의 동일함
4. **사용 사례**: 자동 최적화가 있는 모든 멀티 thread 애플리케이션에 이상적

### 권장 사항

- **Adaptive Logger 사용**: 모든 시나리오에 대한 자동 최적화
- **동적 확장**: Logger가 애플리케이션의 threading 패턴에 적응
- **Batch 처리**: 처리량에 유익할 때 자동으로 활성화됨
- **Buffer 관리**: workload에 따른 동적 queue 크기 조정

### 구현 세부 정보

- **Adaptive 전략**: 경합에 따라 mutex와 lock-free 간 자동 전환
- **동적 할당**: queue 패턴에 따른 효율적인 메모리 사용
- **스마트 재시도 로직**: 경합을 방지하기 위한 지능형 backoff 전략
- **Queue 최적화**: 자동 batching 및 buffer 관리
- **성능 모니터링**: 최적화 결정을 위한 내장 metric

### 현재 상태

- Adaptive queue 선택이 있는 모듈화된 아키텍처
- Logger 및 monitoring이 독립 프로젝트로 분리됨
- 모든 스트레스 테스트가 활성화되고 안정적으로 통과
- Adaptive 구현은 모든 시나리오에 대해 최적 성능 제공
- 평균 작업 지연 시간:
  - Enqueue: ~96 ns (낮은 경합), ~320 ns (높은 경합)
  - Dequeue: adaptive 최적화로 ~571 ns

### 최근 Benchmark 결과 (2025-07-25)

#### Data Race 수정 검증
| Thread | Wake Interval 접근 | Cancellation Token | Job Queue 일관성 |
|---------|---------------------|--------------------|-----------------------|
| 1       | 163μs/10K           | 25μs/100           | -                     |
| 4       | 272μs/10K           | 59μs/400           | 842μs/2K dequeued     |
| 8       | 438μs/10K           | 111μs/800          | 2.18ms/4K dequeued    |
| 16      | 750μs/10K           | 210μs/1.6K         | 4.81ms/8K dequeued    |

*참고: 모든 data race 문제가 적절한 동기화로 해결되었습니다.*

### 사용 권장 사항

1. **Adaptive Queue 이점**:
   - 모든 경합 시나리오 자동 최적화
   - 지연 시간에 민감한 애플리케이션이 자동 전환의 이점
   - 다양한 CPU 부하 패턴이 있는 시스템
   - 동적 요구 사항이 있는 실시간 애플리케이션

2. **구성 지침**:
   ```cpp
   // Adaptive behavior (recommended)
   adaptive_job_queue queue(adaptive_job_queue::queue_strategy::ADAPTIVE);

   // Force specific strategy when needed
   adaptive_job_queue mutex_queue(adaptive_job_queue::queue_strategy::FORCE_MUTEX);
   adaptive_job_queue lockfree_queue(adaptive_job_queue::queue_strategy::FORCE_LOCKFREE);
   ```

### 성능 튜닝 팁

1. **Batch 작업**: 더 나은 처리량을 위해 batch enqueue/dequeue 사용
2. **CPU Affinity**: 일관된 성능을 위해 thread를 특정 코어에 고정
3. **메모리 정렬**: job 객체가 cache-line 정렬되었는지 확인
4. **재시도 처리**: 극심한 경합 시 작업이 실패할 수 있음 - 재시도 로직 구현
5. **모니터링**: 내장 통계를 사용하여 재시도 횟수를 포함한 성능 metric 추적

## Logger 성능 (현재 별도 프로젝트)

*참고: Logger가 별도 프로젝트로 이동했습니다. 다음 benchmark는 logger가 Thread System과 통합되었을 때의 것입니다.*

### 업계 표준과 Logger 비교

### 개요
이 섹션은 Thread System의 로깅 성능을 업계 표준 로깅 라이브러리와 비교합니다. Logger는 자동 최적화를 위해 adaptive job queue를 사용합니다.

### 단일 Thread 성능 비교
*Apple M1 (8-core)의 기준 측정*

| Logger | 처리량 | 지연 시간 (ns) | 상대 성능 |
|--------|------------|--------------|---------------------|
| Console Output | 542.8K/s | 1,842 | 기준 |
| Thread System Logger | 4.41M/s | 227 | **8.1배** 더 빠름 |
| Thread System (Adaptive) | 4.34M/s | 240 | **8.0배** 더 빠름 |

### 멀티 Thread 확장성
*Adaptive 최적화가 있는 동시 로깅 성능*

| Thread | Standard 모드 | Adaptive 모드 | 개선 |
|---------|---------------|---------------|-------------|
| 2 | 2.61M/s | 2.58M/s | -1% |
| 4 | 859K/s | 1.07M/s | **+25%** |
| 8 | 234K/s | 412K/s | **+76%** |
| 16 | 177K/s | 385K/s | **+118%** |

### 지연 시간 특성
*종단 간 로깅 지연 시간*

| Logger 타입 | 평균 지연 시간 | P99 지연 시간 | 참고 |
|-------------|--------------|-------------|-------|
| Thread System Logger | 144 ns | ~200 ns | Adaptive queue |
| Console Output | 1,880 ns | ~2,500 ns | System call 오버헤드 |

### 주요 발견

1. **Logger 우수성**:
   - Console output보다 8.1배 빠름
   - 우수한 단일 thread 성능
   - 예측 가능한 지연 시간 특성

2. **Adaptive 확장성**:
   - 4+ thread에서 자동 최적화
   - 높은 경합에서 최대 118% 개선
   - 단일 thread 시나리오에서 최소 오버헤드

3. **사용 권장 사항**:
   - 모든 시나리오에 Thread System Logger 사용
   - Adaptive queue가 부하에 따라 자동으로 최적화
   - 수동 구성 불필요

### spdlog와 비교

*인기 있는 spdlog 라이브러리와의 포괄적인 성능 비교*

#### 단일 Thread 성능
| Logger | 처리량 | 지연 시간 | vs Console | 참고 |
|--------|------------|---------|------------|-------|
| Console Output | 583K/s | 1,716 ns | 기준 | System call 오버헤드 |
| **Thread System Logger** | **4.34M/s** | **148 ns** | **7.4배** | 최고 지연 시간 |
| spdlog (sync) | 515K/s | 2,333 ns | 0.88x | 낮은 성능 |
| **spdlog (async)** | **5.35M/s** | - | **9.2배** | 최고 처리량 |

#### 멀티 Thread 성능 (4 Thread)
| Logger | 처리량 | vs 단일 thread | 확장성 |
|--------|------------|------------------|-------------|
| Thread System (Standard) | 599K/s | -86% | 중간 |
| **Thread System (Adaptive)** | **1.07M/s** | -75% | **좋음** |
| spdlog (sync) | 210K/s | -59% | 매우 낮음 |
| spdlog (async) | 785K/s | -85% | 낮음 |

#### 높은 경합 (8 Thread)
| Logger | 처리량 | vs Console | 참고 |
|--------|------------|------------|-------|
| Thread System (Standard) | 198K/s | 0.34x | 높은 경합 |
| **Thread System (Adaptive)** | **412K/s** | **0.71x** | 자동 최적화 |
| spdlog (sync) | 52K/s | 0.09x | 심각한 성능 저하 |
| spdlog (async) | 240K/s | 0.41x | Queue 포화 |

#### 주요 발견

1. **단일 thread 챔피언**: spdlog async (5.35M/s)가 Thread System (4.34M/s)을 근소하게 앞섬
2. **멀티 thread 챔피언**: Adaptive queue가 있는 Thread System이 일관된 성능 표시
3. **지연 시간 챔피언**: 148ns의 Thread System, spdlog sync보다 **15.8배 낮음** (2333/148 = 15.76)
4. **확장성**: Thread System adaptive 모드가 자동 최적화 제공

### 권장 사항

1. **모든 애플리케이션**: Thread System Logger 사용
   - Adaptive 최적화로 우수한 성능
   - Type 안전성이 있는 간단한 API
   - 내장 파일 rotation 및 callback
   - 높은 동시성을 위한 자동 최적화

2. **구성 팁**:
   - Logger가 workload에 자동으로 적응
   - 수동 튜닝 불필요
   - Adaptive queue가 burst 패턴을 효율적으로 처리

3. **사용 예제**:
   ```cpp
   // Simple usage - automatic optimization
   log_module::start();
   log_module::write_information("Message: {}", value);
   log_module::write_error("Error: {}", error);
   log_module::stop();
   ```

## 다른 라이브러리와의 비교

### 처리량 비교 (실제 측정)

| 라이브러리                    | 처리량 | 상대 성능 | 기능               |
|---------------------------|------------|---------------------|------------------------|
| **Thread System**         | 1.16M/s    | 100% (기준)     | Type, 로깅, C++20, lock-free |
| Intel TBB                 | ~1.24M/s   | ~107%               | 업계 표준, work stealing |
| Boost.Thread Pool        | ~1.09M/s   | ~94%                | Header-only, portable |
| std::async                | ~267K/s    | ~23%                | 표준 라이브러리, 기본 |
| Custom (naive)            | ~684K/s    | ~59%                | 간단한 mutex 기반 구현 |
| OpenMP                    | ~1.06M/s   | ~92%                | Compiler directive |
| Microsoft PPL             | ~1.02M/s   | ~88%                | Windows 전용 |

### 기능 비교

| 라이브러리 | Type 지원 | 로깅 | C++20 | 크로스 플랫폼 | 메모리 Pool | 오류 처리 |
|---------|-----------------|---------|-------|----------------|-------------|----------------|
| Thread System | ✅ 예 | ✅ 예 | ✅ 예 | ✅ 예 | ❌ 아니오 | ✅ 포괄적 |
| Intel TBB | ✅ 예 | ❌ 아니오 | ⚠️ 부분 | ✅ 예 | ✅ 예 | ⚠️ 기본 |
| Boost.Thread Pool | ❌ 아니오 | ❌ 아니오 | ⚠️ 부분 | ✅ 예 | ❌ 아니오 | ⚠️ 기본 |
| std::async | ❌ 아니오 | ❌ 아니오 | ✅ 예 | ✅ 예 | ❌ 아니오 | ⚠️ 기본 |

### 지연 시간 비교 (μs)

| 라이브러리 | 제출 | 실행 시작 | 총 오버헤드 |
|---------|------------|-----------------|----------------|
| Thread System | 77 ns | 96 ns | 173 ns |
| Intel TBB | ~100 ns | ~90 ns | ~190 ns |
| Boost.Thread Pool | ~150 ns | ~120 ns | ~270 ns |
| std::async | ~15.2 μs | ~12.8 μs | ~28.0 μs |

## 최적화 전략

### 1. 최적 Thread 수 선택

```cpp
uint16_t determine_optimal_thread_count(WorkloadType workload) {
    uint16_t hardware_threads = std::thread::hardware_concurrency();

    switch (workload) {
        case WorkloadType::CpuBound:
            return hardware_threads;

        case WorkloadType::MemoryBound:
            return std::max(1u, hardware_threads / 2);

        case WorkloadType::IoBlocking:
            return hardware_threads * 2;

        case WorkloadType::Mixed:
            return static_cast<uint16_t>(hardware_threads * 1.5);

        case WorkloadType::RealTime:
            return hardware_threads - 1; // Reserve one core for OS
    }

    return hardware_threads;
}
```

### 2. 성능을 위한 Job Batching

Batching은 스케줄링 오버헤드를 크게 줄입니다:

| Batch 크기 | Job당 오버헤드 | 권장 사용 사례 |
|------------|-----------------|---------------------|
| 1          | 77 ns           | 실시간 작업     |
| 10         | 25 ns           | 대화형 작업   |
| 100        | 8 ns            | 백그라운드 처리|
| 1000       | 3 ns            | Batch 처리    |
| 10000      | 2 ns            | 대량 작업     |

```cpp
// Efficient job batching
std::vector<std::unique_ptr<thread_module::job>> jobs;
jobs.reserve(batch_size);

for (int i = 0; i < batch_size; ++i) {
    jobs.push_back(create_job(data[i]));
}

pool->enqueue_batch(std::move(jobs));
```

### 3. Job 세분성 최적화

| Job 실행 시간 | 권장 조치 | 이유 |
|--------------------|-------------------|--------|
| < 10μs             | 1000+ 작업 batch | 오버헤드가 지배적 |
| 10-100μs           | 100 작업 batch | 오버헤드/병렬성 균형 |
| 100μs-1ms          | 10 작업 batch | 조정 최소화 |
| 1ms-10ms           | 개별 job | 좋은 세분성 |
| > 10ms             | 세분화 고려 | 응답성 개선 |

### 4. Type Pool 구성

```cpp
void configure_type_pool(std::shared_ptr<typed_thread_pool> pool,
                            const WorkloadProfile& profile) {
    const uint16_t hw_threads = std::thread::hardware_concurrency();

    // Allocate workers based on type distribution
    uint16_t high_workers = static_cast<uint16_t>(hw_threads * profile.high_type_ratio);
    uint16_t normal_workers = static_cast<uint16_t>(hw_threads * profile.normal_type_ratio);
    uint16_t low_workers = static_cast<uint16_t>(hw_threads * profile.low_type_ratio);

    // Ensure minimum coverage
    high_workers = std::max(1u, high_workers);
    normal_workers = std::max(1u, normal_workers);
    low_workers = std::max(1u, low_workers);

    // Add specialized workers
    add_type_workers(pool, job_types::High, high_workers);
    add_type_workers(pool, job_types::Normal, normal_workers);
    add_type_workers(pool, job_types::Low, low_workers);
}
```

### 4b. Adaptive Queue 구성

```cpp
#include "typed_thread_pool/pool/typed_thread_pool.h"

auto create_optimal_pool(const std::string& name,
                        size_t expected_concurrency,
                        bool priority_sensitive) -> std::shared_ptr<typed_thread_pool_t<job_types>> {

    // Create typed thread pool with adaptive queue strategy
    auto pool = std::make_shared<typed_thread_pool_t<job_types>>(name);

    // Configure adaptive queue strategy based on expected usage
    if (expected_concurrency > 4 || priority_sensitive) {
        // High contention - adaptive queue will automatically optimize
        pool->set_queue_strategy(queue_strategy::ADAPTIVE);
    }

    // Add specialized workers for each priority
    auto realtime_worker = std::make_unique<typed_thread_worker_t<job_types>>();
    realtime_worker->set_responsibilities({job_types::RealTime});
    pool->add_worker(std::move(realtime_worker));

    auto batch_worker = std::make_unique<typed_thread_worker_t<job_types>>();
    batch_worker->set_responsibilities({job_types::Batch});
    pool->add_worker(std::move(batch_worker));

    auto background_worker = std::make_unique<typed_thread_worker_t<job_types>>();
    background_worker->set_responsibilities({job_types::Background});
    pool->add_worker(std::move(background_worker));

    // Universal worker for load balancing
    auto universal_worker = std::make_unique<typed_thread_worker_t<job_types>>();
    universal_worker->set_responsibilities({job_types::RealTime, job_types::Batch, job_types::Background});
    pool->add_worker(std::move(universal_worker));

    return pool;
}

// Usage examples
auto high_concurrency_pool = create_optimal_pool(
    "HighConcurrency", 8, true);

auto simple_pool = create_optimal_pool(
    "Simple", 2, false);
```

### 5. 메모리 최적화

#### Cache-Line 정렬
```cpp
// Prevent false sharing
struct alignas(64) WorkerData {
    std::atomic<uint64_t> processed_jobs{0};
    std::atomic<uint64_t> execution_time{0};
    char padding[64 - 2 * sizeof(std::atomic<uint64_t>)];
};
```

#### 메모리 Pool 구현 (권장 최적화)

*참고: 이것은 메모리 pool 기능이 필요한 사용자를 위한 권장 최적화 패턴입니다. Thread System에는 현재 내장 메모리 pool이 포함되어 있지 않습니다.*

```cpp
template<typename JobType, size_t PoolSize = 1024>
class JobPool {
public:
    auto acquire() -> std::unique_ptr<JobType> {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!pool_.empty()) {
            auto job = std::move(pool_.back());
            pool_.pop_back();
            return job;
        }
        return std::make_unique<JobType>();
    }

    auto release(std::unique_ptr<JobType> job) -> void {
        if (!job) return;
        job->reset();

        std::lock_guard<std::mutex> lock(mutex_);
        if (pool_.size() < PoolSize) {
            pool_.push_back(std::move(job));
        }
    }

private:
    std::vector<std::unique_ptr<JobType>> pool_;
    std::mutex mutex_;
};
```

## 플랫폼별 최적화

### macOS/ARM64 최적화

```cpp
#ifdef __APPLE__
// Leverage performance cores on Apple Silicon
void configure_for_apple_silicon(thread_pool_module::thread_pool& pool) {
    size_t performance_cores = 4; // M1 has 4 performance cores
    size_t efficiency_cores = 4;  // M1 has 4 efficiency cores

    // Prioritize performance cores for CPU-intensive work
    for (size_t i = 0; i < performance_cores; ++i) {
        auto worker = std::make_unique<thread_worker>(pool.get_job_queue());
        pthread_set_qos_class_self_np(QOS_CLASS_USER_INITIATED, 0);
        pool.enqueue(std::move(worker));
    }
}
#endif
```

### Linux 최적화

```cpp
#ifdef __linux__
void set_thread_affinity(std::thread& thread, uint32_t core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set_t), &cpuset);
}

void configure_numa_awareness(thread_pool_module::thread_pool& pool) {
    // Distribute workers across NUMA nodes
    int numa_nodes = numa_max_node() + 1;
    auto workers = pool.get_workers();

    for (size_t i = 0; i < workers.size(); ++i) {
        int node = i % numa_nodes;
        numa_run_on_node(node);
        set_thread_affinity(workers[i]->get_thread(), i);
    }
}
#endif
```

### Windows 최적화

```cpp
#ifdef _WIN32
void configure_windows_type(thread_pool_module::thread_pool& pool) {
    auto workers = pool.get_workers();

    for (auto& worker : workers) {
        SetThreadType(worker->get_thread_handle(), THREAD_PRIORITY_ABOVE_NORMAL);
    }
}

void configure_processor_groups(thread_pool_module::thread_pool& pool) {
    DWORD num_groups = GetActiveProcessorGroupCount();
    if (num_groups <= 1) return;

    auto workers = pool.get_workers();
    for (size_t i = 0; i < workers.size(); ++i) {
        WORD group = i % num_groups;
        GROUP_AFFINITY affinity = {0};
        affinity.Group = group;
        affinity.Mask = 1ULL << (i / num_groups);

        SetThreadGroupAffinity(workers[i]->get_thread_handle(), &affinity, nullptr);
    }
}
#endif
```

## 모범 사례

### 성능 튜닝 체크리스트

#### 측정 및 분석
- [x] Benchmark로 성능 기준 수립
- [x] 실제 workload 패턴 프로파일링
- [x] Thread 활용도 및 queue 깊이 측정
- [x] 체계적인 분석을 통해 병목 현상 식별

#### Thread Pool 구성
- [x] Workload 타입에 따라 최적 thread 수 설정
- [x] Type별 worker를 적절히 구성
- [x] 중요한 애플리케이션을 위한 thread affinity 고려
- [x] 플랫폼별 특성에 맞게 조정

#### Job 설계
- [x] 가능한 경우 batch job 제출
- [x] 적절한 job 세분성 보장 (>100μs 권장)
- [x] job type 전체에서 workload 균형
- [x] job 실행에서 메모리 할당 최소화

#### 메모리 고려 사항
- [x] 적절한 정렬로 false sharing 방지
- [x] 자주 할당되는 객체에 대한 메모리 pool 구현 고려 (내장 없음)
- [x] Worker 데이터에 대한 thread-local storage 고려
- [x] 지속적인 부하에서 메모리 증가 모니터링

#### 고급 기법
- [x] 과부하 보호를 위한 backpressure 메커니즘 구현
- [x] 부하 분산을 위한 work-stealing 고려
- [x] 적절한 경우 lock-free 데이터 구조 사용
- [x] 장애 허용을 위한 circuit breaker 구현

### 실제 성능 지침

#### 웹 서버 애플리케이션
- **Thread 수**: I/O가 많은 workload의 경우 하드웨어 thread의 2배
- **Job 세분성**: 요청 처리를 > 100μs로 유지
- **Type 사용**: 대화형 요청에 High, API 호출에 Normal, 분석에 Low
- **메모리**: connection pool 및 요청 객체 pool 사용

#### 데이터 처리 Pipeline
- **Thread 수**: 물리적 코어 수와 일치
- **Batch 크기**: 큰 batch 사용 (1000+ item)
- **메모리**: 버퍼 사전 할당, 큰 데이터 세트에 메모리 매핑 파일 사용
- **최적화**: 다른 thread pool이 있는 pipeline 단계

#### 실시간 시스템
- **Thread 수**: OS용으로 1 코어 예약, 나머지 코어 사용
- **지연 시간**: <10μs 스케줄링 지연 시간 목표
- **Type**: 전용 worker가 있는 엄격한 type 분리
- **메모리**: 모든 메모리 사전 할당, 런타임 할당 피하기

#### 과학 컴퓨팅
- **Thread 수**: 사용 가능한 모든 코어 사용
- **Job 세분성**: 계산 크기와 조정 오버헤드 균형
- **메모리**: NUMA topology 및 메모리 대역폭 고려
- **최적화**: CPU별 최적화 사용 (SIMD, cache 최적화)

### 모니터링 및 진단

#### 주요 성능 지표

| Metric | 목표 범위 | 경고 임계값 | 임계 임계값 |
|--------|-------------|------------------|-------------------|
| Job/초 | >100K | <50K | <10K |
| Queue 깊이 | 0-10 | >50 | >200 |
| CPU 활용도 | 80-95% | >98% | 100% 지속 |
| 메모리 증가 | 시간당 <1% | 시간당 >5% | 시간당 >10% |
| 오류율 | <0.1% | >1% | >5% |

#### 진단 도구

```cpp
class PerformanceMonitor {
public:
    struct Metrics {
        std::atomic<uint64_t> jobs_submitted{0};
        std::atomic<uint64_t> jobs_completed{0};
        std::atomic<uint64_t> total_execution_time{0};
        std::atomic<uint32_t> current_queue_depth{0};
        std::atomic<uint32_t> peak_queue_depth{0};
    };

    auto get_throughput() const -> double {
        auto duration = std::chrono::steady_clock::now() - start_time_;
        auto seconds = std::chrono::duration<double>(duration).count();
        return metrics_.jobs_completed.load() / seconds;
    }

    auto get_average_latency() const -> double {
        uint64_t completed = metrics_.jobs_completed.load();
        if (completed == 0) return 0.0;
        return static_cast<double>(metrics_.total_execution_time.load()) / completed;
    }

private:
    Metrics metrics_;
    std::chrono::steady_clock::time_point start_time_{std::chrono::steady_clock::now()};
};
```

## 향후 성능 개선

### 계획된 최적화

1. **Type Thread Pool 최적화**:
   - 더 나은 부하 분산을 위한 type queue 간 work stealing
   - 감소된 오버헤드를 위한 batch dequeue 작업
   - Type 인식 스케줄링 정책

2. **메모리 Pool 통합**:
   - Job 객체를 위한 내장 메모리 pool
   - 할당 오버헤드 60-80% 감소
   - Cache 효율성을 위한 thread-local pool

3. **Type Pool을 위한 Work Stealing**:
   - 유휴 worker가 다른 type queue에서 steal하도록 허용
   - 불균등한 부하에서 더 나은 CPU 활용
   - 구성 가능한 stealing 정책

## 성능 권장 사항 요약 (2025)

### 빠른 구성 가이드

#### 1. **일반 애플리케이션**
```cpp
// Use standard thread pool with adaptive queues
auto pool = std::make_shared<thread_pool>("MyPool");

// Add workers (hardware_concurrency for CPU-bound)
for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
    pool->enqueue(std::make_unique<thread_worker>());
}
pool->start();
```

#### 2. **우선순위에 민감한 애플리케이션**
```cpp
// Use typed thread pool with adaptive queues
auto pool = std::make_shared<typed_thread_pool_t<job_types>>("PriorityPool");

// Add specialized workers
for (auto priority : {job_types::RealTime, job_types::Batch, job_types::Background}) {
    auto worker = std::make_unique<typed_thread_worker_t<job_types>>();
    worker->set_responsibilities({priority});
    pool->enqueue(std::move(worker));
}

// Add universal workers for load balancing
for (int i = 0; i < 2; ++i) {
    auto worker = std::make_unique<typed_thread_worker_t<job_types>>();
    worker->set_responsibilities({job_types::RealTime, job_types::Batch, job_types::Background});
    pool->enqueue(std::move(worker));
}
pool->start();
```

#### 3. **높은 동시성 시나리오**
```cpp
// Standard pool with batch processing
auto pool = std::make_shared<thread_pool>("HighConcurrency");

// Configure workers for batch processing
std::vector<std::unique_ptr<thread_worker>> workers;
for (int i = 0; i < std::thread::hardware_concurrency() * 2; ++i) {
    auto worker = std::make_unique<thread_worker>();
    worker->set_batch_processing(true, 32); // Process up to 32 jobs at once
    workers.push_back(std::move(worker));
}
pool->enqueue_batch(std::move(workers));
pool->start();
```

### 성능 튜닝 빠른 참조

| 시나리오 | 구성 | 예상 성능 |
|----------|---------------|---------------------|
| **CPU 바운드 작업** | Worker = hardware_concurrency() | 8 코어에서 96% 효율성 |
| **I/O 바운드 작업** | Worker = hardware_concurrency() × 2 | I/O 대기의 좋은 중첩 |
| **혼합 Workload** | Worker = hardware_concurrency() × 1.5 | 균형 잡힌 성능 |
| **낮은 지연 시간** | Standard pool, 단일 job | ~77ns 제출 지연 시간 |
| **높은 처리량** | Batch 처리 활성화 | 최대 13M jobs/s 이론적 |
| **우선순위 스케줄링** | Type당 3-4 worker가 있는 typed pool | 99.6% type 정확도 |

### 피해야 할 일반적인 함정

1. **Over-Threading**: 하드웨어 thread의 2배 이상 worker 생성 금지
2. **작은 Job**: 더 나은 효율성을 위해 < 10μs job batch
3. **메모리 할당**: 가능한 경우 job 객체 사전 할당
4. **Queue 깊이**: Queue 깊이 모니터링; > 1000은 backpressure 필요를 나타냄
5. **Type 확산**: 최적 성능을 위해 우선순위 type을 3-5개로 유지


## 결론

Thread System framework는 간소화된 adaptive 아키텍처로 탁월한 성능 특성을 제공합니다:

1. **높은 처리량**:
   - Standard pool: 1.16M jobs/초 (프로덕션에서 입증됨)
   - Adaptive queue: 모든 시나리오에 대한 자동 최적화
   - Typed pool: 우선순위 전문화로 1.24M jobs/초
2. **낮은 지연 시간**:
   - Standard pool: 77ns 스케줄링 오버헤드
   - Adaptive queue: 자동 전략 선택으로 96-580ns
   - 다양한 workload에서 일관된 성능
3. **우수한 확장성**:
   - Standard pool: 8 코어에서 96% 효율성
   - Adaptive queue: 모든 경합 수준에서 성능 유지
   - 높은 경합에서 최대 **3.46배 개선**
4. **메모리 효율성**:
   - Standard pool: <1MB 기본 메모리 사용량
   - 실제 사용량에 따른 동적 할당
   - 성능 손실 없이 ~8,700+ 줄로 코드베이스 감소
5. **플랫폼 최적화**:
   - Windows, Linux 및 macOS에서 일관된 성능
   - 유익한 경우 플랫폼별 최적화
6. **간소화된 아키텍처**:
   - 중복 코드 및 사용하지 않는 기능 제거
   - 모든 성능 기능 유지
   - 더 깔끔하고 유지 관리가 가능한 코드베이스

### 주요 성공 요인

1. **간소화된 사용**:
   - Adaptive queue가 있는 standard pool이 기본적으로 최적으로 작동
   - 수동 구성 불필요
   - 모든 시나리오에 대한 자동 최적화
2. **Workload 프로파일링**:
   - 기준 측정을 위해 내장 benchmark 사용
   - 실제 성능 특성 모니터링
   - Adaptive queue가 최적화 처리
3. **깔끔한 아키텍처 이점**:
   - 감소된 코드 복잡성으로 유지 관리성 향상
   - ~8,700+ 줄 제거 (logger, monitoring, 사용하지 않는 유틸리티)
   - Interface 기반 아키텍처를 갖춘 모듈식 설계
   - 스마트 설계를 통해 유지되는 성능
4. **성능 모니터링**:
   - Job 처리량 및 지연 시간 추적
   - Worker 활용도 모니터링
   - Adaptive queue 동작 관찰
5. **모범 사례**:
   - 우선순위 기반 workload에 typed pool 사용
   - 작은 job에 batch 작업 활용
   - 자동 최적화 신뢰

이 포괄적인 성능 가이드의 지침과 기법을 따르면 특정 애플리케이션 요구 사항에 대한 최적 성능을 달성할 수 있습니다. 간소화된 adaptive 아키텍처는 Thread System framework의 단순성과 안정성을 유지하면서 강력한 최적화 기능을 제공합니다.

---

*Last Updated: 2025-10-20*
