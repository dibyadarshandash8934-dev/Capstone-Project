#include "vns/network/network.hpp"
#include <sstream>
#include <iomanip>

namespace vns {

std::optional<Ipv4Address> Ipv4Address::from_string(const std::string& str) {
    uint32_t a, b, c, d;
    std::istringstream iss(str);
    if (!(iss >> a >> std::ws >> std::ws >> b >> std::ws >> std::ws >> c >> std::ws >> std::ws >> d)) {
        // Try with dots
        std::istringstream iss2(str);
        char dot;
        if (!(iss2 >> a >> dot >> b >> dot >> c >> dot >> d)) {
            return std::nullopt;
        }
        if (a > 255 || b > 255 || c > 255 || d > 255) return std::nullopt;
        return Ipv4Address((a << 24) | (b << 16) | (c << 8) | d);
    }
    if (a > 255 || b > 255 || c > 255 || d > 255) return std::nullopt;
    return Ipv4Address((a << 24) | (b << 16) | (c << 8) | d);
}

std::string Ipv4Address::to_string() const {
    return std::to_string((addr_ >> 24) & 0xFF) + "." +
           std::to_string((addr_ >> 16) & 0xFF) + "." +
           std::to_string((addr_ >> 8) & 0xFF) + "." +
           std::to_string(addr_ & 0xFF);
}

Ipv4Address Ipv4Address::network_address(uint32_t addr, uint8_t prefix_len) {
    if (prefix_len >= 32) return Ipv4Address(0);
    uint32_t mask = (prefix_len == 0) ? 0 : (0xFFFFFFFF << (32 - prefix_len));
    return Ipv4Address(addr & mask);
}

Ipv4Address Ipv4Address::broadcast_address(uint32_t addr, uint8_t prefix_len) {
    if (prefix_len >= 32) return Ipv4Address(0);
    uint32_t mask = (prefix_len == 0) ? 0 : (0xFFFFFFFF << (32 - prefix_len));
    return Ipv4Address((addr & mask) | ~mask);
}

bool Ipv4Address::is_network_address(uint8_t prefix_len) const {
    if (prefix_len == 0 || prefix_len > 32) return false;
    uint32_t mask = (prefix_len >= 32) ? 0xFFFFFFFFu : (0xFFFFFFFFu << (32 - prefix_len));
    return (addr_ & mask) == addr_;
}

bool Ipv4Address::is_broadcast_address(uint8_t prefix_len) const {
    if (prefix_len == 0 || prefix_len >= 32) return false;
    uint32_t mask = (0xFFFFFFFFu << (32 - prefix_len));
    return (addr_ & ~mask) == ~mask;
}

bool Ipv4Address::is_valid_host(uint8_t prefix_len) const {
    return prefix_len > 0 && prefix_len < 32 &&
           !is_network_address(prefix_len) &&
           !is_broadcast_address(prefix_len);
}

} // namespace vns