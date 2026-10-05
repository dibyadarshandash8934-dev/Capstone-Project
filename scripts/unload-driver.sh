#!/bin/bash
# Unload VNS kernel module

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
    echo -e "\033[0;31m[ERROR]\033[0m This script requires root privileges. Run with sudo."
    exit 1
fi

# Check if module is loaded
if ! lsmod | grep -q "vns_control"; then
    print_warn "Module vns_control is not loaded"
    exit 0
fi

# Check if device is in use
if lsof /dev/vns_control >/dev/null 2>&1; then
    print_warn "Device /dev/vns_control is in use by:"
    lsof /dev/vns_control
    read -p "Force unload? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        print_info "Aborted"
        exit 1
    fi
fi

print_info "Unloading vns_control kernel module..."
rmmod vns_control

sleep 1

# Verify
if lsmod | grep -q "vns_control"; then
    print_error "Module still loaded (may be in use)"
    exit 1
else
    print_info "Module unloaded successfully"
fi

# Check device node
if [[ -c /dev/vns_control ]]; then
    print_warn "Device node still exists (will be removed on next udev event)"
else
    print_info "Device node removed"
fi

print_info "Done"