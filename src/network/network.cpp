#include "vns/network/network.hpp"
#include <sstream>

namespace vns {

NetworkConfig::NetworkConfig(const Cidr& cidr, const Ipv4Address& gateway, const Ipv4Address& public_ip)
    : cidr_(cidr), gateway_ip_(gateway), public_ip_(public_ip) {}

std::optional<NetworkConfig> NetworkConfig::create(const std::string& cidr_str,
                                                   const std::string& gateway_str,
                                                   const std::string& public_ip_str) {
    auto cidr_opt = Cidr::from_string(cidr_str);
    if (!cidr_opt) return std::nullopt;
    
    auto gateway_opt = Ipv4Address::from_string(gateway_str);
    if (!gateway_opt) return std::nullopt;
    
    auto public_opt = Ipv4Address::from_string(public_ip_str);
    if (!public_opt) return std::nullopt;
    
    // Validate gateway is in network
    if (!cidr_opt->contains(*gateway_opt)) return std::nullopt;
    
    // Validate gateway is not network or broadcast
    if (gateway_opt->is_network_address(cidr_opt->prefix_len()) ||
        gateway_opt->is_broadcast_address(cidr_opt->prefix_len())) {
        return std::nullopt;
    }
    
    return NetworkConfig(*cidr_opt, *gateway_opt, *public_opt);
}

} // namespace vns