#include "vns/cli/formatter.hpp"
#include "vns/cli/cli.hpp"
#include "vns/network/network.hpp"
#include "vns/network/device.hpp"
#include "vns/nat/nat.hpp"
#include "vns/nat/nat_engine.hpp"
#include "vns/nat/nat_table.hpp"
#include "vns/nat/pat_allocator.hpp"
#include "vns/nat/port_forwarding.hpp"
#include "vns/packet/packet.hpp"
#include "vns/packet/packet_engine.hpp"
#include "vns/connection/connection.hpp"
#include "vns/driver/driver_interface.hpp"
#include "vns/services/simulation_service.hpp"
#include "vns/utils/logger.hpp"
#include "vns/utils/helpers.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>

namespace vns {

namespace {

void print_usage(std::ostream& os) {
    os << "VNS - Virtual NAT Gateway Simulator\n"
       << "Usage: vns_sim [options]\n\n"
       << "Options:\n"
       << "  -h, --help          Show this help message and exit\n"
       << "      --version       Print the version and exit\n"
       << "      --driver-stats  Print kernel driver statistics and exit\n"
       << "\nWithout options an interactive menu is started.\n";
}

const char* version_string() {
#ifdef VNS_VERSION_STRING
    return VNS_VERSION_STRING;
#else
    return "1.0.0";
#endif
}

} // namespace

Cli::Cli() : service_(std::make_unique<SimulationService>()), driver_() {}

int Cli::run(int argc, char* argv[]) {
    std::vector<std::string> args(argv, argv + argc);

    if (args.size() > 1) {
        const std::string& cmd = args[1];
        if (cmd == "-h" || cmd == "--help") {
            print_usage(std::cout);
            return 0;
        }
        if (cmd == "--version") {
            std::cout << "vns " << version_string() << "\n";
            return 0;
        }
        if (cmd == "--driver-stats") {
            driver_.open();
            bool printed = false;
            if (auto stats = driver_.get_stats()) {
                print_driver_stats(*stats);
                printed = true;
            }
            if (auto status = driver_.get_status()) {
                print_driver_status(*status);
                printed = true;
            }
            if (!printed) {
                std::cerr << "Driver not available: " << driver_.last_error() << "\n";
                return 1;
            }
            return 0;
        }

        std::cerr << "Unknown option: " << cmd << "\n\n";
        print_usage(std::cerr);
        return 2;
    }

    VNS_LOG_INFO("Starting VNS Virtual NAT Gateway Simulator v" + std::string(version_string()));

    // Driver access needs root; warn early in interactive mode.
    if (geteuid() != 0) {
        VNS_LOG_WARN("Not running as root. Driver interface may not be available.");
        std::cerr << "Warning: Not running as root. Kernel driver access may be limited.\n";
    }

    // Try to connect to driver
    driver_.open();

    int choice = -1;
    while (choice != 0) {
        if (service_->is_configured()) {
            fetch_and_display_stats();
        }

        print_menu();
        std::string input;
        if (!std::getline(std::cin, input)) {
            // End of input (Ctrl-D or a closed pipe)
            std::cout << "\n";
            break;
        }
        input = utils::trim(input);

        if (input.empty()) continue;

        try {
            choice = std::stoi(input);
            handle_choice(choice);
        } catch (...) {
            std::cout << "Invalid choice. Please enter a number.\n";
        }
    }

    std::cout << "\nGoodbye!\n";
    return 0;
}

void Cli::fetch_and_display_stats() {
    const auto metrics = service_->metrics();
    std::cout << "[stats] packets=" << metrics.total_packets
              << " ok=" << metrics.successful_packets
              << " failed=" << metrics.failed_packets;

    if (auto stats = driver_.get_stats()) {
        std::cout << " | [driver] mappings=" << stats->active_nat_mappings()
                  << " rules=" << stats->active_port_forward_rules();
    } else {
        std::cout << " | [driver] unavailable: " << driver_.last_error();
    }
    std::cout << "\n";
}

void Cli::handle_choice(int choice) {
    switch (choice) {
        case 0: break; // handled by the caller's loop condition
        case 1: handle_network_config(); break;
        case 2: handle_device_management(); break;
        case 3: handle_nat_config(); break;
        case 4: handle_view_nat_table(); break;
        case 5: handle_port_forwarding(); break;
        case 6: handle_simulate_outbound(); break;
        case 7: handle_simulate_inbound(); break;
        case 8: handle_view_connections(); break;
        case 9: handle_view_driver_stats(); break;
        case 10: handle_reset(); break;
        default: std::cout << "Invalid choice.\n";
    }
}

void Cli::handle_network_config() {
    if (service_->is_configured()) {
        std::cout << "\nNetwork already configured. Reconfigure? (y/N): ";
        std::string confirm_input;
        std::getline(std::cin, confirm_input);
        if (utils::to_lower(utils::trim(confirm_input)) != "y") return;
    }

    std::string cidr, gateway, public_ip;

    std::cout << "\nEnter CIDR (e.g., 192.168.1.0/24): ";
    std::getline(std::cin, cidr);
    cidr = utils::trim(cidr);

    std::cout << "Enter Gateway IP (e.g., 192.168.1.1): ";
    std::getline(std::cin, gateway);
    gateway = utils::trim(gateway);

    std::cout << "Enter Public NAT IP (e.g., 203.0.113.10): ";
    std::getline(std::cin, public_ip);
    public_ip = utils::trim(public_ip);

    if (cidr.empty() || gateway.empty() || public_ip.empty()) {
        std::cout << "All fields are required.\n";
        return;
    }

    if (!service_->create_network(cidr, gateway, public_ip)) {
        std::cout << "Failed to create network. Check inputs.\n";
        return;
    }

    std::cout << "\nNetwork created successfully!\n";
}

void Cli::handle_device_management() {
    if (!service_->is_configured()) {
        std::cout << "Network not configured. Create network first.\n";
        return;
    }

    while (true) {
        std::cout << "\n--- Device Management ---\n";
        std::cout << "1. Add Device\n";
        std::cout << "2. List Devices\n";
        std::cout << "3. Remove Device\n";
        std::cout << "4. Back\n";
        std::cout << "Choice: ";

        std::string input;
        if (!std::getline(std::cin, input)) return;
        int choice = 0;
        try { choice = std::stoi(utils::trim(input)); } catch (...) { continue; }

        switch (choice) {
            case 1: add_device(); break;
            case 2: list_devices(); break;
            case 3: remove_device(); break;
            case 4: return;
            default: std::cout << "Invalid choice.\n";
        }
    }
}

void Cli::add_device() {
    std::string name, ip_str, type_str;

    std::cout << "Device Name: ";
    std::getline(std::cin, name);
    name = utils::trim(name);
    if (name.empty()) {
        std::cout << "Name is required.\n";
        return;
    }

    std::cout << "Private IP: ";
    std::getline(std::cin, ip_str);
    ip_str = utils::trim(ip_str);

    std::cout << "Type (pc/server/laptop/phone/iot) [pc]: ";
    std::getline(std::cin, type_str);
    type_str = utils::trim(type_str);
    if (type_str.empty()) type_str = "pc";

    VirtualDevice::Type type = VirtualDevice::Type::PC;
    if (type_str == "server") type = VirtualDevice::Type::SERVER;
    else if (type_str == "laptop") type = VirtualDevice::Type::LAPTOP;
    else if (type_str == "phone") type = VirtualDevice::Type::PHONE;
    else if (type_str == "iot") type = VirtualDevice::Type::IOT;

    auto device = VirtualDevice::create(name, Ipv4Address::from_string(ip_str).value_or(Ipv4Address()), type, service_->network());
    if (!device) {
        std::cout << "Invalid IP or IP not in network range.\n";
        return;
    }

    if (!service_->add_device(*device)) {
        std::cout << "Failed to add device (duplicate IP?).\n";
        return;
    }

    std::cout << "Device added successfully!\n";
}

void Cli::list_devices() {
    auto devices = service_->devices();
    print_devices(devices);
}

void Cli::remove_device() {
    list_devices();
    std::string id;
    std::cout << "Enter device ID to remove: ";
    std::getline(std::cin, id);
    id = utils::trim(id);
    if (id.empty()) return;

    if (service_->remove_device(id)) {
        std::cout << "Device removed.\n";
    } else {
        std::cout << "Device not found.\n";
    }
}

void Cli::handle_nat_config() {
    if (!service_->is_configured()) {
        std::cout << "Network not configured.\n";
        return;
    }

    std::cout << "\nNAT Configuration:\n";
    std::cout << "1. View NAT Table\n";
    std::cout << "2. Clear NAT Table\n";
    std::cout << "3. View Statistics\n";
    std::cout << "4. Back\n";
    std::cout << "Choice: ";

    std::string input;
    std::getline(std::cin, input);
    int choice = 0;
    try { choice = std::stoi(utils::trim(input)); } catch (...) { return; }

    switch (choice) {
        case 1: handle_view_nat_table(); break;
        case 2:
            if (confirm("Clear all NAT mappings?")) {
                service_->clear_nat_table();
                std::cout << "NAT table cleared.\n";
            }
            break;
        case 3:
            print_simulation_stats(service_->metrics());
            break;
        default:
            break;
    }
}

void Cli::handle_view_nat_table() {
    auto entries = service_->nat_entries();
    print_nat_table(entries);
}

void Cli::handle_port_forwarding() {
    if (!service_->is_configured()) {
        std::cout << "Network not configured.\n";
        return;
    }

    while (true) {
        std::cout << "\n--- Port Forwarding ---\n";
        std::cout << "1. Add Rule\n";
        std::cout << "2. List Rules\n";
        std::cout << "3. Delete Rule\n";
        std::cout << "4. Back\n";
        std::cout << "Choice: ";

        std::string input;
        if (!std::getline(std::cin, input)) return;
        int choice = 0;
        try { choice = std::stoi(utils::trim(input)); } catch (...) { continue; }

        switch (choice) {
            case 1: add_port_forward_rule(); break;
            case 2: list_port_forward_rules(); break;
            case 3: delete_port_forward_rule(); break;
            case 4: return;
            default: std::cout << "Invalid choice.\n";
        }
    }
}

void Cli::add_port_forward_rule() {
    if (!service_->is_configured()) {
        std::cout << "Network not configured.\n";
        return;
    }

    std::cout << "\nAdd Port Forwarding Rule\n";
    std::cout << "Protocol (TCP/UDP) [TCP]: ";
    std::string proto_str;
    std::getline(std::cin, proto_str);
    proto_str = utils::trim(utils::to_upper(proto_str));
    if (proto_str.empty()) proto_str = "TCP";

    Protocol proto = Protocol::TCP;
    if (proto_str == "UDP") proto = Protocol::UDP;

    std::string pub_ip_str, priv_ip_str;
    Port pub_port = 0, priv_port = 0;

    std::cout << "Public Port (1-65535): ";
    std::string port_str;
    std::getline(std::cin, port_str);
    try { pub_port = static_cast<Port>(std::stoi(utils::trim(port_str))); } catch (...) {
        std::cout << "Invalid port.\n"; return;
    }

    std::cout << "Private IP: ";
    std::getline(std::cin, priv_ip_str);
    priv_ip_str = utils::trim(priv_ip_str);

    std::cout << "Private Port (1-65535): ";
    std::getline(std::cin, port_str);
    try { priv_port = static_cast<Port>(std::stoi(utils::trim(port_str))); } catch (...) {
        std::cout << "Invalid port.\n"; return;
    }

    auto rule = PortForwardRule::create(proto,
        service_->network().public_ip(), pub_port,
        Ipv4Address::from_string(priv_ip_str).value_or(Ipv4Address()),
        priv_port);

    if (!rule) {
        std::cout << "Invalid parameters.\n";
        return;
    }

    if (!service_->add_port_forward_rule(*rule)) {
        std::cout << "Failed to add rule (conflict or invalid device?).\n";
        return;
    }

    std::cout << "Port forwarding rule added!\n";
}

void Cli::list_port_forward_rules() {
    auto rules = service_->port_forward_rules();
    print_port_forward_rules(rules);
}

void Cli::delete_port_forward_rule() {
    list_port_forward_rules();
    std::string id;
    std::cout << "Enter rule ID to delete: ";
    std::getline(std::cin, id);
    id = utils::trim(id);
    if (id.empty()) return;

    service_->remove_port_forward_rule(id);
    std::cout << "Rule removed.\n";
}

void Cli::handle_simulate_outbound() {
    if (!service_->is_configured()) {
        std::cout << "Network not configured.\n";
        return;
    }

    simulate_packet(PacketDirection::LAN_TO_INTERNET);
}

void Cli::handle_simulate_inbound() {
    if (!service_->is_configured()) {
        std::cout << "Network not configured.\n";
        return;
    }

    simulate_packet(PacketDirection::INTERNET_TO_LAN);
}

void Cli::simulate_packet(PacketDirection direction) {
    std::string proto_str, src_ip_str, dst_ip_str, src_port_str, dst_port_str;

    std::cout << "Protocol (TCP/UDP) [TCP]: ";
    std::getline(std::cin, proto_str);
    proto_str = utils::trim(utils::to_upper(proto_str));
    if (proto_str.empty()) proto_str = "TCP";

    Protocol proto = Protocol::TCP;
    if (proto_str == "UDP") proto = Protocol::UDP;

    std::cout << "Source IP: ";
    std::getline(std::cin, src_ip_str);
    src_ip_str = utils::trim(src_ip_str);

    Port src_port = 0;
    std::cout << "Source Port: ";
    std::string port_str;
    std::getline(std::cin, port_str);
    try { src_port = static_cast<Port>(std::stoi(utils::trim(port_str))); } catch (...) {
        std::cout << "Invalid port.\n"; return;
    }

    std::cout << "Destination IP: ";
    std::getline(std::cin, dst_ip_str);
    dst_ip_str = utils::trim(dst_ip_str);

    Port dst_port = 0;
    std::cout << "Destination Port: ";
    std::getline(std::cin, port_str);
    try { dst_port = static_cast<Port>(std::stoi(utils::trim(port_str))); } catch (...) {
        std::cout << "Invalid port.\n"; return;
    }

    auto src_ip_opt = Ipv4Address::from_string(src_ip_str);
    auto dst_ip_opt = Ipv4Address::from_string(dst_ip_str);
    if (!src_ip_opt || !dst_ip_opt) {
        std::cout << "Invalid IP address.\n";
        return;
    }

    Packet packet(proto, *src_ip_opt, src_port, *dst_ip_opt, dst_port);

    auto result = service_->simulate_packet(packet, direction);
    print_packet_simulation(result);
}

void Cli::handle_view_connections() {
    auto connections = service_->get_connections();
    print_connection_tracker(connections);
}

void Cli::handle_view_driver_stats() {
    if (auto stats = driver_.get_stats()) {
        print_driver_stats(*stats);
    } else {
        std::cout << "Driver not available: " << driver_.last_error() << "\n";
        std::cout << "Run 'scripts/load-driver.sh' to load the kernel module.\n";
    }

    if (auto status = driver_.get_status()) {
        print_driver_status(*status);
    }
}

void Cli::handle_reset() {
    if (confirm("Reset simulation? This will clear all configuration and state.")) {
        service_->reset();
        driver_.reset_stats();
        driver_.reset_nat_table();
        std::cout << "Simulation reset.\n";
    }
}

bool Cli::confirm(const std::string& message) {
    std::cout << message << " (y/N): ";
    std::string input;
    std::getline(std::cin, input);
    return utils::to_lower(utils::trim(input)) == "y";
}

} // namespace vns
