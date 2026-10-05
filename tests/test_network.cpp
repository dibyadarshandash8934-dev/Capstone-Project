#include "vns/network/network.hpp"
#include "vns/network/device.hpp"
#include "vns/utils/logger.hpp"
#include <iostream>
#include <cassert>

using namespace vns;

int main() {
    VNS_LOG_INFO("Starting network tests");
    
    // Test Ipv4Address
    {
        auto ip_opt = Ipv4Address::from_string("192.168.1.10");
        assert(ip_opt.has_value());
        assert(ip_opt->to_string() == "192.168.1.10");
        assert(ip_opt->value() == 0xC0A8010A);
        
        // Invalid IPs
        assert(!Ipv4Address::from_string("999.168.1.10"));
        assert(!Ipv4Address::from_string("192.168.1.256"));
        assert(!Ipv4Address::from_string("192.168.1"));
        assert(!Ipv4Address::from_string("invalid"));
    }
    
    // Test Cidr
    {
        auto cidr_opt = Cidr::from_string("192.168.1.0/24");
        assert(cidr_opt.has_value());
        assert(cidr_opt->to_string() == "192.168.1.0/24");
        assert(cidr_opt->prefix_len() == 24);
        assert(cidr_opt->netmask() == 0xFFFFFF00);
        assert(cidr_opt->host_count() == 254);
        
        // Test contains
        auto ip1 = Ipv4Address::from_string("192.168.1.10");
        auto ip2 = Ipv4Address::from_string("192.168.2.10");
        assert(cidr_opt->contains(*ip1));
        assert(!cidr_opt->contains(*ip2));
        
        // Invalid CIDR
        assert(!Cidr::from_string("invalid"));
        assert(!Cidr::from_string("192.168.1.0/33"));
        assert(!Cidr::from_string("192.168.1.0/7"));
    }
    
    // Test NetworkConfig
    {
        auto net_opt = NetworkConfig::create("192.168.1.0/24", "192.168.1.1", "203.0.113.5");
        assert(net_opt.has_value());
        assert(net_opt->cidr().to_string() == "192.168.1.0/24");
        assert(net_opt->gateway_ip().to_string() == "192.168.1.1");
        assert(net_opt->public_ip().to_string() == "203.0.113.5");
        
        // Invalid gateway outside network
        assert(!NetworkConfig::create("192.168.1.0/24", "192.168.2.1", "203.0.113.5").has_value());
        
        // Invalid gateway (network address)
        assert(!NetworkConfig::create("192.168.1.0/24", "192.168.1.0", "203.0.113.5").has_value());
        
        // Invalid gateway (broadcast address)
        assert(!NetworkConfig::create("192.168.1.0/24", "192.168.1.255", "203.0.113.5").has_value());
    }
    
    // Test VirtualDevice
    {
        auto net_opt = NetworkConfig::create("192.168.1.0/24", "192.168.1.1", "203.0.113.5");
        assert(net_opt.has_value());
        
        auto device = VirtualDevice::create("PC-01", 
            Ipv4Address::from_string("192.168.1.10").value(), 
            VirtualDevice::Type::PC, *net_opt);
        assert(device.has_value());
        assert(device->id() == "pc-01");
        assert(device->name() == "PC-01");
        assert(device->ip().to_string() == "192.168.1.10");
        assert(device->type() == VirtualDevice::Type::PC);
        
        // Duplicate IP should fail on network service level
        auto device2 = VirtualDevice::create("PC-02", 
            Ipv4Address::from_string("192.168.1.10").value(), 
            VirtualDevice::Type::PC, *net_opt);
        // Device creation itself doesn't fail - network service handles duplicates
        
        // Invalid IP outside network
        assert(!VirtualDevice::create("PC-03", 
            Ipv4Address::from_string("192.168.2.10").value(), 
            VirtualDevice::Type::PC, *net_opt).has_value());
        
        // Gateway IP cannot be used
        assert(!VirtualDevice::create("GW", 
            Ipv4Address::from_string("192.168.1.1").value(), 
            VirtualDevice::Type::PC, *net_opt).has_value());
    }
    
    // Test Network address calculations
    {
        auto cidr = Cidr::from_string("192.168.1.0/24").value();
        assert(cidr.first_host().to_string() == "192.168.1.1");
        assert(cidr.last_host().to_string() == "192.168.1.254");
        assert(cidr.network().to_string() == "192.168.1.0");
        assert(cidr.broadcast().to_string() == "192.168.1.255");
        assert(cidr.host_count() == 254);
    }
    
    // Test gateway validation
    {
        auto cidr = Cidr::from_string("10.0.0.0/8").value();
        auto gw1 = Ipv4Address::from_string("10.0.0.1").value();
        auto gw2 = Ipv4Address::from_string("10.255.255.254").value();
        assert(cidr.contains(gw1));
        assert(cidr.contains(gw2));
        
        auto net_opt = NetworkConfig::create("10.0.0.0/8", "10.0.0.1", "203.0.113.5");
        assert(net_opt.has_value());
    }
    
    std::cout << "All network tests passed!\n";
    return 0;
}