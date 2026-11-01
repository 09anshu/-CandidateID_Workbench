#pragma once

#include "device_interface.h"

#include <chrono>

namespace workbench {

class JtagAdapter : public IDeviceAdapter {
public:
    JtagAdapter();
    ~JtagAdapter() override = default;

    std::string get_device_id() const override { return "JTAG-JLINK-03"; }
    std::string get_device_name() const override { return "SEGGER J-Link BASE (Hardware Debug Probe)"; }
    std::string get_device_type() const override { return "JTAG Debug Probe"; }
    DeviceCapabilities get_capabilities() const override;

    DeviceStatus get_status() const override { return status_; }
    std::string get_status_string() const override;

    bool connect(std::string& error_out) override;
    bool disconnect() override;
    bool is_connected() const override { return status_ == DeviceStatus::CONNECTED; }

    DeviceTelemetry read_telemetry() override;

    void reset_state() override;
    void set_fault_injection(bool enable_timeout) override;

    /// Enable recoverable timeout simulation: attempt 1 fails with timeout, attempt 2 succeeds!
    void set_recoverable_timeout_mode(bool enable) {
        recoverable_timeout_mode_ = enable;
        has_failed_once_ = false;
    }

private:
    DeviceStatus status_ = DeviceStatus::DISCONNECTED;
    bool fault_injection_ = false;
    bool recoverable_timeout_mode_ = true; // Enabled by default to showcase requirement
    bool has_failed_once_ = false;
    std::chrono::steady_clock::time_point connect_time_;
};

} // namespace workbench

