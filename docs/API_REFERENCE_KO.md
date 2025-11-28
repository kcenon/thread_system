# thread_system API 레퍼런스

> **버전**: 2.0
> **최종 업데이트**: 2025-11-21
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
- `typed_thread_pool` - 타입 안전 스레드 풀
- `mpmc_queue` - Multi-Producer Multi-Consumer 큐
- `spsc_queue` - Single-Producer Single-Consumer 큐
- `adaptive_queue` - 적응형 큐
- 동기화 프리미티브

---

## thread_pool (권장)

### 개요

**헤더**: `#include <kcenon/thread/thread_pool.h>`

**설명**: 고성능 태스크 기반 스레드 풀 (4.5배 성능 향상)

**주요 기능**:
- Work-stealing 알고리즘
- Future/Promise 패턴
- 우선순위 기반 태스크 실행
- 동적 워커 스케일링

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
| 처리량 | 1.2M ops/sec | Work-stealing |
| 지연시간 (p50) | 0.8 μs | 태스크 스케줄링 |
| 확장성 | 16코어까지 선형에 가까움 | 95%+ 효율 |

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

### mpmc_queue

**헤더**: `#include <kcenon/thread/queues/mpmc_queue.h>`

**설명**: Multi-Producer Multi-Consumer 큐 (5.2배 성능 향상)

**알고리즘**: Lock-free 링 버퍼

**성능**: 2.1M ops/sec

#### 사용 예시

```cpp
#include <kcenon/thread/queues/mpmc_queue.h>

using namespace kcenon::thread;

// 생성 (용량: 1024)
mpmc_queue<int> queue(1024);

// 프로듀서 스레드
std::thread producer([&queue]() {
    for (int i = 0; i < 1000; ++i) {
        queue.enqueue(i);
    }
});

// 컨슈머 스레드
std::thread consumer([&queue]() {
    int value;
    while (queue.dequeue(value)) {
        process(value);
    }
});
```

#### 핵심 메서드

```cpp
bool enqueue(const T& item);        // 큐에 추가
bool dequeue(T& item);              // 큐에서 제거
bool try_enqueue(const T& item);    // 논블로킹 추가
bool try_dequeue(T& item);          // 논블로킹 제거
size_t size() const;                // 현재 크기
bool empty() const;                 // 비어있는지 확인
```

---

### spsc_queue

**헤더**: `#include <kcenon/thread/queues/spsc_queue.h>`

**설명**: Single-Producer Single-Consumer 큐

**알고리즘**: Lock-free 순환 버퍼

**성능**: 3.5M ops/sec

#### 사용 예시

```cpp
#include <kcenon/thread/queues/spsc_queue.h>

using namespace kcenon::thread;

// 생성 (용량: 1024)
spsc_queue<int> queue(1024);

// 프로듀서 스레드 (단일만!)
std::thread producer([&queue]() {
    for (int i = 0; i < 1000; ++i) {
        queue.push(i);
    }
});

// 컨슈머 스레드 (단일만!)
std::thread consumer([&queue]() {
    int value;
    while (queue.pop(value)) {
        process(value);
    }
});
```

---

### adaptive_queue

**헤더**: `#include <kcenon/thread/queues/adaptive_queue.h>`

**설명**: 부하 기반 자동 리사이징 큐

**알고리즘**: 동적 리사이징 큐

**성능**: 1.5M ops/sec

#### 사용 예시

```cpp
#include <kcenon/thread/queues/adaptive_queue.h>

using namespace kcenon::thread;

// 생성 (초기 용량: 128)
adaptive_queue<int> queue(128);

// 설정
queue.set_high_watermark(0.8);   // 80% 사용 시 확장
queue.set_low_watermark(0.2);    // 20% 사용 시 축소

// 사용 (자동 리사이징)
for (int i = 0; i < 10000; ++i) {
    queue.enqueue(i);
    // 부하에 따라 큐가 자동으로 리사이징됨
}
```

---

## 동기화 프리미티브

### hazard_pointer

**헤더**: `#include <kcenon/thread/sync/hazard_pointer.h>`

**설명**: Lock-free 구조를 위한 안전한 메모리 회수

**주요 기능**:
- ABA 문제 완화
- 자동 가비지 컬렉션
- 스레드 안전 메모리 회수

#### 사용 예시

```cpp
#include <kcenon/thread/sync/hazard_pointer.h>

using namespace kcenon::thread;

// 해저드 포인터 도메인 생성
hazard_pointer_domain<Node> hp_domain;

// 해저드 포인터 획득
auto hp = hp_domain.acquire();

// 포인터 보호
Node* node = load_node();
hp.protect(node);

// 노드를 안전하게 사용
process(node);

// 해제 (소멸자에서 자동)
```

---

### spinlock

**헤더**: `#include <kcenon/thread/sync/spinlock.h>`

**설명**: 저지연 스핀락

#### 사용 예시

```cpp
#include <kcenon/thread/sync/spinlock.h>

using namespace kcenon::thread;

spinlock lock;

// RAII와 함께 사용
{
    std::lock_guard<spinlock> guard(lock);
    // 크리티컬 섹션
}
```

---

### rw_lock

**헤더**: `#include <kcenon/thread/sync/rw_lock.h>`

**설명**: 읽기-쓰기 락 (읽기 중심 워크로드에 최적화)

#### 사용 예시

```cpp
#include <kcenon/thread/sync/rw_lock.h>

using namespace kcenon::thread;

rw_lock lock;

// 읽기
{
    std::shared_lock<rw_lock> read_guard(lock);
    // 읽기 작업
}

// 쓰기
{
    std::unique_lock<rw_lock> write_guard(lock);
    // 쓰기 작업
}
```

---

## 성능 비교

### 큐 성능 비교

| 큐 타입 | 처리량 | 지연시간 | 사용 사례 |
|---------|------------|---------|----------|
| **spsc_queue** | 3.5M ops/sec | 0.29 μs | 파이프라인, 단일 프로듀서/컨슈머 |
| **mpmc_queue** | 2.1M ops/sec | 0.48 μs | 고처리량 태스크 큐 |
| **adaptive_queue** | 1.5M ops/sec | 0.67 μs | 가변 부하 시스템 |

### 스레드 풀 성능 비교

| 풀 타입 | 처리량 | 지연시간 (p50) | 타입 안전 |
|----------|------------|---------------|-------------|
| **thread_pool** | 1.2M ops/sec | 0.8 μs | 런타임 |
| **typed_thread_pool** | 980K ops/sec | 1.0 μs | 컴파일 타임 |

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

### v1.x에서 v2.0으로

**변경사항**:
- Work-stealing 알고리즘 구현
- Lock-free 큐 최적화 (5.2배)
- 타입 스레드 풀 추가
- 적응형 큐 추가
- 해저드 포인터 구현

**마이그레이션 예시**:
```cpp
// v1.x
thread_pool pool(4);
auto future = pool.submit_task(task);

// v2.0
thread_pool pool(4);
auto future = pool.enqueue(task);
```

---

## 참고사항

### 스레드 안전성

- **thread_pool**: 스레드 안전 (모든 메서드)
- **typed_thread_pool**: 스레드 안전 (모든 메서드)
- **mpmc_queue**: 스레드 안전 (다중 프로듀서/컨슈머)
- **spsc_queue**: 스레드 안전 (단일 프로듀서, 단일 컨슈머만)
- **adaptive_queue**: 스레드 안전 (다중 프로듀서/컨슈머)

### 권장사항

- **일반 태스크 처리**: `thread_pool` 사용 (권장)
- **타입 안전 필요**: `typed_thread_pool` 사용
- **고처리량 큐**: `mpmc_queue` 사용
- **파이프라인**: `spsc_queue` 사용
- **가변 부하**: `adaptive_queue` 사용

---

**작성일**: 2025-11-21
**버전**: 2.0
**관리자**: kcenon@naver.com
