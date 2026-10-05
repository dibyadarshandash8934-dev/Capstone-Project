#include "vns/driver/driver_interface.hpp"
#include "vns/utils/logger.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstring>
#include <cerrno>

namespace vns {

DriverInterface::DriverInterface(DriverInterface&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

DriverInterface::~DriverInterface() { close(); }

DriverInterface& DriverInterface::operator=(DriverInterface&& other) noexcept {
    if (this != &other) {
        close();
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

bool DriverInterface::open(const std::string& device_path) {
    if (is_open()) return true;
    
    fd_ = ::open(device_path.c_str(), O_RDWR);
    if (fd_ < 0) {
        last_error_ = "Failed to open device: " + std::string(strerror(errno));
        VNS_LOG_ERROR_FMT("Failed to open %s: %s", device_path.c_str(), strerror(errno));
        return false;
    }
    
    VNS_LOG_INFO_FMT("Opened device %s", device_path.c_str());
    return true;
}

void DriverInterface::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

bool DriverInterface::is_open() const { return fd_ >= 0; }
int DriverInterface::fd() const { return fd_; }
const std::string& DriverInterface::last_error() const { return last_error_; }

std::optional<VnsStats> DriverInterface::get_stats() {
    if (!is_open()) return std::nullopt;
    
    ::vns_stats stats;
    if (ioctl(fd_, VNS_IOCTL_GET_STATS, &stats) < 0) {
        last_error_ = "ioctl GET_STATS failed: " + std::string(strerror(errno));
        VNS_LOG_ERROR_FMT("ioctl GET_STATS failed: %s", strerror(errno));
        return std::nullopt;
    }
    return VnsStats(stats);
}

bool DriverInterface::reset_stats() {
    if (!is_open()) return false;
    if (ioctl(fd_, VNS_IOCTL_RESET_STATS) < 0) {
        last_error_ = "ioctl RESET_STATS failed: " + std::string(strerror(errno));
        VNS_LOG_ERROR_FMT("ioctl RESET_STATS failed: %s", strerror(errno));
        return false;
    }
    return true;
}

std::optional<VnsStatus> DriverInterface::get_status() {
    if (!is_open()) return std::nullopt;
    
    ::vns_status status;
    if (ioctl(fd_, VNS_IOCTL_GET_STATUS, &status) < 0) {
        last_error_ = "ioctl GET_STATUS failed: " + std::string(strerror(errno));
        VNS_LOG_ERROR_FMT("ioctl GET_STATUS failed: %s", strerror(errno));
        return std::nullopt;
    }
    return VnsStatus(status);
}

bool DriverInterface::set_status(const VnsStatus& status) {
    if (!is_open()) return false;
    if (ioctl(fd_, VNS_IOCTL_SET_STATUS, &status) < 0) {
        last_error_ = "ioctl SET_STATUS failed: " + std::string(strerror(errno));
        VNS_LOG_ERROR_FMT("ioctl SET_STATUS failed: %s", strerror(errno));
        return false;
    }
    return true;
}

std::optional<NatEntry> DriverInterface::get_nat_entry() {
    if (!is_open()) return std::nullopt;
    
    ::vns_nat_entry entry;
    if (ioctl(fd_, VNS_IOCTL_GET_NAT_ENTRY, &entry) < 0) {
        last_error_ = "ioctl GET_NAT_ENTRY failed: " + std::string(strerror(errno));
        VNS_LOG_ERROR_FMT("ioctl GET_NAT_ENTRY failed: %s", strerror(errno));
        return std::nullopt;
    }
    return NatEntry(entry);
}

bool DriverInterface::reset_nat() {
    if (!is_open()) return false;
    if (ioctl(fd_, VNS_IOCTL_RESET_NAT) < 0) {
        last_error_ = "ioctl RESET_NAT failed: " + std::string(strerror(errno));
        VNS_LOG_ERROR_FMT("ioctl RESET_NAT failed: %s", strerror(errno));
        return false;
    }
    return true;
}

std::optional<std::string> DriverInterface::read_info() {
    if (!is_open()) return std::nullopt;
    
    char buffer[1024];
    ssize_t bytes = ::read(fd_, buffer, sizeof(buffer) - 1);
    if (bytes < 0) {
        last_error_ = "read failed: " + std::string(strerror(errno));
        VNS_LOG_ERROR_FMT("read failed: %s", strerror(errno));
        return std::nullopt;
    }
    buffer[bytes] = '\0';
    return std::string(buffer);
}

bool DriverInterface::write_command(const std::string& cmd) {
    if (!is_open()) return false;
    ssize_t bytes = ::write(fd_, cmd.c_str(), cmd.size());
    if (bytes < 0) {
        last_error_ = "write failed: " + std::string(strerror(errno));
        VNS_LOG_ERROR_FMT("write failed: %s", strerror(errno));
        return false;
    }
    return true;
}

bool DriverInterface::reset_nat_table() { return reset_nat(); }
bool DriverInterface::start_simulator() { return write_command("status"); }
bool DriverInterface::stop_simulator() { return write_command("stop"); }
bool DriverInterface::reset_simulator() { return write_command("reset"); }

VnsStats::VnsStats(const ::vns_stats& stats)
    : total_packets_(stats.total_packets)
    , successful_packets_(stats.successful_packets)
    , failed_packets_(stats.failed_packets)
    , snat_packets_(stats.snat_packets)
    , dnat_packets_(stats.dnat_packets)
    , reverse_nat_packets_(stats.reverse_nat_packets)
    , active_nat_mappings_(stats.active_nat_mappings)
    , active_port_forward_rules_(stats.active_port_forward_rules)
    , total_processing_time_ns_(stats.total_processing_time_ns) {}

VnsStatus::VnsStatus(const ::vns_status& status)
    : status_flags_(status.status_flags)
    , active_connections_(status.active_connections)
    , nat_table_entries_(status.nat_table_entries)
    , port_forward_rules_(status.port_forward_rules)
    , simulator_running_(status.simulator_running)
    , simulator_version_(status.simulator_version) {}

} // namespace vns