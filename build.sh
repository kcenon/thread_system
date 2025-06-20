#!/bin/bash

# Thread System Build Script
# Improved version with better error handling and more options

# Color definitions for better readability
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Display banner
echo -e "${BOLD}${BLUE}============================================${NC}"
echo -e "${BOLD}${BLUE}      Thread System Build Script           ${NC}"
echo -e "${BOLD}${BLUE}============================================${NC}"

# Display help information
show_help() {
    echo -e "${BOLD}Usage:${NC} $0 [options]"
    echo ""
    echo -e "${BOLD}Build Options:${NC}"
    echo "  --clean           Perform a clean rebuild by removing the build directory"
    echo "  --debug           Build in debug mode (default is release)"
    echo "  --benchmark       Build with benchmarks enabled"
    echo ""
    echo -e "${BOLD}Target Options:${NC}"
    echo "  --all             Build all targets (default)"
    echo "  --lib-only        Build only the core libraries"
    echo "  --samples         Build only the sample applications"
    echo "  --tests           Build and run the unit tests"
    echo ""
    echo -e "${BOLD}Documentation Options:${NC}"
    echo "  --docs            Generate Doxygen documentation"
    echo "  --clean-docs      Clean and regenerate Doxygen documentation"
    echo ""
    echo -e "${BOLD}Feature Options:${NC}"
    echo "  --no-format       Disable std::format even if supported"
    echo "  --no-jthread      Disable std::jthread even if supported"
    echo "  --no-span         Disable std::span even if supported"
    echo ""
    echo -e "${BOLD}General Options:${NC}"
    echo "  --cores N         Use N cores for compilation (default: auto-detect)"
    echo "  --verbose         Show detailed build output"
    echo "  --help            Display this help and exit"
    echo ""
}

# Function to print status messages
print_status() {
    echo -e "${BOLD}${BLUE}[STATUS]${NC} $1"
}

# Function to print success messages
print_success() {
    echo -e "${BOLD}${GREEN}[SUCCESS]${NC} $1"
}

# Function to print error messages
print_error() {
    echo -e "${BOLD}${RED}[ERROR]${NC} $1"
}

# Function to print warning messages
print_warning() {
    echo -e "${BOLD}${YELLOW}[WARNING]${NC} $1"
}

# Function to check if a command exists
command_exists() {
    command -v "$1" &> /dev/null
}

# Function to check and install dependencies
check_dependencies() {
    print_status "Checking build dependencies..."
    
    local missing_deps=()
    
    # Check for essential build tools
    for cmd in cmake git; do
        if ! command_exists "$cmd"; then
            missing_deps+=("$cmd")
        fi
    done
    
    # Check for at least one build system (make or ninja)
    if ! command_exists "make" && ! command_exists "ninja"; then
        missing_deps+=("make or ninja")
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing required dependencies: ${missing_deps[*]}"
        print_warning "Please install missing dependencies before building."
        print_warning "You can use './dependency.sh' to install all required dependencies."
        return 1
    fi
    
    # Check for vcpkg
    if [ ! -d "../vcpkg" ]; then
        print_warning "vcpkg not found in parent directory."
        print_warning "Running dependency script to set up vcpkg..."
        
        if [ -f "./dependency.sh" ]; then
            bash ./dependency.sh
            if [ $? -ne 0 ]; then
                print_error "Failed to run dependency.sh"
                return 1
            fi
        else
            print_error "dependency.sh script not found"
            return 1
        fi
    fi
    
    # Check for doxygen if building docs
    if [ $BUILD_DOCS -eq 1 ] && ! command_exists doxygen; then
        print_warning "Doxygen not found but documentation was requested."
        print_warning "Documentation will not be generated."
        BUILD_DOCS=0
    fi
    
    print_success "All dependencies are satisfied"
    return 0
}

# Process command line arguments
CLEAN_BUILD=0
BUILD_DOCS=0
CLEAN_DOCS=0
BUILD_TYPE="Release"
BUILD_BENCHMARKS=0
TARGET="all"
DISABLE_STD_FORMAT=0
DISABLE_STD_JTHREAD=0
DISABLE_STD_SPAN=0
BUILD_CORES=0
VERBOSE=0

while [[ $# -gt 0 ]]; do
    case $1 in
        --clean)
            CLEAN_BUILD=1
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --benchmark)
            BUILD_BENCHMARKS=1
            shift
            ;;
        --all)
            TARGET="all"
            shift
            ;;
        --lib-only)
            TARGET="lib-only"
            shift
            ;;
        --samples)
            TARGET="samples"
            shift
            ;;
        --tests)
            TARGET="tests"
            shift
            ;;
        --docs)
            BUILD_DOCS=1
            shift
            ;;
        --clean-docs)
            BUILD_DOCS=1
            CLEAN_DOCS=1
            shift
            ;;
        --no-format)
            DISABLE_STD_FORMAT=1
            shift
            ;;
        --no-jthread)
            DISABLE_STD_JTHREAD=1
            shift
            ;;
        --no-span)
            DISABLE_STD_SPAN=1
            shift
            ;;
        --cores)
            if [[ $2 =~ ^[0-9]+$ ]]; then
                BUILD_CORES=$2
                shift 2
            else
                print_error "Option --cores requires a numeric argument"
                exit 1
            fi
            ;;
        --verbose)
            VERBOSE=1
            shift
            ;;
        --help)
            show_help
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Set number of cores to use for building
if [ $BUILD_CORES -eq 0 ]; then
    if command_exists nproc; then
        BUILD_CORES=$(nproc)
    elif [ "$(uname)" == "Darwin" ]; then
        BUILD_CORES=$(sysctl -n hw.ncpu)
    else
        # Default to 2 if we can't detect
        BUILD_CORES=2
    fi
fi

print_status "Using $BUILD_CORES cores for compilation"

# Store original directory
ORIGINAL_DIR=$(pwd)

# Check for platform-specific settings
if [ "$(uname)" == "Linux" ]; then
    if [ $(uname -m) == "aarch64" ]; then
        export VCPKG_FORCE_SYSTEM_BINARIES=arm
        print_status "Detected ARM64 platform, setting VCPKG_FORCE_SYSTEM_BINARIES=arm"
    fi
fi

# Check dependencies before proceeding
check_dependencies
if [ $? -ne 0 ]; then
    print_error "Failed dependency check. Exiting."
    exit 1
fi

# Clean build if requested
if [ $CLEAN_BUILD -eq 1 ]; then
    print_status "Performing clean build..."
    rm -rf build
fi

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    print_status "Creating build directory..."
    mkdir -p build
fi

# Prepare CMake arguments
CMAKE_ARGS="-DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake"
CMAKE_ARGS+=" -DCMAKE_BUILD_TYPE=$BUILD_TYPE"

# Add feature flags based on options
if [ $DISABLE_STD_FORMAT -eq 1 ]; then
    CMAKE_ARGS+=" -DSET_STD_FORMAT=OFF -DFORCE_FMT_FORMAT=ON"
fi

if [ $DISABLE_STD_JTHREAD -eq 1 ]; then
    CMAKE_ARGS+=" -DSET_STD_JTHREAD=OFF"
fi

if [ $DISABLE_STD_SPAN -eq 1 ]; then
    CMAKE_ARGS+=" -DSET_STD_SPAN=OFF"
fi

if [ $BUILD_BENCHMARKS -eq 1 ]; then
    CMAKE_ARGS+=" -DBUILD_BENCHMARKS=ON"
fi

# Set submodule option if building library only
if [ "$TARGET" == "lib-only" ]; then
    CMAKE_ARGS+=" -DBUILD_THREADSYSTEM_AS_SUBMODULE=ON"
fi

# Enter build directory
cd build || { print_error "Failed to enter build directory"; exit 1; }

# Run CMake configuration
print_status "Configuring project with CMake..."
if [ "$(uname)" == "Darwin" ]; then
    # On macOS, use local modified GTest config
    cmake .. $CMAKE_ARGS -DCMAKE_PREFIX_PATH="$(pwd)"
else
    cmake .. $CMAKE_ARGS
fi

# Check if CMake configuration was successful
if [ $? -ne 0 ]; then
    print_error "CMake configuration failed. See the output above for details."
    cd "$ORIGINAL_DIR"
    exit 1
fi

# Build the project
print_status "Building project in $BUILD_TYPE mode..."

# Determine build target based on option
BUILD_TARGET=""
if [ "$TARGET" == "all" ]; then
    BUILD_TARGET="all"
elif [ "$TARGET" == "lib-only" ]; then
    BUILD_TARGET="thread_base thread_pool priority_thread_pool log_module"
elif [ "$TARGET" == "samples" ]; then
    BUILD_TARGET="builder_sample thread_pool_sample priority_thread_pool_sample logger_sample"
elif [ "$TARGET" == "tests" ]; then
    BUILD_TARGET="thread_base_unit thread_pool_unit typed_thread_pool_unit logger_unit utilities_unit"
fi

# Detect build system (Ninja or Make)
if [ -f "build.ninja" ]; then
    BUILD_COMMAND="ninja"
    if [ $VERBOSE -eq 1 ]; then
        BUILD_ARGS="-v"
    else
        BUILD_ARGS=""
    fi
elif [ -f "Makefile" ]; then
    BUILD_COMMAND="make"
    if [ $VERBOSE -eq 1 ]; then
        BUILD_ARGS="VERBOSE=1"
    else
        BUILD_ARGS=""
    fi
else
    print_error "No build system files found (neither build.ninja nor Makefile)"
    cd "$ORIGINAL_DIR"
    exit 1
fi

print_status "Using build system: $BUILD_COMMAND"

# Run build with appropriate target and cores
if [ "$BUILD_COMMAND" == "ninja" ]; then
    if [ -n "$BUILD_TARGET" ]; then
        $BUILD_COMMAND -j$BUILD_CORES $BUILD_ARGS $BUILD_TARGET
    else
        $BUILD_COMMAND -j$BUILD_CORES $BUILD_ARGS
    fi
elif [ "$BUILD_COMMAND" == "make" ]; then
    if [ -n "$BUILD_TARGET" ]; then
        $BUILD_COMMAND -j$BUILD_CORES $BUILD_ARGS $BUILD_TARGET
    else
        $BUILD_COMMAND -j$BUILD_CORES $BUILD_ARGS
    fi
fi

# Check if build was successful
if [ $? -ne 0 ]; then
    print_error "Build failed. See the output above for details."
    cd "$ORIGINAL_DIR"
    exit 1
fi

# Run tests if requested
if [ "$TARGET" == "tests" ]; then
    print_status "Running tests..."
    
    # Run tests individually
    test_failed=0
    for test in ./bin/*_unit; do
        if [ -x "$test" ]; then
            print_status "Running $(basename $test)..."
            $test
            if [ $? -ne 0 ]; then
                print_error "Test $(basename $test) failed"
                test_failed=1
            else
                print_success "Test $(basename $test) passed"
            fi
        fi
    done
    
    if [ $test_failed -eq 0 ]; then
        print_success "All tests passed!"
    else
        print_error "Some tests failed. See the output above for details."
    fi
fi

# Return to original directory
cd "$ORIGINAL_DIR"

# Show success message
print_success "Build completed successfully!"

# Generate documentation if requested
if [ $BUILD_DOCS -eq 1 ]; then
    print_status "Generating Doxygen documentation..."
    
    # Create docs directory if it doesn't exist
    if [ ! -d "docs" ]; then
        mkdir -p docs
    fi
    
    # Clean docs if requested
    if [ $CLEAN_DOCS -eq 1 ]; then
        print_status "Cleaning documentation directory..."
        rm -rf docs/*
    fi
    
    # Check if doxygen is installed
    if ! command_exists doxygen; then
        print_error "Doxygen is not installed. Please install it to generate documentation."
        exit 1
    fi
    
    # Run doxygen
    doxygen Doxyfile
    
    # Check if documentation generation was successful
    if [ $? -ne 0 ]; then
        print_error "Documentation generation failed. See the output above for details."
    else
        print_success "Documentation generated successfully in the docs directory!"
    fi
fi

# Final success message
echo -e "${BOLD}${GREEN}============================================${NC}"
echo -e "${BOLD}${GREEN}      Thread System Build Complete         ${NC}"
echo -e "${BOLD}${GREEN}============================================${NC}"

if [ -d "build/bin" ]; then
    echo -e "${CYAN}Available executables:${NC}"
    ls -la build/bin/
fi

echo -e "${CYAN}Build type:${NC} $BUILD_TYPE"
echo -e "${CYAN}Target:${NC} $TARGET"

if [ $BUILD_BENCHMARKS -eq 1 ]; then
    echo -e "${CYAN}Benchmarks:${NC} Enabled"
fi

if [ $BUILD_DOCS -eq 1 ]; then
    echo -e "${CYAN}Documentation:${NC} Generated"
fi

exit 0