Virtual NAT Gateway & Port Forwarding Simulator

A simple C++20-based Virtual NAT Gateway Simulator that demonstrates how NAT works in a private network.

The project simulates SNAT/PAT, DNAT, Port Forwarding, Packet Processing, Connection Tracking, and NAT Tables through an interactive CLI.

Note: This is a simulation. It does not send or intercept real network packets.

⸻

🚀 Main Features

* 🌐 Virtual Network Configuration
* 💻 Virtual Host Management
* 🔄 SNAT / PAT
* 🔀 DNAT / Port Forwarding
* 📦 Packet Simulation
* 🔗 Connection Tracking
* 📊 NAT Translation Table
* 📈 Simulation Metrics
* 🖥️ Interactive CLI
* 🐧 Linux Kernel Driver Interface

⸻

🔄 How NAT Works

Outbound Traffic — SNAT/PAT

A private device sends a packet to the Internet:

Private Device
192.168.1.10:50000
        |
        | SNAT / PAT
        ↓
NAT Gateway
203.0.113.5:<translated-port>
        |
        ↓
Internet
8.8.8.8:443

The simulator creates and maintains the translation so that the response can return to the correct private device.

⸻

🔀 Port Forwarding — DNAT

Port forwarding allows an Internet request to reach a private server.

Example:

Internet
   |
   | 203.0.113.5:8080
   ↓
NAT Gateway
   |
   | DNAT
   ↓
192.168.1.20:80
   |
   ↓
Private Server

The simulator supports both TCP and UDP forwarding.

Example rule:

203.0.113.5:8080
        ↓
192.168.1.20:80

⸻

📦 Packet Simulation

The simulator shows how a packet changes during NAT processing.

BEFORE
203.0.113.5:8080
        |
        ↓
      DNAT
        |
        ↓
AFTER
192.168.1.20:80

This makes the NAT process easy to understand without using real network traffic.

⸻

🖥️ Interactive CLI

Run the simulator and use the menu:

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

🛠️ Technology Used

* C++20
* C
* CMake
* Make
* Linux Kernel Module
* Linux Character Device
* CLI-based Interface

⸻

🏗️ Simple Architecture

                CLI
                 |
                 ↓
        Simulation Service
                 |
        ┌────────┴────────┐
        ↓                 ↓
    NAT Engine      Connection Tracker
        |
   ┌────┴─────┐
   ↓          ↓
 SNAT/PAT    DNAT
   |          |
   └────┬─────┘
        ↓
   Packet Processing
        |
        ↓
 Driver Interface
        |
        ↓
 Linux Character Device

⸻

🐧 Linux Kernel Driver

The project also includes a Linux kernel module that demonstrates communication between the C++ simulator and the Linux kernel.

C++ Simulator
      |
      | ioctl / read / write
      ↓
/dev/vns_control
      |
      ↓
Linux Kernel Module

The kernel module is Linux-specific.

The main simulator can run without the driver. Driver functionality requires a Linux system with the appropriate kernel headers.

⸻

🧪 Testing

The project includes 7 test suites covering:

* Network configuration
* NAT
* PAT
* DNAT
* Packet processing
* Connection tracking
* Driver interface

Run the tests:

cd build
ctest --output-on-failure

⸻

▶️ How to Run

Build

./scripts/build.sh

Start

./build/vns_sim

Help

./build/vns_sim --help

Version

./build/vns_sim --version

⸻

📁 Project Structure

vns_new/
├── include/        # Header files
├── src/            # C++ source code
├── tests/          # Test suites
├── scripts/        # Build and run scripts
├── kernel/         # Linux kernel module
├── docs/           # Project documentation
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

📌 Project Status

The project demonstrates the complete simulated flow:

Virtual Network
      ↓
NAT Gateway
      ↓
SNAT / PAT
      ↓
DNAT / Port Forwarding
      ↓
Packet Transformation
      ↓
Connection Tracking

The simulator uses in-memory state and does not require a database, root privileges, or real network traffic.

⸻

👨‍💻 Project

Virtual NAT Gateway & Port Forwarding Simulator

Built using C++20 with Linux Kernel Driver Support.
