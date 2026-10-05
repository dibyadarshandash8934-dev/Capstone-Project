#include "vns/nat/pat_allocator.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <set>

using namespace vns;

int main() {
    std::cout << "Testing PAT Allocator...\n";
    
    // Test basic allocation
    {
        PatAllocatorImpl alloc(40000, 40010);
        std::vector<Port> ports;
        
        for (int i = 0; i < 11; ++i) {
            auto p = alloc.allocate();
            assert(p.has_value());
            ports.push_back(*p);
        }
        
        // All unique
        std::set<Port> unique(ports.begin(), ports.end());
        assert(unique.size() == ports.size());
        
        // Exhausted
        assert(!alloc.allocate().has_value());
    }
    
    // Test release and reuse
    {
        PatAllocatorImpl alloc(40000, 40005);
        auto p1 = alloc.allocate();
        auto p2 = alloc.allocate();
        assert(p1.has_value() && p2.has_value());
        
        alloc.release(*p1);
        auto p3 = alloc.allocate();
        assert(p3.has_value() && *p3 == *p1);
    }
    
    // Test wrap around
    {
        PatAllocatorImpl alloc(40000, 40002);
        auto p1 = alloc.allocate();
        auto p2 = alloc.allocate();
        auto p3 = alloc.allocate();
        assert(p1 && p2 && p3);
        
        // Exhausted
        assert(!alloc.allocate().has_value());
        
        // Release middle
        alloc.release(*p2);
        auto p4 = alloc.allocate();
        assert(p4 && *p4 == *p2);
    }
    
    // Test reset
    {
        PatAllocatorImpl alloc(40000, 40010);
        for (int i = 0; i < 5; ++i) alloc.allocate();
        alloc.reset();
        auto p = alloc.allocate();
        assert(p && *p == 40000);
    }
    
    // Test edge cases
    {
        // Invalid range
        try {
            PatAllocatorImpl alloc(40010, 40000);
            assert(false);
        } catch (const std::invalid_argument&) {}
        
        // Port below 1024
        try {
            PatAllocatorImpl alloc(1000, 2000);
            assert(false);
        } catch (const std::invalid_argument&) {}
        
        // Single port
        PatAllocatorImpl alloc(40000, 40000);
        auto p1 = alloc.allocate();
        assert(p1 && *p1 == 40000);
        assert(!alloc.allocate().has_value());
    }
    
    std::cout << "All PAT tests passed!\n";
    return 0;
}