# Contributing to Thread System

Thank you for considering contributing to Thread System\! This document provides guidelines and instructions for contributors.

## Table of Contents

- [Getting Started](#getting-started)
- [Development Workflow](#development-workflow)
- [Code Standards](#code-standards)
- [Testing](#testing)
- [Pull Requests](#pull-requests)

## Getting Started

### Prerequisites

- C++20 compatible compiler (GCC 13+, Clang 16+, MSVC 2022+)
- CMake 3.20 or higher
- Git
- vcpkg (recommended)

### Building from Source

```bash
git clone https://github.com/kcenon/thread_system.git
cd thread_system
cmake --preset default
cmake --build build
```

### Cleaning Build Directories

To remove all build directories at once:

```bash
./scripts/clean.sh
```

> **Tip**: Prefer `cmake --preset <name>` over manual `mkdir build_*` directories.
> Named presets produce consistent, reproducible builds and avoid build directory proliferation.

### Running Tests

```bash
cd build
ctest --output-on-failure
```

## Development Workflow

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/your-feature`)
3. Make your changes
4. Run tests and ensure they pass
5. Commit your changes (see commit message guidelines below)
6. Push to your fork (`git push origin feature/your-feature`)
7. Open a Pull Request

### Commit Message Guidelines

Follow the [Conventional Commits](https://www.conventionalcommits.org/) format:

```
<type>(<scope>): <description>

[optional body]

[optional footer]
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `refactor`: Code refactoring
- `test`: Adding or modifying tests
- `perf`: Performance improvement
- `chore`: Maintenance tasks

## Code Standards

### C++ Style

- Use C++20 features when beneficial
- Follow existing code style (clang-format configuration provided)
- Prefer RAII for resource management
- Use `auto` for obvious types
- Avoid raw pointers; use smart pointers
- Prefer standard library over custom implementations

### Naming Conventions

- **Classes/Structs**: `snake_case`
- **Functions/Methods**: `snake_case`
- **Variables**: `snake_case`
- **Constants**: `UPPER_SNAKE_CASE`
- **Template Parameters**: `PascalCase`
- **Namespaces**: `kcenon::thread`

### Documentation

Use Doxygen-style comments:

```cpp
/**
 * @brief Brief description
 * @param param Parameter description
 * @return Return value description
 * @thread_safety Thread-safe / Not thread-safe
 */
auto function(int param) -> int;
```

## Testing

### Test Requirements

All code contributions must include tests:

- **Unit tests**: Test individual components
- **Integration tests**: Test component interactions
- **Platform tests**: Test platform-specific code paths

### Writing Tests

Use Google Test framework:

```cpp
#include <gtest/gtest.h>

TEST(ComponentTest, BasicFunctionality) {
    // Test implementation
}
```

### Test Coverage

Aim for:
- New code: > 80% coverage
- Critical paths: 100% coverage
- Error handling: Test failure scenarios

## Pull Requests

### PR Checklist

Before submitting a PR, ensure:

- [ ] Code compiles without warnings
- [ ] All tests pass
- [ ] New tests added for new functionality
- [ ] Documentation updated
- [ ] Code follows project style
- [ ] Commit messages follow conventional format
- [ ] No merge conflicts with main branch

### Review Process

1. Automated checks run (build, tests, linting)
2. Maintainer reviews code
3. Address review comments
4. Approved PR is merged (squash merge preferred)

## Getting Help

- Check [existing issues](https://github.com/kcenon/thread_system/issues)
- Open a [discussion](https://github.com/kcenon/thread_system/discussions) for questions

## License

By contributing, you agree that your contributions will be licensed under the BSD 3-Clause License.
