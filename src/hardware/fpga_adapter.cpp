#include "fpga_adapter.h"

#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace workbench {

FpgaAdapter::FpgaAdapter() {
    reset_state();
}

DeviceCapabilities FpgaAdapter::get_capabilities() const {
    DeviceCapabilities caps;
    caps.device_name = "Digilent Arty A7-35T";
    caps.model = "XC7A35T-1CSG324C";
    caps.manufacturer = "Xilinx / AMD";
    caps.device_type = "FPGA";
    caps.communication_interface = "FTDI FT2232HQ USB-UART / JTAG Bridge (921600 Baud)";
    caps.voltage_domain = "1.0V (VCCINT), 1.8V (VCCAUX), 3.3V (VCCO Banks 0, 14, 15, 34)";
    caps.connector = "Micro-USB (UART + JTAG + 5V Power), 4x 12-pin Pmod headers, Arduino/ChipKIT Shield connector";
    caps.supported_protocols = {"AXI4-Lite", "SPI", "UART", "I2C", "JTAG IEEE 1149.1"};
    caps.features = {
        "33,280 Logic Cells (5,200 slices with 4 6-input LUTs and 8 flip-flops)",
        "1,800 Kbits fast block RAM",
        "5 Clock Management Tiles (PLL + MMCM)",
        "90 DSP slices",
        "256 MB DDR3L memory with 16-bit bus @ 667 MHz",
        "On-chip dual 12-bit 1 MSPS analog-to-digital converter (XADC)"
    };
    caps.signal_ownership_notes =
        "FPGA pins are configurable via bitstream constraints (XDC). Default I/O standard is LVCMOS33. Direct 5V connection will destroy FPGA I/O buffers; bidirectional level shifters (e.g. TXS0108E) must be placed on Pmod interfaces when communicating with 5V external hardware.";
    caps.is_debug_interface = false;
    return caps;
}

std::string FpgaAdapter::get_status_string() const {
    switch (status_) {
        case DeviceStatus::CONNECTED: return "CONNECTED";
        case DeviceStatus::CONNECTING: return "CONNECTING";
        case DeviceStatus::ERROR_TIMEOUT: return "ERROR_TIMEOUT";
        case DeviceStatus::ERROR_COMMUNICATION: return "ERROR_COMMUNICATION";
        case DeviceStatus::DISCONNECTED:
        default: return "DISCONNECTED";
    }
}

bool FpgaAdapter::connect(std::string& error_out) {
    if (fault_injection_) {
        status_ = DeviceStatus::ERROR_TIMEOUT;
        error_out = "FPGA connection failed: FTDI channel B handshake error. No response to sync byte 0xAA. (Simulated Fault)";
        return false;
    }

    status_ = DeviceStatus::CONNECTED;
    connect_time_ = std::chrono::steady_clock::now();
    error_out.clear();
    return true;
}

bool FpgaAdapter::disconnect() {
    status_ = DeviceStatus::DISCONNECTED;
    return true;
}

DeviceTelemetry FpgaAdapter::read_telemetry() {
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

    telem.voltage_v = 1.002 + 0.003 * std::sin(elapsed * 0.15); // VCCINT
    telem.current_ma = 142.0 + 15.0 * std::sin(elapsed * 0.3); // Core current
    telem.temperature_c = 41.2 + 0.8 * std::cos(elapsed * 0.08); // XADC die temp
    telem.status_message = "Bitstream loaded: bitstream_thermal_accel_v1.bit. MMCM locked @ 100.000 MHz.";

    telem.metrics = {
        {"Bitstream Name", "thermal_coprocessor_top.bit"},
        {"LUT Utilization", "3,412 / 20,800 (16.4%)"},
        {"FF Utilization", "4,210 / 41,600 (10.1%)"},
        {"BRAM Utilization", "6 / 45 (13.3%)"},
        {"DSP Slice Utilization", "8 / 90 (8.9%)"},
        {"MMCM0 Status", "LOCKED (Clock out: 100.0 MHz, Jitter: 125 ps)"},
        {"VCCINT (Internal Core)", std::to_string(telem.voltage_v).substr(0, 5) + " V"},
        {"VCCAUX (Auxiliary 1.8V)", "1.802 V"},
        {"VCCO_3.3V (I/O Bank)", "3.310 V"},
        {"XADC Die Temperature", std::to_string(telem.temperature_c).substr(0, 5) + " °C"}
    };

    return telem;
}

void FpgaAdapter::reset_state() {
    status_ = DeviceStatus::DISCONNECTED;
    fault_injection_ = false;
}

void FpgaAdapter::set_fault_injection(bool enable_timeout) {
    fault_injection_ = enable_timeout;
}

} // namespace workbench

