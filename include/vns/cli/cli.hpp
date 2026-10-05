#pragma once

#include "vns/common/types.hpp"
#include "vns/driver/driver_interface.hpp"
#include "vns/services/simulation_service.hpp"
#include "vns/packet/packet.hpp"
#include <memory>
#include <string>

namespace vns {

class Cli {
public:
    Cli();
    int run(int argc, char* argv[]);

private:
    std::unique_ptr<SimulationService> service_;
    DriverInterface driver_;

    void fetch_and_display_stats();
    void handle_choice(int choice);
    void handle_network_config();
    void handle_device_management();
    void add_device();
    void list_devices();
    void remove_device();
    void handle_nat_config();
    void handle_view_nat_table();
    void handle_port_forwarding();
    void add_port_forward_rule();
    void list_port_forward_rules();
    void delete_port_forward_rule();
    void handle_simulate_outbound();
    void handle_simulate_inbound();
    void simulate_packet(PacketDirection direction);
    void handle_view_connections();
    void handle_view_driver_stats();
    void handle_reset();
    bool confirm(const std::string& message);
};

} // namespace vns
