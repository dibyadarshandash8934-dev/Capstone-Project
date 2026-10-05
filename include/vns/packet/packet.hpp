#pragma once

#include "vns/network/network.hpp"
#include "vns/common/types.hpp"
#include <chrono>
#include <optional>
#include <string>
#include <tuple>

namespace vns {

class Packet {
public:
    Packet() = default;
    Packet(Protocol proto, const Ipv4Address& src_ip, Port src_port,
           const Ipv4Address& dst_ip, Port dst_port)
        : protocol_(proto), source_ip_(src_ip), source_port_(src_port),
          destination_ip_(dst_ip), destination_port_(dst_port),
          timestamp_(current_time_ns()) {}
    
    // Flow key for NAT mapping lookup
    std::tuple<Protocol, Ipv4Address, Port, Ipv4Address, Port> flow_key() const;
    
    // Reverse flow key for incoming response lookup
    std::tuple<Protocol, Ipv4Address, Port, Ipv4Address, Port> reverse_flow_key() const;
    
    Protocol protocol() const { return protocol_; }
    const Ipv4Address& source_ip() const { return source_ip_; }
    Port source_port() const { return source_port_; }
    const Ipv4Address& destination_ip() const { return destination_ip_; }
    Port destination_port() const { return destination_port_; }
    uint64_t timestamp() const { return timestamp_; }
    
    Packet copy_with(const Ipv4Address& src_ip = Ipv4Address(),
                     Port src_port = 0,
                     const Ipv4Address& dst_ip = Ipv4Address(),
                     Port dst_port = 0) const;
    
    std::string to_string() const;
    
    static std::optional<Packet> from_string(const std::string& str);
    
private:
    Protocol protocol_;
    Ipv4Address source_ip_;
    Port source_port_;
    Ipv4Address destination_ip_;
    Port destination_port_;
    uint64_t timestamp_;
    
    static uint64_t current_time_ns();
};

} // namespace vns