#pragma once

#include "vns/common/types.hpp"
#include "vns/vns_ioctl.h"
#include "vns/nat/nat.hpp"
#include <string>
#include <optional>
#include <string_view>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

namespace vns {

class DriverInterface {
public:
    DriverInterface() = default;
    ~DriverInterface();
    
    DriverInterface(const DriverInterface&) = delete;
    DriverInterface& operator=(const DriverInterface&) = delete;
    
    DriverInterface(DriverInterface&& other) noexcept;
    DriverInterface& operator=(DriverInterface&& other) noexcept;
    
    bool open(const std::string& device_path = "/dev/vns_control");
    void close();
    
    bool is_open() const;
    int fd() const;
    const std::string& last_error() const;
    
    // Get statistics
    std::optional<VnsStats> get_stats();
    
    // Reset statistics
    bool reset_stats();
    
    // Get status
    std::optional<VnsStatus> get_status();
    
    // Set status
    bool set_status(const VnsStatus& status);
    
    // Get NAT entry
    std::optional<NatEntry> get_nat_entry();
    
    // Reset NAT table
    bool reset_nat();
    
    // Read device info (human readable)
    std::optional<std::string> read_info();
    
    // Write command (reset, status, stop)
    bool write_command(const std::string& cmd);
    
    // Convenience methods
    bool reset_nat_table();
    bool start_simulator();
    bool stop_simulator();
    bool reset_simulator();

private:
    int fd_ = -1;
    std::string last_error_;
};

} // namespace vns