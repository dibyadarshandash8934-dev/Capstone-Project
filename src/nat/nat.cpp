#include "vns/nat/nat.hpp"
#include "vns/utils/logger.hpp"
#include <chrono>
#include <sstream>

namespace vns {

NatEntry::NatEntry(Protocol proto, const Ipv4Address& private_ip, Port private_port,
                   const Ipv4Address& public_ip, Port public_port,
                   const Ipv4Address& dest_ip, Port dest_port)
    : protocol_(proto),
      private_ip_(private_ip),
      private_port_(private_port),
      public_ip_(public_ip),
      public_port_(public_port),
      destination_ip_(dest_ip),
      destination_port_(dest_port),
      state_(NATState::ACTIVE),
      created_at_(current_time_ns()),
      last_activity_(current_time_ns()) {}

NatEntry::NatEntry(const ::vns_nat_entry& entry)
    : protocol_(static_cast<Protocol>(entry.protocol)),
      private_ip_(entry.private_ip),
      private_port_(entry.private_port),
      public_ip_(entry.public_ip),
      public_port_(entry.public_port),
      destination_ip_(entry.destination_ip),
      destination_port_(entry.destination_port),
      state_(entry.state == VNS_NAT_EXPIRED ? NATState::EXPIRED : NATState::ACTIVE),
      created_at_(entry.created_time),
      last_activity_(entry.last_activity) {}

std::tuple<Protocol, Ipv4Address, Port, Ipv4Address, Port> NatEntry::flow_key() const {
    return {protocol_, private_ip_, private_port_, destination_ip_, destination_port_};
}

std::tuple<Protocol, Ipv4Address, Port, Ipv4Address, Port> NatEntry::reverse_flow_key() const {
    return {protocol_, public_ip_, public_port_, destination_ip_, destination_port_};
}

uint64_t NatEntry::current_time_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

std::string NatEntry::protocol_string() const {
    switch (protocol_) {
        case Protocol::TCP: return "TCP";
        case Protocol::UDP: return "UDP";
        case Protocol::ICMP: return "ICMP";
    }
    return "UNKNOWN";
}

std::string NatEntry::state_string() const {
    switch (state_) {
        case NATState::ACTIVE: return "ACTIVE";
        case NATState::EXPIRED: return "EXPIRED";
    }
    return "UNKNOWN";
}

std::string NatEntry::to_string() const {
    std::ostringstream oss;
    oss << protocol_string() << " "
        << private_ip_.to_string() << ":" << private_port_
        << " <-> "
        << public_ip_.to_string() << ":" << public_port_
        << " dest " << destination_ip_.to_string() << ":" << destination_port_
        << " [" << state_string() << "]";
    return oss.str();
}

PortForwardRule::PortForwardRule(Protocol proto, const Ipv4Address& public_ip, Port public_port,
                                 const Ipv4Address& private_ip, Port private_port)
    : protocol_(proto),
      public_ip_(public_ip),
      public_port_(public_port),
      private_ip_(private_ip),
      private_port_(private_port),
      enabled_(true),
      created_at_(current_time_ns()) {
    id_ = protocol_string(proto) + "-" + public_ip.to_string() + "-" + std::to_string(public_port);
}

std::optional<PortForwardRule> PortForwardRule::create(Protocol proto,
                                                       const Ipv4Address& public_ip,
                                                       Port public_port,
                                                       const Ipv4Address& private_ip,
                                                       Port private_port) {
    if (public_port == 0 || private_port == 0) {
        return std::nullopt;
    }
    return PortForwardRule(proto, public_ip, public_port, private_ip, private_port);
}

std::tuple<Protocol, Ipv4Address, Port> PortForwardRule::match_key() const {
    return {protocol_, public_ip_, public_port_};
}

std::string PortForwardRule::protocol_string(Protocol p) {
    switch (p) {
        case Protocol::TCP: return "TCP";
        case Protocol::UDP: return "UDP";
        case Protocol::ICMP: return "ICMP";
    }
    return "UNKNOWN";
}

std::string PortForwardRule::protocol_string() const {
    return protocol_string(protocol_);
}

uint64_t PortForwardRule::current_time_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

std::string PortForwardRule::to_string() const {
    std::ostringstream oss;
    oss << protocol_string() << " "
        << public_ip_.to_string() << ":" << public_port_
        << " -> "
        << private_ip_.to_string() << ":" << private_port_;
    return oss.str();
}

PacketTransformation::PacketTransformation(PacketStage stage, PacketAction action,
                                           const Ipv4Address& src_ip, Port src_port,
                                           const Ipv4Address& dst_ip, Port dst_port,
                                           const std::string& description)
    : stage_(stage), action_(action),
      source_ip_(src_ip), source_port_(src_port),
      destination_ip_(dst_ip), destination_port_(dst_port),
      description_(description) {}

std::string PacketTransformation::to_string() const {
    return description_;
}

SimulationResult::SimulationResult(const Packet& original, const Packet& translated,
                                   const std::vector<PacketTransformation>& transforms)
    : original_packet_(original),
      translated_packet_(translated),
      transformations_(transforms),
      success_(true) {
    if (!transforms.empty()) {
        switch (transforms.back().action()) {
            case PacketAction::SNAT: action_ = "SNAT"; break;
            case PacketAction::DNAT: action_ = "DNAT"; break;
            case PacketAction::REVERSE_SNAT: action_ = "REVERSE_SNAT"; break;
            case PacketAction::FORWARD: {
                for (const auto& t : transforms) {
                    if (t.action() == PacketAction::SNAT) { action_ = "SNAT"; break; }
                    if (t.action() == PacketAction::DNAT) { action_ = "DNAT"; break; }
                    if (t.action() == PacketAction::REVERSE_SNAT) { action_ = "REVERSE_SNAT"; break; }
                }
                break;
            }
            default: break;
        }
    }
}

SimulationResult SimulationResult::failure(const Packet& original, const std::string& error) {
    SimulationResult result;
    result.original_packet_ = original;
    result.success_ = false;
    result.error_ = error;
    return result;
}

} // namespace vns
