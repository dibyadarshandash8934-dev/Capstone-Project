#include "vns/nat/dnat.hpp"
#include "vns/network/network.hpp"
#include "vns/network/device.hpp"
#include "vns/nat/nat.hpp"
#include "vns/nat/nat_engine.hpp"
#include "vns/nat/pat_allocator.hpp"
#include "vns/nat/port_forwarding.hpp"
#include "vns/packet/packet.hpp"
#include <iostream>
#include <cassert>
#include <stdexcept>

using namespace vns;

int main() {
    std::cout << "Testing DNAT / Port Forwarding...\n";
    
    auto net_opt = NetworkConfig::create("192.168.1.0/24", "192.168.1.1", "203.0.113.5");
    assert(net_opt.has_value());
    NetworkConfig network = net_opt.value();
    
    auto dev = VirtualDevice::create("Web-01", Ipv4Address::from_string("192.168.1.20").value(), VirtualDevice::Type::SERVER, network);
    assert(dev.has_value());
    
    NatEngineImpl engine(network, 40000, 50000);
    engine.add_device(*dev);
    
    // Test add rule
    {
        auto rule = PortForwardRule::create(Protocol::TCP,
                                          Ipv4Address::from_string("203.0.113.5").value(), 8080,
                                          Ipv4Address::from_string("192.168.1.20").value(), 80);
        assert(rule.has_value());
        auto result = engine.add_port_forward(*rule);
        assert(result.has_value());
        
        // Duplicate
        auto rule2 = PortForwardRule::create(Protocol::TCP,
                                            Ipv4Address::from_string("203.0.113.5").value(), 8080,
                                            Ipv4Address::from_string("192.168.1.20").value(), 8080);
        try {
            engine.add_port_forward(*rule2);
            assert(false);
        } catch (const std::invalid_argument&) {
            // Expected
        }
        
        // Clean up so the next block starts with an empty rule set.
        engine.remove_port_forward(result->id());
        assert(engine.get_port_forward_rules().empty());
    }
    
    // Test DNAT
    {
        auto rule = PortForwardRule::create(Protocol::TCP,
                                          Ipv4Address::from_string("203.0.113.5").value(), 8080,
                                          Ipv4Address::from_string("192.168.1.20").value(), 80);
        auto add_result = engine.add_port_forward(*rule);
        assert(add_result.has_value());
        
        Packet pkt(Protocol::TCP,
                  Ipv4Address::from_string("198.51.100.20").value(), 45000,
                  Ipv4Address::from_string("203.0.113.5").value(), 8080);
        
        auto sim_result = engine.process_incoming_packet(pkt);
        assert(sim_result.success());
        assert(sim_result.action() == "DNAT");
        assert(sim_result.translated_packet()->destination_ip().to_string() == "192.168.1.20");
        assert(sim_result.translated_packet()->destination_port() == 80);
        assert(sim_result.translated_packet()->source_ip().to_string() == "198.51.100.20");
        assert(sim_result.translated_packet()->source_port() == 45000);
        
        // Clean up so the next block starts with an empty rule set.
        engine.remove_port_forward(add_result->id());
        assert(engine.get_port_forward_rules().empty());
    }
    
    // Test no matching rule
    {
        Packet pkt(Protocol::TCP,
                  Ipv4Address::from_string("198.51.100.20").value(), 45000,
                  Ipv4Address::from_string("203.0.113.5").value(), 8081);
        auto result = engine.process_incoming_packet(pkt);
        assert(!result.success());
        assert(result.error().find("No port forward rule") != std::string::npos);
    }
    
    // TCP/UDP independence
    {
        auto rule_tcp = PortForwardRule::create(Protocol::TCP,
                                              Ipv4Address::from_string("203.0.113.5").value(), 8080,
                                              Ipv4Address::from_string("192.168.1.20").value(), 80);
        auto rule_udp = PortForwardRule::create(Protocol::UDP,
                                              Ipv4Address::from_string("203.0.113.5").value(), 8080,
                                              Ipv4Address::from_string("192.168.1.20").value(), 53);
        assert(engine.add_port_forward(*rule_tcp).has_value());
        assert(engine.add_port_forward(*rule_udp).has_value());
        
        auto rules = engine.get_port_forward_rules();
        assert(rules.size() == 2);
    }
    
    // Test invalid private IP (no device)
    {
        auto rule = PortForwardRule::create(Protocol::TCP,
                                          Ipv4Address::from_string("203.0.113.5").value(), 9090,
                                          Ipv4Address::from_string("192.168.1.99").value(), 80);
        try {
            engine.add_port_forward(*rule);
            assert(false);
        } catch (const std::invalid_argument&) {
            // Expected
        }
    }
    
    // Test delete rule
    {
        auto rule = PortForwardRule::create(Protocol::TCP,
                                          Ipv4Address::from_string("203.0.113.5").value(), 9090,
                                          Ipv4Address::from_string("192.168.1.20").value(), 80);
        auto result = engine.add_port_forward(*rule);
        assert(result.has_value());
        engine.remove_port_forward(result->id());
        auto rules = engine.get_port_forward_rules();
        assert(rules.size() == 2); // The two from before
    }
    
    // Test delete non-existent
    {
        engine.remove_port_forward("nonexistent");
        // Should not crash
    }
    
    std::cout << "All DNAT tests passed!\n";
    return 0;
}