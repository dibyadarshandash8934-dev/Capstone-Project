#pragma once

#include "vns/network/network.hpp"
#include "vns/common/types.hpp"
#include <chrono>
#include <string>
#include <optional>
#include <unordered_map>
#include <vector>

namespace vns {

class Connection {
public:
    Connection() = default;
    Connection(const std::string& id, Protocol proto,
               const Ipv4Address& src_ip, Port src_port,
               const Ipv4Address& dst_ip, Port dst_port)
        : id_(id), protocol_(proto),
          source_ip_(src_ip), source_port_(src_port),
          destination_ip_(dst_ip), destination_port_(dst_port),
          state_(ConnectionState::NEW),
          created_at_(current_time_ns()),
          last_activity_(current_time_ns()) {}
    
    const std::string& id() const { return id_; }
    Protocol protocol() const { return protocol_; }
    const Ipv4Address& source_ip() const { return source_ip_; }
    Port source_port() const { return source_port_; }
    const Ipv4Address& destination_ip() const { return destination_ip_; }
    Port destination_port() const { return destination_port_; }
    ConnectionState state() const { return state_; }
    uint64_t created_at() const { return created_at_; }
    uint64_t last_activity() const { return last_activity_; }
    
    void set_state(ConnectionState state) { state_ = state; }
    void update_activity() { last_activity_ = current_time_ns(); }
    
    bool is_expired(uint64_t timeout_ns) const {
        auto now = current_time_ns();
        return (now - last_activity_) > timeout_ns;
    }
    
    bool is_active() const {
        return state_ == ConnectionState::ACTIVE || 
               state_ == ConnectionState::ESTABLISHED;
    }
    
    std::string to_string() const;
    
private:
    std::string id_;
    Protocol protocol_;
    Ipv4Address source_ip_;
    Port source_port_;
    Ipv4Address destination_ip_;
    Port destination_port_;
    ConnectionState state_;
    uint64_t created_at_;
    uint64_t last_activity_;
    
    static uint64_t current_time_ns();
};

class ConnectionTracker {
public:
    ConnectionTracker() = default;
    explicit ConnectionTracker(uint64_t timeout_ns) : timeout_ns_(timeout_ns) {}
    
    std::optional<Connection> create_connection(Protocol proto,
                                                const Ipv4Address& src_ip, Port src_port,
                                                const Ipv4Address& dst_ip, Port dst_port);
    
    std::optional<Connection> find_connection(const std::string& id);
    std::optional<Connection> find_by_flow(Protocol proto,
                                           const Ipv4Address& src_ip, Port src_port,
                                           const Ipv4Address& dst_ip, Port dst_port);
    
    void update_activity(const std::string& id);
    void remove_expired();
    size_t active_count() const;
    size_t total_count() const;
    std::vector<Connection> all_connections() const;
    
private:
    std::unordered_map<std::string, Connection> connections_;
    uint64_t timeout_ns_ = 300'000'000'000; // 5 minutes default
    
    static uint64_t current_time_ns();
    std::string generate_id(Protocol proto, const Ipv4Address& src_ip, Port src_port,
                            const Ipv4Address& dst_ip, Port dst_port);
};

} // namespace vns