#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../core/types.h"
#include "device_interface.h"
#include "fpga_adapter.h"
#include "jtag_adapter.h"
#include "mcu_adapter.h"

namespace workbench {

/// Summary of a discovered device for UI listing
struct DeviceSummary {
    std::string device_id;
    std::string device_name;
    std::string device_type;
    std::string status;
    bool is_connected = false;
    bool is_debug_interface = false;
};

class DeviceManager {
public:
    DeviceManager();
    ~DeviceManager() = default;

    /// Discover and list all supported embedded devices
    std::vector<DeviceSummary> discover_devices();

    /// Get detailed capabilities of a device
    Result<DeviceCapabilities> get_device_capabilities(const std::string& device_id);

    /// Connect to a device by device_id
    Result<bool> connect_device(const std::string& device_id);

    /// Disconnect from a device
    Result<bool> disconnect_device(const std::string& device_id);

    /// Read telemetry from a device
    Result<DeviceTelemetry> get_telemetry(const std::string& device_id);

    /// Reset all devices to initial state
    void reset_all();

private:
    std::shared_ptr<McuAdapter> mcu_;
    std::shared_ptr<FpgaAdapter> fpga_;
    std::shared_ptr<JtagAdapter> jtag_;

    std::shared_ptr<IDeviceAdapter> find_device(const std::string& device_id);
};

} // namespace workbench
