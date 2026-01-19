# Logger Interface 마이그레이션 가이드

> **Language:** [English](LOGGER_INTERFACE_MIGRATION_GUIDE.md) | **한국어**

**버전:** 1.0
**날짜:** 2025-12-06
**적용 대상:** thread_system v1.x 이상

## 개요

이 가이드는 deprecated된 `kcenon::thread::logger_interface`에서 common_system의 통합 `kcenon::common::interfaces::ILogger`로 마이그레이션하는 방법을 설명합니다.

> **⚠️ Breaking Change (v3.0.0):** thread_system의 로컬 로거 인터페이스가 v3.0.0에서 **제거**되었습니다 (Issue #311). 이전 인터페이스를 사용 중이라면 즉시 `common::interfaces::ILogger`로 마이그레이션해야 합니다.

## 마이그레이션 이유

### thread_system의 logger_interface 문제점

1. **코드 중복**: common_system에서 이미 제공하는 로깅 인터페이스를 중복 정의
2. **유지보수 부담**: 기능이 분기되는 두 개의 인터페이스 유지 필요
3. **호환되지 않는 타입**: 어댑터 없이 thread_system 로거를 common_system에 전달 불가
4. **역순 로그 레벨**: 업계 표준과 반대인 critical=0...trace=5 사용

### common_system의 ILogger 장점

1. **통합 인터페이스**: 모든 시스템(thread_system, logger_system 등)에서 단일 인터페이스 사용
2. **Result 기반 오류 처리**: 일관된 오류 처리를 위한 `VoidResult` 반환
3. **표준 로그 레벨 순서**: trace=0...critical=5 (spdlog, log4j, syslog와 일치)
4. **확장된 기능**: 명명된 로거, ILoggerRegistry, 구조화된 로깅 지원
5. **향상된 통합**: 시스템 간 원활한 상호 운용성

## Deprecation 일정

| 단계 | 버전 | 상태 | 설명 |
|------|------|------|------|
| 1단계 | v1.x | 완료 | Deprecation 경고 추가 |
| 2단계 | v1.x | 완료 | 마이그레이션 문서 |
| 3단계 | v3.0 | **완료** | 파일 완전 제거 (Issue #311) |

## 마이그레이션 단계

### 1단계: Include 문 업데이트

**변경 전:**
```cpp
#include <kcenon/thread/interfaces/logger_interface.h>
```

**변경 후:**
```cpp
#include <kcenon/common/interfaces/logger_interface.h>
```

### 2단계: 네임스페이스 및 타입 참조 업데이트

**변경 전:**
```cpp
using kcenon::thread::logger_interface;
using kcenon::thread::logger_registry;
using kcenon::thread::log_level;

class MyLogger : public logger_interface {
    void log(log_level level, const std::string& message) override;
    void log(log_level level, const std::string& message,
             const std::string& file, int line, const std::string& function) override;
    bool is_enabled(log_level level) const override;
    void flush() override;
};
```

**변경 후:**
```cpp
using kcenon::common::interfaces::ILogger;
using kcenon::common::interfaces::ILoggerRegistry;
using kcenon::common::interfaces::log_level;

class MyLogger : public ILogger {
    void log(log_level level, const std::string& message) override;
    void log(log_level level, const std::string& message,
             const std::string& file, int line, const std::string& function) override;
    bool is_enabled(log_level level) const override;
    void flush() override;
};
```

### 3단계: 로그 레벨 값 업데이트

두 인터페이스 간의 로그 레벨 순서가 **역전**되어 있습니다:

| 레벨 | thread::log_level | common::log_level |
|------|-------------------|-------------------|
| trace | 5 | 0 |
| debug | 4 | 1 |
| info | 3 | 2 |
| warning | 2 | 3 |
| error | 1 | 4 |
| critical | 0 | 5 |

**변경 전 (thread_system - 역순):**
```cpp
// 높은 심각도 = 낮은 값 (비표준)
if (static_cast<int>(level) <= static_cast<int>(min_level)) {
    // 메시지 로깅
}
```

**변경 후 (common_system - 표준 순서):**
```cpp
// 높은 심각도 = 높은 값 (표준)
if (level >= min_level) {
    // 메시지 로깅
}
```

### 4단계: 비교문 업데이트

**변경 전:**
```cpp
// thread_system: critical=0, trace=5
if (level <= log_level::warning) {
    // warning, error, critical 로깅
}
```

**변경 후:**
```cpp
// common_system: trace=0, critical=5
if (level >= log_level::warning) {
    // warning, error, critical 로깅
}
```

### 5단계: Registry 사용 업데이트

**변경 전:**
```cpp
// 전역 로거 설정
kcenon::thread::logger_registry::set_logger(my_logger);

// 전역 로거 가져오기
auto logger = kcenon::thread::logger_registry::get_logger();

// 로거 제거
kcenon::thread::logger_registry::clear_logger();
```

**변경 후:**
```cpp
// 명명된 로거를 위한 ILoggerRegistry 사용
auto& registry = kcenon::common::interfaces::ILoggerRegistry::instance();

// 이름으로 등록
registry.register_logger("my_app", my_logger);

// 이름으로 가져오기
auto logger = registry.get_logger("my_app");

// 기본 로거 가져오기 또는 생성
auto default_logger = registry.get_default_logger();
```

### 6단계: 매크로 업데이트 (사용 시)

`THREAD_LOG_*` 매크로는 deprecated 되었습니다. 직접 로거 호출 또는 자체 매크로로 대체하세요.

**변경 전:**
```cpp
THREAD_LOG_ERROR("Something went wrong");
THREAD_LOG_INFO("Operation completed");
```

**변경 후:**
```cpp
// 옵션 1: 직접 로거 호출
if (auto logger = get_logger()) {
    logger->log(log_level::error, "Something went wrong", __FILE__, __LINE__, __FUNCTION__);
}

// 옵션 2: ILogger를 사용하는 자체 매크로 정의
#define APP_LOG(level, msg) \
    do { \
        if (auto logger = MyApp::get_logger()) { \
            if (logger->is_enabled(level)) { \
                logger->log(level, msg, __FILE__, __LINE__, __FUNCTION__); \
            } \
        } \
    } while(0)
```

## 전체 마이그레이션 예제

### 마이그레이션 전

```cpp
#include <kcenon/thread/interfaces/logger_interface.h>

namespace myapp {

class ConsoleLogger : public kcenon::thread::logger_interface {
public:
    void log(kcenon::thread::log_level level, const std::string& message) override {
        if (static_cast<int>(level) <= static_cast<int>(min_level_)) {
            std::cout << "[" << level_to_string(level) << "] " << message << "\n";
        }
    }

    void log(kcenon::thread::log_level level, const std::string& message,
             const std::string& file, int line, const std::string& function) override {
        if (static_cast<int>(level) <= static_cast<int>(min_level_)) {
            std::cout << "[" << level_to_string(level) << "] "
                      << file << ":" << line << " " << message << "\n";
        }
    }

    bool is_enabled(kcenon::thread::log_level level) const override {
        return static_cast<int>(level) <= static_cast<int>(min_level_);
    }

    void flush() override {
        std::cout.flush();
    }

private:
    kcenon::thread::log_level min_level_ = kcenon::thread::log_level::info;

    std::string level_to_string(kcenon::thread::log_level level) {
        switch (level) {
            case kcenon::thread::log_level::critical: return "CRIT";
            case kcenon::thread::log_level::error: return "ERROR";
            case kcenon::thread::log_level::warning: return "WARN";
            case kcenon::thread::log_level::info: return "INFO";
            case kcenon::thread::log_level::debug: return "DEBUG";
            case kcenon::thread::log_level::trace: return "TRACE";
            default: return "???";
        }
    }
};

void setup() {
    auto logger = std::make_shared<ConsoleLogger>();
    kcenon::thread::logger_registry::set_logger(logger);
}

} // namespace myapp
```

### 마이그레이션 후

```cpp
#include <kcenon/common/interfaces/logger_interface.h>

namespace myapp {

class ConsoleLogger : public kcenon::common::interfaces::ILogger {
public:
    void log(kcenon::common::interfaces::log_level level,
             const std::string& message) override {
        if (level >= min_level_) {
            std::cout << "[" << level_to_string(level) << "] " << message << "\n";
        }
    }

    void log(kcenon::common::interfaces::log_level level,
             const std::string& message,
             const std::string& file, int line, const std::string& function) override {
        if (level >= min_level_) {
            std::cout << "[" << level_to_string(level) << "] "
                      << file << ":" << line << " " << message << "\n";
        }
    }

    bool is_enabled(kcenon::common::interfaces::log_level level) const override {
        return level >= min_level_;
    }

    void flush() override {
        std::cout.flush();
    }

private:
    kcenon::common::interfaces::log_level min_level_
        = kcenon::common::interfaces::log_level::info;

    std::string level_to_string(kcenon::common::interfaces::log_level level) {
        switch (level) {
            case kcenon::common::interfaces::log_level::trace: return "TRACE";
            case kcenon::common::interfaces::log_level::debug: return "DEBUG";
            case kcenon::common::interfaces::log_level::info: return "INFO";
            case kcenon::common::interfaces::log_level::warning: return "WARN";
            case kcenon::common::interfaces::log_level::error: return "ERROR";
            case kcenon::common::interfaces::log_level::critical: return "CRIT";
            default: return "???";
        }
    }
};

void setup() {
    auto logger = std::make_shared<ConsoleLogger>();
    auto& registry = kcenon::common::interfaces::ILoggerRegistry::instance();
    registry.set_default_logger(logger);
}

} // namespace myapp
```

## FAQ

### Q: common_system이 없으면 어떻게 하나요?

A: 프로젝트에서 common_system을 사용하지 않는 경우 두 가지 옵션이 있습니다:
1. common_system을 의존성으로 추가 (권장)
2. 마이그레이션할 수 있을 때까지 deprecated 인터페이스 계속 사용

### Q: deprecation 후에도 코드가 컴파일되나요?

A: 예, v1.x 릴리스에서는 가능합니다. 컴파일러 경고가 표시되지만 코드는 작동합니다. v2.0에서는 deprecated 인터페이스가 제거되어 컴파일이 실패합니다.

### Q: deprecation 경고를 일시적으로 무시하려면?

A: 컴파일러별 pragma를 사용하세요:
```cpp
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

// deprecated 인터페이스를 사용하는 코드

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
```

### Q: 점진적 마이그레이션을 위한 어댑터가 있나요?

A: 예, 다양한 로거 인터페이스 간을 연결하는 `logger_system_adapter` 사용을 고려해 보세요.

## 관련 문서

- [LOG_LEVEL_MIGRATION_GUIDE.kr.md](LOG_LEVEL_MIGRATION_GUIDE.kr.md) - 로그 레벨 enum 마이그레이션
- [이슈 #263](https://github.com/kcenon/thread_system/issues/263) - Deprecation 추적 이슈

## 개정 이력

| 버전 | 날짜 | 변경 사항 |
|------|------|----------|
| 1.0 | 2025-12-06 | 최초 릴리스 |
