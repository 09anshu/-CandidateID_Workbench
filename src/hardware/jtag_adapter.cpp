#include "jtag_adapter.h"

#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace workbench {

JtagAdapter::JtagAdapter() {
    reset_state();
}

DeviceCapabilities JtagAdapter::get_capabilities() const {
    DeviceCapabilities caps;
    caps.device_name = "SEGGER J-Link BASE";
    caps.model = "J-Link BASE V11";
    caps.manufacturer = "SEGGER Microcontroller GmbH";
    caps.device_type = "JTAG_PROBE";
    caps.communication_interface = "USB 2.0 High Speed (480 Mbps) to Host PC";
    caps.voltage_domain = "Target Reference Voltage VTarget: 1.2V to 5.0V (Auto-sensing)";
    caps.connector = "Standard 20-pin 0.1\" JTAG / SWD male header (Target side); USB-B (Host side)";
    caps.supported_protocols = {"IEEE 1149.1 JTAG", "IEEE 1149.7 cJTAG", "ARM SWD (Serial Wire Debug)", "SWO (Serial Wire Output)"};
    caps.features = {
        "Hardware in-circuit emulator and boundary-scan debug probe",
        "Target interface speed up to 15 MHz JTAG / SWD clock",
        "Target voltage auto-sensing from 1.2 V to 5.0 V with internal DAC references",
        "Automatic target CPU detection (ARM7/9/11, Cortex-M/R/A, RISC-V)",
        "Hardware breakpoint management and direct memory access (AHB-AP)",
        "Zero-power consumption from target (passive voltage monitoring on pin 1)"
    };
    caps.signal_ownership_notes =
        "ARCHITECTURAL DISTINCTION: A JTAG probe is NOT a generic computing device. It is a bus MASTER for hardware-level test access ports (TAPs). "
        "The JTAG probe drives TCK (Test Clock), TMS (Test Mode Select), TDI (Test Data In) and monitors TDO (Test Data Out). "
        "It controls CPU halts, single-stepping, flash programming, and registers extraction. It cannot execute user application code. "
        "Target pin 1 (VTref) is monitored by J-Link to drive output buffers at matching target voltage levels, preventing level mismatch damage.";
    caps.is_debug_interface = true;
    return caps;
}

std::string JtagAdapter::get_status_string() const {
    switch (status_) {
        case DeviceStatus::CONNECTED: return "CONNECTED";
        case DeviceStatus::CONNECTING: return "CONNECTING";
        case DeviceStatus::ERROR_TIMEOUT: return "ERROR_TIMEOUT";
        case DeviceStatus::ERROR_COMMUNICATION: return "ERROR_COMMUNICATION";
        case DeviceStatus::DISCONNECTED:
        default: return "DISCONNECTED";
    }
}

bool JtagAdapter::connect(std::string& error_out) {
    if (fault_injection_) {
        status_ = DeviceStatus::ERROR_TIMEOUT;
        error_out = "JTAG Probe Error: Target power not detected on Pin 1 (VTref = 0.00 V). Check target power supply.";
        return false;
    }

    // Demonstrates the required recoverable timeout failure!
    if (recoverable_timeout_mode_ && !has_failed_once_) {
        has_failed_once_ = true;
        status_ = DeviceStatus::ERROR_TIMEOUT;
        error_out = "Recoverable Failure Demo: SWD connection timed out after 1500 ms waiting for Target ACK [WDATA/RDATA]. Actionable recovery: Click 'Connect' again to retry handshake.";
        return false;
    }

    status_ = DeviceStatus::CONNECTED;
    connect_time_ = std::chrono::steady_clock::now();
    error_out.clear();
    return true;
}

bool JtagAdapter::disconnect() {
    status_ = DeviceStatus::DISCONNECTED;
    return true;
}

DeviceTelemetry JtagAdapter::read_telemetry() {
    DeviceTelemetry telem;
    telem.device_id = get_device_id();
    telem.device_type = get_device_type();
    telem.is_simulated = true;

    auto now = std::chrono::system_clock::now();
    std::time_t t_now = std::chrono::system_clock::to_time_t(now);
    char time_str[30];
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", std::localtime(&t_now));
    telem.timestamp = std::string(time_str);

    if (status_ != DeviceStatus::CONNECTED) {
        telem.status_message = "Device disconnected. Telemetry unavailable.";
        return telem;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - connect_time_).count();
    telem.uptime_seconds = elapsed;

    telem.voltage_v = 3.312 + 0.002 * std::sin(elapsed * 0.2); // Target VRef
    telem.current_ma = 12.4; // Probe draw
    telem.temperature_c = 28.5; // Probe internal temp
    telem.status_message = "Connected to ARM Cortex-M4 Core (IDCODE: 0x4BA00477). Target CPU: RUNNING.";

    telem.metrics = {
        {"Probe Firmware", "J-Link BASE V11 compiled Sep 2026"},
        {"Target Reference Voltage (VTref)", std::to_string(telem.voltage_v).substr(0, 5) + " V"},
        {"Interface Clock (TCK/SWCLK)", "4,000 kHz"},
        {"Target Interface Mode", "SWD (Serial Wire Debug)"},
        {"Target Device IDCODE", "0x4BA00477 (ARM CoreSight SW-DP)"},
        {"CPU Halt State", "RUNNING (No breakpoints hit)"},
        {"Reset Pin (nSRST) State", "DEASSERTED (HIGH, 3.3V)"},
        {"Data Transfer Speed", "824 KB/s"}
    };

    return telem;
}

void JtagAdapter::reset_state() {
    status_ = DeviceStatus::DISCONNECTED;
    fault_injection_ = false;
    has_failed_once_ = false;
}

void JtagAdapter::set_fault_injection(bool enable_timeout) {
    fault_injection_ = enable_timeout;
}

} // namespace workbench
