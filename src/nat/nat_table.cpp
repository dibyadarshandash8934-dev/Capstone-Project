#include "vns/nat/nat_table.hpp"

namespace vns {

std::optional<NatEntry> NatTableImpl::find_by_flow(
    const std::tuple<Protocol, Ipv4Address, Port, Ipv4Address, Port>& key) const {
    auto it = forward_table_.find(key);
    if (it != forward_table_.end() && it->second.is_active()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<NatEntry> NatTableImpl::find_by_reverse_flow(
    const std::tuple<Protocol, Ipv4Address, Port, Ipv4Address, Port>& key) const {
    auto it = reverse_table_.find(key);
    if (it != reverse_table_.end() && it->second.is_active()) {
        return it->second;
    }
    return std::nullopt;
}

void NatTableImpl::add_entry(const NatEntry& entry) {
    forward_table_[entry.flow_key()] = entry;
    reverse_table_[entry.reverse_flow_key()] = entry;
}

void NatTableImpl::remove_entry(const NatEntry& entry) {
    forward_table_.erase(entry.flow_key());
    reverse_table_.erase(entry.reverse_flow_key());
}

std::vector<NatEntry> NatTableImpl::get_active_entries() const {
    std::vector<NatEntry> entries;
    for (const auto& [key, entry] : forward_table_) {
        if (entry.is_active()) {
            entries.push_back(entry);
        }
    }
    return entries;
}

size_t NatTableImpl::active_count() const {
    size_t count = 0;
    for (const auto& [key, entry] : forward_table_) {
        if (entry.is_active()) count++;
    }
    return count;
}

void NatTableImpl::clear() {
    forward_table_.clear();
    reverse_table_.clear();
}

} // namespace vns
