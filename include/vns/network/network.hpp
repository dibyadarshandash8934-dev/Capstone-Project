#pragma once

#include "vns/common/types.hpp"
#include <string>
#include <optional>
#include <vector>
#include <sstream>

namespace vns {

class Ipv4Address {
public:
    Ipv4Address() : addr_(0) {}
    explicit Ipv4Address(uint32_t addr) : addr_(addr) {}
    
    static std::optional<Ipv4Address> from_string(const std::string& str);
    
    std::string to_string() const;
    
    uint32_t value() const { return addr_; }
    bool operator==(const Ipv4Address& other) const { return addr_ == other.addr_; }
    bool operator!=(const Ipv4Address& other) const { return addr_ != other.addr_; }
    bool operator<(const Ipv4Address& other) const { return addr_ < other.addr_; }
    
    static Ipv4Address network_address(uint32_t addr, uint8_t prefix_len);
    static Ipv4Address broadcast_address(uint32_t addr, uint8_t prefix_len);
    
    bool is_network_address(uint8_t prefix_len) const;
    bool is_broadcast_address(uint8_t prefix_len) const;
    bool is_valid_host(uint8_t prefix_len) const;
    
private:
    uint32_t addr_;
};

class Cidr {
public:
    Cidr() : prefix_len_(0) {}
    Cidr(const Ipv4Address& network, uint8_t prefix_len);
    
    static std::optional<Cidr> from_string(const std::string& str);
    
    std::string to_string() const;
    
    const Ipv4Address& network() const { return network_; }
    uint8_t prefix_len() const { return prefix_len_; }
    uint32_t netmask() const;
    Ipv4Address broadcast() const;
    Ipv4Address first_host() const;
    Ipv4Address last_host() const;
    size_t host_count() const;
    bool contains(const Ipv4Address& ip) const;
    
private:
    Ipv4Address network_;
    uint8_t prefix_len_;
};

class NetworkConfig {
public:
    NetworkConfig() = default;
    NetworkConfig(const Cidr& cidr, const Ipv4Address& gateway, const Ipv4Address& public_ip);
    
    static std::optional<NetworkConfig> create(const std::string& cidr_str,
                                               const std::string& gateway_str,
                                               const std::string& public_ip_str);
    
    const Cidr& cidr() const { return cidr_; }
    const Ipv4Address& gateway_ip() const { return gateway_ip_; }
    const Ipv4Address& public_ip() const { return public_ip_; }
    bool is_configured() const { return cidr_.prefix_len() > 0; }
    
private:
    Cidr cidr_;
    Ipv4Address gateway_ip_;
    Ipv4Address public_ip_;
};

} // namespace vns