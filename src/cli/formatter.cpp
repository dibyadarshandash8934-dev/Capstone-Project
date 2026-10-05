#include "vns/cli/formatter.hpp"
#include "vns/utils/helpers.hpp"
#include "vns/nat/nat.hpp"
#include "vns/network/network.hpp"
#include "vns/packet/packet.hpp"
#include "vns/connection/connection.hpp"
#include "vns/driver/driver_interface.hpp"
#include <iostream>
#include <iomanip>

namespace vns {

using utils::TableBuilder;
using utils::format_number;
using utils::format_duration_ns;

std::string format_ip_port(const Ipv4Address& ip, Port port) {
    return ip.to_string() + ":" + std::to_string(port);
}

std::string format_protocol(Protocol proto) {
    switch (proto) {
        case Protocol::TCP: return "TCP";
        case Protocol::UDP: return "UDP";
        case Protocol::ICMP: return "ICMP";
    }
    return "UNKNOWN";
}

std::string format_nat_state(NATState state) {
    switch (state) {
        case NATState::ACTIVE: return "ACTIVE";
        case NATState::EXPIRED: return "EXPIRED";
    }
    return "UNKNOWN";
}

std::string format_connection_state(ConnectionState state) {
    switch (state) {
        case ConnectionState::NEW: return "NEW";
        case ConnectionState::ACTIVE: return "ACTIVE";
        case ConnectionState::ESTABLISHED: return "ESTABLISHED";
        case ConnectionState::CLOSED: return "CLOSED";
        case ConnectionState::EXPIRED: return "EXPIRED";
    }
    return "UNKNOWN";
}

void print_separator(const std::string& title) {
    std::cout << "\n";
    std::cout << std::string(60, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(60, '=') << "\n";
}

void print_network_config(const NetworkConfig& network) {
    print_separator("NETWORK CONFIGURATION");
    std::cout << "  CIDR:        " << network.cidr().to_string() << "\n";
    std::cout << "  Gateway:     " << network.gateway_ip().to_string() << "\n";
    std::cout << "  Public IP:   " << network.public_ip().to_string() << "\n";
}

void print_devices(const std::vector<VirtualDevice>& devices) {
    print_separator("VIRTUAL DEVICES");
    if (devices.empty()) {
        std::cout << "  (no devices configured)\n";
        return;
    }
    
    TableBuilder table;
    table.add_column("ID", 12);
    table.add_column("Name", 16);
    table.add_column("IP", 16);
    table.add_column("Type", 10);
    table.add_column("Status", 10);
    
    for (const auto& d : devices) {
        table.add_row({d.id(), d.name(), d.ip().to_string(), d.type_string(), 
                      d.status() ? "ACTIVE" : "INACTIVE"});
    }
    
    std::cout << table.build();
}

void print_nat_table(const std::vector<NatEntry>& entries) {
    print_separator("NAT TRANSLATION TABLE");
    if (entries.empty()) {
        std::cout << "  (no active NAT mappings)\n";
        return;
    }
    
    TableBuilder table;
    table.add_column("Protocol", 8);
    table.add_column("Private", 22);
    table.add_column("Public", 22);
    table.add_column("Destination", 22);
    table.add_column("State", 10);
    
    for (const auto& e : entries) {
        table.add_row({
            format_protocol(e.protocol()),
            format_ip_port(e.private_ip(), e.private_port()),
            format_ip_port(e.public_ip(), e.public_port()),
            format_ip_port(e.destination_ip(), e.destination_port()),
            format_nat_state(e.state())
        });
    }
    
    std::cout << table.build();
}

void print_port_forward_rules(const std::vector<PortForwardRule>& rules) {
    print_separator("PORT FORWARDING RULES");
    if (rules.empty()) {
        std::cout << "  (no port forwarding rules)\n";
        return;
    }
    
    TableBuilder table;
    table.add_column("ID", 18);
    table.add_column("Protocol", 8);
    table.add_column("Public", 22);
    table.add_column("Private", 22);
    table.add_column("Status", 8);
    
    for (const auto& r : rules) {
        table.add_row({
            r.id(),
            format_protocol(r.protocol()),
            format_ip_port(r.public_ip(), r.public_port()),
            format_ip_port(r.private_ip(), r.private_port()),
            r.enabled() ? "ENABLED" : "DISABLED"
        });
    }
    
    std::cout << table.build();
}

void print_packet_simulation(const SimulationResult& result) {
    print_separator("PACKET SIMULATION RESULT");
    
    if (!result.success()) {
        std::cout << "  FAILED: " << result.error() << "\n";
        return;
    }
    
    std::cout << "Action: " << result.action() << "\n\n";
    
    std::cout << "  BEFORE NAT:\n";
    std::cout << "    " << result.original_packet().to_string() << "\n\n";
    
    if (result.translated_packet()) {
        std::cout << "  AFTER NAT:\n";
        std::cout << "    " << result.translated_packet()->to_string() << "\n\n";
    }
    
    std::cout << "  TRANSFORMATION STEPS:\n";
    for (const auto& t : result.transformations()) {
        std::cout << "    " << t.to_string() << "\n";
    }
    
    if (result.nat_entry()) {
        std::cout << "\n  NAT ENTRY:\n";
        std::cout << "    " << result.nat_entry()->to_string() << "\n";
    }
}

void print_connection_tracker(const std::vector<Connection>& connections) {
    print_separator("ACTIVE CONNECTIONS");
    if (connections.empty()) {
        std::cout << "  (no active connections)\n";
        return;
    }
    
    TableBuilder table;
    table.add_column("ID", 30);
    table.add_column("Protocol", 8);
    table.add_column("Source", 22);
    table.add_column("Destination", 22);
    table.add_column("State", 12);
    
    for (const auto& c : connections) {
        table.add_row({
            c.id(),
            format_protocol(c.protocol()),
            format_ip_port(c.source_ip(), c.source_port()),
            format_ip_port(c.destination_ip(), c.destination_port()),
            format_connection_state(c.state())
        });
    }
    
    std::cout << table.build();
}

void print_driver_stats(const vns::VnsStats& stats) {
    print_separator("DRIVER STATISTICS");
    std::cout << "  Total Packets:         " << format_number(stats.total_packets()) << "\n";
    std::cout << "  Successful Packets:    " << format_number(stats.successful_packets()) << "\n";
    std::cout << "  Failed Packets:        " << format_number(stats.failed_packets()) << "\n";
    std::cout << "  SNAT Packets:          " << format_number(stats.snat_packets()) << "\n";
    std::cout << "  DNAT Packets:          " << format_number(stats.dnat_packets()) << "\n";
    std::cout << "  Reverse NAT Packets:   " << format_number(stats.reverse_nat_packets()) << "\n";
    std::cout << "  Active NAT Mappings:   " << format_number(stats.active_nat_mappings()) << "\n";
    std::cout << "  Active Port Rules:     " << format_number(stats.active_port_forward_rules()) << "\n";
    std::cout << "  Avg Processing Time:   " << format_duration_ns(stats.total_processing_time_ns()) << "\n";
}

void print_driver_status(const VnsStatus& status) {
    print_separator("DRIVER STATUS");
    std::cout << "  Status Flags:    0x" << std::hex << status.status_flags() << std::dec << "\n";
    std::cout << "  Active Connections: " << status.active_connections() << "\n";
    std::cout << "  NAT Entries:       " << status.nat_table_entries() << "\n";
    std::cout << "  Port Forward Rules: " << status.port_forward_rules() << "\n";
    std::cout << "  Simulator Running: " << (status.simulator_running() ? "YES" : "NO") << "\n";
    std::cout << "  Version:           " << status.simulator_version() << "\n";
}

void print_simulation_stats(const SimulationMetrics& stats) {
    print_separator("SIMULATION STATISTICS");
    std::cout << "  Total Packets:         " << format_number(stats.total_packets) << "\n";
    std::cout << "  Successful Packets:    " << format_number(stats.successful_packets) << "\n";
    std::cout << "  Failed Packets:        " << format_number(stats.failed_packets) << "\n";
    std::cout << "  SNAT Packets:          " << format_number(stats.snat_packets) << "\n";
    std::cout << "  DNAT Packets:          " << format_number(stats.dnat_packets) << "\n";
    std::cout << "  Reverse NAT Packets:   " << format_number(stats.reverse_nat_packets) << "\n";
    std::cout << "  Avg Processing Time:   " << stats.average_processing_time_ms() << " ms\n";
}

void print_packet_transformation(const PacketTransformation& t) {
    std::cout << "  " << t.to_string() << "\n";
}

void print_menu() {
    print_separator("VNS - VIRTUAL NAT GATEWAY SIMULATOR");
    std::cout << "  1. Configure Network\n";
    std::cout << "  2. Manage Virtual Hosts\n";
    std::cout << "  3. Configure NAT\n";
    std::cout << "  4. View NAT Table\n";
    std::cout << "  5. Manage Port Forwarding\n";
    std::cout << "  6. Simulate Outbound Packet\n";
    std::cout << "  7. Simulate Inbound Packet\n";
    std::cout << "  8. View Connections\n";
    std::cout << "  9. View Driver Statistics\n";
    std::cout << "  10. Reset Simulation\n";
    std::cout << "  0. Exit\n";
    std::cout << "\n  Choice: ";
}

} // namespace vns