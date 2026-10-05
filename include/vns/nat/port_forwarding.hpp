#pragma once

#include "vns/nat/nat.hpp"
#include "vns/network/network.hpp"
#include "vns/network/device.hpp"
#include "vns/common/types.hpp"
#include <vector>
#include <optional>

namespace vns {

class PortForwardingImpl {
public:
    std::optional<PortForwardRule> add_rule(const PortForwardRule& rule,
                                            const NetworkConfig& network,
                                            const std::vector<VirtualDevice>& devices);
    void remove_rule(const std::string& rule_id);
    std::vector<PortForwardRule> get_rules() const;
    const PortForwardRule* find_rule(const std::tuple<Protocol, Ipv4Address, Port>& key) const;

private:
    std::vector<PortForwardRule> rules_;
};

} // namespace vns