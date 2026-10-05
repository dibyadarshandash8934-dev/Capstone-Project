# VNS — Virtual NAT Gateway Simulator (C++)

A self-contained, interactive simulator of a **virtual NAT gateway**: it
performs SNAT/PAT address translation, DNAT (port forwarding), connection
tracking and per-packet transformation tracing, and exposes everything through a
CLI.

The whole simulator runs in user space with in-memory state — no database, no
network access and no root privileges required. The optional Linux kernel module
(`kernel/vns_control/`) is only needed for driver-statistics demos.

---

## Requirements

| Tool                    | Version                             |
| ----------------------- | ----------------------------------- |
| CMake                   | 3.16+                               |
| C++ compiler with C++20 | GCC 11+, Clang 14+, Apple Clang 14+ |
| Make                    | any                                 |

Optional (Linux only): kernel headers matching the running kernel, to build the
`vns_control` module.

macOS and Linux both build and run the simulator; only the kernel module is
Linux-specific.

---

## Quick start

```bash
./scripts/build.sh            # configure + build (Release)
cd build
./vns_sim                     # interactive menu
```

Or with plain CMake:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/vns_sim
```

### Command-line options

```
vns_sim -h | --help          show usage
vns_sim --version            print the version
vns_sim --driver-stats       print kernel driver statistics (needs the module)
```

---

## Running the tests

```bash
cd build
ctest --output-on-failure     # runs every suite
./vns_tests                   # runner that executes all 7 suites in order
```

The individual suites are separate binaries:

| Binary                  | Covers                                                        |
| ----------------------- | ------------------------------------------------------------- |
| `test_network`          | IPv4 addresses, CIDR, network config, virtual devices         |
| `test_nat`              | SNAT/PAT, reverse NAT, DNAT, NAT table, metrics               |
| `test_pat`              | Public port allocator (allocate / release / exhaust / reset)  |
| `test_dnat`             | Port-forwarding rules: add, duplicate, delete, TCP vs UDP     |
| `test_packet`           | Packet model, flow keys, `to_string`/`from_string` round trip |
| `test_connection`       | Connection tracker: create, dedupe, activity, expiry          |
| `test_driver_interface` | Driver ioctl wrappers (graceful if module not loaded)         |

Each test file is a plain `main()` with `assert()` — no external test framework.
To add one: create `tests/test_yourthing.cpp`, then add `yourthing` to
`VNS_TEST_NAMES` in `CMakeLists.txt`.

---

## Project layout

```
vns_new/
├── CMakeLists.txt          # builds libvns_core, vns_sim, and the test suites
├── include/vns/            # public headers
│   ├── common/             # Ipv4Address helpers, VnsStats, metrics, error types
│   ├── network/            # Ipv4Address, Cidr, NetworkConfig, VirtualDevice
│   ├── nat/                # NatTable, PatAllocator, PortForwarding, NatEngine
│   ├── packet/             # Packet, PacketEngine
│   ├── connection/         # Connection, ConnectionTracker
│   ├── driver/             # DriverInterface (ioctl wrapper)
│   ├── services/           # SimulationService (wires engine + devices together)
│   └── cli/                # Cli menu + console formatter
├── src/                    # implementations, mirroring include/vns/
├── tests/                  # one assert-based suite per area
├── scripts/                # build.sh, run.sh, clean.sh, driver helpers
├── kernel/vns_control/     # optional Linux kernel module
└── docs/                   # architecture, networking, driver, testing, demo guide
```

---

## What the simulator does

- **SNAT / PAT** — LAN packets get a public address and a port from a
  configurable pool (default `40000-50000`); identical flows reuse the mapping.
- **Reverse NAT** — responses from the Internet are matched against the reverse
  flow key and translated back to the originating host and port.
- **DNAT / port forwarding** — inbound packets to a forwarded public port are
  rewritten to the private host:port; TCP and UDP rules are independent and
  validated (public IP must be the gateway's, private IP must be a known LAN
  device, no duplicate matches).
- **Connection tracking** — every translated flow is tracked with activity
  timestamps and an expiry timeout.
- **Transformation tracing** — every packet produces an ordered list of steps
  (`ORIGINAL` → `SNAT`/`DNAT` → `FORWARD`) plus before/after packets, which the
  CLI prints.
- **Metrics** — totals, successes, failures, per-action counts and average
  processing time.

State lives in memory only: `Reset Simulation` (menu 10) or restarting the
process returns everything to its initial state.

---

## Optional: kernel module (Linux only)

```bash
cd kernel/vns_control
make                     # requires kernel headers for the running kernel
sudo insmod vns_control.ko
./build/vns_sim --driver-stats
sudo rmmod vns_control
```

Without the module the simulator runs normally and reports
`Driver not available` for driver-specific menu entries.

---

## Scripts

```bash
./scripts/build.sh [clean|debug|release]   # configure and build
./scripts/run.sh --help                    # list all options
./scripts/run.sh                           # build if needed, then run the CLI
./scripts/run.sh --test                    # ctest + full test runner
./scripts/run.sh --driver-stats            # driver statistics
./scripts/clean.sh                         # remove build artifacts
```

---

## Documentation

| File                    | Contents                                      |
| ----------------------- | --------------------------------------------- |
| `docs/architecture.md`  | Module boundaries and data flow               |
| `docs/networking.md`    | NAT/PAT/DNAT behaviour as implemented         |
| `docs/testing.md`       | Test suite layout and how to extend it        |
| `docs/device-driver.md` | ioctl interface of the kernel module          |
| `docs/demo-guide.md`    | Guided walkthrough of a demo session          |
| `docs/build-and-run.md` | Detailed build, run and troubleshooting steps |
# Capstone-Project
