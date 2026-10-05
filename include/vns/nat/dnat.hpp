#pragma once

#include "vns/nat/port_forwarding.hpp"

namespace vns {

// DNAT is implemented as port-forwarding rules in PortForwardingImpl
// and applied by NatEngineImpl::process_incoming_packet().

} // namespace vns
