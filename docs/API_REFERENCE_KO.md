# thread_system API 레퍼런스

> **버전**: 3.0.0
> **최종 업데이트**: 2025-12-19
> **언어**: [English](API_REFERENCE.md) | [한국어]

## 목차

1. [네임스페이스](#네임스페이스)
2. [thread_pool (권장)](#thread_pool-권장)
3. [typed_thread_pool](#typed_thread_pool)
4. [Lock-Free 큐](#lock-free-큐)
5. [동기화 프리미티브](#동기화-프리미티브)

---

## 네임스페이스

### `kcenon::thread`

thread_system의 모든 공개 API는 이 네임스페이스에 포함됩니다.

**포함 항목**:
- `thread_pool` - 태스크 기반 스레드 풀
- `typed_thread_pool_t` - 타입 안전 스레드 풀
- `concurrent_queue<T>` - 스레드 안전 MPMC 큐
- `lockfree_job_queue` - Lock-free 작업 큐
- `adaptive_job_queue` - 적응형 작업 큐
- 동기화 프리미티브

---

## thread_pool (권장)

### 개요

**헤더**: `#include <kcenon/thread/thread_pool.h>`

**설명**: 고성능 태스크 기반 스레드 풀

**주요 기능**:
- Future/Promise 패턴
- 우선순위 기반 태스크 실행 (선택사항)
- 동적 워커 설정
- Common system IExecutor 통합 (사용 가능 시)

> **참고**: Work-stealing은 계획 중이지만 아직 구현되지 않았습니다. 설정 옵션은 `config.h`를 참조하세요.

### 생성 및 사용

```cpp
#include <kcenon/thread/thread_pool.h>

using namespace kcenon::thread;

// 4개의 워커 스레드로 생성
thread_pool pool(4);

// 태스크 추가 (결과 있음)
auto future = pool.enqueue([]() {
    return 42;
});

// 결과 대기
int result = future.get();  // 42

// Fire-and-forget 태스크
pool.enqueue([]() {
    std::cout << "백그라운드 태스크" << std::endl;
});
```

### 핵심 메서드

#### `enqueue()`

```cpp
template<typename F, typename... Args>
auto enqueue(F&& func, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>;
```

**설명**: 태스크를 큐에 추가하고 Future 반환

**매개변수**:
- `func`: 실행할 함수 또는 람다
- `args...`: 함수 인수

**반환값**: `std::future<Result>` - 태스크 결과

**예시**:
```cpp
// 매개변수 없는 함수
auto future1 = pool.enqueue([]() { return 42; });

// 매개변수 있는 함수
auto future2 = pool.enqueue([](int x, int y) { return x + y; }, 10, 20);

// 멤버 함수
struct Calculator {
    int add(int x, int y) { return x + y; }
};
Calculator calc;
auto future3 = pool.enqueue(&Calculator::add, &calc, 10, 20);
```

#### `thread_count()`

```cpp
size_t thread_count() const noexcept;
```

**설명**: 워커 스레드 수 반환

### 성능 특성

| 메트릭 | 값 | 비고 |
|--------|-------|-------|
| 처리량 | ~1M ops/sec | 워크로드에 따라 다름 |
| 지연시간 (p50) | ~1 μs | 태스크 스케줄링 |
| 확장성 | 16코어까지 선형에 가까움 | 태스크 크기에 따라 다름 |

> **참고**: 실제 성능은 워크로드 특성, 하드웨어 및 설정에 따라 다릅니다.

---

## typed_thread_pool

### 개요

**헤더**: `#include <kcenon/thread/typed_thread_pool.h>`

**설명**: 타입 안전 스레드 풀 (3.8배 성능 향상)

**주요 기능**:
- 컴파일 타임 타입 안전성
- 자동 타입 추론
- 커스텀 처리 함수
- 적응형 큐 관리

### 생성 및 사용

```cpp
#include <kcenon/thread/typed_thread_pool.h>

using namespace kcenon::thread;

// int 타입 태스크 처리용 스레드 풀
typed_thread_pool<int> pool(4);

// 처리 함수 설정
pool.set_process_function([](int value) {
    std::cout << "처리 중: " << value << std::endl;
    return value * 2;
});

// 타입 안전 태스크 추가
pool.enqueue(10);
pool.enqueue(20);
pool.enqueue(30);

// 시작
pool.start();
```

### 핵심 메서드

#### `enqueue()`

```cpp
template<typename T>
void enqueue(T&& item);
```

**설명**: 타입 안전 태스크 추가

#### `set_process_function()`

```cpp
void set_process_function(std::function<void(T)> func);
```

**설명**: 태스크 처리 함수 설정

#### `start()` / `stop()`

```cpp
void start();
void stop();
```

**설명**: 스레드 풀 시작/정지

### 성능 특성

| 메트릭 | 값 | 비고 |
|--------|-------|-------|
| 처리량 | 980K ops/sec | 타입 안전 |
| 지연시간 (p50) | 1.0 μs | 큐 오버헤드 |
| 메모리 오버헤드 | +10% vs 일반 | 타입 메타데이터 |

---

## Lock-Free 큐

### concurrent_queue

**헤더**: `#include <kcenon/thread/lockfree/lockfree_queue.h>`

**설명**: 블로킹 대기를 지원하는 스레드 안전 MPMC 큐

**알고리즘**: 분리된 head/tail 뮤텍스를 사용하는 세밀한 잠금

> **참고**: `lockfree/` 디렉토리에 있지만, 이 구현은 진정한 lock-free 알고리즘이 아닌
> 세밀한 잠금(fine-grained locking)을 사용합니다. `lockfree_queue<T>` 별칭은 deprecated되었으며,
> 대신 `concurrent_queue<T>`를 사용하세요.

**주요 기능**:
- 다중 프로듀서/컨슈머의 스레드 안전한 동시 접근
- 타임아웃을 지원하는 블로킹 대기
- 이동 생성 가능한 모든 타입에서 동작
- enqueue와 dequeue 작업 간 낮은 경합

#### 사용 예시

```cpp
#include <kcenon/thread/lockfree/lockfree_queue.h>

using namespace kcenon::thread;

concurrent_queue<std::string> queue;

// 프로듀서 스레드
std::thread producer([&queue]() {
    queue.enqueue("message");
});

// 컨슈머 스레드 (논블로킹)
std::thread consumer([&queue]() {
    if (auto value = queue.try_dequeue()) {
        process(*value);
    }
});

// 컨슈머 스레드 (타임아웃 블로킹)
if (auto value = queue.wait_dequeue(std::chrono::milliseconds{100})) {
    process(*value);
}
```

#### 핵심 메서드

```cpp
void enqueue(T value);                              // 큐에 추가
std::optional<T> try_dequeue();                     // 논블로킹 제거
std::optional<T> wait_dequeue(duration timeout);    // 타임아웃 블로킹 제거
size_t size() const;                                // 현재 크기 (근사값)
bool empty() const;                                 // 비어있는지 확인
void shutdown();                                    // 종료 신호
```

---

### adaptive_job_queue

**헤더**: `#include <kcenon/thread/queue/adaptive_job_queue.h>`

**설명**: 뮤텍스 모드와 Lock-free 모드를 자동 전환하는 적응형 큐

**알고리즘**: 정책 기반 동적 모드 전환

**주요 기능**:
- 뮤텍스 기반 및 Lock-free 큐 구현을 래핑
- 다중 선택 정책 (정확도, 성능, 균형, 수동)
- 임시 정확도 모드를 위한 RAII 가드
- 데이터 마이그레이션과 함께 스레드 안전한 모드 전환

#### 사용 예시

```cpp
#include <kcenon/thread/queue/adaptive_job_queue.h>

using namespace kcenon::thread;

// 적응형 큐 생성 (기본값: balanced 정책)
auto queue = std::make_unique<adaptive_job_queue>();

// 일반 큐처럼 사용
queue->enqueue(std::make_unique<my_job>());
auto job = queue->dequeue();

// 임시로 정확도 요구
{
    auto guard = queue->require_accuracy();
    size_t exact = queue->size();  // 이제 정확한 값 보장
}

// 특정 정책으로 생성
auto perf_queue = std::make_unique<adaptive_job_queue>(
    adaptive_job_queue::policy::performance_first
);
```

#### 정책

| 정책 | 설명 |
|------|------|
| `accuracy_first` | 항상 뮤텍스 모드 사용 |
| `performance_first` | 항상 Lock-free 모드 사용 |
| `balanced` | 사용량에 따라 자동 전환 |
| `manual` | 사용자가 모드 제어 |

#### 핵심 메서드

```cpp
auto enqueue(std::unique_ptr<job>&& j) -> common::VoidResult;
auto dequeue() -> std::unique_ptr<job>;
size_t size() const;
bool empty() const;
auto require_accuracy() -> accuracy_guard;  // 정확한 크기를 위한 RAII 가드
void set_mode(mode m);                      // 모드 전환 (manual 정책만)
mode current_mode() const;                  // 현재 모드 조회
```

---

## 동기화 프리미티브

### safe_hazard_pointer

**헤더**: `#include <kcenon/thread/core/safe_hazard_pointer.h>`

**설명**: Lock-free 데이터 구조를 위한 스레드 안전 해저드 포인터 구현

**주요 기능**:
- ARM 및 약한 메모리 모델 아키텍처를 위한 명시적 메모리 순서
- 자동 등록이 있는 스레드별 해저드 포인터 레코드
- 설정 가능한 임계값을 사용한 지연 회수
- 적절한 동기화를 통한 스레드 안전

> **참고**: 레거시 `hazard_pointer.h`는 메모리 순서 문제(TICKET-002)로 인해 더 이상 사용되지 않습니다.
> 대신 `safe_hazard_pointer.h` 또는 `atomic_shared_ptr.h`를 사용하세요.

#### 사용 예시

```cpp
#include <kcenon/thread/core/safe_hazard_pointer.h>

using namespace kcenon::thread;

// 해저드 포인터 도메인 생성
safe_hazard_pointer_domain<Node> hp_domain;

// 레코드 획득
auto* record = hp_domain.acquire();

// 포인터 보호 (기본값: 슬롯 0)
Node* node = shared_head.load();
record->protect(node);

// 노드를 안전하게 사용 - 다른 스레드가 회수하지 않음
process(node);

// 완료 시 보호 해제
record->clear();

// 지연 회수를 위해 노드 퇴역
hp_domain.retire(old_node);
```

---

### scoped_lock_guard

**헤더**: `#include <kcenon/thread/core/sync_primitives.h>`

**설명**: 선택적 타임아웃을 지원하는 RAII 기반 스코프 락 가드

**주요 기능**:
- 자동 락 획득 및 해제
- 타이밍 뮤텍스를 위한 선택적 타임아웃
- 예외 안전 락 관리

#### 사용 예시

```cpp
#include <kcenon/thread/core/sync_primitives.h>

using namespace kcenon::thread::sync;

std::mutex mtx;

// 기본 사용
{
    scoped_lock_guard<std::mutex> guard(mtx);
    // 크리티컬 섹션
}

// 타임아웃 사용 (timed_mutex용)
std::timed_mutex timed_mtx;
{
    scoped_lock_guard<std::timed_mutex> guard(
        timed_mtx, std::chrono::milliseconds{100}
    );
    if (guard.owns_lock()) {
        // 락 획득됨
    }
}
```

---

### atomic_shared_ptr

**헤더**: `#include <kcenon/thread/core/atomic_shared_ptr.h>`

**설명**: shared_ptr에 대한 스레드 안전 원자적 연산 (해저드 포인터 대안)

**주요 기능**:
- shared_ptr에 대한 Lock-free 원자적 load/store/exchange
- 해저드 포인터보다 간단한 API
- 표준 shared_ptr과 호환

#### 사용 예시

```cpp
#include <kcenon/thread/core/atomic_shared_ptr.h>

using namespace kcenon::thread;

atomic_shared_ptr<Node> shared_node;

// 원자적 저장
shared_node.store(std::make_shared<Node>());

// 원자적 로드
auto node = shared_node.load();

// 원자적 교환
auto old = shared_node.exchange(std::make_shared<Node>());
```

---

## 성능 비교

### 큐 성능 비교

| 큐 타입 | 설명 | 사용 사례 |
|---------|------|----------|
| **job_queue** | 뮤텍스 기반, 정확한 크기 | 정확한 크기가 중요할 때 |
| **lockfree_job_queue** | Lock-free MPMC | 고처리량 태스크 큐 |
| **concurrent_queue<T>** | 블로킹 대기 지원 스레드 안전 큐 | 일반 MPMC 사용 사례 |
| **adaptive_job_queue** | 모드 자동 전환 | 가변 부하 시스템 |

> **참고**: 실제 성능은 워크로드 특성과 하드웨어에 따라 다릅니다.

### 스레드 풀 비교

| 풀 타입 | 타입 안전 | 사용 사례 |
|---------|-----------|----------|
| **thread_pool** | 런타임 | 범용 태스크 실행 |
| **typed_thread_pool_t** | 컴파일 타임 | 우선순위 기반 작업 스케줄링 |

---

## 사용 예시

### 기본 사용

```cpp
#include <kcenon/thread/thread_pool.h>

using namespace kcenon::thread;

int main() {
    // 4개의 워커 스레드로 스레드 풀 생성
    thread_pool pool(4);

    // 태스크 추가
    auto future = pool.enqueue([]() {
        return 42;
    });

    // 결과 대기
    int result = future.get();
    std::cout << "결과: " << result << std::endl;

    return 0;
}
```

### 타입 안전 스레드 풀

```cpp
#include <kcenon/thread/typed_thread_pool.h>

using namespace kcenon::thread;

int main() {
    // int 타입 태스크 처리용 스레드 풀
    typed_thread_pool<int> pool(4);

    // 타입 안전 태스크 추가
    pool.enqueue(10);
    pool.enqueue(20);
    pool.enqueue(30);

    // 태스크 처리 함수 등록
    pool.set_process_function([](int value) {
        std::cout << "처리 중: " << value << std::endl;
    });

    pool.start();

    // 처리 대기
    std::this_thread::sleep_for(std::chrono::seconds(1));

    pool.stop();

    return 0;
}
```

---

## 마이그레이션 가이드

### v2.x에서 v3.0으로

**변경사항**:
- `common_system` Result/Error 타입으로 마이그레이션
- 레거시 `thread::result<T>` 및 `thread::error` 타입 제거
- `hazard_pointer.h` 폐기 (`safe_hazard_pointer.h` 사용)
- 안정적인 umbrella 헤더 추가 (`<kcenon/thread/thread_pool.h>`)
- `shared_interfaces.h` 제거 - `common_system` 인터페이스 사용

**마이그레이션 예시**:
```cpp
// v2.x - 레거시 include
#include <kcenon/thread/core/thread_pool.h>

// v3.0 - 안정적인 umbrella 헤더
#include <kcenon/thread/thread_pool.h>

// v2.x - thread-specific Result
thread::result<int> result = ...;

// v3.0 - common system Result
common::Result<int> result = ...;
```

---

## 참고사항

### 스레드 안전성

- **thread_pool**: 스레드 안전 (모든 메서드)
- **typed_thread_pool_t**: 스레드 안전 (모든 메서드)
- **concurrent_queue<T>**: 스레드 안전 (다중 프로듀서/컨슈머)
- **lockfree_job_queue**: 스레드 안전 (다중 프로듀서/컨슈머)
- **job_queue**: 스레드 안전 (뮤텍스 기반)
- **adaptive_job_queue**: 스레드 안전 (다중 프로듀서/컨슈머)

### 권장사항

- **일반 태스크 처리**: `thread_pool` 사용 (권장)
- **우선순위 기반 스케줄링**: `typed_thread_pool_t` 사용
- **고처리량 큐**: `lockfree_job_queue` 사용
- **범용 타입 큐**: `concurrent_queue<T>` 사용
- **가변 부하**: `adaptive_job_queue` 사용
- **안전한 메모리 회수**: `safe_hazard_pointer` 또는 `atomic_shared_ptr` 사용

---

**작성일**: 2025-11-21
**업데이트**: 2025-12-19
**버전**: 3.0.0
**관리자**: kcenon@naver.com
