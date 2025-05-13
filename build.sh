#!/bin/bash

# Display help information
show_help() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  --clean           Perform a clean rebuild by removing the build directory"
    echo "  --docs            Generate Doxygen documentation"
    echo "  --clean-docs      Clean and regenerate Doxygen documentation"
    echo "  --help            Display this help and exit"
    echo ""
}

# Process command line arguments
CLEAN_BUILD=0
BUILD_DOCS=0
CLEAN_DOCS=0

for arg in "$@"; do
    case $arg in
        --clean)
            CLEAN_BUILD=1
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
        --help)
            show_help
            exit 0
            ;;
        *)
            echo "Unknown option: $arg"
            show_help
            exit 1
            ;;
    esac
done

ORIGINAL_DIR=$(pwd)

if [ "$(uname)" == "Linux" ]; then
    if [ $(uname -m) == "aarch64" ]; then
        export VCPKG_FORCE_SYSTEM_BINARIES=arm
    fi
fi

# Clean build if requested
if [ $CLEAN_BUILD -eq 1 ]; then
    echo "Performing clean build..."
    rm -rf build
    mkdir build
else
    # Create build directory if it doesn't exist
    if [ ! -d "build" ]; then
        echo "Creating build directory..."
        mkdir build
    fi
fi

pushd build

# Always run cmake
echo "Configuring project with CMake..."
cmake .. -DCMAKE_TOOLCHAIN_FILE="../../vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building project..."
if [ $CLEAN_BUILD -eq 1 ]; then
    # Force rebuild everything with -B flag
    make -B
else
    # Normal build - only rebuild what's necessary
    make
fi

export LC_ALL=C
unset LANGUAGE

popd

cd "$ORIGINAL_DIR"

# Show success message
echo "Build completed successfully!"

# Generate documentation if requested
if [ $BUILD_DOCS -eq 1 ]; then
    echo "Generating Doxygen documentation..."
    
    # Create docs directory if it doesn't exist
    if [ ! -d "docs" ]; then
        mkdir -p docs
    fi
    
    # Clean docs if requested
    if [ $CLEAN_DOCS -eq 1 ]; then
        echo "Cleaning documentation directory..."
        rm -rf docs/*
    fi
    
    # Check if doxygen is installed
    if ! command -v doxygen &> /dev/null; then
        echo "Error: Doxygen is not installed. Please install it to generate documentation."
        exit 1
    fi
    
    # Run doxygen
    doxygen Doxyfile
    
    echo "Documentation generated successfully in the docs directory!"
fi