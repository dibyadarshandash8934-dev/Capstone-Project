#include "vns/nat/nat.hpp"
#include "vns/network/network.hpp"
#include "vns/network/device.hpp"
#include "vns/nat/nat_engine.hpp"
#include "vns/nat/nat_table.hpp"
#include "vns/nat/pat_allocator.hpp"
#include "vns/nat/port_forwarding.hpp"
#include "vns/packet/packet.hpp"
#include "vns/utils/logger.hpp"
#include <iostream>
#include <cassert>
#include <vector>

using namespace vns;

int main() {
    VNS_LOG_INFO("Starting NAT tests");
    
    // Setup network
    auto net_opt = NetworkConfig::create("192.168.1.0/24", "192.168.1.1", "203.0.113.5");
    assert(net_opt.has_value());
    NetworkConfig network = *net_opt;
    
    // Add devices
    auto dev1 = VirtualDevice::create("PC-01", Ipv4Address::from_string("192.168.1.10").value(), VirtualDevice::Type::PC, network);
    auto dev2 = VirtualDevice::create("Web-01", Ipv4Address::from_string("192.168.1.20").value(), VirtualDevice::Type::SERVER, network);
    assert(dev1.has_value() && dev2.has_value());
    
    std::vector<VirtualDevice> devices = {*dev1, *dev2};
    
    // Create NAT engine
    NatEngineImpl engine(network, 40000, 50000);
    engine.add_device(*dev1);
    engine.add_device(*dev2);
    
    // Test PAT Allocator
    {
        PatAllocatorImpl allocator(40000, 40010);
        
        // Allocate all ports
        std::vector<Port> ports;
        for (int i = 0; i < 11; ++i) {
            auto port = allocator.allocate();
            assert(port.has_value());
            ports.push_back(*port);
        }
        
        // All ports should be unique
        for (size_t i = 0; i < ports.size(); ++i) {
            for (size_t j = i + 1; j < ports.size(); ++j) {
                assert(ports[i] != ports[j]);
            }
        }
        
        // Exhausted
        auto exhausted = allocator.allocate();
        assert(!exhausted.has_value());
        
        // Release and reallocate
        allocator.release(ports[0]);
        auto reused = allocator.allocate();
        assert(reused.has_value());
        assert(*reused == ports[0]);
        
        // Reset
        allocator.reset();
        auto first = allocator.allocate();
        assert(first.has_value());
        assert(*first == 40000);
    }
    
    // Test SNAT
    {
        // First packet - new mapping
        Packet pkt1(Protocol::TCP, 
                    Ipv4Address::from_string("192.168.1.10").value(), 50000,
                    Ipv4Address::from_string("8.8.8.8").value(), 443);
        
        auto result = engine.process_outgoing_packet(pkt1);
        assert(result.success());
        assert(result.action() == "SNAT");
        assert(result.translated_packet().has_value());
        assert(result.translated_packet()->source_ip().to_string() == "203.0.113.5");
        assert(result.translated_packet()->source_port() == 40000);
        assert(result.translated_packet()->destination_ip().to_string() == "8.8.8.8");
        assert(result.translated_packet()->destination_port() == 443);
        
        // Same packet - should reuse mapping
        auto result2 = engine.process_outgoing_packet(pkt1);
        assert(result2.success());
        assert(result2.translated_packet()->source_port() == 40000);
        
        // Different destination - new mapping
        Packet pkt2(Protocol::TCP, 
                    Ipv4Address::from_string("192.168.1.10").value(), 50000,
                    Ipv4Address::from_string("1.1.1.1").value(), 443);
        auto result3 = engine.process_outgoing_packet(pkt2);
        assert(result3.success());
        assert(result3.translated_packet()->source_port() == 40001);
        
        // UDP - different protocol, different port
        Packet pkt3(Protocol::UDP, 
                    Ipv4Address::from_string("192.168.1.10").value(), 50000,
                    Ipv4Address::from_string("8.8.8.8").value(), 53);
        auto result4 = engine.process_outgoing_packet(pkt3);
        assert(result4.success());
        assert(result4.translated_packet()->source_port() == 40002);
    }
    
    // Test Reverse NAT
    {
        // First create a mapping
        Packet pkt(Protocol::TCP, 
                   Ipv4Address::from_string("192.168.1.10").value(), 50001,
                   Ipv4Address::from_string("8.8.8.8").value(), 443);
        auto mapping = engine.process_outgoing_packet(pkt);
        assert(mapping.success());
        assert(mapping.translated_packet().has_value());
        Port public_port = mapping.translated_packet()->source_port();
        assert(public_port != 40000); // 40000 belongs to 192.168.1.10:50000
        
        // Simulate response to the public port of this mapping
        Packet response(Protocol::TCP,
                       Ipv4Address::from_string("8.8.8.8").value(), 443,
                       Ipv4Address::from_string("203.0.113.5").value(), public_port);
        
        auto result = engine.process_incoming_response(response);
        assert(result.success());
        assert(result.action() == "REVERSE_SNAT");
        assert(result.translated_packet()->destination_ip().to_string() == "192.168.1.10");
        assert(result.translated_packet()->destination_port() == 50001);
        
        // Unknown response
        Packet unknown(Protocol::TCP,
                      Ipv4Address::from_string("8.8.8.8").value(), 443,
                      Ipv4Address::from_string("203.0.113.5").value(), 49999);
        auto result2 = engine.process_incoming_response(unknown);
        assert(!result2.success());
        assert(result2.error().find("No active NAT mapping") != std::string::npos);
    }
    
    // Test DNAT / Port Forwarding
    {
        // Add port forward rule
        auto rule = PortForwardRule::create(Protocol::TCP,
                                          Ipv4Address::from_string("203.0.113.5").value(), 8080,
                                          Ipv4Address::from_string("192.168.1.20").value(), 80);
        assert(rule.has_value());
        engine.add_port_forward(*rule);
        
        // Inbound packet
        Packet pkt(Protocol::TCP,
                  Ipv4Address::from_string("198.51.100.20").value(), 45000,
                  Ipv4Address::from_string("203.0.113.5").value(), 8080);
        
        auto result = engine.process_incoming_packet(pkt);
        assert(result.success());
        assert(result.action() == "DNAT");
        assert(result.translated_packet()->destination_ip().to_string() == "192.168.1.20");
        assert(result.translated_packet()->destination_port() == 80);
        assert(result.translated_packet()->source_ip().to_string() == "198.51.100.20");
        assert(result.translated_packet()->source_port() == 45000);
        
        // No matching rule
        Packet pkt2(Protocol::TCP,
                   Ipv4Address::from_string("198.51.100.20").value(), 45000,
                   Ipv4Address::from_string("203.0.113.5").value(), 8081);
        auto result2 = engine.process_incoming_packet(pkt2);
        assert(!result2.success());
        assert(result2.error().find("No port forward rule") != std::string::npos);
        
        // Duplicate rule
        auto rule2 = PortForwardRule::create(Protocol::TCP,
                                           Ipv4Address::from_string("203.0.113.5").value(), 8080,
                                           Ipv4Address::from_string("192.168.1.20").value(), 8080);
        assert(rule2.has_value());
        try {
            engine.add_port_forward(*rule2);
            assert(false); // Should throw
        } catch (const std::invalid_argument&) {
            // Expected
        }
        
        // TCP/UDP independence
        auto rule3 = PortForwardRule::create(Protocol::UDP,
                                           Ipv4Address::from_string("203.0.113.5").value(), 8080,
                                           Ipv4Address::from_string("192.168.1.20").value(), 53);
        assert(rule3.has_value());
        engine.add_port_forward(*rule3);
        
        // Invalid private IP (no device)
        auto rule4 = PortForwardRule::create(Protocol::TCP,
                                           Ipv4Address::from_string("203.0.113.5").value(), 9090,
                                           Ipv4Address::from_string("192.168.1.99").value(), 80);
        assert(rule4.has_value());
        try {
            engine.add_port_forward(*rule4);
            assert(false);
        } catch (const std::invalid_argument&) {
            // Expected
        }
    }
    
    // Test the reply path for a port-forwarded service
    {
        // The forward rule was installed by the DNAT block above.
        auto rules = engine.get_port_forward_rules();
        bool rule_present = false;
        for (const auto& r : rules) {
            if (r.protocol() == Protocol::TCP && r.public_port() == 8080) {
                rule_present = true;
            }
        }
        assert(rule_present);
        
        // Inbound request to the forwarded port
        Packet pkt(Protocol::TCP,
                  Ipv4Address::from_string("198.51.100.20").value(), 45000,
                  Ipv4Address::from_string("203.0.113.5").value(), 8080);
        auto inbound = engine.process_incoming_packet(pkt);
        assert(inbound.success());
        assert(inbound.action() == "DNAT");
        
        // The LAN server replies to the Internet client: the source must be
        // translated back to the gateway public IP.
        Packet reply(Protocol::TCP,
                    Ipv4Address::from_string("192.168.1.20").value(), 80,
                    Ipv4Address::from_string("198.51.100.20").value(), 45000);
        auto out = engine.process_outgoing_packet(reply);
        assert(out.success());
        assert(out.action() == "SNAT");
        assert(out.translated_packet()->source_ip().to_string() == "203.0.113.5");
        // The reply leaves on the forwarded public port (8080) because the
        // DNAT entry already covers this flow.
        assert(out.translated_packet()->source_port() == 8080);
        assert(out.translated_packet()->destination_ip().to_string() == "198.51.100.20");
        assert(out.translated_packet()->destination_port() == 45000);
        Port reply_public_port = out.translated_packet()->source_port();
        
        // The client's next packet to that public port is reverse-NAT'ed
        // back to the server.
        Packet from_client(Protocol::TCP,
                          Ipv4Address::from_string("198.51.100.20").value(), 45000,
                          Ipv4Address::from_string("203.0.113.5").value(), reply_public_port);
        auto reverse = engine.process_incoming_response(from_client);
        assert(reverse.success());
        assert(reverse.action() == "REVERSE_SNAT");
        assert(reverse.translated_packet()->destination_ip().to_string() == "192.168.1.20");
        assert(reverse.translated_packet()->destination_port() == 80);
    }
    
    // Test NAT Table
    {
        auto entries = engine.get_nat_entries();
        assert(entries.size() > 0);
        
        // Check entries have correct fields
        for (const auto& e : entries) {
            assert(e.is_active());
            assert(e.protocol() == Protocol::TCP || e.protocol() == Protocol::UDP);
            assert(e.private_port() >= 1 && e.private_port() <= 65535);
            assert(e.public_port() >= 1 && e.public_port() <= 65535);
            
            // The public port must come from the PAT pool or from a forward rule.
            bool from_pat_pool = e.public_port() >= 40000 && e.public_port() <= 50000;
            bool from_forward_rule = false;
            for (const auto& r : engine.get_port_forward_rules()) {
                if (r.protocol() == e.protocol() &&
                    r.public_ip() == e.public_ip() &&
                    r.public_port() == e.public_port()) {
                    from_forward_rule = true;
                }
            }
            assert(from_pat_pool || from_forward_rule);
        }
    }
    
    // Test clear NAT table
    {
        engine.clear_nat_table();
        auto entries = engine.get_nat_entries();
        assert(entries.empty());
    }
    
    // Test Metrics
    {
        auto metrics = engine.metrics();
        assert(metrics.total_packets > 0);
        assert(metrics.successful_packets > 0);
        assert(metrics.average_processing_time_ms() >= 0);
    }
    
    std::cout << "All NAT tests passed!\n";
    return 0;
}