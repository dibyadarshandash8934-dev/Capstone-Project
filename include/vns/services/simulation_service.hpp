#pragma once

#include "vns/network/network.hpp"
#include "vns/network/device.hpp"
#include "vns/nat/nat.hpp"
#include "vns/nat/nat_engine.hpp"
#include "vns/packet/packet.hpp"
#include "vns/connection/connection.hpp"
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace vns {

class SimulationService {
public:
    SimulationService();

    bool is_configured() const;
    const NetworkConfig& network() const;

    bool create_network(const std::string& cidr, const std::string& gateway, const std::string& public_ip);
    bool add_device(const VirtualDevice& device);
    bool remove_device(const std::string& id);
    std::vector<VirtualDevice> devices() const;

    bool add_port_forward_rule(const PortForwardRule& rule);
    void remove_port_forward_rule(const std::string& id);
    std::vector<PortForwardRule> port_forward_rules() const;
    std::vector<NatEntry> nat_entries() const;

    SimulationResult simulate_packet(const Packet& packet, PacketDirection direction);
    std::vector<Connection> get_connections() const;
    SimulationMetrics metrics() const;

    void clear_nat_table();
    void reset();
    void reset_stats();

private:
    NetworkConfig network_;
    bool network_configured_ = false;
    std::unique_ptr<NatEngineImpl> nat_engine_;
};

} // namespace vns
