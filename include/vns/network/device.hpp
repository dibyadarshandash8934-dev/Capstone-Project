#pragma once

#include "vns/network/network.hpp"
#include "vns/common/types.hpp"
#include <string>

namespace vns {

class VirtualDevice {
public:
    enum class Type {
        PC,
        SERVER,
        LAPTOP,
        PHONE,
        IOT
    };
    
    VirtualDevice() = default;
    VirtualDevice(const std::string& id, const std::string& name, 
                  const Ipv4Address& ip, Type type)
        : id_(id), name_(name), ip_(ip), type_(type), 
          status_(true), created_at_(current_time_ns()) {}
    
    static std::optional<VirtualDevice> create(const std::string& name,
                                               const Ipv4Address& ip,
                                               Type type,
                                               const NetworkConfig& network);
    
    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    const Ipv4Address& ip() const { return ip_; }
    Type type() const { return type_; }
    bool status() const { return status_; }
    uint64_t created_at() const { return created_at_; }
    
    void set_status(bool status) { status_ = status; }
    
    std::string type_string() const;
    
private:
    std::string id_;
    std::string name_;
    Ipv4Address ip_;
    Type type_;
    bool status_ = true;
    uint64_t created_at_;
    
    static uint64_t current_time_ns();
};

} // namespace vns