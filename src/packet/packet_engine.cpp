#include "vns/packet/packet_engine.hpp"
#include "vns/nat/nat_engine.hpp"
#include "vns/utils/logger.hpp"

namespace vns {

PacketEngineImpl::PacketEngineImpl(NatEngineImpl* nat_engine) : nat_engine_(nat_engine) {}

SimulationResult PacketEngineImpl::process_packet(const Packet& packet, PacketDirection direction) {
    if (direction == PacketDirection::LAN_TO_INTERNET) {
        return nat_engine_->process_outgoing_packet(packet);
    } else {
        // Try DNAT first
        auto dnat_result = nat_engine_->process_incoming_packet(packet);
        if (dnat_result.success()) {
            return dnat_result;
        }
        // Try reverse NAT
        return nat_engine_->process_incoming_response(packet);
    }
}

} // namespace vns