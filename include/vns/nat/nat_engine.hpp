#pragma once

#include "vns/nat/nat.hpp"
#include "vns/network/network.hpp"
#include "vns/network/device.hpp"
#include "vns/packet/packet.hpp"
#include "vns/connection/connection.hpp"
#include "vns/common/types.hpp"
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace vns {

class NatTableImpl;
class PatAllocatorImpl;
class PortForwardingImpl;

class NatEngineImpl {
public:
    NatEngineImpl(const NetworkConfig& network, 
                  uint16_t port_start = 40000, 
                  uint16_t port_end = 50000);
    
    const NetworkConfig& network() const { return network_; }
    
    SimulationResult process_outgoing_packet(const Packet& packet);
    SimulationResult process_incoming_response(const Packet& packet);
    
    std::optional<PortForwardRule> add_port_forward(const PortForwardRule& rule);
    void remove_port_forward(const std::string& id);
    std::vector<PortForwardRule> get_port_forward_rules() const;
    
    SimulationResult process_incoming_packet(const Packet& packet);
    SimulationResult process_packet(const Packet& packet, PacketDirection direction);
    
    std::vector<NatEntry> get_nat_entries() const;
    std::optional<NatEntry> find_nat_entry(const std::tuple<Protocol, Ipv4Address, Port, Ipv4Address, Port>& key) const;
    void clear_nat_table();
    std::vector<Connection> get_connections() const;
    
    SimulationMetrics metrics() const { return metrics_; }
    void reset_metrics() { metrics_.reset(); }
    
    void add_device(const VirtualDevice& device);
    void remove_device(const std::string& id);
    std::vector<VirtualDevice> devices() const;
    ~NatEngineImpl();  // Defined in .cpp for unique_ptr destruction

private:
    NetworkConfig network_;
    std::vector<VirtualDevice> devices_;
    struct NatTableDeleter {
        void operator()(NatTableImpl* ptr) const;
    };
    std::unique_ptr<NatTableImpl, NatTableDeleter> nat_table_;
    struct PatAllocatorDeleter {
        void operator()(PatAllocatorImpl* ptr) const;
    };
    std::unique_ptr<PatAllocatorImpl, PatAllocatorDeleter> port_allocator_;
    struct PortForwardingDeleter {
        void operator()(PortForwardingImpl* ptr) const;
    };
    std::unique_ptr<PortForwardingImpl, PortForwardingDeleter> port_forwarding_;
    ConnectionTracker connection_tracker_;
    SimulationMetrics metrics_;
    
    uint64_t elapsed_ns(std::chrono::high_resolution_clock::time_point start);
    std::string protocol_string(Protocol p) const;
    std::optional<VirtualDevice> find_device_by_ip(const Ipv4Address& ip) const;
};

} // namespace vns
