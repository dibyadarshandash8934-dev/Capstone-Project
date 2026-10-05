#include "vns/network/network.hpp"
#include <sstream>
#include <iomanip>

namespace vns {

Cidr::Cidr(const Ipv4Address& network, uint8_t prefix_len)
    : network_(network), prefix_len_(prefix_len) {
    // Normalize network address
    network_ = Ipv4Address::network_address(network.value(), prefix_len);
}

std::optional<Cidr> Cidr::from_string(const std::string& str) {
    size_t slash_pos = str.find('/');
    if (slash_pos == std::string::npos) return std::nullopt;
    
    auto ip_opt = Ipv4Address::from_string(str.substr(0, slash_pos));
    if (!ip_opt) return std::nullopt;
    
    try {
        int prefix = std::stoi(str.substr(slash_pos + 1));
        if (prefix < 0 || prefix > 32) return std::nullopt;
        if (prefix < 8 || prefix > 30) return std::nullopt;
        
        Cidr cidr(*ip_opt, static_cast<uint8_t>(prefix));
        // Normalize network address
        cidr.network_ = Ipv4Address::network_address(ip_opt->value(), prefix);
        return cidr;
    } catch (...) {
        return std::nullopt;
    }
}

std::string Cidr::to_string() const {
    return network_.to_string() + "/" + std::to_string(prefix_len_);
}

uint32_t Cidr::netmask() const {
    if (prefix_len_ == 0) return 0;
    if (prefix_len_ >= 32) return 0xFFFFFFFF;
    return 0xFFFFFFFF << (32 - prefix_len_);
}

Ipv4Address Cidr::broadcast() const {
    return Ipv4Address::broadcast_address(network_.value(), prefix_len_);
}

Ipv4Address Cidr::first_host() const {
    return Ipv4Address(network_.value() + 1);
}

Ipv4Address Cidr::last_host() const {
    return Ipv4Address(broadcast().value() - 1);
}

size_t Cidr::host_count() const {
    if (prefix_len_ >= 31) return 0;
    return (1ULL << (32 - prefix_len_)) - 2;
}

bool Cidr::contains(const Ipv4Address& ip) const {
    uint32_t mask = netmask();
    return (ip.value() & mask) == (network_.value() & mask);
}

} // namespace vns