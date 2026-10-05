#include "vns/packet/packet.hpp"
#include "vns/network/network.hpp"
#include <iostream>
#include <cassert>

using namespace vns;

int main() {
    std::cout << "Testing Packet...\n";
    
    // Test packet creation
    {
        Packet pkt(Protocol::TCP, 
                   Ipv4Address::from_string("192.168.1.10").value(), 50000,
                   Ipv4Address::from_string("8.8.8.8").value(), 443);
        
        assert(pkt.protocol() == Protocol::TCP);
        assert(pkt.source_ip().to_string() == "192.168.1.10");
        assert(pkt.source_port() == 50000);
        assert(pkt.destination_ip().to_string() == "8.8.8.8");
        assert(pkt.destination_port() == 443);
    }
    
    // Test flow key
    {
        Packet pkt(Protocol::TCP, 
                   Ipv4Address::from_string("192.168.1.10").value(), 50000,
                   Ipv4Address::from_string("8.8.8.8").value(), 443);
        
        auto key = pkt.flow_key();
        auto [proto, src_ip, src_port, dst_ip, dst_port] = key;
        assert(proto == Protocol::TCP);
        assert(src_ip.to_string() == "192.168.1.10");
        assert(src_port == 50000);
        assert(dst_ip.to_string() == "8.8.8.8");
        assert(dst_port == 443);
        
        // Reverse
        auto rev_key = pkt.reverse_flow_key();
        auto [r_proto, r_dst_ip, r_dst_port, r_src_ip, r_src_port] = rev_key;
        assert(r_proto == Protocol::TCP);
        assert(r_dst_ip.to_string() == "8.8.8.8");
        assert(r_dst_port == 443);
        assert(r_src_ip.to_string() == "192.168.1.10");
        assert(r_src_port == 50000);
    }
    
    // Test copy_with
    {
        Packet pkt(Protocol::TCP, 
                   Ipv4Address::from_string("192.168.1.10").value(), 50000,
                   Ipv4Address::from_string("8.8.8.8").value(), 443);
        
        auto translated = pkt.copy_with(
            Ipv4Address::from_string("203.0.113.5").value(), 40000,
            Ipv4Address(), 0
        );
        
        assert(translated.source_ip().to_string() == "203.0.113.5");
        assert(translated.source_port() == 40000);
        assert(translated.destination_ip().to_string() == "8.8.8.8");
        assert(translated.destination_port() == 443);
        
        // Original unchanged
        assert(pkt.source_ip().to_string() == "192.168.1.10");
        assert(pkt.source_port() == 50000);
    }
    
    // Test UDP
    {
        Packet pkt(Protocol::UDP,
                   Ipv4Address::from_string("192.168.1.10").value(), 50000,
                   Ipv4Address::from_string("8.8.8.8").value(), 53);
        
        assert(pkt.protocol() == Protocol::UDP);
        assert(pkt.source_port() == 50000);
        assert(pkt.destination_port() == 53);
    }
    
    // Test ICMP
    {
        Packet pkt(Protocol::ICMP,
                   Ipv4Address::from_string("192.168.1.10").value(), 0,
                   Ipv4Address::from_string("8.8.8.8").value(), 0);
        
        assert(pkt.protocol() == Protocol::ICMP);
    }
    
    // Test to_string
    {
        Packet pkt(Protocol::TCP, 
                   Ipv4Address::from_string("192.168.1.10").value(), 50000,
                   Ipv4Address::from_string("8.8.8.8").value(), 443);
        
        std::string s = pkt.to_string();
        assert(s.find("TCP") != std::string::npos);
        assert(s.find("192.168.1.10:50000") != std::string::npos);
        assert(s.find("8.8.8.8:443") != std::string::npos);
    }
    
    // Test from_string round trip
    {
        Packet pkt(Protocol::UDP,
                   Ipv4Address::from_string("192.168.1.10").value(), 50000,
                   Ipv4Address::from_string("8.8.8.8").value(), 53);
        
        auto parsed = Packet::from_string(pkt.to_string());
        assert(parsed.has_value());
        assert(parsed->protocol() == Protocol::UDP);
        assert(parsed->source_ip().to_string() == "192.168.1.10");
        assert(parsed->source_port() == 50000);
        assert(parsed->destination_ip().to_string() == "8.8.8.8");
        assert(parsed->destination_port() == 53);
        
        // Malformed input is rejected
        assert(!Packet::from_string("").has_value());
        assert(!Packet::from_string("not a packet").has_value());
        assert(!Packet::from_string("TCP 192.168.1.10:50000").has_value());
        assert(!Packet::from_string("XYZ 192.168.1.10:50000 -> 8.8.8.8:53").has_value());
        assert(!Packet::from_string("TCP 192.168.1.10:99999 -> 8.8.8.8:53").has_value());
    }
    
    std::cout << "All packet tests passed!\n";
    return 0;
}