# VNS Device Driver Documentation

## Overview

The `vns_control` kernel module provides a character device interface
(`/dev/vns_control`) for communication between the C++ userspace simulator and
the Linux kernel. It exposes simulation statistics, status, and control commands
via ioctl, read, and write interfaces.

**Platform note:** the module builds only on Linux with matching kernel headers
(not on macOS). It is entirely optional — the simulator runs without it, and
`DriverInterface` calls then fail gracefully with `Driver not available`.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                      Userspace (C++)                            │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │ DriverInterface (C++)                                     │  │
│  │  - open("/dev/vns_control")                               │  │
│  │  - ioctl(VNS_IOCTL_GET_STATS, &stats)                     │  │
│  │  - ioctl(VNS_IOCTL_GET_STATUS, &status)                   │  │
│  │  - ioctl(VNS_IOCTL_RESET_STATS)                           │  │
│  │  - read()/write() for human-readable status               │  │
│  └───────────────────────────────────────────────────────────┘  │
│                              │                                  │
│                    ioctl / read / write                         │
│                              ▼                                  │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Kernel Space                                 │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │ vns_control Kernel Module                                 │  │
│  │  - Character device: /dev/vns_control                     │  │
│  │  - Major/Minor dynamic allocation                         │  │
│  │  - Mutex-protected shared state                           │  │
│  │  - Statistics counters (mutex-protected)                  │  │
│  │  - ioctl handlers                                         │  │
│  │  - read()/write() file operations                         │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## Device Interface

### Device Node

```
/dev/vns_control
Major: dynamic (auto-assigned)
Minor: 0
Permissions: 0666 (configurable via udev rules)
```

### File Operations

| Operation   | Purpose                                          |
| ----------- | ------------------------------------------------ |
| `open()`    | Acquire device access, mark simulator as running |
| `release()` | Release device, mark simulator as stopped        |
| `read()`    | Human-readable status/statistics text            |
| `write()`   | Simple commands: "reset", "status", "stop"       |
| `ioctl()`   | Structured control/status operations             |

## IOCTL Commands

Defined in `include/vns/vns_ioctl.h` with magic `'V'`:

| Command                   | Code           | Direction   | Structure              | Description            |
| ------------------------- | -------------- | ----------- | ---------------------- | ---------------------- |
| `VNS_IOCTL_GET_STATS`     | `_IOR('V', 1)` | Kernel→User | `struct vns_stats`     | Get all statistics     |
| `VNS_IOCTL_RESET_STATS`   | `_IO('V', 2)`  | —           | —                      | Reset all counters     |
| `VNS_IOCTL_GET_STATUS`    | `_IOR('V', 3)` | Kernel→User | `struct vns_status`    | Get status flags       |
| `VNS_IOCTL_SET_STATUS`    | `_IOW('V', 4)` | User→Kernel | `struct vns_status`    | Set status flags       |
| `VNS_IOCTL_GET_NAT_ENTRY` | `_IOR('V', 5)` | Kernel→User | `struct vns_nat_entry` | Get NAT entry (future) |
| `VNS_IOCTL_RESET_NAT`     | `_IO('V', 6)`  | —           | —                      | Clear NAT table        |

### Data Structures

#### vns_stats

```c
struct vns_stats {
    unsigned long long total_packets;
    unsigned long long successful_packets;
    unsigned long long failed_packets;
    unsigned long long snat_packets;
    unsigned long long dnat_packets;
    unsigned long long reverse_nat_packets;
    unsigned long long active_nat_mappings;
    unsigned long long active_port_forward_rules;
    unsigned long long total_processing_time_ns;
};
```

#### vns_status

```c
struct vns_status {
    unsigned int status_flags;      // Bit flags: RUNNING=0x01, ERROR=0x02
    unsigned int active_connections;
    unsigned int nat_table_entries;
    unsigned int port_forward_rules;
    unsigned int simulator_running;
    char simulator_version[32];
};
```

#### vns_nat_entry

```c
struct vns_nat_entry {
    unsigned char protocol;         // 6=TCP, 17=UDP, 1=ICMP
    unsigned int private_ip;        // Network byte order
    unsigned short private_port;
    unsigned int public_ip;
    unsigned short public_port;
    unsigned int destination_ip;
    unsigned short destination_port;
    unsigned char state;            // 0=NEW, 1=ACTIVE, 2=ESTABLISHED, etc.
    unsigned long long created_time;
    unsigned long long last_activity;
};
```

## Module Lifecycle

### Initialization (`module_init`)

1. Allocate driver state structure (`kmalloc`)
2. Initialize mutex (`mutex_init`)
3. Allocate character device region (`alloc_chrdev_region`)
4. Initialize cdev (`cdev_init`, `cdev_add`)
5. Create device class (`class_create`)
6. Create device node (`device_create`)
7. Initialize default values

### Cleanup (`module_exit`)

1. Destroy device node (`device_destroy`)
2. Destroy class (`class_destroy`)
3. Remove cdev (`cdev_del`)
4. Unregister character device region (`unregister_chrdev_region`)
5. Free driver state (`kfree`)

## Synchronization

### Mutex Protection

All shared state is protected by a mutex (`struct mutex lock`):

- Statistics counters
- NAT table entries count
- Port forward rules count
- Status flags
- Simulator version string

### Atomic Operations

For simple counters where mutex overhead is undesirable, consider `atomic64_t`:

```c
atomic64_t total_packets;
atomic64_inc(&total_packets);
```

## Data Transfer

### copy_to_user / copy_from_user

All user/kernel data transfers use safe kernel APIs:

```c
// Kernel → User
if (copy_to_user(user_ptr, &kernel_struct, sizeof(kernel_struct)))
    return -EFAULT;

// User → Kernel
if (copy_from_user(&kernel_struct, user_ptr, sizeof(kernel_struct)))
    return -EFAULT;
```

### Buffer Validation

- Always validate user buffer sizes
- Check bounds before copying
- Handle partial reads/writes correctly

## Building the Module

### Makefile

```make
obj-m += vns_control.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod vns_control.ko

unload:
	sudo rmmod vns_control
```

### Build Requirements

- **Linux only** (the module cannot build on macOS)
- Linux kernel headers (`linux-headers-$(uname -r)`)
- GCC with kernel build support
- Module signing (if Secure Boot enabled)

## Loading/Unloading

```bash
# Build
cd kernel/vns_control && make

# Load (requires root)
sudo insmod vns_control.ko

# Verify
ls -l /dev/vns_control
dmesg | tail -5

# Unload
sudo rmmod vns_control

# Verify unload
ls -l /dev/vns_control  # Should not exist
```

## Userspace Integration

### C++ DriverInterface

```cpp
#include "vns/driver/driver_interface.hpp"
#include <iostream>

using namespace vns;

DriverInterface driver;
if (driver.open("/dev/vns_control")) {          // default path
    if (auto stats = driver.get_stats()) {      // std::optional<VnsStats>
        std::cout << "Packets: " << stats->total_packets() << std::endl;
    }

    if (auto status = driver.get_status()) {    // std::optional<VnsStatus>
        if (status->simulator_running()) {
            // Simulator is running
        }
    }

    driver.reset_stats();                       // bool
    driver.close();
}
```

### Error Handling

```cpp
DriverInterface driver;
if (!driver.open("/dev/vns_control")) {
    std::cerr << "Error: " << driver.last_error() << std::endl;
    // Graceful degradation - continue without driver
}
```

## Error Handling

### Kernel Side

- Return `-EFAULT` for bad user pointers
- Return `-ENOTTY` for unknown ioctl commands
- Return `-ENODEV` if device state not initialized
- Return `-EINVAL` for invalid arguments
- Return `-EFAULT` for copy_to_user/copy_from_user failures
- Return `-EPERM` for permission issues (if enforced)

### Userspace Side

- Check return values of all ioctl/read/write calls
- Handle `ENOENT` (device not found)
- Handle `EACCES` (permission denied)
- Handle `EFAULT` (bad address)
- Graceful degradation when driver unavailable

## Security Considerations

### Access Control

- Device permissions: `0666` (world readable/writable)
- Consider udev rules for restricted access:
  ```
  KERNEL=="vns_control", MODE="0660", GROUP="vns"
  ```

### Input Validation

- Validate all ioctl arguments
- Bounds check array indices
- Verify structure sizes
- Check for integer overflows

### Memory Safety

- No arbitrary kernel memory access
- Bounded copy operations
- Proper cleanup on error paths
- No memory leaks on error paths

## Testing the Driver

### Manual Testing

```bash
# Load module
sudo insmod vns_control.ko

# Check device
ls -l /dev/vns_control

# Read status
cat /dev/vns_control

# Write command
echo "reset" > /dev/vns_control

# IOCTL test (using the built test binary, from the build directory)
./build/test_driver_interface

# Unload
sudo rmmod vns_control
```

### Automated Testing

The userspace test suite includes:

- Device open/close
- Move semantics
- Stats retrieval
- Status retrieval
- Read/write operations
- Ioctl commands
- Error handling (non-existent device)

## Debugging

### Kernel Logs

```bash
dmesg -T | grep VNS
```

### Debug Output

The module uses `pr_info`, `pr_err`, `pr_debug`:

```c
pr_info("VNS: Device opened\n");
pr_err("VNS: Failed to allocate device number\n");
```

### Debugfs (if enabled)

```bash
# Mount debugfs
mount -t debugfs none /sys/kernel/debug

# Check module info
cat /sys/kernel/debug/vns_control/*
```

## Module Parameters

Future enhancement: add module parameters for configuration:

```c
static unsigned int port_range_start = 40000;
module_param(port_range_start, uint, 0644);
MODULE_PARM_DESC(port_range_start, "PAT port range start");
```

## Future Enhancements

- Netlink socket for async notifications
- Procfs/sysfs integration for stats
- Per-CPU counters for better scalability
- eBPF integration for packet inspection
- Multi-queue support for high throughput

## References

- LDD3: Linux Device Drivers, 3rd Edition
- Linux Kernel Module Programming Guide
- Documentation/driver-api/
- include/uapi/asm-generic/ioctl.h
- Documentation/ioctl/ioctl-number.txt
