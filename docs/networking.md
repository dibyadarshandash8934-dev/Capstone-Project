# VNS Networking Documentation

## Overview

This document explains the networking concepts implemented in the Virtual NAT
Gateway Simulator (VNS).

## IP Addressing

### IPv4 Address Structure

```
32-bit address: a.b.c.d where each octet is 0-255
Example: 192.168.1.10 = 0xC0A8010A = 3232235786
```

### CIDR Notation

```
Network/Prefix: 192.168.1.0/24
- Network: 192.168.1.0
- Prefix: /24 (24 bits for network, 8 bits for hosts)
- Netmask: 255.255.255.0 (0xFFFFFF00)
- Hosts: 254 usable (192.168.1.1 - 192.168.1.254)
```

### Special Addresses

- **Network Address**: First address (all host bits = 0) - not assignable
- **Broadcast Address**: Last address (all host bits = 1) - not assignable
- **Gateway**: Typically first usable host IP

## NAT Concepts

### SNAT (Source NAT) / PAT (Port Address Translation)

```
Outbound: Private → Public
──────────────────────────────────────────────────────────
Client:     192.168.1.10:50000          → 8.8.8.8:443 (TCP)
                │
                ▼ NAT Gateway (SNAT/PAT)
                │
Gateway:    203.0.113.10:40001    → 8.8.8.8:443 (TCP)
                │
                ▼ Internet
Server:                 8.8.8.8:443 ← 203.0.113.10:40001
```

**Key Points:**

- Only source IP and port change
- Destination unchanged
- Port allocated from pool (40000-50000)
- Mapping reused for same 5-tuple flow

### Reverse NAT (Response)

```
Response: Server → Client
──────────────────────────────────────────────────────────
Server:     8.8.8.8:443          → 203.0.113.10:40001
                │
                ▼ NAT Gateway (Reverse NAT)
                │
Gateway:                 192.168.1.10:50000 ← 8.8.8.8:443
                │
                ▼
Client:     192.168.1.10:50000 ← 8.8.8.8:443
```

### DNAT / Port Forwarding

```
Inbound: Internet → LAN
──────────────────────────────────────────────────────────
Internet:   198.51.100.20:45000  →  203.0.113.10:8080
                    │
                    ▼ NAT Gateway (DNAT)
                    │
Gateway:              192.168.1.20:80  ←  198.51.100.20:45000
                            │
                            ▼
Server:      192.168.1.20:80  ←  198.51.100.20:45000
```

**Key Points:**

- Only destination IP and port change
- Source unchanged (preserved for response)
- Requires explicit port forwarding rule
- TCP and UDP can share same public port

## NAT Table Structure

### Forward Table (Outbound)

```
Key: (protocol, private_ip, private_port, dest_ip, dest_port)
Value: {public_ip, public_port, state, timestamps}
```

### Reverse Table (Inbound Response)

```
Key: (protocol, public_ip, public_port, dest_ip, dest_port)
Value: Same NATEntry reference
```

### State Machine

NAT entries only carry `NATState::ACTIVE` / `NATState::EXPIRED`. The full
five-state machine belongs to the connection tracker:

```
NEW → ACTIVE → ESTABLISHED → CLOSED/EXPIRED
```

### PAT Port Allocation

- Range: 40000-50000 (10,001 ports, configurable via the `NatEngineImpl`
  constructor)
- Deterministic: sequential with wrap-around
- Collision detection via hash set
- `release()` frees an individual port (used by tests / `reset()`)
- Exhaustion handling: returns error

## Port Forwarding Rules

### Rule Structure

```
Rule = {
    protocol: TCP | UDP
    public_ip: 203.0.113.10      (must match NAT public IP)
    public_port: 8080
    private_ip: 192.168.1.20
    private_port: 80
    enabled: true
}
```

### Validation Rules

1. Public IP must match NAT gateway public IP
2. Private IP must be in LAN subnet
3. Private IP must have a registered device
4. Protocol + public_ip + public_port must be unique
5. TCP and UDP can share same public port
6. Port range: 1-65535

### Conflict Detection

```
Conflict if: (proto1 == proto2) AND (pub_ip1 == pub_ip2) AND (pub_port1 == pub_port2)
TCP:8080 and UDP:8080 → OK (different protocols)
TCP:8080 and TCP:8080 → CONFLICT
```

## Packet Flow Examples

### Example 1: SNAT (Outbound HTTPS)

```
Client:     192.168.1.10:54321  →  142.250.190.46:443 (TCP)
                      │
                      ▼ NAT Gateway
                      │ SNAT: 192.168.1.10:54321 → 203.0.113.10:40000
Internet:   203.0.113.10:40000 → 142.250.190.46:443
```

### Example 2: DNAT (Inbound HTTP)

```
Internet:   198.51.100.20:55555 → 203.0.113.10:8080
                       ↓ DNAT
Server:              192.168.1.20:80
```

### Example 3: Reverse NAT (HTTPS Response)

```
Google:     142.250.190.46:443 → 203.0.113.10:40001
                       │
                       ▼ Reverse NAT
Private:    192.168.1.10:54321 ← 142.250.190.46:443
```

## Connection Tracking

### State Transitions

```
NEW → ACTIVE → ESTABLISHED → CLOSED
                    ↓
              EXPIRED (timeout)
```

### Timeout Values

- Default connection timeout: 5 minutes (300 seconds) for every protocol
  (`ConnectionTracker` default `timeout_ns`; construct it with a custom
  `timeout_ns` to change it)

### Cleanup

- Expired connections are removed when `ConnectionTracker::remove_expired()` is
  called explicitly (there is no background thread)
- NAT entries stay in the table until cleared — `clear_nat_table()` (menu option
  `3. Configure NAT` → `2. Clear NAT Table`) or `SimulationService::reset()`
  (menu option `10. Reset Simulation`)
- PAT ports are freed only by `PatAllocatorImpl::release()` (exercised by the
  test suite) or by `PatAllocatorImpl::reset()`

## Error Conditions

| Condition          | Behavior                       |
| ------------------ | ------------------------------ |
| Unknown source IP  | Packet dropped, error logged   |
| IP outside network | Packet dropped, error logged   |
| Port exhausted     | Packet dropped, error returned |
| No DNAT rule       | Packet dropped, error returned |
| No reverse mapping | Packet dropped, error logged   |
| Port collision     | Retry with next available port |
| Port exhausted     | Error returned to caller       |

## Implementation Notes

### Thread Safety

The simulator is single-threaded (one CLI, in-process engine):

- Logger: mutex-protected
- NAT table, PAT allocator, connection tracker, metrics: not synchronized — they
  are only ever used from the calling thread (The kernel module has its own
  mutex protecting its shared state.)

### Performance Considerations

- Hash tables for O(1) lookups
- Deterministic PAT allocation (no random)
- Minimal allocations in hot path
- Batch statistics updates

### Limits

- No hard cap on NAT table entries, port-forward rules, or connections — tables
  grow until cleared (`clear_nat_table()`, `SimulationService::reset()`)
- Port range: 40000-50000 by default (10,001 ports, settable via the
  `NatEngineImpl` constructor; must be within 1024-65535)

## References

- RFC 3022: Traditional NAT
- RFC 3489: STUN
- RFC 4787: NAT Behavioral Requirements
- RFC 6887: Port Control Protocol
- Linux kernel networking documentation
