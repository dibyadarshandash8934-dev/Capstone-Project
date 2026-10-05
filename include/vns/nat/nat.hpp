#pragma once

#include "vns/network/network.hpp"
#include "vns/common/types.hpp"
#include "vns/packet/packet.hpp"
#include <chrono>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace vns {

class NatEntry {
public:
    NatEntry() = default;
    NatEntry(Protocol proto, const Ipv4Address& private_ip, Port private_port,
             const Ipv4Address& public_ip, Port public_port,
             const Ipv4Address& dest_ip, Port dest_port);
    explicit NatEntry(const ::vns_nat_entry& entry);
    
    std::tuple<Protocol, Ipv4Address, Port, Ipv4Address, Port> flow_key() const;
    std::tuple<Protocol, Ipv4Address, Port, Ipv4Address, Port> reverse_flow_key() const;
    
    Protocol protocol() const { return protocol_; }
    const Ipv4Address& private_ip() const { return private_ip_; }
    Port private_port() const { return private_port_; }
    const Ipv4Address& public_ip() const { return public_ip_; }
    Port public_port() const { return public_port_; }
    const Ipv4Address& destination_ip() const { return destination_ip_; }
    Port destination_port() const { return destination_port_; }
    NATState state() const { return state_; }
    uint64_t created_at() const { return created_at_; }
    uint64_t last_activity() const { return last_activity_; }
    
    void set_state(NATState state) { state_ = state; }
    void update_activity() { last_activity_ = current_time_ns(); }
    bool is_active() const { return state_ == NATState::ACTIVE; }
    
    std::string to_string() const;
    
private:
    Protocol protocol_;
    Ipv4Address private_ip_;
    Port private_port_;
    Ipv4Address public_ip_;
    Port public_port_;
    Ipv4Address destination_ip_;
    Port destination_port_;
    NATState state_ = NATState::ACTIVE;
    uint64_t created_at_;
    uint64_t last_activity_;
    
    static uint64_t current_time_ns();
    std::string protocol_string() const;
    std::string state_string() const;
};

class PortForwardRule {
public:
    PortForwardRule() = default;
    PortForwardRule(Protocol proto, const Ipv4Address& public_ip, Port public_port,
                    const Ipv4Address& private_ip, Port private_port);
    
    static std::optional<PortForwardRule> create(Protocol proto,
                                                 const Ipv4Address& public_ip,
                                                 Port public_port,
                                                 const Ipv4Address& private_ip,
                                                 Port private_port);
    
    std::tuple<Protocol, Ipv4Address, Port> match_key() const;
    
    Protocol protocol() const { return protocol_; }
    const Ipv4Address& public_ip() const { return public_ip_; }
    Port public_port() const { return public_port_; }
    const Ipv4Address& private_ip() const { return private_ip_; }
    Port private_port() const { return private_port_; }
    const std::string& id() const { return id_; }
    bool enabled() const { return enabled_; }
    uint64_t created_at() const { return created_at_; }
    
    void set_enabled(bool enabled) { enabled_ = enabled; }
    void set_id(const std::string& id) { id_ = id; }
    
    std::string to_string() const;
    
private:
    Protocol protocol_;
    Ipv4Address public_ip_;
    Port public_port_;
    Ipv4Address private_ip_;
    Port private_port_;
    std::string id_;
    bool enabled_ = true;
    uint64_t created_at_;
    
    static uint64_t current_time_ns();
public:
    static std::string protocol_string(Protocol p);
    std::string protocol_string() const;
};

class PacketTransformation {
public:
    PacketTransformation() = default;
    PacketTransformation(PacketStage stage, PacketAction action,
                         const Ipv4Address& src_ip, Port src_port,
                         const Ipv4Address& dst_ip, Port dst_port,
                         const std::string& description);
    
    PacketStage stage() const { return stage_; }
    PacketAction action() const { return action_; }
    const Ipv4Address& source_ip() const { return source_ip_; }
    Port source_port() const { return source_port_; }
    const Ipv4Address& destination_ip() const { return destination_ip_; }
    Port destination_port() const { return destination_port_; }
    const std::string& description() const { return description_; }
    
    std::string to_string() const;
    
private:
    PacketStage stage_;
    PacketAction action_;
    Ipv4Address source_ip_;
    Port source_port_;
    Ipv4Address destination_ip_;
    Port destination_port_;
    std::string description_;
};

class SimulationResult {
public:
    SimulationResult() : success_(false) {}
    SimulationResult(const Packet& original, const Packet& translated,
                     const std::vector<PacketTransformation>& transforms);
    
    static SimulationResult failure(const Packet& original, const std::string& error);
    
    bool success() const { return success_; }
    const std::string& error() const { return error_; }
    const std::string& action() const { return action_; }
    const Packet& original_packet() const { return original_packet_; }
    const std::optional<Packet>& translated_packet() const { return translated_packet_; }
    const std::vector<PacketTransformation>& transformations() const { return transformations_; }
    const std::optional<NatEntry>& nat_entry() const { return nat_entry_; }

    void set_action(const std::string& action) { action_ = action; }
    void set_nat_entry(const NatEntry& entry) { nat_entry_ = entry; }
    
private:
    Packet original_packet_;
    std::optional<Packet> translated_packet_;
    std::vector<PacketTransformation> transformations_;
    bool success_;
    std::string error_;
    std::string action_;
    std::optional<NatEntry> nat_entry_;
};

} // namespace vns
