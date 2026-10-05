# VNS Architecture Document

## Overview

The Virtual NAT Gateway & Port-Forwarding Simulator (VNS) is a C++20 application
(builds on Linux and macOS) that simulates NAT gateway behavior including SNAT,
DNAT, and connection tracking. It is driven by an interactive CLI (`vns_sim`).
An optional Linux kernel module (`kernel/vns_control/`) provides telemetry and
control; it builds only on Linux with kernel headers, and the simulator runs
without it (driver calls then report "Driver not available").

## Architecture Overview

```mermaid
flowchart TB
    CLI[CLI Interface] --> Service[Simulation Service]
    Service --> NAT[NAT Engine]
    Service --> PF[Port Forwarding]
    Service --> CT[Connection Tracker]
    Service --> Driver[Driver Interface]
    Driver --> Kernel[/dev/vns_control]
    Kernel --> KMod[vns_control.ko]

    NAT --> NATTable[NAT Table]
    NAT --> PAT[PAT Allocator]
    PF --> PFRules[Port Forward Rules]
    CT --> Connections[Connection Map]

    NAT --> Metrics[Simulation Metrics]
    PF --> Metrics
    CT --> Metrics
    Driver --> Metrics
```

## Component Responsibilities

### Core Components

| Component             | Responsibility                                                      |
| --------------------- | ------------------------------------------------------------------- |
| **NetworkConfig**     | CIDR validation, IP address management, subnet operations           |
| **VirtualDevice**     | Host management, IP validation, device lifecycle                    |
| **Packet**            | IPv4/TCP/UDP packet representation, flow keys                       |
| **PacketEngine**      | Packet processing pipeline (SNAT/DNAT/Reverse NAT)                  |
| **NatEngine**         | SNAT/PAT/DNAT logic, NAT table management                           |
| **NatTable**          | Bidirectional flow lookup (forward + reverse)                       |
| **PatAllocator**      | Deterministic port allocation (40000-50000), explicit release/reset |
| **PortForwarding**    | DNAT rule management                                                |
| **ConnectionTracker** | Connection state machine (NEW→ACTIVE→ESTABLISHED→CLOSED/EXPIRED)    |
| **SimulationService** | High-level orchestration, state management                          |
| **DriverInterface**   | Kernel/userspace communication via /dev/vns_control                 |

### Kernel Module (Optional, Linux Only)

The `vns_control` kernel module provides:

- Character device `/dev/vns_control`
- ioctl interface for statistics and control
- Read/write for human-readable status
- Statistics counters stored in kernel space and returned via ioctl (reset/read;
  the per-packet counting itself happens in the userspace engine)
- Synchronization with mutexes

The module is optional: the userspace simulator runs without it, and the
`DriverInterface` degrades gracefully (`Driver not available`). It cannot be
built on macOS — only on Linux with matching kernel headers.

## Data Flow

### Outbound Packet (SNAT)

```
Packet → PacketEngine → NatEngine → NatTable (lookup)
    → PAT Allocator (if new) → NatTable (store) → Translated Packet
```

### Inbound Response (Reverse NAT)

```
Packet → PacketEngine → NatEngine → NatTable (reverse lookup)
    → Reverse Translation → Translated Packet
```

### Inbound Request (DNAT)

```
Packet → PacketEngine → PortForwarding (lookup)
    → DNAT Translation → Translated Packet
```

## Key Data Structures

### NAT Table (Bidirectional)

- **Forward Table**: `(proto, src_ip, src_port, dst_ip, dst_port) → NatEntry`
- **Reverse Table**:
  `(proto, public_ip, public_port, dst_ip, dst_port) → NatEntry`

### PAT Allocator

- Range: 40000-50000 (configurable)
- Deterministic allocation with wrap-around
- Explicit `release()`/`reset()` (ports are not released automatically on
  connection close)

### Connection States

```
NEW → ACTIVE → ESTABLISHED → CLOSED/EXPIRED
```

## Build System

```bash
# Application (Linux and macOS)
cmake -S . -B build
cmake --build build -j      # portable; on Linux `cd build && make -j$(nproc)` works too

# Kernel module (Linux only, optional)
cd kernel/vns_control && make
sudo insmod vns_control.ko
```

Convenience wrappers: `./scripts/build.sh [clean|debug|release]` and
`./scripts/run.sh [--help|--cli|--test|--driver-stats|--load-driver|--unload-driver|--build|--build-run|--clean]`.

## Security Considerations

- Input validation at all boundaries
- No arbitrary code execution paths
- No network sockets — the simulator is a single-process stdin/stdout CLI
- Kernel module validates all user input
- Uses `copy_to_user`/`copy_from_user` for safe data transfer
- Mutex protection for shared state in the kernel module

## Testing Strategy

Seven assert-based test suites (`tests/test_*.cpp`, plain `assert()` with one
`main()` per file), run via `ctest` and the generated `build/vns_tests` runner:

| Test Suite                                      | `assert` Checks | Coverage                                     |
| ----------------------------------------------- | --------------- | -------------------------------------------- |
| Unit Tests (NAT Engine) `test_nat.cpp`          | 68              | NAT logic, PAT, DNAT, Reverse NAT, Metrics   |
| Unit Tests (Network) `test_network.cpp`         | 40              | CIDR, IP validation, device management       |
| Unit Tests (Packet) `test_packet.cpp`           | 39              | Packet creation, flow keys, copy_with        |
| Unit Tests (Connection) `test_connection.cpp`   | 33              | Connection tracking                          |
| Unit Tests (DNAT) `test_dnat.cpp`               | 22              | Port forwarding, validation, TCP/UDP         |
| Unit Tests (PAT) `test_pat.cpp`                 | 13              | Port allocation, release, exhaustion         |
| Unit Tests (Driver) `test_driver_interface.cpp` | 10              | Driver interface, error handling             |
| **Total**                                       | **225**         | **7 suites, run by `./vns_tests` / `ctest`** |
