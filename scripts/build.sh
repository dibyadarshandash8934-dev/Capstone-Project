#!/bin/bash
# Build script for VNS Virtual NAT Gateway Simulator
# Usage: ./scripts/build.sh [clean|debug|release]

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
BUILD_TYPE="${1:-Release}"

# Normalise the build type so "debug"/"release" map to the CMake values.
case "$(printf '%s' "$BUILD_TYPE" | tr '[:lower:]' '[:upper:]')" in
    DEBUG)   BUILD_TYPE="Debug" ;;
    RELEASE) BUILD_TYPE="Release" ;;
    RELWITHDEBINFO) BUILD_TYPE="RelWithDebInfo" ;;
    MINSIZEREL) BUILD_TYPE="MinSizeRel" ;;
    CLEAN)   BUILD_TYPE="clean" ;;
esac

echo "========================================="
echo "VNS Virtual NAT Gateway Simulator Build"
echo "========================================="
echo "Build type: $BUILD_TYPE"
echo "Project root: $PROJECT_ROOT"
echo "Build directory: $BUILD_DIR"
echo ""

# Clean if requested
if [[ "$1" == "clean" ]]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
    echo "Clean complete."
    exit 0
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring with CMake (Build type: $BUILD_TYPE)..."
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" "$PROJECT_ROOT"

# Build
if command -v nproc >/dev/null 2>&1; then
    JOBS="$(nproc)"
elif command -v sysctl >/dev/null 2>&1 && sysctl -n hw.ncpu >/dev/null 2>&1; then
    JOBS="$(sysctl -n hw.ncpu)"
else
    JOBS=4
fi
echo "Building with $JOBS parallel jobs..."
make -j"$JOBS"

echo ""
echo "========================================="
echo "Build completed successfully!"
echo "========================================="
echo ""
echo "Executable: $BUILD_DIR/vns_sim"
echo "Tests:      $BUILD_DIR/vns_tests"
echo ""
echo "To run:"
echo "  ./vns_sim"
echo "  ./vns_tests"
echo ""
echo "To build kernel module:"
echo "  cd $PROJECT_ROOT/kernel/vns_control && make"
echo ""
echo "To run tests:"
echo "  ./vns_tests"
echo "  ctest --output-on-failure"