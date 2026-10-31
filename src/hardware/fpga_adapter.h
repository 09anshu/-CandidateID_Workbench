#pragma once

#include "device_interface.h"

#include <chrono>

namespace workbench {

class FpgaAdapter : public IDeviceAdapter {
public:
    FpgaAdapter();
    ~FpgaAdapter() override = default;

    std::string get_device_id() const override { return "FPGA-ARTIX7-02"; }
    std::string get_device_name() const override { return "Digilent Arty A7 (Xilinx Artix-7 XC7A35T)"; }
    std::string get_device_type() const override { return "FPGA"; }
    DeviceCapabilities get_capabilities() const override;

    DeviceStatus get_status() const override { return status_; }
    std::string get_status_string() const override;

    bool connect(std::string& error_out) override;
    bool disconnect() override;
    bool is_connected() const override { return status_ == DeviceStatus::CONNECTED; }

    DeviceTelemetry read_telemetry() override;

    void reset_state() override;
    void set_fault_injection(bool enable_timeout) override;

private:
    DeviceStatus status_ = DeviceStatus::DISCONNECTED;
    bool fault_injection_ = false;
    std::chrono::steady_clock::time_point connect_time_;
};

} // namespace workbench
