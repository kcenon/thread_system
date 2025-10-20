# 자주 묻는 질문 (FAQ)

> **Language:** [English](faq.md) | **한국어**

## 일반 질문

### Q: Thread System이란 무엇인가요?
**A:** Thread System은 동시성 애플리케이션 작성을 위한 포괄적인 프레임워크를 제공하는 C++20 라이브러리입니다. thread 관리, thread pool, 타입 기반 스케줄링, 동시성 로깅을 위한 컴포넌트를 포함합니다.

### Q: Thread System은 어떤 C++ 표준을 요구하나요?
**A:** Thread System은 C++20용으로 설계되었지만 부분적인 C++20 지원을 가진 컴파일러를 위한 fallback을 포함합니다. `std::format`, `std::jthread`, `std::chrono::current_zone`과 같은 기능의 가용성을 감지하고 적응할 수 있습니다.

### Q: Thread System은 모든 주요 플랫폼에서 작동하나요?
**A:** 예, Thread System은 Windows, macOS, Linux에서 작동합니다. 각 플랫폼별 최적화가 포함되어 있으며 빌드 시스템은 모든 환경에서 작동하도록 설계되었습니다.

### Q: Thread System은 외부 의존성이 필요한가요?
**A:** Thread System은 컴파일러에서 `std::format`을 사용할 수 없는 경우 {fmt} 라이브러리에 의존합니다. 단위 테스트를 위해서는 GoogleTest를 사용합니다. 이러한 의존성은 vcpkg를 통해 관리됩니다.

### Q: Thread System은 thread-safe한가요?
**A:** 예, Thread System의 모든 public interface는 thread-safe하게 설계되었습니다. 내부 데이터 구조는 thread safety를 보장하기 위해 적절한 동기화 메커니즘을 사용합니다.

## Thread Base 질문

### Q: `thread_base`와 `std::thread`의 차이점은 무엇인가요?
**A:** `thread_base`는 `std::thread` (또는 가능한 경우 `std::jthread`) 위에 구축된 고수준 추상화입니다. 다음을 제공합니다:
- 구조화된 생명주기 (start, do_work, stop)
- virtual method를 통한 사용자 정의 지점
- 간격 기반 wake-up 기능
- 통합 에러 처리
- 상태 추적을 통한 향상된 관리 가능성

### Q: 커스텀 worker thread를 어떻게 생성하나요?
**A:** `thread_base`를 상속하고 필요에 따라 virtual method를 재정의합니다. 자세한 패턴과 예제는 [patterns 문서](PATTERNS.md#worker-thread-pattern)를 참조하세요.

### Q: worker thread를 주기적으로 깨우려면 어떻게 하나요?
**A:** `set_wake_interval` method를 사용합니다. 최적화 가이드라인은 [patterns 문서](PATTERNS.md#wake-interval-optimization)를 참조하세요.

### Q: worker thread method에서 에러를 어떻게 처리하나요?
**A:** `result_void` 타입을 사용하여 에러를 반환합니다. 포괄적인 에러 처리 패턴은 [patterns 문서](PATTERNS.md#best-practices)를 참조하세요.

## Thread Pool 질문

### Q: thread pool에 몇 개의 worker thread를 생성해야 하나요?
**A:** 좋은 경험 규칙은 시스템에서 사용 가능한 hardware thread 수를 사용하는 것입니다. 다양한 워크로드 유형에 대한 자세한 크기 조정 가이드라인은 [patterns 문서](PATTERNS.md#thread-pool-sizing-guidelines)를 참조하세요.

### Q: 여러 작업에 thread pool을 재사용할 수 있나요?
**A:** 예, thread pool은 재사용되도록 설계되었습니다. pool에 계속 job을 제출할 수 있으며 worker thread는 사용 가능해지면 처리합니다.

### Q: thread pool의 모든 job이 완료될 때까지 어떻게 기다리나요?
**A:** thread pool의 `stop()` method를 호출하면 모든 worker thread가 현재 job 처리를 완료할 때까지 기다린 후 반환됩니다.

### Q: job이 exception을 throw하면 어떻게 되나요?
**A:** Job은 exception을 throw해서는 안 됩니다. 대신 `std::optional<std::string>` 또는 `result_void` 반환 값을 사용하여 에러를 반환해야 합니다. job이 exception을 throw하면 worker thread가 이를 catch하고 에러로 변환한 후 로깅합니다.

### Q: thread pool에 제출된 job의 결과를 어떻게 처리하나요?
**A:** 여러 접근 방식이 있습니다:
1. job이 공유 상태를 업데이트하도록 함 (적절한 동기화 포함)
2. job이 결과 collector에 결과를 보고하는 callback 메커니즘 사용
3. 결과 queue와 함께 future/promise 패턴 구현

## Type Thread Pool 질문

### Q: `thread_pool`과 `typed_thread_pool`의 차이점은 무엇인가요?
**A:** `typed_thread_pool`은 job 타입 지원을 추가하여 `thread_pool`을 확장합니다. Job은 타입에 따라 처리되며, 높은 타입의 job이 낮은 타입보다 먼저 처리됩니다.

### Q: 몇 개의 타입 레벨을 사용해야 하나요?
**A:** 대부분의 애플리케이션은 3-5개의 타입 레벨로 잘 작동합니다. 그 이상은 큰 이점 없이 복잡성을 초래할 수 있습니다. 표준 enum은 좋은 출발점으로 `High`, `Normal`, `Low`를 제공합니다.

### Q: 커스텀 타입 타입을 생성할 수 있나요?
**A:** 예, `typed_thread_pool`은 비교 연산자를 제공하는 모든 타입과 작동할 수 있는 template 클래스입니다. 애플리케이션의 타입 시스템을 나타내는 커스텀 enum 또는 클래스를 정의할 수 있습니다.

### Q: 특정 타입 레벨에 worker를 어떻게 할당하나요?
**A:** worker를 생성할 때 처리해야 할 타입 레벨을 지정합니다. 완전한 타입 기반 실행 패턴은 [patterns 문서](PATTERNS.md#type-based-job-execution-pattern)를 참조하세요.

### Q: 특정 타입 레벨에 대한 worker가 없으면 어떻게 되나요?
**A:** 해당 타입 레벨의 job은 처리할 수 있는 worker가 사용 가능해지거나 thread pool이 중지될 때까지 queue에 남아 있습니다.

## 로깅 질문

### Q: 로깅은 현재 어디에 구현되어 있나요?
**A:** Thread System은 로깅 interface (`logger_interface`)만 제공합니다. 구체적인 구현은 별도 프로젝트 [logger_system](https://github.com/kcenon/logger_system)에 있습니다. `service_container` 또는 `logger_registry`를 통해 logger 구현을 등록하여 통합합니다.

### Q: 로깅은 thread-safe한가요?
**A:** 권장되는 `logger_system` 구현의 경우 예 (설계상 thread-safe). 자체 logger를 제공하는 경우, 여러 worker thread에서 로깅이 발생할 수 있으므로 thread-safe한지 확인하세요.

### Q: 로그가 작성되는 위치를 어떻게 구성하나요?
**A:** 구성은 logger 구현 (예: `logger_system`)에서 제공됩니다. Thread System에서는 logger를 주입합니다, 예:
```cpp
// 전역 logger 등록
thread_module::service_container::global()
  .register_singleton<thread_module::logger_interface>(my_logger);

// 또는
thread_module::logger_registry::set_logger(my_logger);
```
sink/target 및 레벨에 대해서는 logger의 자체 문서를 참조하세요.

### Q: 로그 형식을 어떻게 사용자 정의하나요?
**A:** logger 구현의 기능 (format 문자열, 구조화된 로깅)을 사용합니다. Thread System은 `logger_interface`를 통해 메시지를 변경하지 않고 전달합니다.

### Q: 로깅이 애플리케이션 성능에 영향을 미치나요?
**A:** 비동기 logger (`logger_system`과 같은)를 사용하고 tight loop에서 과도한 로깅을 피하세요. 적절한 로그 레벨을 선택하고 빈번한 이벤트에 대해 샘플링 또는 집계를 고려하세요.

### Q: 로그 rotation을 어떻게 처리하나요?
**A:** 로그 rotation/retention은 logger 구현에서 처리됩니다. rotation 정책 및 구성에 대해서는 `logger_system` 문서 (또는 선택한 logger)를 참조하세요.

## 성능 질문

### Q: Thread System의 성능을 어떻게 측정하나요?
**A:** 표준 C++ 프로파일링 도구 (perf, gprof, Visual Studio Profiler)를 사용하여 애플리케이션의 성능을 측정합니다. Thread System 자체는 내장 프로파일링을 포함하지 않지만 외부 profiler와 잘 작동하도록 설계되었습니다.

### Q: raw thread와 비교하여 Thread System 사용의 오버헤드는 무엇인가요?
**A:** Thread System은 raw thread 조작과 비교하여 작은 오버헤드 (일반적으로 작업당 마이크로초)를 추가하지만, 코드 구성, 에러 처리 및 유지 관리성 측면의 이점이 대부분의 애플리케이션에서 이 비용을 상쇄합니다.

### Q: Thread System은 thread 경합을 어떻게 처리하나요?
**A:** Thread System은 경합을 최소화하기 위해 신중한 설계와 함께 표준 C++ 동기화 primitives (mutex, condition variable)를 사용합니다. 특히 job queue 구현은 높은 처리량 시나리오에서 lock 경합을 줄이도록 최적화되어 있습니다.

### Q: thread pool에 제출할 수 있는 job 수에 제한이 있나요?
**A:** 실질적인 제한은 시스템의 메모리에 의해 결정됩니다. Job은 필요에 따라 동적으로 증가하는 queue에 저장됩니다. 그러나 최상의 성능을 위해 job을 일괄 처리하고 매우 작은 job을 극도로 많이 queue에 넣지 않는 것을 고려하세요.

## 문제 해결

다음과 같은 일반적인 동시성 문제에 대한 솔루션을 포함한 포괄적인 문제 해결 지침은:
- Thread pool 처리 문제
- Deadlock 및 race condition
- 성능 문제
- 디버깅 전략

[patterns 문서](PATTERNS.md#troubleshooting-common-issues)를 참조하세요.

## 빌드 및 통합

### Q: Thread System을 내 프로젝트에 어떻게 포함하나요?
**A:** Thread System을 독립 실행형 프로젝트 또는 submodule로 포함할 수 있습니다. CMake 프로젝트의 경우 `add_subdirectory()`를 사용하여 포함하고 `BUILD_THREADSYSTEM_AS_SUBMODULE=ON`을 설정합니다.

### Q: Thread System을 소스에서 어떻게 빌드하나요?
**A:** 제공된 빌드 스크립트를 사용합니다:
```bash
./build.sh         # 기본 빌드
./build.sh --clean # 클린 빌드
./build.sh --docs  # 문서와 함께 빌드
```

### Q: Thread System 단위 테스트를 어떻게 실행하나요?
**A:** 기본 옵션으로 빌드한 후 bin 디렉토리에 있는 테스트를 실행합니다:
```bash
./bin/thread_pool_test
./bin/typed_thread_pool_test
./bin/utilities_test
```

### Q: Thread System을 빌드하는 데 필요한 CMake 버전은 무엇인가요?
**A:** Thread System은 CMake 3.16 이상이 필요합니다.

### Q: CMake가 아닌 프로젝트에서 Thread System을 사용할 수 있나요?
**A:** 예, 하지만 빌드 시스템을 구성하여 적절한 소스 파일을 포함하고 필요한 컴파일 플래그 (C++20 지원, thread 라이브러리 링킹 등)를 설정해야 합니다.
