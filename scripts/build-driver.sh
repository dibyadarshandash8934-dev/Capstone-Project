#!/bin/bash
# Build kernel module for VNS

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
KERNEL_DIR="$PROJECT_ROOT/kernel/vns_control"

print_info() {
    echo -e "\033[0;32m[INFO]\033[0m $1"
}

print_error() {
    echo -e "\033[0;31m[ERROR]\033[0m $1"
}

print_info "Building VNS Kernel Module"

# Check kernel headers
KERNEL_VERSION=$(uname -r)
KERNEL_HEADERS="/lib/modules/$KERNEL_VERSION/build"

if [[ ! -d "$KERNEL_HEADERS" ]]; then
    echo "Kernel headers not found at $KERNEL_HEADERS"
    echo "Install with: sudo apt-get install linux-headers-$(uname -r)"
    exit 1
fi

print_info "Kernel version: $(uname -r)"
print_info "Kernel headers: $KERNEL_HEADERS"

cd "$KERNEL_DIR"

print_info "Building kernel module..."
make clean
make

if [[ -f vns_control.ko ]]; then
    print_info "Build successful: vns_control.ko"
    ls -lh vns_control.ko
else
    echo "Build failed - module not created"
    exit 1
fi