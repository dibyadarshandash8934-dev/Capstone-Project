#pragma once

#include "vns/common/types.hpp"
#include <optional>
#include <unordered_set>

namespace vns {

class PatAllocatorImpl {
public:
    PatAllocatorImpl(uint16_t start, uint16_t end);
    
    std::optional<Port> allocate();
    void release(Port port);
    bool is_allocated(Port port) const;
    size_t allocated_count() const;
    size_t available_count() const;
    void reset();
    
private:
    uint16_t range_start_;
    uint16_t range_end_;
    uint16_t next_port_;
    std::unordered_set<Port> allocated_ports_;
};

} // namespace vns