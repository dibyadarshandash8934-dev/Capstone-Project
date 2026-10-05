#include "vns/services/simulation_service.hpp"
#include "vns/nat/nat_engine.hpp"
#include "vns/utils/logger.hpp"

namespace vns {

SimulationService::SimulationService()
    : network_configured_(false) {}

bool SimulationService::create_network(const std::string& cidr, const std::string& gateway, const std::string& public_ip) {
    auto net_opt = NetworkConfig::create(cidr, gateway, public_ip);
    if (!net_opt) return false;

    network_ = *net_opt;
    network_configured_ = true;
    nat_engine_ = std::make_unique<NatEngineImpl>(network_);
    return true;
}

bool SimulationService::add_device(const VirtualDevice& device) {
    if (!network_configured_ || !nat_engine_) return false;
    try {
        nat_engine_->add_device(device);
        return true;
    } catch (...) {
        return false;
    }
}

bool SimulationService::remove_device(const std::string& id) {
    if (!nat_engine_) return false;
    nat_engine_->remove_device(id);
    return true;
}

std::vector<VirtualDevice> SimulationService::devices() const {
    if (!nat_engine_) return {};
    return nat_engine_->devices();
}

bool SimulationService::add_port_forward_rule(const PortForwardRule& rule) {
    if (!nat_engine_) return false;
    try {
        return nat_engine_->add_port_forward(rule).has_value();
    } catch (...) {
        return false;
    }
}

void SimulationService::remove_port_forward_rule(const std::string& id) {
    if (nat_engine_) nat_engine_->remove_port_forward(id);
}

std::vector<PortForwardRule> SimulationService::port_forward_rules() const {
    if (!nat_engine_) return {};
    return nat_engine_->get_port_forward_rules();
}

std::vector<NatEntry> SimulationService::nat_entries() const {
    if (!nat_engine_) return {};
    return nat_engine_->get_nat_entries();
}

SimulationResult SimulationService::simulate_packet(const Packet& packet, PacketDirection direction) {
    if (!nat_engine_) return SimulationResult::failure(packet, "NAT engine not initialized");
    return nat_engine_->process_packet(packet, direction);
}

std::vector<Connection> SimulationService::get_connections() const {
    if (!nat_engine_) return {};
    return nat_engine_->get_connections();
}

SimulationMetrics SimulationService::metrics() const {
    if (!nat_engine_) return SimulationMetrics{};
    return nat_engine_->metrics();
}

void SimulationService::clear_nat_table() {
    if (nat_engine_) nat_engine_->clear_nat_table();
}

void SimulationService::reset() {
    nat_engine_.reset();
    network_configured_ = false;
}

void SimulationService::reset_stats() {
    if (nat_engine_) nat_engine_->reset_metrics();
}

bool SimulationService::is_configured() const {
    return network_configured_;
}

const NetworkConfig& SimulationService::network() const {
    return network_;
}

} // namespace vns
