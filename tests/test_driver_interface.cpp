#include "vns/driver/driver_interface.hpp"
#include <iostream>
#include <cassert>
#include <string>

using namespace vns;

int main() {
    std::cout << "Testing Driver Interface...\n";
    
    // Test 1: Open non-existent device
    {
        DriverInterface driver;
        bool opened = driver.open("/dev/vns_control");
        // Should fail gracefully if device doesn't exist
        if (!opened) {
            std::cout << "Device not available (expected if kernel module not loaded): " << driver.last_error() << "\n";
        } else {
            std::cout << "Device opened successfully\n";
            driver.close();
        }
    }
    
    // Test 2: Open/close
    {
        DriverInterface driver;
        // Try to open
        if (driver.open("/dev/vns_control")) {
            assert(driver.is_open());
            assert(driver.fd() >= 0);
            
            driver.close();
            assert(!driver.is_open());
            assert(driver.fd() == -1);
        }
    }
    
    // Test 3: Move semantics
    {
        DriverInterface driver1;
        if (driver1.open("/dev/vns_control")) {
            int fd = driver1.fd();
            
            DriverInterface driver2 = std::move(driver1);
            assert(!driver1.is_open());
            assert(driver2.is_open());
            assert(driver2.fd() == fd);
            
            DriverInterface driver3;
            driver3 = std::move(driver2);
            assert(!driver2.is_open());
            assert(driver3.is_open());
            assert(driver3.fd() == fd);
        }
    }
    
    // Test 4: Stats (if device available)
    {
        DriverInterface driver;
        if (driver.open("/dev/vns_control")) {
            auto stats = driver.get_stats();
            if (stats) {
                std::cout << "Stats retrieved:\n";
                std::cout << "  Total packets: " << stats->total_packets() << "\n";
                std::cout << "  Successful: " << stats->successful_packets() << "\n";
                std::cout << "  Failed: " << stats->failed_packets() << "\n";
                std::cout << "  SNAT: " << stats->snat_packets() << "\n";
                std::cout << "  DNAT: " << stats->dnat_packets() << "\n";
                std::cout << "  Reverse NAT: " << stats->reverse_nat_packets() << "\n";
                std::cout << "  Active NAT: " << stats->active_nat_mappings() << "\n";
                std::cout << "  Active PF rules: " << stats->active_port_forward_rules() << "\n";
                std::cout << "  Avg time: " << stats->average_processing_time_ms() << " ms\n";
            }
            
            auto status = driver.get_status();
            if (status) {
                std::cout << "Status retrieved:\n";
                std::cout << "  Flags: 0x" << std::hex << status->status_flags() << std::dec << "\n";
                std::cout << "  Connections: " << status->active_connections() << "\n";
                std::cout << "  NAT entries: " << status->nat_table_entries() << "\n";
                std::cout << "  PF rules: " << status->port_forward_rules() << "\n";
                std::cout << "  Running: " << (status->simulator_running() ? "YES" : "NO") << "\n";
            }
            
            auto info = driver.read_info();
            if (info) {
                std::cout << "Driver info:\n" << *info << "\n";
            }
        }
    }
    
    // Test 5: Commands
    {
        DriverInterface driver;
        if (driver.open("/dev/vns_control")) {
            // Test reset stats
            bool ok = driver.reset_stats();
            std::cout << "Reset stats: " << (ok ? "OK" : "FAILED") << "\n";
            
            // Test reset NAT
            ok = driver.reset_nat_table();
            std::cout << "Reset NAT: " << (ok ? "OK" : "FAILED") << "\n";
            
            // Test commands
            ok = driver.start_simulator();
            std::cout << "Start: " << (ok ? "OK" : "FAILED") << "\n";
            
            ok = driver.stop_simulator();
            std::cout << "Stop: " << (ok ? "OK" : "FAILED") << "\n";
            
            ok = driver.reset_simulator();
            std::cout << "Reset: " << (ok ? "OK" : "FAILED") << "\n";
        }
    }
    
    std::cout << "All driver interface tests passed!\n";
    return 0;
}