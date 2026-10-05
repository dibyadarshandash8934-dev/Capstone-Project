#include "vns/network/device.hpp"
#include <algorithm>
#include <chrono>

namespace vns {

uint64_t VirtualDevice::current_time_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

std::optional<VirtualDevice> VirtualDevice::create(const std::string& name,
                                                   const Ipv4Address& ip,
                                                   Type type,
                                                   const NetworkConfig& network) {
    // Validate IP is in network
    if (!network.cidr().contains(ip)) return std::nullopt;
    
    // Must be a valid host IP (not network/broadcast)
    if (!ip.is_valid_host(network.cidr().prefix_len())) return std::nullopt;
    
    // Cannot use gateway IP
    if (ip == network.gateway_ip()) return std::nullopt;
    
    std::string id = name;
    std::transform(id.begin(), id.end(), id.begin(), ::tolower);
    std::replace(id.begin(), id.end(), ' ', '-');
    
    return VirtualDevice(id, name, ip, type);
}

std::string VirtualDevice::type_string() const {
    switch (type_) {
        case Type::PC: return "PC";
        case Type::SERVER: return "SERVER";
        case Type::LAPTOP: return "LAPTOP";
        case Type::PHONE: return "PHONE";
        case Type::IOT: return "IOT";
    }
    return "UNKNOWN";
}

} // namespace vns