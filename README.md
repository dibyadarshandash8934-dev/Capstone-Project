# 🚀 Virtual NAT Gateway & Port Forwarding Simulator

**VNS** is a self-contained, interactive simulator of a virtual NAT gateway. It performs **SNAT/PAT** translation, **DNAT** port forwarding, connection tracking and per-packet transformation tracing, and exposes everything through a **CLI**.

> ⚠️ **This is a simulator, not a gateway.** It is a **C++20** program that runs entirely in **user space** against **in-memory** state. It does **not** send, capture or intercept real network traffic, needs no database, no network access and no root privileges.

---

## 📖 Introduction

VNS models what a home/office router does — it translates private LAN addresses into a public address on the way out (**SNAT/PAT**), rewrites inbound destinations so a single public port reaches a LAN host (**DNAT / port forwarding**), and remembers every flow in a connection table.

Everything is configured and driven from a menu-driven console application (`vns_sim`). The optional Linux kernel module in `kernel/vns_control/` is **only** used for driver-statistics demos — the simulator itself is plain user-space code.

| Property | Value |
| --- | --- |
| **Language** | C++20 (GCC 11+, Clang 14+, Apple Clang 14+) |
| **Build system** | CMake 3.16+ / Make |
| **Platforms** | Linux and macOS (kernel module: **Linux only**) |
| **Interface** | Interactive CLI — `vns_sim` |
| **State** | In-memory only, resettable at any time |
| **Privileges** | None required (driver demos need `sudo`) |

---

## ✨ Key Features

### 🌐 Virtual Network

Configure a private LAN with a **CIDR** block, a **gateway IP** and a **public NAT IP**. All addresses, subnet membership and gateway checks are validated before the network is accepted.

### 💻 Virtual Hosts

Add, list and remove **virtual devices** (`pc`, `server`, `laptop`, `phone`, `iot`). A device IP must fall inside the configured LAN range, and duplicate IPs are rejected.

### 🔄 SNAT / PAT

Outbound LAN packets receive the gateway's public address plus a **public port** taken from a pool (`40000-50000` by default). Identical flows reuse their existing mapping.

### 🔀 DNAT / Port Forwarding

Publish one public port and map it to a `private IP : private port` inside the LAN. **TCP** and **UDP** rules are independent and validated: the public IP must be the gateway's, the private IP must be a known LAN device, and duplicate rules are rejected.

### 📦 Packet Simulation

Simulate an **outbound** or **inbound** packet and see the transformation applied: the before/after packets plus an ordered list of steps (`ORIGINAL` → `SNAT`/`DNAT` → `FORWARD`).

### 🔗 Connection Tracking

Every translated flow is tracked with **activity timestamps** and an **expiry timeout**, and moves through the states `NEW → ACTIVE → ESTABLISHED → CLOSED / EXPIRED`.

### 📊 NAT Table

A bidirectional table of all live mappings, viewable as a formatted table. It supports forward and reverse lookup, plus **metrics**: totals, successes, failures, per-action counts and average processing time.

### 🐧 Linux Kernel Driver

An optional **Linux-only** kernel module (`vns_control`) exposes a character device at `/dev/vns_control` for statistics and control via `ioctl`/`read`/`write`. The simulator degrades gracefully to `Driver not available` when the module is not loaded.

---

## 🔄 SNAT / PAT — Outbound Traffic

**SNAT/PAT** replaces a private source address with the gateway's public address, and picks a public port so many hosts can share one public IP.

```text
 Private Device   ->   SNAT/PAT   ->   NAT Gateway   ->   Internet
 192.168.1.10:50000                    203.0.113.5:40000  198.51.100.7:443
```

- The **source** `IP:port` is rewritten to the gateway's public address plus an allocated port.
- The reverse direction is handled by a **reverse flow lookup** in the NAT table.
- A repeated flow (same 5-tuple) **reuses** its mapping instead of consuming a new port.
- Ports come from a configurable pool (`40000-50000` by default) with deterministic allocation and wrap-around.

---

## 🔀 DNAT / Port Forwarding — Inbound Traffic

A private LAN is normally unreachable from the Internet. Port forwarding solves that **one service at a time**: publish exactly one public port and let the gateway rewrite the destination of matching inbound packets.

```text
 Internet Client   ->   Public IP:Port   ->   NAT Gateway   ->   Private Host
 198.51.100.7:51515     203.0.113.5:8080                         192.168.1.20:80
```

The gateway matches the inbound destination against the **rule**, then rewrites only the destination — the Internet source stays untouched.

A rule is a **match plus a rewrite**:

| Part | Key |
| --- | --- |
| Match | `(protocol, public IP, public port)` |
| Rewrite target | `(private IP, private port)` |

Rules are identified as `<PROTOCOL>-<public IP>-<public port>`, e.g. `TCP-203.0.113.5-8080`. Port forwarding **is** DNAT in this project (`include/vns/nat/dnat.hpp` documents that there is no separate DNAT code path) — it is the mirror image of SNAT/PAT:

| Direction | Feature | Reads / writes |
| --- | --- | --- |
| LAN → Internet | **SNAT / PAT** | private `IP:port` → public `IP:allocated port` |
| Internet → LAN | **DNAT** | public `IP:rule port` → private `IP:rule port` |
| Internet → LAN (established) | **Reverse NAT** | NAT table reverse flow lookup |

Rules and the NAT table are **independent**: clearing the NAT table (menu `3` → `2`) keeps the rules, and deleting a rule does not clear the table.

---

## 📦 Packet Simulation

Each simulated packet produces a **before** packet, an **after** packet and an ordered **transformation trace**.

### Outbound — SNAT

```text
BEFORE NAT:  TCP 192.168.1.20:80     -> 198.51.100.7:443
                        |  SNAT
                        v
AFTER NAT:   TCP 203.0.113.5:8080   -> 198.51.100.7:443
```

### Inbound — DNAT

```text
BEFORE NAT:  TCP 198.51.100.7:51515 -> 203.0.113.5:8080
                        |  DNAT
                        v
AFTER NAT:   TCP 198.51.100.7:51515 -> 192.168.1.20:80
```

### Actual CLI output

```text
============================================================
  PACKET SIMULATION RESULT
============================================================
Action: DNAT

  BEFORE NAT:
    TCP 198.51.100.7:51515 -> 203.0.113.5:8080

  AFTER NAT:
    TCP 198.51.100.7:51515 -> 192.168.1.20:80

  TRANSFORMATION STEPS:
    Incoming packet from Internet
    DNAT: 203.0.113.5:8080 -> 192.168.1.20:80
    Packet forwarded to LAN device

  NAT ENTRY:
    TCP 192.168.1.20:80 <-> 203.0.113.5:8080 dest 198.51.100.7:51515 [ACTIVE]
```

The reply direction (`6`, packet from the LAN host on the forwarded port) reuses the rule's public port instead of taking one from the PAT pool:

```text
Action: SNAT

  BEFORE NAT:
    TCP 192.168.1.20:80 -> 198.51.100.7:443

  AFTER NAT:
    TCP 203.0.113.5:8080 -> 198.51.100.7:443

  TRANSFORMATION STEPS:
    Original packet from LAN device
    SNAT: 192.168.1.20:80 -> 203.0.113.5:8080
    Packet forwarded to Internet
```

An inbound packet to a port with no rule and no NAT mapping is rejected:

```text
  FAILED: No active NAT mapping for incoming packet
```

---

## 🖥️ Interactive CLI

Running `vns_sim` without arguments starts the interactive menu:

```text
============================================================
  VNS - VIRTUAL NAT GATEWAY SIMULATOR
============================================================
  1. Configure Network
  2. Manage Virtual Hosts
  3. Configure NAT
  4. View NAT Table
  5. Manage Port Forwarding
  6. Simulate Outbound Packet
  7. Simulate Inbound Packet
  8. View Connections
  9. View Driver Statistics
  10. Reset Simulation
  0. Exit

  Choice:
```

| Option | Action |
| --- | --- |
| `1` | **Configure Network** — CIDR, gateway IP, public NAT IP |
| `2` | **Manage Virtual Hosts** — add / list / remove devices |
| `3` | **Configure NAT** — view table, clear table, view statistics |
| `4` | **View NAT Table** |
| `5` | **Manage Port Forwarding** — add / list / delete rules |
| `6` | **Simulate Outbound Packet** (LAN → Internet) |
| `7` | **Simulate Inbound Packet** (Internet → LAN) |
| `8` | **View Connections** |
| `9` | **View Driver Statistics** |
| `10` | **Reset Simulation** — clears all configuration and state |

Command-line options:

```text
vns_sim -h | --help          show usage
vns_sim --version            print the version
vns_sim --driver-stats       print kernel driver statistics (needs the module)
```

---

## 🏗️ Architecture

```text
                        CLI  (vns_sim)
                          │
                          ▼
                  Simulation Service
                          │
                          ▼
                       NAT Engine
                ┌─────────┼─────────┐
                │         │         │
             SNAT/PAT    DNAT    NAT Table
                │         │         │
                └─────────┴─────────┘
                          │
                          ▼
                Connection Tracker
                          │
                          ▼
                   Packet Processing
                          │
                          ▼
                   Driver Interface
                          │
                          ▼
              Linux Character Device  (/dev/vns_control)
```

| Component | Responsibility |
| --- | --- |
| **NetworkConfig** | CIDR validation, IP address management, subnet operations |
| **VirtualDevice** | Host management, IP validation, device lifecycle |
| **Packet / PacketEngine** | Packet model, flow keys, processing pipeline |
| **NatEngine** | SNAT/PAT/DNAT logic and NAT table management |
| **NatTable** | Bidirectional flow lookup (forward + reverse) |
| **PatAllocator** | Deterministic port allocation, release / reset |
| **PortForwarding** | DNAT rule management |
| **ConnectionTracker** | Connection state machine and expiry |
| **SimulationService** | High-level orchestration and state management |
| **DriverInterface** | Kernel/userspace communication via `/dev/vns_control` |

---

## 🧰 Technology Stack

| Technology | Purpose |
| --- | --- |
| **C++20** | Core language for the simulator (`libvns_core`, `vns_sim`) |
| **CMake 3.16+** | Builds the core library, the CLI and the 7 test suites |
| **GCC / Clang** | Supported compilers (C++20 support required) |
| **CTest** | Runs the registered test suites |
| **Plain `assert()`** | Test style — no external test framework |
| **Bash** | Helper scripts in `scripts/` (build, run, clean, driver helpers) |
| **Make / Kbuild** | Builds the optional `vns_control` Linux kernel module |
| **Linux kernel API** | Character device, `ioctl`, mutexes in the optional module |
| **Standard library** | `Ipv4Address`, `Cidr`, `std::optional`, containers, iostreams |

---

## 🐧 Linux Kernel Driver

The `vns_control` kernel module is **Linux-specific** and **optional**. It provides a character device interface (`/dev/vns_control`) for communication between the C++ userspace simulator and the kernel, exposing statistics, status and control commands via `ioctl`, `read` and `write`.

```text
        C++ Simulator  (DriverInterface)
                  │
                  │  ioctl / read / write
                  ▼
        /dev/vns_control
                  │
                  ▼
        Linux Kernel Module  (vns_control.ko)
```

> ℹ️ The module is a **telemetry and control** channel only. It does **not** hook the network stack, and the simulator never intercepts real network traffic. Per-packet counting happens in the userspace engine.

Build and load (Linux only, requires kernel headers for the running kernel):

```bash
cd kernel/vns_control
make                     # requires kernel headers for the running kernel
sudo insmod vns_control.ko
./build/vns_sim --driver-stats
sudo rmmod vns_control
```

Without the module, the simulator runs normally and reports `Driver not available` for driver-specific menu entries.

`ioctl` commands are defined in `include/vns/vns_ioctl.h` with magic `'V'`:

| Command | Code | Purpose |
| --- | --- | --- |
| `VNS_IOCTL_GET_STATS` | `_IOR('V', 1)` | Get all statistics |
| `VNS_IOCTL_RESET_STATS` | `_IO('V', 2)` | Reset all counters |
| `VNS_IOCTL_GET_STATUS` | `_IOR('V', 3)` | Get status flags |
| `VNS_IOCTL_SET_STATUS` | `_IOW('V', 4)` | Set status flags |
| `VNS_IOCTL_GET_NAT_ENTRY` | `_IOR('V', 5)` | Get NAT entry |
| `VNS_IOCTL_RESET_NAT` | `_IO('V', 6)` | Clear NAT table |

---

## 🧪 Testing

Seven `assert()`-based suites, each its own binary with its own `main()` — **no external test framework**.

```bash
cd build
ctest --output-on-failure     # runs every suite
./vns_tests                   # runner that executes all 7 suites in order
```

Or through the run script:

```bash
./scripts/run.sh --test
```

| Binary | `assert` checks | Covers |
| --- | --- | --- |
| `test_network` | 40 | IPv4 addresses, CIDR, network config, virtual devices |
| `test_nat` | 68 | SNAT/PAT, reverse NAT, DNAT, NAT table, metrics |
| `test_pat` | 13 | Public port allocator (allocate / release / exhaust / reset) |
| `test_dnat` | 22 | Port-forwarding rules: add, duplicate, delete, TCP vs UDP |
| `test_packet` | 39 | Packet model, flow keys, `to_string`/`from_string` round trip |
| `test_connection` | 33 | Connection tracker: create, dedupe, activity, expiry |
| `test_driver_interface` | 10 | Driver ioctl wrappers (graceful if module not loaded) |
| **Total** | **225** | **7 suites** |

Individual suites can be run directly:

```bash
./build/test_dnat             # port-forwarding / DNAT suite only
./build/test_nat              # also covers DNAT and the reply path
```

Latest verified `ctest` run in this repository:

```text
    Start 1: test_network
1/7 Test #1: test_network .....................   Passed    0.01 sec
    Start 2: test_nat
2/7 Test #2: test_nat .........................   Passed    0.00 sec
    Start 3: test_pat
3/7 Test #3: test_pat .........................   Passed    0.01 sec
    Start 4: test_dnat
4/7 Test #4: test_dnat ........................   Passed    0.01 sec
    Start 5: test_packet
5/7 Test #5: test_packet ......................   Passed    0.00 sec
    Start 6: test_connection
6/7 Test #6: test_connection ..................   Passed    0.16 sec
    Start 7: test_driver_interface
7/7 Test #7: test_driver_interface ............   Passed    0.01 sec

100% tests passed out of 7
```

To add a suite: create `tests/test_yourthing.cpp`, then add `yourthing` to `VNS_TEST_NAMES` in `CMakeLists.txt`.

---

## ▶️ How to Run

### Build

```bash
./scripts/build.sh            # configure + build (Release)
```

Or with plain CMake:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### Start

```bash
cd build
./vns_sim                     # interactive menu
```

Or let the script build-if-needed and start it:

```bash
./scripts/run.sh
```

### Help

```bash
./build/vns_sim --help
./scripts/run.sh --help
```

### Version

```bash
./build/vns_sim --version     # vns 1.0.0
```

### All helper scripts

```bash
./scripts/build.sh [clean|debug|release]   # configure and build
./scripts/run.sh --help                    # list all options
./scripts/run.sh                           # build if needed, then run the CLI
./scripts/run.sh --test                    # ctest + full test runner
./scripts/run.sh --driver-stats            # driver statistics
./scripts/clean.sh                         # remove build artifacts
```

---

## 📁 Project Structure

```text
vns_new/
├── CMakeLists.txt          # builds libvns_core, vns_sim, and the test suites
├── README.md               # this file
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

## 📋 Requirements

| Tool | Version |
| --- | --- |
| CMake | 3.16+ |
| C++ compiler with C++20 | GCC 11+, Clang 14+, Apple Clang 14+ |
| Make | any |

Optional (Linux only): kernel headers matching the running kernel, to build the
`vns_control` module.

macOS and Linux both build and run the simulator; only the kernel module is
Linux-specific.

---

## 🔄 Project Flow

```text
              Virtual Network
                    │
                    ▼
                NAT Gateway
                    │
                    ▼
             SNAT/PAT  +  DNAT
                    │
                    ▼
             Packet Processing
                    │
                    ▼
           Connection Tracking
                    │
                    ▼
                 NAT Table
```

Outbound (SNAT): `Packet → PacketEngine → NatEngine → NatTable (lookup) → PAT allocator (if new) → NatTable (store) → translated packet`

Inbound response (reverse NAT): `Packet → PacketEngine → NatEngine → NatTable (reverse lookup) → reverse translation → translated packet`

Inbound request (DNAT): `Packet → PacketEngine → PortForwarding (lookup) → DNAT translation → translated packet`

---

## 📚 Port Forwarding Reference

### Purpose

A private LAN is normally unreachable from the Internet because its hosts use RFC 1918 addresses and the gateway only translates *outgoing* traffic (SNAT/PAT). Port forwarding solves this for one service at a time: instead of exposing the whole LAN, you publish exactly one public port and let the gateway rewrite the destination of any inbound packet that matches it.

### DNAT flow in the simulator

`Simulate Inbound Packet` (menu `7`) runs this path (`NatEngineImpl::process_incoming_packet`):

1. **`ORIGINAL`** — record the packet as received from the Internet.
2. Look up a **rule** whose match key equals `(protocol, destination IP, destination port)`. Disabled rules are skipped.
3. **`DNAT`** — rewrite the destination to the rule's private IP and private port. If no rule matches, the DNAT step fails and the gateway falls back to the reverse-NAT path for the packet.
4. Record a **NAT entry** (`private <-> public`, state `ACTIVE`) and register the flow with the connection tracker.
5. **`FORWARD`** — deliver the translated packet to the LAN device and count a success in the metrics.

The return direction is handled by the same rule: an outgoing packet whose source is exactly the rule's private IP and private port reuses the rule's public port instead of taking a port from the PAT pool.

### Supported capabilities

| Capability | Behaviour |
| --- | --- |
| Protocols | `TCP` and `UDP`; rules of different protocols are independent |
| Public IP | Always the gateway's public IP from menu `1` — a rule cannot point anywhere else |
| Private IP | Must be inside the LAN CIDR **and** must already exist as a virtual host (menu `2`) |
| Ports | Prompted as `1-65535`; a zero port is rejected (`Invalid parameters.`) |
| Duplicate rules | Rejected — two enabled rules may not share the same protocol + public IP + public port |
| Rule identity | `<PROTOCOL>-<public IP>-<public port>`, e.g. `TCP-203.0.113.5-8080` |
| Validation errors | Surfaced by the CLI as `Invalid parameters.` or `Failed to add rule (conflict or invalid device?).` |
| Visibility | `List Rules` table with `Status` (`ENABLED`/`DISABLED`), `View NAT Table`, `View Connections` |
| Observability | Per-packet transformation trace plus `DNAT Packets` in the statistics |
| Lifecycle | `Add Rule`, `List Rules`, `Delete Rule`; cleared by `Reset Simulation` (menu `10`) |

### Configuring a rule from the CLI

1. `1` **Configure Network** — enter CIDR, gateway IP and public NAT IP.
2. `2` **Manage Virtual Hosts** → `1` **Add Device** — add the private host that will receive the traffic (its IP is required by the rule validation).
3. `5` **Manage Port Forwarding** → `1` **Add Rule**, then answer the prompts:

```text
Add Port Forwarding Rule
Protocol (TCP/UDP) [TCP]: TCP
Public Port (1-65535): 8080
Private IP: 192.168.1.20
Private Port (1-65535): 80
Port forwarding rule added!
```

The public IP is taken from the network configuration, so it is not asked for. Then use `2` **List Rules** to display the table and `3` **Delete Rule** to remove one by its ID.

Listing the rule created above (`5` → `2`):

```text
============================================================
  PORT FORWARDING RULES
============================================================
ID                   | Protocol | Public                 | Private                | Status
---------------------+----------+------------------------+------------------------+---------
TCP-203.0.113.5-8080 | TCP      | 203.0.113.5:8080       | 192.168.1.20:80        | ENABLED
```

### Running and testing port forwarding

```bash
./scripts/build.sh                        # build
./build/vns_sim                           # interactive menu, then use 5 and 7
./scripts/run.sh                          # build if needed, then start the CLI
./scripts/run.sh --test                   # ctest + all 7 suites
cd build && ctest --output-on-failure     # every suite
./build/test_dnat                         # port-forwarding / DNAT suite only
./build/test_nat                          # also covers DNAT and the reply path
```

---

## 📄 Documentation

| File | Contents |
| --- | --- |
| `docs/architecture.md` | Module boundaries and data flow |
| `docs/networking.md` | NAT/PAT/DNAT behaviour as implemented |
| `docs/testing.md` | Test suite layout and how to extend it |
| `docs/device-driver.md` | ioctl interface of the kernel module |
| `docs/demo-guide.md` | Guided walkthrough of a demo session |
| `docs/build-and-run.md` | Detailed build, run and troubleshooting steps |

---

## 📍 Project Status

- ✅ Virtual network, virtual hosts, SNAT/PAT, DNAT port forwarding, connection tracking, NAT table, metrics and per-packet transformation tracing are implemented.
- ✅ Interactive CLI with 10 menu entries plus `--help`, `--version` and `--driver-stats`.
- ✅ 7 `assert()`-based test suites (225 checks) registered with CTest, plus the `vns_tests` runner.
- ✅ Builds and runs on Linux and macOS; the `vns_control` kernel module is optional and Linux-only.
- ⏳ All state is in-memory — a restart or `Reset Simulation` (menu `10`) returns everything to its initial state.
- 📌 No real network traffic is generated or captured; the simulator is a single-process stdin/stdout CLI.

---

## 🎓 Capstone Project

**VNS — Virtual NAT Gateway Simulator** &nbsp;•&nbsp; C++20 &nbsp;•&nbsp; user space &nbsp;•&nbsp; in-memory

| Artifact | Location |
| --- | --- |
| CLI executable | `build/vns_sim` |
| Core library | `libvns_core` |
| Test suites | `build/test_*` + `build/vns_tests` |
| Kernel module (Linux) | `kernel/vns_control/vns_control.ko` |
| Extended docs | `docs/` |

Happy simulating! 🌐