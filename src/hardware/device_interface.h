#pragma once

#include <string>
#include <vector>

namespace workbench {

/// Connection status enum
enum class DeviceStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR_TIMEOUT,
    ERROR_COMMUNICATION
};

/// Hardware telemetry packet
struct DeviceTelemetry {
    std::string timestamp;
    std::string device_id;
    std::string device_type;
    double voltage_v = 0.0;
    double current_ma = 0.0;
    double temperature_c = 0.0;
    long long uptime_seconds = 0;
    std::string status_message;
    std::vector<std::pair<std::string, std::string>> metrics; // key-value pairs
    bool is_simulated = true;
};

/// Device capability report
struct DeviceCapabilities {
    std::string device_name;
    std::string model;
    std::string manufacturer;
    std::string device_type; // "MCU", "FPGA", "JTAG_PROBE"
    std::string communication_interface; // "USB-CDC / VCP", "FTDI USB-UART", "USB 2.0 High-Speed"
    std::string voltage_domain; // "3.3V LVCMOS", "1.0V Core / 3.3V I/O", "1.2V - 5.0V Target Voltage"
    std::string connector; // "Mini-USB / Header", "Micro-USB / PMOD", "20-pin 0.1\" JTAG / 10-pin Cortex"
    std::vector<std::string> supported_protocols;
    std::vector<std::string> features;
    std::string signal_ownership_notes;
    bool is_debug_interface = false;
};

/// Abstract interface for embedded device adapters
class IDeviceAdapter {
public:
    virtual ~IDeviceAdapter() = default;

    virtual std::string get_device_id() const = 0;
    virtual std::string get_device_name() const = 0;
    virtual std::string get_device_type() const = 0;
    virtual DeviceCapabilities get_capabilities() const = 0;

    virtual DeviceStatus get_status() const = 0;
    virtual std::string get_status_string() const = 0;

    virtual bool connect(std::string& error_out) = 0;
    virtual bool disconnect() = 0;
    virtual bool is_connected() const = 0;

    virtual DeviceTelemetry read_telemetry() = 0;

    /// Reset any mock state or fault injection
    virtual void reset_state() = 0;

    /// Configure intentional fault simulation (e.g. timeout on next connect)
    virtual void set_fault_injection(bool enable_timeout) = 0;
};

} // namespace workbench

