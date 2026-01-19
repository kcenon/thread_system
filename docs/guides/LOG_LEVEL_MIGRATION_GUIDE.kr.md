# 로그 레벨 마이그레이션 가이드

> **Language:** [English](LOG_LEVEL_MIGRATION_GUIDE.md) | **한국어**

**버전:** 1.0
**작성일:** 2025-11-26
**적용 대상:** thread_system v2.x 이상

## 개요

이 가이드는 레거시 `log_level` enum에서 thread_system에 새로 도입된 `log_level_v2` enum으로 마이그레이션하는 방법을 안내합니다. 새 enum은 **오름차순** (trace=0부터 critical=5)을 사용하며, 이는 산업 표준(syslog, log4j, spdlog) 및 unified_system 생태계의 나머지 부분과 일치합니다.

## 마이그레이션이 필요한 이유

### `log_level_v2`의 장점

1. **자연스러운 비교 의미론**: `if (level >= log_level_v2::warn)`이 직관적으로 동작
2. **생태계 정렬**: logger_system 및 common_system과 일치
3. **산업 표준**: syslog RFC 5424, log4j, spdlog 규약 준수
4. **단순화된 어댑터**: 시스템 간 변환 불필요

### 순서 비교

| 레벨 | 레거시 `log_level` | 새 `log_level_v2` |
|------|-------------------|-------------------|
| trace | 0 | 0 |
| debug | 1 | 1 |
| info | 2 | 2 |
| warning/warn | 3 | 3 |
| error | 4 | 4 |
| critical | 5 | 5 |
| off | N/A | 6 |

> **참고:** thread_system의 레거시 `log_level`은 이미 내부적으로 오름차순을 사용합니다. 이 마이그레이션은 주로 명시적 문서화, `off` 레벨, 그리고 더 나은 상호운용성 API를 제공합니다.

## 마이그레이션 단계

### 1단계: 새 헤더 포함

```cpp
// 이 include 추가
#include <kcenon/thread/core/log_level.h>
```

### 2단계: 타입 선언 업데이트

**이전:**
```cpp
using kcenon::thread::log_level;

void set_log_level(log_level level);
log_level current_level = log_level::info;
```

**이후:**
```cpp
using kcenon::thread::log_level_v2;

void set_log_level(log_level_v2 level);
log_level_v2 current_level = log_level_v2::info;
```

### 3단계: 레벨 비교 업데이트

새 API는 더 명확한 의미론을 위해 `should_log()` 헬퍼를 제공합니다:

**이전:**
```cpp
if (static_cast<int>(level) >= static_cast<int>(min_level)) {
    // 메시지 로깅
}
```

**이후:**
```cpp
if (should_log(level, min_level)) {
    // 메시지 로깅
}

// 또는 자연스러운 비교 사용
if (level >= min_level) {
    // 메시지 로깅
}
```

### 4단계: 문자열 파싱 업데이트

**이전:**
```cpp
// 커스텀 파싱 로직
log_level parse_level(const std::string& str) {
    if (str == "debug") return log_level::debug;
    // ...
}
```

**이후:**
```cpp
// 내장 파서 사용
auto level = kcenon::thread::parse_log_level("debug");
```

### 5단계: 문자열 변환 업데이트

**이전:**
```cpp
std::string level_to_string(log_level level) {
    switch (level) {
        case log_level::debug: return "DEBUG";
        // ...
    }
}
```

**이후:**
```cpp
// 내장 변환기 사용
auto str = kcenon::thread::to_string(log_level_v2::debug);  // "DEBUG" 반환
```

## 변환 헬퍼

점진적 마이그레이션을 위한 변환 헬퍼가 제공됩니다:

```cpp
#include <kcenon/thread/core/log_level.h>

using namespace kcenon::thread;

// 레거시에서 새 버전으로 변환
log_level old_level = log_level::error;
log_level_v2 new_level = to_v2(old_level);

// 새 버전에서 레거시로 변환
log_level_v2 new_level = log_level_v2::warn;
log_level old_level = from_v2(new_level);
```

## `off` 레벨 사용

새 `log_level_v2::off` 레벨은 완전한 로깅 억제를 허용합니다:

```cpp
void configure_logging(bool enabled) {
    if (enabled) {
        set_min_level(log_level_v2::trace);  // 모든 것 로깅
    } else {
        set_min_level(log_level_v2::off);    // 아무것도 로깅하지 않음
    }
}

// should_log로 확인
if (should_log(log_level_v2::critical, log_level_v2::off)) {
    // 이것은 로깅되지 않음 - off는 모든 것을 비활성화
}
```

## 완전한 예제

### 마이그레이션 전

```cpp
#include <kcenon/thread/thread_logger.h>

using namespace kcenon::thread;

class MyLogger {
public:
    void log(log_level level, const std::string& message) {
        if (static_cast<int>(level) >= static_cast<int>(min_level_)) {
            // 메시지 로깅
            std::cout << "[" << level_to_string(level) << "] " << message << "\n";
        }
    }

    void set_min_level(log_level level) {
        min_level_ = level;
    }

private:
    log_level min_level_ = log_level::info;

    std::string level_to_string(log_level level) {
        switch (level) {
            case log_level::trace: return "TRACE";
            case log_level::debug: return "DEBUG";
            case log_level::info: return "INFO";
            case log_level::warning: return "WARN";
            case log_level::error: return "ERROR";
            case log_level::critical: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }
};
```

### 마이그레이션 후

```cpp
#include <kcenon/thread/core/log_level.h>

using namespace kcenon::thread;

class MyLogger {
public:
    void log(log_level_v2 level, const std::string& message) {
        if (should_log(level, min_level_)) {
            std::cout << "[" << to_string(level) << "] " << message << "\n";
        }
    }

    void set_min_level(log_level_v2 level) {
        min_level_ = level;
    }

    void disable_logging() {
        min_level_ = log_level_v2::off;
    }

private:
    log_level_v2 min_level_ = log_level_v2::info;
};
```

## 다른 시스템과의 상호운용성

### logger_system과 함께

```cpp
#include <kcenon/thread/core/log_level.h>
#include <kcenon/logger/log_level.h>

// 둘 다 오름차순을 사용하므로 직접 캐스트 가능
auto thread_level = kcenon::thread::log_level_v2::error;
auto logger_level = static_cast<kcenon::logger::log_level>(
    static_cast<int>(thread_level)
);
```

### common_system과 함께

```cpp
#include <kcenon/thread/core/log_level.h>
#include <kcenon/common/log_level.h>

// 호환되는 순서로 직접 비교 가능
assert(static_cast<int>(thread::log_level_v2::error) ==
       static_cast<int>(common::log_level::error));
```

## FAQ

### Q: 레거시 `log_level`이 deprecated 되었나요?

A: 현재 두 enum 모두 사용 가능합니다. 레거시 enum은 향후 마이너 버전에서 deprecated 되고 다음 메이저 버전(v3.0)에서 제거될 예정입니다.

### Q: 즉시 마이그레이션해야 하나요?

A: 아니요. 레거시 enum은 계속 작동합니다. 그러나 다음 경우에 마이그레이션을 권장합니다:
- 새 코드
- logger_system 또는 common_system과 상호작용하는 코드
- 더 명확한 비교 의미론이 필요한 코드

### Q: `warning` vs `warn`?

A: `log_level_v2`는 `warn`을 사용합니다(spdlog/log4j 규약과 일치). 파서는 입력에 대해 `warning`과 `warn` 모두 허용합니다.

### Q: 레거시 코드에서 `off` 레벨을 어떻게 처리하나요?

A: `log_level_v2::off`를 레거시로 변환할 때 `critical`로 매핑됩니다. `off` 기능이 필요하면 새 API를 사용하세요.

## 타임라인

| 단계 | 버전 | 상태 |
|------|------|------|
| Phase 1: `log_level_v2` 추가 | v2.x (현재) | 완료 |
| Phase 2: 문서화 & 예제 | v2.x | 진행 중 |
| Phase 3: 레거시 deprecate | v2.x+1 | 계획됨 |
| Phase 4: 레거시 제거 | v3.0 | 계획됨 |

## 관련 문서

- [LOG_LEVEL_UNIFICATION_PLAN.md](../advanced/LOG_LEVEL_UNIFICATION_PLAN.md)
- [ERROR_SYSTEM_MIGRATION_GUIDE.md](../advanced/ERROR_SYSTEM_MIGRATION_GUIDE.md)
- [logger_system LOG_LEVEL_SEMANTIC_STANDARD.md](../../../logger_system/docs/advanced/LOG_LEVEL_SEMANTIC_STANDARD.md)

## 개정 이력

| 버전 | 날짜 | 변경 사항 |
|------|------|----------|
| 1.0 | 2025-11-26 | 최초 릴리스 |
