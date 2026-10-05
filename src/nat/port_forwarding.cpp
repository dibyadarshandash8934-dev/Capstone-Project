#include "vns/nat/port_forwarding.hpp"
#include "vns/network/network.hpp"
#include "vns/network/device.hpp"
#include "vns/utils/logger.hpp"
#include <algorithm>

namespace vns {

std::optional<PortForwardRule> PortForwardingImpl::add_rule(const PortForwardRule& rule,
                                                            const NetworkConfig& network,
                                                            const std::vector<VirtualDevice>& devices) {
    // Validate public IP matches NAT gateway
    if (rule.public_ip() != network.public_ip()) {
        return std::nullopt;
    }
    
    // Validate private IP is in LAN
    if (!network.cidr().contains(rule.private_ip())) {
        return std::nullopt;
    }
    
    // Check if device exists at private IP
    auto it = std::find_if(devices.begin(), devices.end(),
        [&rule](const VirtualDevice& d) { return d.ip() == rule.private_ip(); });
    if (it == devices.end()) {
        return std::nullopt;
    }
    
    // Check for conflicts (same protocol, public IP, public port)
    auto match_key = rule.match_key();
    for (const auto& existing : rules_) {
        if (existing.enabled() && existing.match_key() == match_key) {
            return std::nullopt;
        }
    }
    
    rules_.push_back(rule);
    return rule;
}

void PortForwardingImpl::remove_rule(const std::string& rule_id) {
    rules_.erase(std::remove_if(rules_.begin(), rules_.end(),
        [&rule_id](const PortForwardRule& r) { return r.id() == rule_id; }), 
    rules_.end());
}

std::vector<PortForwardRule> PortForwardingImpl::get_rules() const {
    return rules_;
}

const PortForwardRule* PortForwardingImpl::find_rule(const std::tuple<Protocol, Ipv4Address, Port>& key) const {
    for (const auto& rule : rules_) {
        if (rule.enabled() && rule.match_key() == key) {
            return &rule;
        }
    }
    return nullptr;
}

} // namespace vns