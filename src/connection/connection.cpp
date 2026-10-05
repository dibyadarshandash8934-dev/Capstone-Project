#include "vns/connection/connection.hpp"
#include "vns/utils/logger.hpp"
#include <chrono>

namespace vns {

uint64_t Connection::current_time_ns() {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

std::string Connection::to_string() const {
    std::string proto_str;
    switch (protocol_) {
        case Protocol::TCP: proto_str = "TCP"; break;
        case Protocol::UDP: proto_str = "UDP"; break;
        case Protocol::ICMP: proto_str = "ICMP"; break;
    }
    
    std::string state_str;
    switch (state_) {
        case ConnectionState::NEW: state_str = "NEW"; break;
        case ConnectionState::ACTIVE: state_str = "ACTIVE"; break;
        case ConnectionState::ESTABLISHED: state_str = "ESTABLISHED"; break;
        case ConnectionState::CLOSED: state_str = "CLOSED"; break;
        case ConnectionState::EXPIRED: state_str = "EXPIRED"; break;
    }
    
    return proto_str + " " + source_ip_.to_string() + ":" + std::to_string(source_port_) +
           " -> " + destination_ip_.to_string() + ":" + std::to_string(destination_port_) +
           " [" + state_str + "]";
}

uint64_t ConnectionTracker::current_time_ns() {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

std::string ConnectionTracker::generate_id(Protocol proto, const Ipv4Address& src_ip, Port src_port,
                                           const Ipv4Address& dst_ip, Port dst_port) {
    return std::to_string(static_cast<uint8_t>(proto)) + "_" +
           src_ip.to_string() + "_" + std::to_string(src_port) + "_" +
           dst_ip.to_string() + "_" + std::to_string(dst_port);
}

std::optional<Connection> ConnectionTracker::create_connection(Protocol proto,
                                                               const Ipv4Address& src_ip, Port src_port,
                                                               const Ipv4Address& dst_ip, Port dst_port) {
    std::string id = generate_id(proto, src_ip, src_port, dst_ip, dst_port);
    if (connections_.find(id) != connections_.end()) {
        return std::nullopt;
    }
    Connection conn(id, proto, src_ip, src_port, dst_ip, dst_port);
    connections_[id] = std::move(conn);
    return connections_[id];
}

std::optional<Connection> ConnectionTracker::find_connection(const std::string& id) {
    auto it = connections_.find(id);
    if (it != connections_.end()) return it->second;
    return std::nullopt;
}

std::optional<Connection> ConnectionTracker::find_by_flow(Protocol proto,
                                                          const Ipv4Address& src_ip, Port src_port,
                                                          const Ipv4Address& dst_ip, Port dst_port) {
    std::string id = generate_id(proto, src_ip, src_port, dst_ip, dst_port);
    return find_connection(id);
}

void ConnectionTracker::update_activity(const std::string& id) {
    auto it = connections_.find(id);
    if (it != connections_.end()) {
        it->second.update_activity();
    }
}

void ConnectionTracker::remove_expired() {
    for (auto it = connections_.begin(); it != connections_.end(); ) {
        if (it->second.is_expired(timeout_ns_)) {
            it = connections_.erase(it);
        } else {
            ++it;
        }
    }
}

size_t ConnectionTracker::active_count() const {
    size_t count = 0;
    for (const auto& [id, conn] : connections_) {
        if (conn.state() != ConnectionState::CLOSED &&
            conn.state() != ConnectionState::EXPIRED) {
            count++;
        }
    }
    return count;
}

size_t ConnectionTracker::total_count() const { return connections_.size(); }

std::vector<Connection> ConnectionTracker::all_connections() const {
    std::vector<Connection> result;
    for (const auto& [id, conn] : connections_) {
        result.push_back(conn);
    }
    return result;
}

} // namespace vns