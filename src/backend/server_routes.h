#pragma once

#include "http_server.h"
#include "../hardware/device_manager.h"

namespace workbench {

/// Register all API endpoints with the HTTP server
void register_all_routes(HttpServer& server, DeviceManager& dev_mgr);

} // namespace workbench
