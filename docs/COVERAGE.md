# Code Coverage Guide

## Build with Coverage

```bash
cmake -S . -B build -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
cmake --build build --target coverage
```

Outputs are generated under `build/coverage/`.

Notes
- Requires `lcov` and `genhtml`.
- On macOS, install via Homebrew: `brew install lcov`.

