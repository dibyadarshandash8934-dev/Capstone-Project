#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include <chrono>
#include <string_view>
#include <stdexcept>
#include "vns/vns_ioctl.h"

namespace vns {

// Forward declarations
class Ipv4Address;
class Cidr;
class NetworkConfig;
class VirtualDevice;
class Packet;
class NatEntry;
class PortForwardRule;
class Connection;
class SimulationMetrics;

using IpAddress = uint32_t;
using Port = uint16_t;
using Timestamp = uint64_t;

enum class Protocol : uint8_t {
    TCP = 6,
    UDP = 17,
    ICMP = 1
};

enum class DeviceType {
    PC,
    SERVER,
    LAPTOP,
    PHONE,
    IOT
};

enum class PacketDirection {
    LAN_TO_INTERNET,
    INTERNET_TO_LAN
};

enum class NATState {
    ACTIVE,
    EXPIRED
};

enum class PacketAction {
    ORIGINAL,
    SNAT,
    DNAT,
    FORWARD,
    REVERSE_SNAT
};

enum class PacketStage {
    LAN,
    NAT_GATEWAY,
    INTERNET
};

enum class ConnectionState {
    NEW,
    ACTIVE,
    ESTABLISHED,
    CLOSED,
    EXPIRED
};

enum class VnsErrorCode {
    SUCCESS = 0,
    INVALID_CIDR,
    INVALID_IP,
    IP_OUTSIDE_NETWORK,
    DUPLICATE_IP,
    INVALID_PORT,
    INVALID_PROTOCOL,
    PORT_EXHAUSTED,
    DEVICE_NOT_FOUND,
    NETWORK_NOT_CONFIGURED,
    PORT_FORWARD_CONFLICT,
    DRIVER_UNAVAILABLE,
    DRIVER_IOCTL_FAILED,
    INVALID_PACKET,
    NAT_MAPPING_NOT_FOUND,
    PORT_FORWARD_NOT_FOUND
};

/* Result type for operations that can fail */
template<typename T>
class Result {
public:
    Result(T value) : value_(std::move(value)), has_value_(true) {}
    Result(VnsErrorCode error, std::string message) 
        : error_(error), message_(std::move(message)), has_value_(false) {}
    
    bool ok() const { return has_value_; }
    VnsErrorCode error() const { return error_; }
    const std::string& message() const { return message_; }
    T& value() { 
        if (!has_value_) throw std::runtime_error("No value: " + message_);
        return value_; 
    }
    const T& value() const { 
        if (!has_value_) throw std::runtime_error("No value: " + message_);
        return value_; 
    }
    
private:
    T value_;
    VnsErrorCode error_ = VnsErrorCode::SUCCESS;
    std::string message_;
    bool has_value_ = false;
};

/* VnsStats - C++ wrapper for vns_stats C struct */
class VnsStats {
public:
    VnsStats() = default;
    explicit VnsStats(const ::vns_stats& stats);
    
    uint64_t total_packets() const { return total_packets_; }
    uint64_t successful_packets() const { return successful_packets_; }
    uint64_t failed_packets() const { return failed_packets_; }
    uint64_t snat_packets() const { return snat_packets_; }
    uint64_t dnat_packets() const { return dnat_packets_; }
    uint64_t reverse_nat_packets() const { return reverse_nat_packets_; }
    uint64_t active_nat_mappings() const { return active_nat_mappings_; }
    uint64_t active_port_forward_rules() const { return active_port_forward_rules_; }
    uint64_t total_processing_time_ns() const { return total_processing_time_ns_; }
    double average_processing_time_ms() const {
        if (total_packets_ == 0) return 0.0;
        return static_cast<double>(total_processing_time_ns_) / static_cast<double>(total_packets_) / 1'000'000.0;
    }
    
private:
    uint64_t total_packets_ = 0;
    uint64_t successful_packets_ = 0;
    uint64_t failed_packets_ = 0;
    uint64_t snat_packets_ = 0;
    uint64_t dnat_packets_ = 0;
    uint64_t reverse_nat_packets_ = 0;
    uint64_t active_nat_mappings_ = 0;
    uint64_t active_port_forward_rules_ = 0;
    uint64_t total_processing_time_ns_ = 0;
};

/* VnsStatus - C++ wrapper for vns_status C struct */
class VnsStatus {
public:
    VnsStatus() = default;
    explicit VnsStatus(const ::vns_status& status);
    
    unsigned int status_flags() const { return status_flags_; }
    unsigned int active_connections() const { return active_connections_; }
    unsigned int nat_table_entries() const { return nat_table_entries_; }
    unsigned int port_forward_rules() const { return port_forward_rules_; }
    unsigned int simulator_running() const { return simulator_running_; }
    const std::string& simulator_version() const { return simulator_version_; }
    
private:
    unsigned int status_flags_ = 0;
    unsigned int active_connections_ = 0;
    unsigned int nat_table_entries_ = 0;
    unsigned int port_forward_rules_ = 0;
    unsigned int simulator_running_ = 0;
    std::string simulator_version_;
};

/* Simulation metrics */
class SimulationMetrics {
public:
    SimulationMetrics() = default;
    
    void record_success(const std::string& action, uint64_t elapsed_ns) {
        total_packets++;
        successful_packets++;
        total_processing_time_ns += elapsed_ns;
        if (action == "SNAT") snat_packets++;
        else if (action == "DNAT") dnat_packets++;
        else if (action == "REVERSE_SNAT") reverse_nat_packets++;
    }
    
    void record_failure(const std::string& /* action */, uint64_t elapsed_ns) {
        total_packets++;
        failed_packets++;
        total_processing_time_ns += elapsed_ns;
    }
    
    void reset() {
        total_packets = 0;
        successful_packets = 0;
        failed_packets = 0;
        snat_packets = 0;
        dnat_packets = 0;
        reverse_nat_packets = 0;
        total_processing_time_ns = 0;
    }
    
    uint64_t total_packets = 0;
    uint64_t successful_packets = 0;
    uint64_t failed_packets = 0;
    uint64_t snat_packets = 0;
    uint64_t dnat_packets = 0;
    uint64_t reverse_nat_packets = 0;
    uint64_t total_processing_time_ns = 0;
    
    double average_processing_time_ms() const {
        if (total_packets == 0) return 0.0;
        return static_cast<double>(total_processing_time_ns) / total_packets / 1'000'000.0;
    }
};

} // namespace vns
