#pragma once

#include "vns/common/types.hpp"
#include "vns/network/network.hpp"
#include "vns/network/device.hpp"
#include "vns/nat/nat.hpp"
#include "vns/connection/connection.hpp"
#include "vns/driver/driver_interface.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

namespace vns {

std::string format_ip_port(const Ipv4Address& ip, Port port);
std::string format_protocol(Protocol proto);
std::string format_nat_state(NATState state);
std::string format_connection_state(ConnectionState state);

void print_separator(const std::string& title);
void print_network_config(const NetworkConfig& network);
void print_devices(const std::vector<VirtualDevice>& devices);
void print_nat_table(const std::vector<NatEntry>& entries);
void print_port_forward_rules(const std::vector<PortForwardRule>& rules);
void print_packet_simulation(const SimulationResult& result);
void print_connection_tracker(const std::vector<Connection>& connections);
void print_driver_stats(const VnsStats& stats);
void print_driver_status(const VnsStatus& status);
void print_simulation_stats(const SimulationMetrics& stats);
void print_packet_transformation(const PacketTransformation& t);
void print_menu();

} // namespace vns