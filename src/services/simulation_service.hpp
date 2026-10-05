#pragma once

#include "vns/network/network.hpp"
#include "vns/network/device.hpp"
#include "vns/nat/nat.hpp"
#include "vns/nat/nat_engine.hpp"
#include "vns/packet/packet.hpp"
#include "vns/connection/connection.hpp"
#include "vns/driver/driver_interface.hpp"
#include "vns/utils/helpers.hpp"
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace vns {

class SimulationService {
public:
    SimulationService() = default;
    
    bool is_configured() const { return network_configured_; }
    const NetworkConfig& network() const { return network_; }
    
    bool create_network(const std::string& cidr, const std::string& gateway, const std::string& public_ip) {
        auto net_opt = NetworkConfig::create(cidr, gateway, public_ip);
        if (!net_opt) return false;
        
        network_ = *net_opt;
        network_configured_ = true;
        
        // Initialize NAT engine
        nat_engine_ = std::make_unique<NatEngineImpl>(network_);
        return true;
    }
    
    bool add_device(const VirtualDevice& device) {
        if (!network_configured_ || !nat_engine_) return false;
        try {
            nat_engine_->add_device(device);
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool remove_device(const std::string& id) {
        if (!nat_engine_) return false;
        nat_engine_->remove_device(id);
        return true;
    }
    
    std::vector<VirtualDevice> devices() const {
        if (!nat_engine_) return {};
        return nat_engine_->devices();
    }
    
    bool add_port_forward_rule(const PortForwardRule& rule) {
        if (!nat_engine_) return false;
        try {
            nat_engine_->add_port_forward_rule(rule);
            return true;
        } catch (...) {
            return false;
        }
    }
    
    void remove_port_forward_rule(const std::string& id) {
        if (nat_engine_) nat_engine_->remove_port_forward(id);
    }
    
    std::vector<PortForwardRule> port_forward_rules() const {
        if (!nat_engine_) return {};
        return nat_engine_->get_port_forward_rules();
    }
    
    std::vector<NatEntry> nat_entries() const {
        if (!nat_engine_) return {};
        return nat_engine_->get_nat_entries();
    }
    
    SimulationResult simulate_packet(const Packet& packet, PacketDirection direction) {
        if (!nat_engine_) return SimulationResult::failure(packet, "NAT engine not initialized");
        return nat_engine_->process_packet(packet, direction);
    }
    
    std::vector<Connection> get_connections() const {
        // Return empty for now - connection tracking is part of NAT engine
        return {};
    }
    
    void clear_nat_table() {
        if (nat_engine_) nat_engine_->clear_nat_table();
    }
    
    void reset() {
        nat_engine_.reset();
        network_configured_ = false;
    }
    
    void reset_stats() {
        if (nat_engine_) nat_engine_->reset_metrics();
    }

private:
    NetworkConfig network_;
    bool network_configured_ = false;
    std::unique_ptr<NatEngineImpl> nat_engine_;
};

} // namespace vns