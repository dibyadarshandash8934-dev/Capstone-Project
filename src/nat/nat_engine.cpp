#include "vns/nat/nat_engine.hpp"
#include "vns/nat/nat_table.hpp"
#include "vns/nat/pat_allocator.hpp"
#include "vns/nat/port_forwarding.hpp"
#include "vns/network/network.hpp"
#include "vns/network/device.hpp"
#include "vns/packet/packet.hpp"
#include "vns/nat/nat.hpp"
#include "vns/connection/connection.hpp"
#include "vns/utils/logger.hpp"
#include <chrono>
#include <algorithm>
#include <stdexcept>

namespace vns {

void NatEngineImpl::PortForwardingDeleter::operator()(PortForwardingImpl* ptr) const {
    delete ptr;
}

void NatEngineImpl::NatTableDeleter::operator()(NatTableImpl* ptr) const {
    delete ptr;
}

void NatEngineImpl::PatAllocatorDeleter::operator()(PatAllocatorImpl* ptr) const {
    delete ptr;
}

NatEngineImpl::NatEngineImpl(const NetworkConfig& network,
                  uint16_t port_start,
                  uint16_t port_end)
    : network_(network),
      nat_table_(new NatTableImpl(), NatTableDeleter{}),
      port_allocator_(new PatAllocatorImpl(port_start, port_end), PatAllocatorDeleter{}),
      port_forwarding_(new PortForwardingImpl(), PortForwardingDeleter{}),
      metrics_{} {
    if (port_start < 1024 || port_end > 65535) {
        throw std::invalid_argument("Port range must be within 1024-65535");
    }
    if (port_start > port_end) {
        throw std::invalid_argument("Port range start must be less than or equal to end");
    }
}

SimulationResult NatEngineImpl::process_outgoing_packet(const Packet& packet) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<PacketTransformation> transformations;

    auto source_device = find_device_by_ip(packet.source_ip());
    if (!source_device) {
        metrics_.record_failure("SNAT", elapsed_ns(start));
        return SimulationResult::failure(packet,
            "Source device " + packet.source_ip().to_string() + " not found");
    }

    if (!network_.cidr().contains(packet.source_ip())) {
        metrics_.record_failure("SNAT", elapsed_ns(start));
        return SimulationResult::failure(packet,
            "Source IP " + packet.source_ip().to_string() + " not in LAN");
    }

    transformations.emplace_back(
        PacketStage::LAN, PacketAction::ORIGINAL,
        packet.source_ip(), packet.source_port(),
        packet.destination_ip(), packet.destination_port(),
        "Original packet from LAN device"
    );

    auto existing = nat_table_->find_by_flow(packet.flow_key());
    Port public_port = 0;
    NatEntry entry;

    if (existing) {
        public_port = existing->public_port();
        entry = *existing;
        entry.update_activity();
        nat_table_->add_entry(entry);
    } else if (const auto* pf = port_forwarding_->find_rule(
                   std::make_tuple(packet.protocol(), network_.public_ip(), packet.source_port()))) {
        // Not a PF match on public port; check private side below.
        (void)pf;
    }

    if (!existing) {
        for (const auto& rule : port_forwarding_->get_rules()) {
            if (rule.enabled() &&
                rule.protocol() == packet.protocol() &&
                rule.private_ip() == packet.source_ip() &&
                rule.private_port() == packet.source_port()) {
                public_port = rule.public_port();
                entry = NatEntry(packet.protocol(),
                                 packet.source_ip(), packet.source_port(),
                                 network_.public_ip(), public_port,
                                 packet.destination_ip(), packet.destination_port());
                nat_table_->add_entry(entry);
                existing = entry;
                break;
            }
        }
    }

    if (!existing) {
        auto allocated = port_allocator_->allocate();
        if (!allocated) {
            metrics_.record_failure("SNAT", elapsed_ns(start));
            return SimulationResult::failure(packet, "Port exhausted: no free public ports");
        }
        public_port = *allocated;
        entry = NatEntry(packet.protocol(),
                         packet.source_ip(), packet.source_port(),
                         network_.public_ip(), public_port,
                         packet.destination_ip(), packet.destination_port());
        nat_table_->add_entry(entry);
    }

    Packet translated = packet.copy_with(network_.public_ip(), public_port);

    transformations.emplace_back(
        PacketStage::NAT_GATEWAY, PacketAction::SNAT,
        packet.source_ip(), packet.source_port(),
        packet.destination_ip(), packet.destination_port(),
        "SNAT: " + packet.source_ip().to_string() + ":" +
        std::to_string(packet.source_port()) + " -> " +
        network_.public_ip().to_string() + ":" + std::to_string(public_port)
    );
    transformations.emplace_back(
        PacketStage::INTERNET, PacketAction::FORWARD,
        network_.public_ip(), public_port,
        packet.destination_ip(), packet.destination_port(),
        "Packet forwarded to Internet"
    );

    connection_tracker_.create_connection(
        packet.protocol(), packet.source_ip(), packet.source_port(),
        packet.destination_ip(), packet.destination_port());

    auto result = SimulationResult(packet, translated, transformations);
    result.set_nat_entry(entry);
    metrics_.record_success("SNAT", elapsed_ns(start));
    return result;
}

SimulationResult NatEngineImpl::process_incoming_response(const Packet& packet) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<PacketTransformation> transformations;

    transformations.emplace_back(
        PacketStage::INTERNET, PacketAction::ORIGINAL,
        packet.source_ip(), packet.source_port(),
        packet.destination_ip(), packet.destination_port(),
        "Incoming response from Internet"
    );

    auto reverse_key = std::make_tuple(
        packet.protocol(),
        packet.destination_ip(), packet.destination_port(),
        packet.source_ip(), packet.source_port());

    auto existing = nat_table_->find_by_reverse_flow(reverse_key);
    if (!existing) {
        metrics_.record_failure("REVERSE_SNAT", elapsed_ns(start));
        return SimulationResult::failure(packet, "No active NAT mapping for incoming packet");
    }

    Packet translated = packet.copy_with(
        Ipv4Address(), 0,
        existing->private_ip(), existing->private_port());

    transformations.emplace_back(
        PacketStage::NAT_GATEWAY, PacketAction::REVERSE_SNAT,
        packet.source_ip(), packet.source_port(),
        packet.destination_ip(), packet.destination_port(),
        "Reverse SNAT: " + packet.destination_ip().to_string() + ":" +
        std::to_string(packet.destination_port()) + " -> " +
        existing->private_ip().to_string() + ":" + std::to_string(existing->private_port())
    );
    transformations.emplace_back(
        PacketStage::LAN, PacketAction::FORWARD,
        packet.source_ip(), packet.source_port(),
        existing->private_ip(), existing->private_port(),
        "Packet forwarded to LAN device"
    );

    auto result = SimulationResult(packet, translated, transformations);
    result.set_nat_entry(*existing);
    metrics_.record_success("REVERSE_SNAT", elapsed_ns(start));
    return result;
}

std::optional<PortForwardRule> NatEngineImpl::add_port_forward(const PortForwardRule& rule) {
    if (rule.public_ip() != network_.public_ip()) {
        throw std::invalid_argument("Port forward public IP must match NAT gateway public IP");
    }

    if (!network_.cidr().contains(rule.private_ip())) {
        throw std::invalid_argument("Private IP not in LAN");
    }

    if (!find_device_by_ip(rule.private_ip())) {
        throw std::invalid_argument("Unknown host for private IP " + rule.private_ip().to_string());
    }

    auto match_key = rule.match_key();
    for (const auto& existing : port_forwarding_->get_rules()) {
        if (existing.enabled() && existing.match_key() == match_key) {
            throw std::invalid_argument(
                "Port forward rule for " + PortForwardRule::protocol_string(existing.protocol()) + " " +
                existing.public_ip().to_string() + ":" +
                std::to_string(existing.public_port()) + " already exists"
            );
        }
    }

    return port_forwarding_->add_rule(rule, network_, devices_);
}

void NatEngineImpl::remove_port_forward(const std::string& id) {
    port_forwarding_->remove_rule(id);
}

std::vector<PortForwardRule> NatEngineImpl::get_port_forward_rules() const {
    return port_forwarding_->get_rules();
}

SimulationResult NatEngineImpl::process_incoming_packet(const Packet& packet) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<PacketTransformation> transformations;

    transformations.emplace_back(
        PacketStage::INTERNET, PacketAction::ORIGINAL,
        packet.source_ip(), packet.source_port(),
        packet.destination_ip(), packet.destination_port(),
        "Incoming packet from Internet"
    );

    auto match_key = std::make_tuple(packet.protocol(), packet.destination_ip(), packet.destination_port());
    const PortForwardRule* matched_rule = port_forwarding_->find_rule(match_key);

    if (!matched_rule) {
        metrics_.record_failure("DNAT", elapsed_ns(start));
        return SimulationResult::failure(packet,
            "No port forward rule for " + packet.destination_ip().to_string() + ":" +
            std::to_string(packet.destination_port()) + " (" +
            protocol_string(packet.protocol()) + ")");
    }

    Packet translated = packet.copy_with(
        Ipv4Address(), 0,
        matched_rule->private_ip(), matched_rule->private_port()
    );

    NatEntry entry(packet.protocol(),
                   matched_rule->private_ip(), matched_rule->private_port(),
                   network_.public_ip(), matched_rule->public_port(),
                   packet.source_ip(), packet.source_port());
    nat_table_->add_entry(entry);

    transformations.emplace_back(
        PacketStage::NAT_GATEWAY, PacketAction::DNAT,
        packet.source_ip(), packet.source_port(),
        packet.destination_ip(), packet.destination_port(),
        "DNAT: " + packet.destination_ip().to_string() + ":" +
        std::to_string(packet.destination_port()) + " -> " +
        matched_rule->private_ip().to_string() + ":" + std::to_string(matched_rule->private_port())
    );
    transformations.emplace_back(
        PacketStage::LAN, PacketAction::FORWARD,
        translated.source_ip(), translated.source_port(),
        translated.destination_ip(), translated.destination_port(),
        "Packet forwarded to LAN device"
    );

    connection_tracker_.create_connection(
        packet.protocol(), packet.source_ip(), packet.source_port(),
        matched_rule->private_ip(), matched_rule->private_port());

    auto result = SimulationResult(packet, translated, transformations);
    result.set_nat_entry(entry);
    metrics_.record_success("DNAT", elapsed_ns(start));
    return result;
}

SimulationResult NatEngineImpl::process_packet(const Packet& packet, PacketDirection direction) {
    if (direction == PacketDirection::LAN_TO_INTERNET) {
        return process_outgoing_packet(packet);
    }

    auto dnat_result = process_incoming_packet(packet);
    if (dnat_result.success()) {
        return dnat_result;
    }
    return process_incoming_response(packet);
}

std::vector<NatEntry> NatEngineImpl::get_nat_entries() const {
    return nat_table_->get_active_entries();
}

std::optional<NatEntry> NatEngineImpl::find_nat_entry(
    const std::tuple<Protocol, Ipv4Address, Port, Ipv4Address, Port>& key) const {
    return nat_table_->find_by_flow(key);
}

void NatEngineImpl::clear_nat_table() {
    nat_table_->clear();
    port_allocator_->reset();
}

std::vector<Connection> NatEngineImpl::get_connections() const {
    return connection_tracker_.all_connections();
}

void NatEngineImpl::add_device(const VirtualDevice& device) {
    if (find_device_by_ip(device.ip())) {
        throw std::invalid_argument("Duplicate host IP " + device.ip().to_string());
    }
    devices_.push_back(device);
}

void NatEngineImpl::remove_device(const std::string& id) {
    devices_.erase(std::remove_if(devices_.begin(), devices_.end(),
        [&id](const VirtualDevice& d) { return d.id() == id; }),
        devices_.end());
}

std::vector<VirtualDevice> NatEngineImpl::devices() const {
    return devices_;
}

NatEngineImpl::~NatEngineImpl() = default;

uint64_t NatEngineImpl::elapsed_ns(std::chrono::high_resolution_clock::time_point start) {
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}

std::string NatEngineImpl::protocol_string(Protocol p) const {
    switch (p) {
        case Protocol::TCP: return "TCP";
        case Protocol::UDP: return "UDP";
        case Protocol::ICMP: return "ICMP";
    }
    return "UNKNOWN";
}

std::optional<VirtualDevice> NatEngineImpl::find_device_by_ip(const Ipv4Address& ip) const {
    for (const auto& device : devices_) {
        if (device.ip() == ip) {
            return device;
        }
    }
    return std::nullopt;
}

} // namespace vns
