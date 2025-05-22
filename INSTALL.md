# Installation Guide

This guide provides comprehensive instructions for building and installing the Thread System library.

## System Requirements

### Minimum Requirements

| Component | Requirement |
|-----------|-------------|
| **Compiler** | GCC 9+, Clang 10+, MSVC 2019+ |
| **C++ Standard** | C++17 (C++20 recommended) |
| **CMake** | 3.16 or later |
| **Memory** | 512 MB RAM for compilation |
| **Disk Space** | 200 MB for full build |

### Supported Platforms

| Platform | Architecture | Status |
|----------|-------------|---------|
| **Linux** | x86_64, ARM64 | ✅ Fully Supported |
| **Windows** | x86_64 | ✅ Fully Supported |
| **macOS** | x86_64, ARM64 (M1/M2) | ✅ Fully Supported |

## Quick Installation

### One-Command Setup

```bash
# Clone and build
git clone https://github.com/kcenon/thread_system.git
cd thread_system

# Install dependencies and build
./dependency.sh && ./build.sh    # Linux/macOS
./dependency.bat && ./build.bat  # Windows
```

## Detailed Installation

### Step 1: Clone Repository

```bash
git clone https://github.com/kcenon/thread_system.git
cd thread_system
```

### Step 2: Install Dependencies

```bash
# Linux/macOS
./dependency.sh

# Windows
./dependency.bat
```

### Step 3: Build Project

```bash
# Linux/macOS
./build.sh

# Windows
./build.bat
```

### Step 4: Run Tests (Linux only)

```bash
cd build
ctest --verbose
```

**Note**: Tests are disabled on macOS due to pthread/gtest linkage issues.

## Platform-Specific Instructions

### Linux (Ubuntu/Debian)

```bash
# Prerequisites
sudo apt update
sudo apt install -y build-essential cmake git curl zip unzip tar

# Build
export CC=gcc
export CXX=g++
./build.sh
```

### Windows (Visual Studio)

```cmd
# Set environment for Visual Studio
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

# Build project
.\dependency.bat
.\build.bat
```

### macOS

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Build project
./dependency.sh
./build.sh
```

## Configuration Options

### CMake Configuration

```bash
# Basic configuration
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

# Advanced configuration
cmake -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DSET_STD_FORMAT=ON \
    -DSET_STD_JTHREAD=ON \
    -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
```

## Verification

### Test Installation

```bash
# Run sample applications
./build/bin/thread_pool_sample
./build/bin/priority_thread_pool_sample
./build/bin/logger_sample
```

### Integration Test

Create a simple test program:

```cpp
// test_integration.cpp
#include "thread_pool.h"
#include "logger.h"
#include <iostream>

using namespace thread_pool_module;

int main() {
    log_module::start();
    
    auto [pool, error] = create_default(2);
    if (error.has_value()) {
        std::cerr << "Failed to create pool: " << *error << std::endl;
        return 1;
    }
    
    std::cout << "Thread System integration test: SUCCESS" << std::endl;
    return 0;
}
```

Compile and run:
```bash
g++ -std=c++20 -I build/include test_integration.cpp -L build/lib -lthread_system -o test_integration
./test_integration
```

## Troubleshooting

### Common Issues

#### 1. CMake can't find vcpkg toolchain

**Solution**:
```bash
export CMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake -B build
```

#### 2. Compiler not found

**Solution**:
```bash
export CXX=g++
export CC=gcc
cmake -B build
```

#### 3. Missing dependencies

**Solution**:
```bash
rm -rf vcpkg/
./dependency.sh
```

## Integration Guide

### Using as Submodule

```bash
# Add as git submodule
git submodule add https://github.com/kcenon/thread_system.git external/thread_system

# In your CMakeLists.txt
add_subdirectory(external/thread_system)
target_link_libraries(your_target PRIVATE thread_system)
```

### Using with CMake FetchContent

```cmake
include(FetchContent)

FetchContent_Declare(
    thread_system
    GIT_REPOSITORY https://github.com/kcenon/thread_system.git
    GIT_TAG v1.0.0
)

FetchContent_MakeAvailable(thread_system)
target_link_libraries(your_target PRIVATE thread_system)
```

## Support

If you encounter issues:

1. Check existing [GitHub issues](https://github.com/kcenon/thread_system/issues)
2. Create new issue with build logs and system information
3. Ask questions in [GitHub Discussions](https://github.com/kcenon/thread_system/discussions)

## Next Steps

After successful installation:

1. Read the [README.md](README.md) for project overview
2. Explore [samples](samples/) for usage examples
3. Review [docs/architecture.md](docs/architecture.md) for design details
4. Check [docs/performance_tuning.md](docs/performance_tuning.md) for optimization tips