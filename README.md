

#  Virtual NAT Gateway & Port Forwarding Simulator
A **C++20-based Virtual NAT Gateway Simulator** that demonstrates how Network Address Translation works inside a private network.
The project simulates:
**SNAT/PAT • DNAT • Port Forwarding • Packet Processing • Connection Tracking • NAT Tables**
> **Note:** This is a simulation. It does not send or intercept real network packets.
---
#  Key Features
### 🌐 Virtual Network
Configure a private network using a **CIDR block**, gateway, and public NAT IP.
### 💻 Virtual Hosts
Create and manage simulated devices inside the private network.
### 🔄 SNAT / PAT
Simulate outbound traffic from private devices to the Internet.
### 🔀 DNAT / Port Forwarding
Forward incoming traffic from a public IP and port to a private device.
### 📦 Packet Simulation
Visualize how packets change during NAT processing.
### 🔗 Connection Tracking
Track active network connections and their translations.
### 📊 NAT Table
View the active NAT translation mappings.
###  Linux Kernel Driver
Includes an optional Linux character-device kernel module.
---
# 🔄SNAT / PAT
When a private device sends traffic to the Internet, the NAT gateway translates its private address into a public address.

Private Device
192.168.1.10:50000
        │
        │  SNAT / PAT
        ▼
NAT Gateway
203.0.113.5:<translated-port>
        │
        ▼
Internet
8.8.8.8:443

The simulator maintains the translation so that the response can return to the correct private device.

⸻

🔀 DNAT / Port Forwarding

Port forwarding allows incoming Internet traffic to reach a private device.

Internet
   │
   │ 203.0.113.5:8080
   ▼
NAT Gateway
   │
   │ DNAT
   ▼
192.168.1.20:80
   │
   ▼
Private Server

Example Rule

203.0.113.5:8080
        │
        ▼
192.168.1.20:80

The simulator supports both TCP and UDP forwarding.

⸻

📦 Packet Simulation

The simulator shows the packet before and after NAT processing.

BEFORE
203.0.113.5:8080
        │
        ▼
      DNAT
        │
        ▼
AFTER
192.168.1.20:80

This makes the NAT process easy to understand without using real network traffic.

⸻

🖥️ Interactive CLI

The simulator provides a simple menu-driven interface:

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

⸻

🏗️ Architecture

                    CLI
                     │
                     ▼
            Simulation Service
                     │
          ┌──────────┴──────────┐
          ▼                     ▼
      NAT Engine        Connection Tracker
          │
     ┌────┴─────┐
     ▼          ▼
  SNAT/PAT     DNAT
     │          │
     └────┬─────┘
          ▼
   Packet Processing
          │
          ▼
   Driver Interface
          │
          ▼
 Linux Character Device

⸻

🛠️ Technology Stack

Technology	Purpose
C++20	Main simulator
C	Linux kernel module
CMake	Build system
Make	Driver/build support
Linux Kernel	Device driver
CLI	User interface

⸻

🐧 Linux Kernel Driver

The project includes an optional Linux character-device kernel module.

C++ Simulator
      │
      │ ioctl / read / write
      ▼
/dev/vns_control
      │
      ▼
Linux Kernel Module

The kernel module is Linux-specific.

The main simulator can run without the driver. Driver functionality requires a Linux system with the appropriate kernel headers.

⸻

🧪 Testing

The project contains 7 test suites covering:

* Network configuration
* NAT
* PAT
* DNAT / Port Forwarding
* Packet processing
* Connection tracking
* Driver interface

Run all tests:

cd build
ctest --output-on-failure

⸻

▶️ How to Run

1. Build

./scripts/build.sh

2. Start

./build/vns_sim

3. Help

./build/vns_sim --help

4. Version

./build/vns_sim --version

⸻

📁 Project Structure

vns_new/
│
├── include/        # Header files
├── src/            # C++ source code
├── tests/          # Test suites
├── scripts/        # Build and run scripts
├── kernel/         # Linux kernel module
├── docs/           # Documentation
│
├── CMakeLists.txt
├── Makefile
└── README.md

⸻

📋 Requirements

* CMake 3.16+
* C++20 compiler
* Make
* Linux kernel headers for the kernel module

⸻

🎯 Project Flow

Virtual Network
       │
       ▼
 NAT Gateway
       │
       ├──────────────┐
       ▼              ▼
   SNAT / PAT       DNAT
       │              │
       └──────┬───────┘
              ▼
      Packet Processing
              │
              ▼
     Connection Tracking
              │
              ▼
         NAT Table

⸻

📌 Project Status

The simulator demonstrates the complete Virtual NAT Gateway and Port Forwarding workflow using C++20.

The project provides:

Virtual Network → NAT/PAT → DNAT/Port Forwarding → Packet Transformation → Connection Tracking

The simulator uses in-memory state and does not require a database, root privileges, or real network traffic.

⸻

👨‍💻 Project

Virtual NAT Gateway & Port Forwarding Simulator

Built with C++20 + Linux Kernel Driver Support
**clear large headings, smaller subheadings, bold keywords, tables, and code blocks**, instead of looking like one continuous block of identical text.
