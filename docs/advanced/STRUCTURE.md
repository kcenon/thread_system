---
doc_id: "THR-ARCH-008"
doc_title: "Thread System - New Structure"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "thread_system"
category: "ARCH"
---

# Thread System - New Structure

**English** | [한국어](STRUCTURE.kr.md)

## Directory Layout

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

## Namespace Structure

- Root: `kcenon::thread`
- Core functionality: `kcenon::thread::core`
- Interfaces: `kcenon::thread::interfaces`
- Implementation details: `kcenon::thread::impl`
- Utilities: `kcenon::thread::utils`

## Migration Notes

1. Old structure backed up in `old_structure/` directory
2. Compatibility header provided at `include/kcenon/thread/compatibility.h`
3. Run `./migrate_namespaces.sh` to update all namespaces
4. Update CMakeLists.txt to reflect new structure
