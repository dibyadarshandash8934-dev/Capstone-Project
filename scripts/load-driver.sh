#!/bin/bash
# Load VNS kernel module

set -e

print_info() {
    echo -e "\033[0;32m[INFO]\033[0m $1"
}

print_error() {
    echo -e "\033[0;31m[ERROR]\033[0m $1"
}

print_warn() {
    echo -e "\033[1;33m[WARN]\033[0m $1"
}

# Check root
if [[ $EUID -ne 0 ]]; then
    print_error "This script requires root privileges. Run with sudo."
    exit 1
fi

MODULE_PATH="/Users/dibyadarshandash/Desktop/dsa visualizer/vns_new/kernel/vns_control/vns_control.ko"

if [[ ! -f "$MODULE_PATH" ]]; then
    print_error "Kernel module not found at $MODULE_PATH"
    print_info "Build it first: ./scripts/build-driver.sh"
    exit 1
fi

# Check if already loaded
if lsmod | grep -q "vns_control"; then
    print_warn "Module vns_control is already loaded"
    print_info "Unloading first..."
    rmmod vns_control
    sleep 1
fi

print_info "Loading vns_control kernel module..."
insmod "$MODULE_PATH"

sleep 1

# Verify
if lsmod | grep -q "vns_control"; then
    print_info "Module loaded successfully"
    lsmod | grep vns_control
    
    if [[ -c /dev/vns_control ]]; then
        print_info "Device node created: /dev/vns_control"
        ls -l /dev/vns_control
    else
        print_warn "Device node not created automatically"
    fi
    
    # Show recent kernel messages
    print_info "Recent kernel messages:"
    dmesg -T | grep -i vns | tail -10
else
    print_error "Module failed to load"
    dmesg -T | tail -20
    exit 1
fi

print_info "Done! Device available at /dev/vns_control"