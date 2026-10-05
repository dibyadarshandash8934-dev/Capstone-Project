# Build and Run Instructions

## Prerequisites

### System Requirements

- Linux or macOS (the build scripts work on both)
- GCC 11+ or Clang 14+ with C++20 support
- CMake 3.16+
- Make
- Linux kernel headers — **only** if you want to build the optional kernel
  module (Linux only; it cannot build on macOS)

### Install Dependencies

**Ubuntu/Debian:**

```bash
sudo apt update
sudo apt install build-essential cmake g++
# Only for the optional kernel module:
sudo apt install linux-headers-$(uname -r)
```

**Fedora/RHEL:**

```bash
sudo dnf install gcc-c++ cmake make
# Only for the optional kernel module:
sudo dnf install kernel-devel kernel-headers
```

**Arch Linux:**

```bash
sudo pacman -S base-devel cmake gcc
# Only for the optional kernel module:
sudo pacman -S linux-headers
```

**macOS:**

```bash
xcode-select --install   # clang toolchain
brew install cmake
# The kernel module cannot be built on macOS; the simulator runs without it.
```

No other third-party libraries are required (no gtest, no libfmt — tests use
plain `assert()`).

## Building

### Quick Build (Application Only)

```bash
cd vns_new
cmake -S . -B build
cmake --build build -j
```

(`make -j$(nproc)` inside `build/` also works, but only on Linux —
`cmake --build` is portable.)

Alternatively use the helper script (works on Linux and macOS):

```bash
./scripts/build.sh            # Release (default)
./scripts/build.sh debug
./scripts/build.sh clean
```

### Build with Kernel Module (Linux Only)

```bash
cd vns_new
cmake -S . -B build
cmake --build build -j
cmake --build build --target build-driver
```

### Kernel Module Only (Linux Only)

```bash
cd kernel/vns_control
make
```

This cannot build on macOS — it needs Linux kernel headers.

## Running the Simulator

### 1. Load Kernel Module (Optional, Requires Root, Linux Only)

```bash
sudo insmod kernel/vns_control/vns_control.ko
# Verify
ls -l /dev/vns_control
dmesg -T | grep VNS
```

The simulator works fine without the module; driver operations then report
`Driver not available`.

### 2. Run Simulator

```bash
# From build directory
./vns_sim

# Or from project root
./build/vns_sim
```

### 3. CLI Flags

```bash
./build/vns_sim --help          # -h also works
./build/vns_sim --version
./build/vns_sim --driver-stats  # needs the kernel module loaded, else "Driver not available"
```

Without options an interactive menu is started.

## Running Tests

### C++ Test Suites

```bash
cd vns_new/build
ctest --output-on-failure
# Or run the generated runner script, which executes all 7 suites in order:
./vns_tests
```

Individual suites (one executable per `tests/test_*.cpp`):

```bash
./test_network  ./test_nat  ./test_pat  ./test_dnat
./test_packet   ./test_connection  ./test_driver_interface
```

### Kernel Module Tests

```bash
# Load module (Linux only)
sudo insmod kernel/vns_control/vns_control.ko

# Run driver tests (the driver suite degrades gracefully if the module is absent)
./vns_tests  # Includes driver interface tests

# Manual verification
cat /dev/vns_control
echo "reset" > /dev/vns_control
cat /dev/vns_control
```

## Running the Application

### Start the CLI

```bash
cd vns_new
./build/vns_sim
# Interactive menu over stdin/stdout — there is no server, port, or web UI.
```

### Helper Script

```bash
./scripts/run.sh --cli          # start the CLI (default)
./scripts/run.sh --test         # ctest + ./vns_tests
./scripts/run.sh --build        # build first
./scripts/run.sh --build-run    # build, then start the CLI
./scripts/run.sh --driver-stats # requires the kernel module
./scripts/run.sh --load-driver / --unload-driver   # Linux only, sudo
./scripts/run.sh --clean        # remove build/
./scripts/run.sh --help
```

### Release Build

```bash
./scripts/build.sh release
# Or manually:
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Usage Examples

### Interactive CLI

```bash
./build/vns_sim
# Menu appears:
#  1. Configure Network
#  2. Manage Virtual Hosts
#  3. Configure NAT
#  4. View NAT Table
#  5. Manage Port Forwarding
#  6. Simulate Outbound Packet
#  7. Simulate Inbound Packet
#  8. View Connections
#  9. View Driver Statistics
# 10. Reset Simulation
#  0. Exit
```

### Programmatic Usage

```cpp
#include "vns/network/network.hpp"
#include "vns/network/device.hpp"
#include "vns/nat/nat_engine.hpp"
#include "vns/packet/packet.hpp"
#include <cassert>

using namespace vns;

// Create network (returns std::optional<NetworkConfig>)
auto net = NetworkConfig::create("192.168.1.0/24", "192.168.1.1", "203.0.113.10");
assert(net);

// Create engine (NatEngineImpl is used by value)
NatEngineImpl engine(*net);

// Add device (returns std::optional<VirtualDevice>)
auto dev = VirtualDevice::create("PC-01",
    Ipv4Address::from_string("192.168.1.10").value(),
    VirtualDevice::Type::PC, *net);
assert(dev);
engine.add_device(*dev);

// Simulate packet
Packet pkt(Protocol::TCP,
    Ipv4Address::from_string("192.168.1.10").value(), 50000,
    Ipv4Address::from_string("8.8.8.8").value(), 443);
auto result = engine.process_outgoing_packet(pkt);
if (result.success()) {
    // result.action()         -> "SNAT"
    // result.translated_packet() -> packet after translation
    const auto& metrics = engine.metrics();  // totals per action
}
```

## Kernel Module Management

### Load Module

```bash
sudo insmod kernel/vns_control/vns_control.ko
# Verify
ls -l /dev/vns_control
dmesg -T | grep VNS
```

### Unload Module

```bash
sudo rmmod vns_control
# Verify
ls -l /dev/vns_control  # Should not exist
```

### View Kernel Logs

```bash
dmesg -T | grep VNS
# Or follow
dmesg -w | grep VNS
```

### Module Parameters (Future)

```bash
# View parameters
cat /sys/module/vns_control/parameters/*

# Set parameter (if implemented)
echo 40000 > /sys/module/vns_control/parameters/port_range_start
```

## Troubleshooting

### Module Won't Load

```bash
# Check kernel headers
ls /lib/modules/$(uname -r)/build

# Check kernel version match
uname -r
ls /lib/modules/

# Rebuild against current kernel
cd kernel/vns_control
make clean
make
```

### Permission Denied

```bash
# Check device permissions
ls -l /dev/vns_control

# Fix with udev rule
echo 'KERNEL=="vns_control", MODE="0666"' | sudo tee /etc/udev/rules.d/99-vns.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### Module Won't Unload

```bash
# Check what's using it
lsof /dev/vns_control

# Force unload (dangerous)
sudo rmmod -f vns_control
```

### Build Failures

```bash
# Clean rebuild (portable, works on Linux and macOS)
rm -rf build
cmake -S . -B build
cmake --build build -j

# Kernel module (Linux only)
cd kernel/vns_control
make clean
make
```

## Environment Variables

**None.** The simulator reads no environment variables — there is no `getenv`
anywhere in `src/` or `include/`. All configuration happens at runtime through
the interactive CLI; the driver device path is the fixed default
`/dev/vns_control` (overridable only in code via `DriverInterface::open(path)`).

## Development Workflow

### Typical Cycle

```bash
# 1. Make changes
vim src/nat/nat_engine.cpp

# 2. Rebuild
cmake --build build -j

# 2b. If kernel changes (Linux only)
cd kernel/vns_control
make
sudo rmmod vns_control
sudo insmod vns_control.ko
cd ../..

# 3. Test
./build/vns_sim
# or
cd build && ctest --output-on-failure && ./vns_tests
```

### Debug Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
# Or: ./scripts/build.sh debug
```

### Release Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
# Or: ./scripts/build.sh release
```

## CI/CD Integration

### GitHub Actions Example

(Example workflow — add it as `.github/workflows/build.yml`; nothing similar is
checked in yet.)

```yaml
name: Build and Test
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install dependencies
        run: sudo apt-get update && sudo apt-get install -y cmake g++
      - name: Build
        run: |
          cmake -S . -B build
          cmake --build build -j
      - name: Test
        run: |
          cd build
          ctest --output-on-failure
```

## Cross-Compilation (ARM64 Example)

```bash
# Install cross-compiler
sudo apt install g++-aarch64-linux-gnu

# Build
cmake -S . -B build-arm64 \
      -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
      -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
      -DCMAKE_SYSTEM_NAME=Linux \
      -DCMAKE_SYSTEM_PROCESSOR=aarch64
cmake --build build-arm64 -j
```

## Performance Tuning

### Compiler Flags

```cmake
# Release
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")

# Profile-guided optimization
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fprofile-generate")
# Run workload
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fprofile-use")
```

### Kernel Module Optimization

```makefile
# In kernel/vns_control/Makefile
ccflags-y += -O2 -pipe
```

## Packaging

Not yet configured — the repository contains no `debian/` directory and no
`vns.spec`. If you add them later:

### DEB Package (Debian/Ubuntu)

```bash
# Install packaging tools
sudo apt install devscripts debhelper dh-make

# Create package
dh_make --createorig -s -c gpl3 -p vns_1.0.0
# Edit debian/control, debian/rules
dpkg-buildpackage -us -uc
```

### RPM Package (Fedora/RHEL)

```bash
# Create spec file
rpmbuild -ba vns.spec
```

## Docker Support

No `Dockerfile` is checked in yet. Starting point:

### Dockerfile

```dockerfile
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y \
    g++ cmake make \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY . .
RUN cmake -S . -B build && cmake --build build -j
CMD ["./build/vns_sim"]
```

```bash
docker build -t vns .
docker run -it vns
# Add --privileged -v /lib/modules:/lib/modules only if you want the
# optional kernel module inside the container (Linux hosts only).
```
