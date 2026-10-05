#include "vns/nat/pat_allocator.hpp"
#include "vns/utils/logger.hpp"
#include <algorithm>
#include <stdexcept>

namespace vns {

PatAllocatorImpl::PatAllocatorImpl(uint16_t start, uint16_t end) 
    : range_start_(start), range_end_(end), next_port_(start) {
    if (start > end) {
        throw std::invalid_argument("Port range start must be less than or equal to end");
    }
    if (start < 1024) {
        throw std::invalid_argument("Port range must be within 1024-65535");
    }
}

std::optional<Port> PatAllocatorImpl::allocate() {
    for (Port port = next_port_; port <= range_end_; ++port) {
        if (allocated_ports_.find(port) == allocated_ports_.end()) {
            allocated_ports_.insert(port);
            next_port_ = port + 1;
            return port;
        }
    }
    // Wrap around and try from start
    for (Port port = range_start_; port < next_port_; ++port) {
        if (allocated_ports_.find(port) == allocated_ports_.end()) {
            allocated_ports_.insert(port);
            next_port_ = port + 1;
            return port;
        }
    }
    return std::nullopt;
}

void PatAllocatorImpl::release(Port port) {
    allocated_ports_.erase(port);
    if (port < next_port_) {
        next_port_ = port;
    }
}

bool PatAllocatorImpl::is_allocated(Port port) const {
    return allocated_ports_.find(port) != allocated_ports_.end();
}

size_t PatAllocatorImpl::allocated_count() const { return allocated_ports_.size(); }
size_t PatAllocatorImpl::available_count() const { 
    return static_cast<size_t>(range_end_ - range_start_ + 1) - allocated_ports_.size();
}

void PatAllocatorImpl::reset() {
    allocated_ports_.clear();
    next_port_ = range_start_;
}

} // namespace vns