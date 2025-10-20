# Thread System 사용자 가이드

> **Language:** [English](USER_GUIDE.md) | **한국어**

## 빌드

### 필수 요구사항
- CMake 3.16 이상
- C++20 지원 컴파일러 (GCC 9+, Clang 10+, MSVC 2019+)
- vcpkg 패키지 매니저 (의존성 스크립트에 의해 자동 설치됨)

### 빠른 빌드
```bash
# vcpkg를 통해 의존성 설치
./dependency.sh       # Linux/macOS
# ./dependency.bat    # Windows

# 프로젝트 빌드
./build.sh           # Linux/macOS
# ./build.bat        # Windows

# 또는 수동 cmake 빌드
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### 빌드 옵션
```bash
# 서브모듈로 빌드 (라이브러리만, 샘플/테스트 제외)
cmake -S . -B build -DBUILD_THREADSYSTEM_AS_SUBMODULE=ON

# 문서 빌드 활성화
cmake -S . -B build -DBUILD_DOCUMENTATION=ON

# Sanitizer를 사용한 디버그 빌드
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON -DENABLE_UBSAN=ON
```

플랫폼별 상세 빌드 지침은 [Platform Build Guide](./PLATFORM_BUILD_GUIDE.md)를 참조하세요.

## 모듈

- core: `thread_base`, `job`, `job_queue`, `sync` (sync_primitives, cancellation_token, error_handling 포함)
- implementations: `thread_pool`, `typed_thread_pool`, adaptive queue
- interfaces: logging, monitoring, executor/scheduler, service registry
- utilities: formatting, string conversion, span

## 빠른 시작: Thread Pool

```cpp
using namespace thread_pool_module;
using thread_module::result_void;

auto pool = std::make_shared<thread_pool>("pool");
std::vector<std::unique_ptr<thread_worker>> workers;
workers.emplace_back(std::make_unique<thread_worker>(/*use_time_tag=*/false));
pool->enqueue_batch(std::move(workers));
pool->start();

pool->execute(std::make_unique<thread_module::callback_job>([]() -> result_void {
  // 작업 수행
  return result_void();
}));

pool->shutdown(); // 또는 pool->stop(false)
```

## 빠른 시작: Typed Thread Pool (우선순위/타입 라우팅)

```cpp
using namespace typed_thread_pool_module;

// 완전한 예제는 samples/typed_thread_pool_sample 참조
// 타입별로 job을 정의 (예: job_types::RealTime / Batch / Background)
// typed pool에 제출하여 타입/우선순위별로 라우팅
```

## Adaptive Job Queue (자동 전략)

Adaptive queue는 경합 및 지연 패턴에 따라 내부적으로 mutex 기반과 lock-free
전략 사이를 자동으로 전환합니다.

```cpp
thread_module::adaptive_job_queue q{
  thread_module::adaptive_job_queue::queue_strategy::ADAPTIVE
};

q.enqueue(std::make_unique<thread_module::callback_job>([](){ return thread_module::result_void(); }));
auto job = q.dequeue();
```

## 의존성 주입 및 Context

`service_container`를 사용하여 선택적 서비스(logger, monitoring)를 등록한 후,
`thread_context`가 pool/worker에 대해 자동으로 검색합니다.

```cpp
// 서비스 등록
thread_module::service_container::global()
  .register_singleton<thread_module::logger_interface>(my_logger);
thread_module::service_container::global()
  .register_singleton<monitoring_interface::monitoring_interface>(my_monitoring);

// Context를 인식하는 pool/worker는 서비스가 존재할 때 로그를 기록하고 메트릭을 보고함
thread_pool_module::thread_worker w{true, thread_module::thread_context{}};
```

## 모니터링 및 메트릭

- `monitoring_interface`는 `system_metrics`, `thread_pool_metrics` (`pool_name`/`pool_instance_id` 포함),
  `worker_metrics`, snapshot API를 정의합니다.
- `thread_pool`은 context를 통해 pool 메트릭(worker, idle count, queue size)을 보고합니다.
- 모니터링이 설정되지 않은 경우 `null_monitoring`을 사용하세요.

## 오류 처리 및 취소

- 작업에서 `result_void` / `result<T>`를 반환하며, 실패 시 `error_code`를 포함합니다.
- 협력적 취소를 위해 `cancellation_token`을 사용합니다 (연결 가능한 token, callback).

## 문서

Doxygen이 설치된 경우:

```bash
cmake --build build --target docs
# thread_system/documents/html/index.html 열기
```

인터페이스 세부사항을 포함한 완전한 API 문서는 `docs/API_REFERENCE.md`도 참조하세요.

## 샘플

### 핵심 Threading
- **minimal_thread_pool**: 최소 thread pool 사용법
- **thread_pool_sample**: 완전한 pool 라이프사이클 관리
- **typed_thread_pool_sample**: 타입 기반 우선순위 라우팅
- **typed_thread_pool_sample_2**: 고급 typed pool 사용법

### Queue 시스템
- **adaptive_queue_sample**: Adaptive vs lock-free vs mutex 비교
- **mpmc_queue_sample**: Multi-producer multi-consumer queue 사용법
- **hierarchical_queue_sample**: 우선순위 기반 job 큐잉
- **typed_job_queue_sample**: 타입별 job queue 작업

### 메모리 관리
- **hazard_pointer_sample**: Lock-free 데이터 구조를 위한 안전한 메모리 회수
- **node_pool_sample**: Memory pool 작업
- **memory_pooled_jobs_sample**: Job별 memory pooling

### 통합 및 서비스
- **composition_example**: 인터페이스를 사용한 의존성 주입
- **integration_example**: 외부 logger/monitoring 통합
- **service_registry_sample**: Service container 및 registry 사용법
- **multi_process_monitoring_integration**: 프로세스 인식 모니터링

### 고급 기능
- **builder_sample**: Thread pool builder 패턴
- **combined_optimizations_sample**: 다중 성능 최적화
- **data_oriented_job_sample**: 데이터 지향 job 처리
- **crash_protection**: 오류 처리 및 복구 메커니즘

### 외부 의존성 (선택적)
- **logger_sample**: 고성능 로깅 (별도의 logger_system 필요)
- **metrics_sample**: 실시간 메트릭 수집 (별도의 monitoring_system 필요)
