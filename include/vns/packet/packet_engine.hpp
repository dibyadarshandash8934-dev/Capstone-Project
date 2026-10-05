#pragma once

#include "vns/packet/packet.hpp"
#include "vns/nat/nat.hpp"
#include "vns/common/types.hpp"

namespace vns {

class NatEngineImpl;
class PacketEngineImpl {
public:
    explicit PacketEngineImpl(class NatEngineImpl* nat_engine);
    
    SimulationResult process_packet(const Packet& packet, PacketDirection direction);
    
private:
    class NatEngineImpl* nat_engine_;
};

} // namespace vns
