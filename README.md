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

## Port Forwarding Simulator

### What it is

The **Port Forwarding Simulator** is the DNAT (Destination NAT) half of VNS. It
lets you define rules that map a port on the gateway's public IP to a port on a
host inside the private LAN, and then simulate the traffic that flows through
those rules. Everything runs in user space against the in-memory
`PortForwardingImpl` rule list owned by `NatEngineImpl` — no real network, no
root, no configuration files.

### Purpose

A private LAN is normally unreachable from the Internet because its hosts use
RFC 1918 addresses and the gateway only translates *outgoing* traffic
(SNAT/PAT). Port forwarding solves this for one service at a time: instead of
exposing the whole LAN, you publish exactly one public port and let the gateway
rewrite the destination of any inbound packet that matches it.

### How it works

A rule is just a match and a rewrite. The match key is
`(protocol, public IP, public port)` and the rewrite target is
`(private IP, private port)`:

```text
Public IP:Port
      ↓
Port Forwarding Rule
      ↓
Private IP:Port
```

Realistic example — a web server on the LAN published on public port 8080:

```text
203.0.113.5:8080
        ↓
192.168.1.20:80
```

An Internet client sends `TCP 198.51.100.7:51515 -> 203.0.113.5:8080`. The
gateway finds the matching rule, rewrites only the destination, and forwards
`TCP 198.51.100.7:51515 -> 192.168.1.20:80` to the LAN host.

### DNAT flow in the simulator

`Simulate Inbound Packet` (menu 7) runs this path
(`NatEngineImpl::process_incoming_packet`):

1. `ORIGINAL` — record the packet as received from the Internet.
2. Look up a **rule** whose match key equals `(protocol, destination IP,
   destination port)`. Disabled rules are skipped.
3. `DNAT` — rewrite the destination to the rule's private IP and private port.
   If no rule matches, the DNAT step fails and the gateway falls back to the
   reverse-NAT path for the packet.
4. Record a NAT entry (`private <-> public`, state `ACTIVE`) and register the
   flow with the connection tracker.
5. `FORWARD` — deliver the translated packet to the LAN device and count a
   success in the metrics.

The return direction is handled by the same rule: an outgoing packet whose
source is exactly the rule's private IP and private port reuses the rule's
public port instead of taking a port from the PAT pool.

### Supported capabilities

| Capability                     | Behaviour                                                                                  |
| ------------------------------ | ------------------------------------------------------------------------------------------ |
| Protocols                      | `TCP` and `UDP`; rules of different protocols are independent                                |
| Public IP                      | Always the gateway's public IP from menu 1 — a rule cannot point anywhere else             |
| Private IP                     | Must be inside the LAN CIDR **and** must already exist as a virtual host (menu 2)            |
| Ports                          | Prompted as `1-65535`; a zero port is rejected (`Invalid parameters.`)                      |
| Duplicate rules                | Rejected — two enabled rules may not share the same protocol + public IP + public port      |
| Rule identity                  | `<PROTOCOL>-<public IP>-<public port>`, e.g. `TCP-203.0.113.5-8080`                        |
| Validation errors              | Surfaced by the CLI as `Invalid parameters.` or `Failed to add rule (conflict or invalid device?).` |
| Visibility                     | `List Rules` table with `Status` (`ENABLED`/`DISABLED`), `View NAT Table`, `View Connections`|
| Observability                  | Per-packet transformation trace plus `DNAT Packets` in the statistics                       |
| Lifecycle                      | `Add Rule`, `List Rules`, `Delete Rule`; cleared by `Reset Simulation` (menu 10)             |

### Relation to DNAT and NAT

Port forwarding *is* DNAT in this project — `include/vns/nat/dnat.hpp` documents
that DNAT is implemented as port-forwarding rules, so there is no separate DNAT
code path. It is the mirror image of SNAT/PAT:

| Direction                     | Feature       | Reads / writes                |
| ----------------------------- | ------------- | ----------------------------- |
| LAN → Internet                | SNAT / PAT    | private IP:port → public IP:allocated port |
| Internet → LAN                | DNAT          | public IP:rule port → private host IP:rule port |
| Internet → LAN (established)  | Reverse NAT   | NAT table reverse flow lookup |

Rules and the NAT table are independent: clearing the NAT table (menu 3 → 2)
removes mappings but keeps port-forwarding rules, and deleting a rule does not
clear the table.

### Configuring a rule from the CLI

1. `1` **Configure Network** — enter CIDR, gateway IP and public NAT IP.
2. `2` **Manage Virtual Hosts** → `1` **Add Device** — add the private host that
   will receive the traffic (its IP is required by the rule validation).
3. `5` **Manage Port Forwarding** → `1` **Add Rule**, then answer the prompts:

```text
Add Port Forwarding Rule
Protocol (TCP/UDP) [TCP]: TCP
Public Port (1-65535): 8080
Private IP: 192.168.1.20
Private Port (1-65535): 80
Port forwarding rule added!
```

The public IP is taken from the network configuration, so it is not asked for.
Then use `2` **List Rules** to display the table and `3` **Delete Rule** to remove
one by its ID.

### Running and testing

```bash
./scripts/build.sh                        # build
./build/vns_sim                           # interactive menu, then use 5 and 7
./scripts/run.sh                          # build if needed, then start the CLI
./scripts/run.sh --test                   # ctest + all 7 suites
cd build && ctest --output-on-failure     # every suite
./build/test_dnat                         # port-forwarding / DNAT suite only
./build/test_nat                          # also covers DNAT and the reply path
```

### Example output

Listing the rule created above (`5` → `2`):

```text
============================================================
  PORT FORWARDING RULES
============================================================
ID                   | Protocol | Public                 | Private                | Status
---------------------+----------+------------------------+------------------------+---------
TCP-203.0.113.5-8080 | TCP      | 203.0.113.5:8080       | 192.168.1.20:80        | ENABLED
```

Simulating an inbound packet (`7`):

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

The reverse direction (`6`, packet from the LAN host on the forwarded port):

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
