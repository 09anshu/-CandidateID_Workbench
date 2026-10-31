#include "device_manager.h"

namespace workbench {

DeviceManager::DeviceManager() {
    mcu_ = std::make_shared<McuAdapter>();
    fpga_ = std::make_shared<FpgaAdapter>();
    jtag_ = std::make_shared<JtagAdapter>();
}

std::shared_ptr<IDeviceAdapter> DeviceManager::find_device(const std::string& device_id) {
    if (device_id == mcu_->get_device_id() || device_id == "mcu" || device_id == "MCU") return mcu_;
    if (device_id == fpga_->get_device_id() || device_id == "fpga" || device_id == "FPGA") return fpga_;
    if (device_id == jtag_->get_device_id() || device_id == "jtag" || device_id == "JTAG") return jtag_;
    return nullptr;
}

std::vector<DeviceSummary> DeviceManager::discover_devices() {
    std::vector<DeviceSummary> list;

    auto add_summary = [&](const std::shared_ptr<IDeviceAdapter>& dev) {
        DeviceSummary ds;
        ds.device_id = dev->get_device_id();
        ds.device_name = dev->get_device_name();
        ds.device_type = dev->get_device_type();
        ds.status = dev->get_status_string();
        ds.is_connected = dev->is_connected();
        ds.is_debug_interface = dev->get_capabilities().is_debug_interface;
        list.push_back(ds);
    };

    add_summary(mcu_);
    add_summary(fpga_);
    add_summary(jtag_);

    return list;
}

Result<DeviceCapabilities> DeviceManager::get_device_capabilities(const std::string& device_id) {
    auto dev = find_device(device_id);
    if (!dev) {
        return Result<DeviceCapabilities>::fail("Device not found: " + device_id);
    }
    return Result<DeviceCapabilities>::ok(dev->get_capabilities());
}

Result<bool> DeviceManager::connect_device(const std::string& device_id) {
    auto dev = find_device(device_id);
    if (!dev) {
        return Result<bool>::fail("Device not found: " + device_id);
    }
    std::string err;
    bool ok = dev->connect(err);
    if (!ok) {
        return Result<bool>::fail(err);
    }
    return Result<bool>::ok(true);
}

Result<bool> DeviceManager::disconnect_device(const std::string& device_id) {
    auto dev = find_device(device_id);
    if (!dev) {
        return Result<bool>::fail("Device not found: " + device_id);
    }
    dev->disconnect();
    return Result<bool>::ok(true);
}

Result<DeviceTelemetry> DeviceManager::get_telemetry(const std::string& device_id) {
    auto dev = find_device(device_id);
    if (!dev) {
        return Result<DeviceTelemetry>::fail("Device not found: " + device_id);
    }
    return Result<DeviceTelemetry>::ok(dev->read_telemetry());
}

void DeviceManager::reset_all() {
    mcu_->reset_state();
    fpga_->reset_state();
    jtag_->reset_state();
}

} // namespace workbench
