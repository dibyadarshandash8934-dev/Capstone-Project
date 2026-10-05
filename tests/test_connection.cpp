#include "vns/connection/connection.hpp"
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>

using namespace vns;

int main() {
    std::cout << "Testing Connection Tracker...\n";
    
    // Test Connection creation
    {
        Connection conn("test-1", Protocol::TCP,
                       Ipv4Address::from_string("192.168.1.10").value(), 50000,
                       Ipv4Address::from_string("8.8.8.8").value(), 443);
        
        assert(conn.id() == "test-1");
        assert(conn.protocol() == Protocol::TCP);
        assert(conn.source_ip().to_string() == "192.168.1.10");
        assert(conn.source_port() == 50000);
        assert(conn.destination_ip().to_string() == "8.8.8.8");
        assert(conn.destination_port() == 443);
        assert(conn.state() == ConnectionState::NEW);
        assert(conn.is_active() == false); // NEW is not active
        
        conn.set_state(ConnectionState::ESTABLISHED);
        assert(conn.state() == ConnectionState::ESTABLISHED);
        assert(conn.is_active() == true);
    }
    
    // Test ConnectionTracker
    {
        ConnectionTracker tracker(1000000000); // 1 second timeout for testing
        
        // Create connection
        auto conn = tracker.create_connection(Protocol::TCP,
            Ipv4Address::from_string("192.168.1.10").value(), 50000,
            Ipv4Address::from_string("8.8.8.8").value(), 443);
        assert(conn.has_value());
        assert(conn->id() == "6_192.168.1.10_50000_8.8.8.8_443");
        
        // Duplicate
        auto dup = tracker.create_connection(Protocol::TCP,
            Ipv4Address::from_string("192.168.1.10").value(), 50000,
            Ipv4Address::from_string("8.8.8.8").value(), 443);
        assert(!dup.has_value());
        
        // Find
        auto found = tracker.find_connection("6_192.168.1.10_50000_8.8.8.8_443");
        assert(found.has_value());
        assert(found->id() == "6_192.168.1.10_50000_8.8.8.8_443");
        
        // Find by flow
        auto found2 = tracker.find_by_flow(Protocol::TCP,
            Ipv4Address::from_string("192.168.1.10").value(), 50000,
            Ipv4Address::from_string("8.8.8.8").value(), 443);
        assert(found2.has_value());
        assert(found2->id() == "6_192.168.1.10_50000_8.8.8.8_443");
        
        // Not found
        auto not_found = tracker.find_connection("nonexistent");
        assert(!not_found.has_value());
        
        // Update activity
        tracker.update_activity("6_192.168.1.10_50000_8.8.8.8_443");
        
        // Active count
        assert(tracker.active_count() == 1);
        assert(tracker.total_count() == 1);
    }
    
    // Test expiration
    {
        ConnectionTracker tracker(100000000); // 100ms timeout
        
        auto conn = tracker.create_connection(Protocol::TCP,
            Ipv4Address::from_string("192.168.1.10").value(), 50000,
            Ipv4Address::from_string("8.8.8.8").value(), 443);
        assert(conn.has_value());
        
        assert(conn->is_expired(100000000) == false);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        
        assert(conn->is_expired(100000000) == true);
        
        tracker.remove_expired();
        assert(tracker.total_count() == 0);
    }
    
    // Test multiple connections
    {
        ConnectionTracker tracker(1000000000);
        
        for (int i = 0; i < 10; ++i) {
            auto conn = tracker.create_connection(Protocol::TCP,
                Ipv4Address::from_string("192.168.1." + std::to_string(10 + i)).value(), 50000 + i,
                Ipv4Address::from_string("8.8.8.8").value(), 443);
            assert(conn.has_value());
        }
        
        assert(tracker.total_count() == 10);
        assert(tracker.active_count() == 10);
        
        auto all = tracker.all_connections();
        assert(all.size() == 10);
    }
    
    // Test state transitions
    {
        ConnectionTracker tracker(1000000000);
        
        auto conn = tracker.create_connection(Protocol::TCP,
            Ipv4Address::from_string("192.168.1.10").value(), 50000,
            Ipv4Address::from_string("8.8.8.8").value(), 443);
        
        assert(conn->state() == ConnectionState::NEW);
        assert(!conn->is_active());
        
        conn->set_state(ConnectionState::ACTIVE);
        assert(conn->is_active());
        
        conn->set_state(ConnectionState::ESTABLISHED);
        assert(conn->is_active());
        
        conn->set_state(ConnectionState::CLOSED);
        assert(!conn->is_active());
    }
    
    std::cout << "All connection tests passed!\n";
    return 0;
}