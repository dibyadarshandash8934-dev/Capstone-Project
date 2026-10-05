#!/bin/bash
# Clean build artifacts for VNS

set -e

print_info() {
    echo -e "\033[0;32m[INFO]\033[0m $1"
}

print_warn() {
    echo -e "\033[1;33m[WARN]\033[0m $1"
}

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

print_info "Cleaning VNS build artifacts..."

# Clean C++ build
if [[ -d "$PROJECT_ROOT/build" ]]; then
    rm -rf "$PROJECT_ROOT/build"
    print_info "Removed C++ build directory"
fi

# Clean kernel module
if [[ -d "$PROJECT_ROOT/kernel/vns_control" ]]; then
    cd "$PROJECT_ROOT/kernel/vns_control"
    if [[ -f Makefile ]]; then
        make clean 2>/dev/null || true
        print_info "Cleaned kernel module build"
    fi
fi

# Clean Python cache
find "$PROJECT_ROOT" -name "__pycache__" -type d -exec rm -rf {} + 2>/dev/null || true
find "$PROJECT_ROOT" -name "*.pyc" -delete 2>/dev/null || true
print_info "Removed Python cache files"

# Clean test artifacts
find "$PROJECT_ROOT" -name "*.gcda" -delete 2>/dev/null || true
find "$PROJECT_ROOT" -name "*.gcno" -delete 2>/dev/null || true
find "$PROJECT_ROOT" -name "coverage.info" -delete 2>/dev/null || true
rm -rf "$PROJECT_ROOT/coverage_report" 2>/dev/null || true
print_info "Removed coverage artifacts"

print_info "Clean complete!"