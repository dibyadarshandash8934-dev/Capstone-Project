#!/bin/bash
# Run script for VNS Virtual NAT Gateway Simulator
# Usage: ./scripts/run.sh [--help|--cli|--test|--driver-stats|--load-driver|--unload-driver|--build|--build-run|--clean]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
BINARY="$BUILD_DIR/vns_sim"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo -e "${BLUE}=========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}=========================================${NC}"
}

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

show_help() {
    echo "VNS Virtual NAT Gateway Simulator - Run Script"
    echo ""
    echo "Usage: $0 [OPTION]"
    echo ""
    echo "Options:"
    echo "  --help         Show this help message"
    echo "  --cli          Run CLI simulator (default)"
    echo "  --test         Run all tests"
    echo "  --driver-stats Show driver statistics (requires loaded module)"
    echo "  --load-driver  Load kernel module (requires sudo)"
    echo "  --unload-driver Unload kernel module (requires sudo)"
    echo "  --build        Build the project first"
    echo "  --build-run    Build the project, then start the CLI simulator"
    echo "  --clean        Clean build directory"
    echo ""
    echo "Examples:"
    echo "  $0                    # Run CLI simulator"
    echo "  $0 --test             # Run all tests"
    echo "  $0 --load-driver      # Load kernel module"
    echo "  $0 --build-run        # Build and run CLI"
}

check_binary() {
    if [[ ! -f "$BINARY" ]]; then
        print_error "Binary not found at $BINARY"
        print_info "Run '$0 --build' first"
        exit 1
    fi
}

check_root() {
    if [[ $EUID -ne 0 ]]; then
        print_error "This operation requires root privileges"
        exit 1
    fi
}

build_project() {
    print_header "Building VNS Simulator"
    "$SCRIPT_DIR/build.sh" Release
}

run_cli() {
    check_binary
    print_header "Starting VNS CLI Simulator"
    print_info "Press Ctrl+C to exit"
    echo ""
    exec "$BINARY"
}

run_tests() {
    print_header "Running Tests"
    
    if [[ ! -d "$BUILD_DIR" ]]; then
        print_error "Build directory not found. Run '$0 --build' first."
        exit 1
    fi
    
    cd "$BUILD_DIR"
    
    print_info "Running CTest suites..."
    ctest --output-on-failure
    
    print_info "Running the full test runner..."
    if [[ -x "$BUILD_DIR/vns_tests" ]]; then
        "$BUILD_DIR/vns_tests"
    else
        print_warn "vns_tests runner not built. Run with --build first."
        exit 1
    fi
}

load_driver() {
    check_root
    print_header "Loading Kernel Module"
    cd "$PROJECT_ROOT/kernel/vns_control"
    if [[ -f vns_control.ko ]]; then
        print_info "Loading vns_control module..."
        sudo insmod vns_control.ko
        sleep 1
        if ls /dev/vns_control >/dev/null 2>&1; then
            print_info "Module loaded successfully"
            ls -l /dev/vns_control
            dmesg -T | grep VNS | tail -5
        else
            print_error "Failed to create device node"
            exit 1
        fi
    else
        print_error "Kernel module not built. Run 'make' in kernel/vns_control first."
        exit 1
    fi
}

unload_driver() {
    check_root
    print_header "Unloading Kernel Module"
    print_info "Unloading vns_control module..."
    sudo rmmod vns_control 2>/dev/null || true
    sleep 1
    if ls /dev/vns_control >/dev/null 2>&1; then
        print_warn "Device node still exists (may be in use)"
    else
        print_info "Module unloaded successfully"
    fi
}

show_driver_stats() {
    check_binary
    print_header "Driver Statistics"
    exec "$BINARY" --driver-stats
}

build_and_run() {
    build_project
    run_cli
}

clean_project() {
    print_header "Cleaning Build Directory"
    rm -rf "$PROJECT_ROOT/build"
    cd "$PROJECT_ROOT/kernel/vns_control" && make clean 2>/dev/null || true
    print_info "Build directory cleaned"
}

# Main
case "${1:-}" in
    --help|-h)
        show_help
        ;;
    --cli)
        check_binary
        run_cli
        ;;
    --test)
        run_tests
        ;;
    --driver-stats)
        show_driver_stats
        ;;
    --load-driver)
        load_driver
        ;;
    --unload-driver)
        unload_driver
        ;;
    --build)
        build_project
        ;;
    --clean)
        clean_project
        ;;
    --build-run)
        build_and_run
        ;;
    *)
        # Default: build if needed, then run CLI
        if [[ ! -f "$BINARY" ]]; then
            print_warn "Binary not found. Building..."
            build_project
        fi
        run_cli
        ;;
esac