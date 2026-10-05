# VNS Testing Documentation

## Test Overview

Seven assert-based suites (`tests/test_*.cpp`). Each file has its own
`int main()` and uses plain `assert()` — there is no gtest or other test
framework in this project.

| Test Suite                                     | `assert` checks | Coverage                                       |
| ---------------------------------------------- | --------------- | ---------------------------------------------- |
| NAT Engine (`test_nat.cpp`)                    | 68              | SNAT, PAT, DNAT, Reverse NAT, Metrics          |
| Network (`test_network.cpp`)                   | 40              | CIDR, IP validation, device management         |
| Packet (`test_packet.cpp`)                     | 39              | Creation, flow keys, copy_with                 |
| Connection (`test_connection.cpp`)             | 33              | State machine, expiration                      |
| DNAT (`test_dnat.cpp`)                         | 22              | Port forwarding, validation, TCP/UDP           |
| PAT Allocator (`test_pat.cpp`)                 | 13              | Port allocation, release, exhaustion           |
| Driver Interface (`test_driver_interface.cpp`) | 10              | Interface, error handling                      |
| **Total**                                      | **225**         | **7 suites, run by `ctest` and `./vns_tests`** |

## Running Tests

### C++ Unit Tests

```bash
cd vns_new
cmake -S . -B build
cmake --build build -j

cd build
ctest --output-on-failure   # runs all 7 suites through CTest
./vns_tests                 # or the generated runner script (executes every suite in order)
```

### Individual Suites

```bash
cd vns_new/build
./test_network
./test_nat
./test_pat
./test_dnat
./test_packet
./test_connection
./test_driver_interface
```

## Test Categories

### Network Tests (`test_network.cpp`)

- IPv4 address parsing/validation
- CIDR parsing and validation
- Network configuration validation
- Device IP validation
- Subnet membership
- Gateway validation

### NAT Engine Tests (`test_nat.cpp`)

- SNAT translation
- PAT port allocation
- Mapping reuse (same 5-tuple)
- Different destinations = different ports
- TCP/UDP independence
- Reverse NAT (response packets)
- DNAT / Port forwarding
- Reverse NAT for DNAT responses
- NAT table management
- Port exhaustion handling
- Metrics collection

### PAT Allocator Tests (`test_pat.cpp`)

- Basic allocation
- Port release and reuse
- Wrap-around behavior
- Exhaustion detection
- Reset functionality
- Edge cases (single port, invalid range)

### DNAT Tests (`test_dnat.cpp`)

- Rule creation
- DNAT translation
- No matching rule handling
- TCP/UDP port independence
- Invalid private IP rejection
- Rule deletion

### Packet Tests (`test_packet.cpp`)

- Packet creation
- Flow key generation
- Reverse flow key
- copy_with() transformations
- Protocol support (TCP/UDP/ICMP)
- String representation

### Connection Tracker Tests (`test_connection.cpp`)

- Connection creation
- State machine transitions
- Activity tracking
- Expiration handling
- Multiple concurrent connections
- Flow-based lookup

### Driver Interface Tests (`test_driver_interface.cpp`)

- Device open/close
- Move semantics
- Stats retrieval
- Status retrieval
- Read/write operations
- Ioctl commands
- Error handling (non-existent device — degrades gracefully when the kernel
  module is not loaded)

## Test Execution

### Running All Tests

```bash
cd vns_new/build

# Via CTest (each suite registered as test_network, test_nat, ...)
ctest --output-on-failure

# Via the generated runner script — executes all 7 suites in order and
# stops at the first failure (set -e):
./vns_tests
```

### Running Individual Suites

```bash
./test_network   ./test_nat       ./test_pat
./test_dnat      ./test_packet    ./test_connection
./test_driver_interface
```

### Test Output

- Each suite prints progress to stdout and ends with a line such as
  `All NAT tests passed!`
- A failed `assert()` aborts the process with a non-zero exit code; `ctest` and
  the `./vns_tests` runner report it as a failure
- There is no test filtering, XML, or JSON output — these are plain `assert()`
  programs, so flags like `--gtest_filter` / `--gtest_output` do not exist

## Test Coverage

### Coverage Report

```bash
# Generate coverage (requires gcov/lcov)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage"
cmake --build build -j
cd build && ./vns_tests
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

### Coverage Targets

| Component          | Target |
| ------------------ | ------ |
| NAT Engine         | >90%   |
| PAT Allocator      | >95%   |
| Network Utils      | >90%   |
| Packet Engine      | >85%   |
| Connection Tracker | >85%   |
| Driver Interface   | >80%   |

## Continuous Integration

### GitHub Actions

(Example workflow — add it as `.github/workflows/test.yml`.)

```yaml
# .github/workflows/test.yml
name: Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install deps
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

## Test Data

### Test Networks

```cpp
// Standard test network (returns std::optional<NetworkConfig>)
auto net = NetworkConfig::create("192.168.1.0/24", "192.168.1.1", "203.0.113.5").value();

// Large network
auto large_net = NetworkConfig::create("10.0.0.0/8", "10.0.0.1", "203.0.113.10").value();

// Small network
auto small_net = NetworkConfig::create("10.0.0.0/24", "10.0.0.1", "203.0.113.20").value();
```

### Test Devices

```cpp
// Valid device (returns std::optional<VirtualDevice>)
auto pc = VirtualDevice::create("PC-01", Ipv4Address::from_string("192.168.1.10").value(),
                     VirtualDevice::Type::PC, network);

// Server
auto web = VirtualDevice::create("Web-01", Ipv4Address::from_string("192.168.1.20").value(),
                     VirtualDevice::Type::SERVER, network);
```

### Test Packets

```cpp
// Outbound HTTPS
Packet(Protocol::TCP,
    Ipv4Address::from_string("192.168.1.10").value(), 50000,
    Ipv4Address::from_string("8.8.8.8").value(), 443);

// Inbound HTTP (for DNAT)
Packet(Protocol::TCP,
    Ipv4Address::from_string("198.51.100.20").value(), 45000,
    Ipv4Address::from_string("203.0.113.5").value(), 8080);

// Response packet (for reverse NAT)
Packet(Protocol::TCP,
    Ipv4Address::from_string("8.8.8.8").value(), 443,
    Ipv4Address::from_string("203.0.113.5").value(), 40000);
```

### Port Forwarding Rules

```cpp
// HTTP forwarding (returns std::optional<PortForwardRule>)
auto http_rule = PortForwardRule::create(Protocol::TCP,
    Ipv4Address::from_string("203.0.113.5").value(), 8080,
    Ipv4Address::from_string("192.168.1.20").value(), 80);

// DNS forwarding
auto dns_rule = PortForwardRule::create(Protocol::UDP,
    Ipv4Address::from_string("203.0.113.5").value(), 53,
    Ipv4Address::from_string("192.168.1.20").value(), 53);
```

## Mocking and Stubs

### Driver Interface (no mock required)

`DriverInterface` is non-copyable and non-polymorphic — its methods are not
`virtual`, so it cannot be subclassed for mocking. The driver suite
(`tests/test_driver_interface.cpp`) instead degrades gracefully:

- **Module loaded:** exercises open/close, move semantics, stats/status
  retrieval, read/write, and ioctl commands
- **Module absent (or macOS):** `open()` returns `false`; the suite prints an
  informational message and skips device-dependent checks

```cpp
DriverInterface driver;
if (driver.open("/dev/vns_control")) {
    if (auto stats = driver.get_stats()) {   // std::optional<VnsStats>
        std::cout << "Total packets: " << stats->total_packets() << "\n";
    }
} else {
    std::cout << "Device not available (expected if kernel module not loaded): "
              << driver.last_error() << "\n";
}
```

### Network Config

```cpp
auto network = NetworkConfig::create("192.168.1.0/24", "192.168.1.1", "203.0.113.5").value();
```

## Performance Testing

### Benchmark NAT Engine

```cpp
// Inside a suite's main() (e.g. tests/test_nat.cpp)
auto start = std::chrono::high_resolution_clock::now();
for (int i = 0; i < 10000; ++i) {
    Packet pkt(Protocol::TCP, src_ip, static_cast<Port>(50000 + i), dst_ip, 443);
    (void)engine.process_outgoing_packet(pkt);
}
auto elapsed = std::chrono::high_resolution_clock::now() - start;
auto ns_per_op = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count() / 10000;
std::cout << ns_per_op << " ns/op\n";
assert(ns_per_op < 100'000);   // < 100 µs per operation
```

## Test Quality Guidelines

### Good Test Practices

1. **One assertion per scenario** (mostly)
2. **Descriptive scenario comments**: `// SNAT: reuses mapping for same flow`
3. **Arrange-Act-Assert** pattern
4. **Edge cases**: boundaries, invalid input, exhaustion
5. **Independence**: no test depends on another
6. **Deterministic**: no randomness, no time-dependent logic

### Test Organization Convention

Each suite is a single `main()` containing scoped blocks, one per scenario:

```cpp
int main() {
    std::cout << "Testing NAT...\n";

    // Test: SNAT reuses mapping for same 5-tuple flow
    {
        Packet pkt(Protocol::TCP, src_ip, 50000, dst_ip, 443);
        auto first  = engine.process_outgoing_packet(pkt);
        auto second = engine.process_outgoing_packet(pkt);
        assert(first.success() && second.success());
        assert(first.nat_entry()->public_port() == second.nat_entry()->public_port());
    }

    std::cout << "All NAT tests passed!\n";
    return 0;
}
```

- One block per scenario, named by a leading comment
- `assert(...)` for expected behavior, `std::cout` for progress
- Finish `main()` with a `All ... tests passed!` line and `return 0`

## Test Maintenance

### Adding New Tests

1. Create `tests/test_x.cpp` with its own `int main()` and `assert()` checks:

```cpp
#include "vns/nat/nat_engine.hpp"   // whatever the suite needs
#include <cassert>
#include <iostream>

using namespace vns;

int main() {
    std::cout << "Testing X...\n";
    {
        // arrange / act
        assert(/* expected behavior */);
    }
    std::cout << "All X tests passed!\n";
    return 0;
}
```

2. Add the suite name to `VNS_TEST_NAMES` in `CMakeLists.txt`:

```cmake
set(VNS_TEST_NAMES
    network
    nat
    pat
    dnat
    packet
    connection
    driver_interface
    x
)
```

3. Re-run `cmake -S . -B build` to register the new `test_x` target and its
   CTest entry — the `./vns_tests` runner script is regenerated by CMake and
   picks it up automatically.

### Updating Tests

- Update tests when behavior changes
- Remove obsolete tests
- Keep tests fast (< 100ms each)

## Reporting

### Test Report

CTest keeps its log in the build directory:

```bash
cd build
ctest --output-on-failure
cat Testing/Temporary/LastTest.log
```

### CI Integration

```bash
# Exit code 0 on success, non-zero on failure
cd build
ctest --output-on-failure
echo $?  # 0 = pass, non-zero = fail
```
