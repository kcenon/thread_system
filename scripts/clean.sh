#!/bin/bash
# Clean all build directories
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "Cleaning build directories in $(basename "$PROJECT_DIR")..."
find "$PROJECT_DIR" -maxdepth 1 -type d -name "build*" -exec rm -rf {} +
echo "Done. Use CMakePresets.json for reproducible build configurations."
