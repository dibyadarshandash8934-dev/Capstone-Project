#include "vns/packet/packet.hpp"
#include <sstream>
#include <iomanip>
#include <chrono>

namespace vns {

std::tuple<Protocol, Ipv4Address, Port, Ipv4Address, Port> Packet::flow_key() const {
    return {protocol_, source_ip_, source_port_, destination_ip_, destination_port_};
}

std::tuple<Protocol, Ipv4Address, Port, Ipv4Address, Port> Packet::reverse_flow_key() const {
    return {protocol_, destination_ip_, destination_port_, source_ip_, source_port_};
}

Packet Packet::copy_with(const Ipv4Address& src_ip, Port src_port,
                         const Ipv4Address& dst_ip, Port dst_port) const {
    Packet p = *this;
    if (src_ip != Ipv4Address()) p.source_ip_ = src_ip;
    if (src_port != 0) p.source_port_ = src_port;
    if (dst_ip != Ipv4Address()) p.destination_ip_ = dst_ip;
    if (dst_port != 0) p.destination_port_ = dst_port;
    return p;
}

uint64_t Packet::current_time_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

std::string Packet::to_string() const {
    std::string proto_str;
    switch (protocol_) {
        case Protocol::TCP: proto_str = "TCP"; break;
        case Protocol::UDP: proto_str = "UDP"; break;
        case Protocol::ICMP: proto_str = "ICMP"; break;
    }
    return proto_str + " " + source_ip_.to_string() + ":" + std::to_string(source_port_) +
           " -> " + destination_ip_.to_string() + ":" + std::to_string(destination_port_);
}

std::optional<Packet> Packet::from_string(const std::string& str) {
    // Expected format produced by to_string():
    //   "<PROTOCOL> <src_ip>:<src_port> -> <dst_ip>:<dst_port>"
    const std::string arrow = " -> ";
    auto split_at = str.find(arrow);
    if (split_at == std::string::npos) return std::nullopt;

    std::istringstream left(str.substr(0, split_at));
    std::string proto_str, src_endpoint;
    if (!(left >> proto_str >> src_endpoint)) return std::nullopt;

    std::istringstream right(str.substr(split_at + arrow.size()));
    std::string dst_endpoint;
    if (!(right >> dst_endpoint)) return std::nullopt;

    Protocol proto;
    if (proto_str == "TCP") proto = Protocol::TCP;
    else if (proto_str == "UDP") proto = Protocol::UDP;
    else if (proto_str == "ICMP") proto = Protocol::ICMP;
    else return std::nullopt;

    auto parse_endpoint = [](const std::string& endpoint, Ipv4Address& ip, Port& port) -> bool {
        auto colon = endpoint.rfind(':');
        if (colon == std::string::npos) return false;

        auto ip_opt = Ipv4Address::from_string(endpoint.substr(0, colon));
        if (!ip_opt) return false;

        int parsed = 0;
        try {
            parsed = std::stoi(endpoint.substr(colon + 1));
        } catch (...) {
            return false;
        }
        if (parsed < 0 || parsed > 65535) return false;

        ip = *ip_opt;
        port = static_cast<Port>(parsed);
        return true;
    };

    Ipv4Address src_ip, dst_ip;
    Port src_port = 0, dst_port = 0;
    if (!parse_endpoint(src_endpoint, src_ip, src_port)) return std::nullopt;
    if (!parse_endpoint(dst_endpoint, dst_ip, dst_port)) return std::nullopt;

    return Packet(proto, src_ip, src_port, dst_ip, dst_port);
}

} // namespace vns
