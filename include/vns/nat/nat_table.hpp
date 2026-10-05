#pragma once

#include "vns/nat/nat.hpp"
#include "vns/common/types.hpp"
#include <unordered_map>
#include <tuple>
#include <functional>
#include <vector>
#include <optional>

// Hash specialization for Ipv4Address - must be in global namespace
namespace std {
    template<>
    struct hash<vns::Ipv4Address> {
        size_t operator()(const vns::Ipv4Address& ip) const noexcept {
            return std::hash<uint32_t>{}(ip.value());
        }
    };
}

namespace vns {

struct TupleHash {
    template<typename... Args>
    size_t operator()(const std::tuple<Args...>& t) const noexcept {
        size_t seed = 0;
        hash_tuple(t, seed);
        return seed;
    }
    
private:
    template<typename Tuple, size_t... I>
    void hash_tuple(const Tuple& t, size_t& seed, std::index_sequence<I...>) const noexcept {
        ((hash_combine(seed, std::get<I>(t))), ...);
    }
    
    template<typename Tuple>
    void hash_tuple(const Tuple& t, size_t& seed) const noexcept {
        hash_tuple(t, seed, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
    }
    
    template<typename T>
    void hash_combine(size_t& seed, const T& v) const noexcept {
        seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
};

class NatTableImpl {
public:
    std::optional<NatEntry> find_by_flow(const std::tuple<Protocol, Ipv4Address, Port, Ipv4Address, Port>& key) const;
    std::optional<NatEntry> find_by_reverse_flow(const std::tuple<Protocol, Ipv4Address, Port, Ipv4Address, Port>& key) const;
    void add_entry(const NatEntry& entry);
    void remove_entry(const NatEntry& entry);
    std::vector<NatEntry> get_active_entries() const;
    size_t active_count() const;
    void clear();

private:
    std::unordered_map<
        std::tuple<Protocol, Ipv4Address, Port, Ipv4Address, Port>,
        NatEntry,
        TupleHash> forward_table_;
    
    std::unordered_map<
        std::tuple<Protocol, Ipv4Address, Port, Ipv4Address, Port>,
        NatEntry,
        TupleHash> reverse_table_;
};

} // namespace vns