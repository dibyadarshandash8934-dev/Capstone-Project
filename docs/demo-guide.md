# VNS Demo Guide

## 5-Minute Demo Script

### Pre-Demo Setup (30 seconds before)

```bash
# Terminal 1: build (once) and start the CLI
cd vns_new
./scripts/build.sh                 # or: cmake -S . -B build && cmake --build build -j
./build/vns_sim

# Terminal 2 (optional, Linux only): load the kernel module for driver stats
cd vns_new/kernel/vns_control
sudo insmod vns_control.ko
ls -l /dev/vns_control
```

---

## Demo Sequence (5 Minutes)

### 0:00 – 0:30 | Introduction (30 sec)

**Say:** "This is the Virtual NAT Gateway & Port-Forwarding Simulator, a C++20
CLI application. It demonstrates SNAT, DNAT, and reverse NAT with a real
in-process NAT engine, plus an optional Linux kernel module for telemetry."

**Show:** The interactive menu and the `[stats] packets=0 ok=0 failed=0` line
printed above it.

---

### 0:30 – 1:00 | Create Private LAN (30 sec)

**Actions:**

1. Choose `1. Configure Network` and enter:
   - CIDR: `192.168.1.0/24`
   - Gateway IP: `192.168.1.1`
   - Public NAT IP: `203.0.113.5`
2. Press Enter

**Say:** "First, I'll create a private LAN with CIDR 192.168.1.0/24. The gateway
is 192.168.1.1, and the NAT gateway's public IP is 203.0.113.5 — a reserved
documentation address."

**Show:** `Network created successfully!` and the `[stats]` line that follows on
each menu redraw.

---

### 1:00 – 1:30 | Add Virtual Device (30 sec)

**Actions:**

1. Choose `2. Manage Virtual Hosts` → `1. Add Device` and enter:
   - Name: `PC-01`
   - IP: `192.168.1.10`
   - Type: `pc`
2. Choose `2. List Devices` to show it

**Say:** "Now I'll add a virtual PC to our LAN. The IP 192.168.1.10 is validated
to be within our 192.168.1.0/24 subnet. The simulator rejects IPs outside the
subnet, network/broadcast addresses, and duplicates."

**Show:** `Device added successfully!` and the device in the list.

---

### 1:30 – 2:00 | Explain NAT Gateway (30 sec)

**Say:** "The NAT gateway runs inside the simulator: LAN side is 192.168.1.1,
the public IP is 203.0.113.5, and PAT ports are allocated deterministically from
40000–50000. The `[stats]` line shows packet totals after every operation;
option 9 shows the kernel driver's counters when the module is loaded."

**Point to:** `[stats]` line, menu option `9. View Driver Statistics`

---

### 2:00 – 2:30 | Send Outbound Packet (SNAT) (30 sec)

**Actions:**

1. Choose `6. Simulate Outbound Packet` and enter:
   - Protocol: `TCP`
   - Source IP: `192.168.1.10`
   - Source Port: `50000`
   - Destination IP: `8.8.8.8`
   - Destination Port: `443`
2. Press Enter

**Say:** "Now I'll simulate an outbound HTTPS request from our PC to Google DNS.
Watch the output: the packet is translated at the NAT gateway, then continues to
the Internet."

**Watch:** The `PACKET SIMULATION RESULT` block: `Action: SNAT`, BEFORE NAT /
AFTER NAT lines, and the transformation steps.

**Point out:** "Notice SNAT: private 192.168.1.10:50000 becomes public
203.0.113.5:40000. The destination stays unchanged."

---

### 2:30 – 3:00 | Show NAT Translation Table (30 sec)

**Actions:**

1. Choose `4. View NAT Table`

**Say:** "The NAT table now shows our active mapping. The private
192.168.1.10:50000 is mapped to public 203.0.113.5:40000 for destination
8.8.8.8:443. If the same flow is simulated again, the same public port is reused
— no new allocation."

**Show:** NAT table entry and the `Active NAT Mappings` counter in the stats
line

---

### 3:00 – 3:30 | Add Port Forward Rule (30 sec)

**Actions:**

1. Choose `5. Manage Port Forwarding` → `1. Add Rule` and enter:
   - Protocol: `TCP`
   - Public Port: `8080`
   - Private IP: `192.168.1.20`
   - Private Port: `80`
2. Choose `2. List Rules` to show it

**Say:** "Now I'll configure port forwarding. External traffic to port 8080 on
our public IP will be forwarded to our web server at 192.168.1.20 on port 80.
The simulator validates that the private IP exists in our LAN with a registered
device and that the rule doesn't conflict with existing rules."

**Show:** Rule appears in the list; the port-forward counter increments

---

### 4:00 – 4:30 | Simulate Inbound Packet (DNAT) (30 sec)

**Actions:**

1. Choose `7. Simulate Inbound Packet` and enter:
   - Protocol: `TCP`
   - Source IP: `198.51.100.20`
   - Source Port: `45000`
   - Destination IP: `203.0.113.5`
   - Destination Port: `8080`
2. Press Enter

**Say:** "Now I'll simulate an inbound HTTP request from the Internet to our
public IP on port 8080. This triggers DNAT — the destination gets translated to
our internal web server."

**Watch:** `PACKET SIMULATION RESULT` with `Action: DNAT`

**Point out:** "DNAT: public destination 203.0.113.5:8080 becomes private
192.168.1.20:80. The source IP/port stays unchanged."

---

### 4:30 – 5:00 | Show Metrics & Architecture (30 sec)

**Actions:**

1. Choose `3. Configure NAT` → `3. View Statistics`
2. Choose `8. View Connections` to show tracked flows
3. Choose `10. Reset Simulation` to clean up

**Say:** "The statistics view shows real-time metrics: total packets, broken
down by SNAT, DNAT, and reverse NAT, plus average processing time. Everything
runs in-process — it's a pure C++ simulation with no sockets, so processing is
sub-millisecond per packet. The engine is exposed through `SimulationService`,
the same layer the CLI uses, so results are available programmatically."

**Reset:** "The reset option clears all state — network, devices, NAT mappings,
port forwards, connections, metrics, and driver counters — returning to a clean
slate."

---

## Key Talking Points During Demo

| Moment           | Emphasize                                                                                     |
| ---------------- | --------------------------------------------------------------------------------------------- |
| Network creation | IPv4/CIDR validation via `Ipv4Address::from_string` / `Cidr::from_string`, not string parsing |
| Device addition  | Subnet validation, no duplicates, no network/broadcast                                        |
| SNAT output      | Source IP/port changes, destination unchanged                                                 |
| NAT table        | Mapping reuse for same flow                                                                   |
| DNAT output      | Destination IP/port changes, source unchanged                                                 |
| Port forwarding  | Validation: public IP matches gateway, private IP has a registered device                     |
| Reverse NAT      | Response packet translated back automatically                                                 |
| Metrics          | Sub-millisecond processing, per-action breakdown                                              |
| Reset            | Complete state cleanup                                                                        |

---

## Common Questions & Answers

| Question                              | Answer                                                                                                                                                                               |
| ------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| "Does this use real network packets?" | No, it's a pure software simulation. The NAT engine runs in-process with no socket operations.                                                                                       |
| "Can this handle real traffic?"       | No, this is an educational simulator. The C++ NAT engine could theoretically be adapted, but lacks kernel integration, performance optimization, and production hardening.           |
| "How does the output work?"           | Each simulation prints structured transformation steps (stage, action, before/after IP:port) plus the resulting `SimulationResult` (`success()`, `action()`, `translated_packet()`). |
| "Why C++ for the NAT engine?"         | C++20 gives value semantics, `std::optional` for fallible operations, and deterministic PAT allocation with no runtime dependencies or framework lock-in.                            |
| "How would you scale this?"           | The NAT engine is completely decoupled from the CLI behind `SimulationService`. For production: persist state, add a network service layer, metrics export, authentication.          |

---

## Troubleshooting During Demo

| Issue                                              | Fix                                                                                 |
| -------------------------------------------------- | ----------------------------------------------------------------------------------- |
| `Driver not available` / option 9 fails            | Kernel module not loaded (Linux only) — expected on macOS; load it or skip option 9 |
| `Network not configured. Create network first.`    | Create the network first (option 1)                                                 |
| `Invalid IP or IP not in network range.`           | Use an address inside the configured CIDR (not network/broadcast)                   |
| Port forward fails                                 | Verify a device exists in the LAN with that exact private IP                        |
| `Failed to add rule (conflict or invalid device?)` | Protocol + public port must be unique; private IP needs a registered device         |
| Stats show 0                                       | Run simulations first; the `[stats]` line refreshes on every menu redraw            |

---

## Post-Demo Cleanup

1. Choose `10. Reset Simulation` and confirm with `y`
2. Verify the stats line is back to `packets=0 ok=0 failed=0`
3. Exit with `0` (`Goodbye!`) or Ctrl+C
4. Unload kernel module (Linux only): `sudo rmmod vns_control`

---

## Backup Plan (If Live Demo Fails)

1. **Screenshots** of key CLI output — capture them into `docs/screenshots/`
   before the demo (create the folder when you do)
2. **Recorded terminal session** of the full demo flow (e.g. saved with
   `script`/asciinema)
3. **Saved output transcript** such as `docs/demo-output.txt`
4. **Static slides** with key diagrams

---

_Demo guide for Wipro Capstone Project_
