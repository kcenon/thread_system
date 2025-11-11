# Thread System - 새로운 구조

[English](STRUCTURE.md) | **한국어**

## 디렉토리 구조

```
thread_system/
├── include/kcenon/thread/       # Public headers
│   ├── core/                   # Core APIs
│   ├── interfaces/             # Interface definitions
│   └── utils/                  # Utilities
├── src/                        # Implementation
│   ├── core/                   # Core implementation
│   ├── impl/                   # Private implementations
│   │   ├── thread_pool/        # Thread pool impl
│   │   └── typed_pool/         # Typed thread pool impl
│   └── utils/                  # Utility implementations
├── tests/                      # All tests
│   ├── unit/                   # Unit tests
│   ├── integration/            # Integration tests
│   └── benchmarks/             # Performance tests
├── examples/                   # Usage examples
├── docs/                       # Documentation
└── CMakeLists.txt             # Build configuration
```

## Namespace 구조

- 루트: `kcenon::thread`
- 핵심 기능: `kcenon::thread::core`
- 인터페이스: `kcenon::thread::interfaces`
- 구현 세부사항: `kcenon::thread::impl`
- 유틸리티: `kcenon::thread::utils`

## 마이그레이션 노트

1. 기존 구조는 `old_structure/` 디렉토리에 백업됨
2. 호환성 헤더는 `include/kcenon/thread/compatibility.h`에서 제공됨
3. 모든 namespace를 업데이트하려면 `./migrate_namespaces.sh` 실행
4. 새로운 구조를 반영하도록 CMakeLists.txt 업데이트
