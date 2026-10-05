#include "vns/cli/cli.hpp"
#include "vns/utils/logger.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    vns::Logger::instance().set_level(vns::LogLevel::INFO);
    
    try {
        vns::Cli cli;
        return cli.run(argc, argv);
    } catch (const std::exception& e) {
        VNS_LOG_ERROR_FMT("Fatal error: %s", e.what());
        std::cerr << "Fatal error: " << e.what() << "\n";
        return EXIT_FAILURE;
    } catch (...) {
        VNS_LOG_ERROR("Unknown fatal error");
        std::cerr << "Unknown fatal error\n";
        return EXIT_FAILURE;
    }
}