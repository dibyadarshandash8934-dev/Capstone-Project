// DNAT (destination NAT / port forwarding) is implemented in two places:
//   * PortForwardingImpl  - stores and validates the forward rules
//   * NatEngineImpl::process_incoming_packet() / add_port_forward() - applies them
//
// There is no separate "DNAT engine" object; keeping the logic next to the NAT
// engine avoids a second, diverging copy of the rule-matching code.
#include "vns/nat/dnat.hpp"
