#include <csignal>
#include <cstdlib>
#include <iostream>

#include "http_server.h"
#include "server_routes.h"
#include "../hardware/device_manager.h"

namespace {
workbench::HttpServer* g_server_ptr = nullptr;

void signal_handler(int sig) {
    std::cout << "\n[Workbench Backend] Caught signal " << sig << ", shutting down gracefully...\n";
    if (g_server_ptr) {
        g_server_ptr->stop();
    }
}
} // anonymous namespace

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) {
        port = std::atoi(argv[1]);
        if (port <= 0 || port > 65535) port = 8080;
    }

    std::cout << "========================================================\n";
    std::cout << " PASS SCIENCE WORKS - Unified Engineering Workbench\n";
    std::cout << " C++17 Computational Backend Service\n";
    std::cout << "========================================================\n";

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    workbench::DeviceManager device_manager;
    workbench::HttpServer server(port);
    g_server_ptr = &server;

    workbench::register_all_routes(server, device_manager);

    std::cout << "[Workbench Backend] Initialized modules: Thermal FEA, Geometry, Multicore, Hardware Mocks, Backpropagation." << std::endl;
    std::cout << "[Workbench Backend] Ready for incoming REST requests." << std::endl;

    if (!server.start()) {
        std::cerr << "[Workbench Backend] Failed to start HTTP server on port " << port << std::endl;
        return 1;
    }

    std::cout << "[Workbench Backend] Server terminated cleanly." << std::endl;
    return 0;
}
