#include "mcu_adapter.h"

#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace workbench {

McuAdapter::McuAdapter() {
    reset_state();
}

DeviceCapabilities McuAdapter::get_capabilities() const {
    DeviceCapabilities caps;
    caps.device_name = "STM32F4 Discovery";
    caps.model = "STM32F407VGT6";
    caps.manufacturer = "STMicroelectronics";
    caps.device_type = "MCU";
    caps.communication_interface = "USB-CDC (Virtual COM Port, 115200 8N1)";
    caps.voltage_domain = "3.3V I/O, 5.0V USB VBUS In, GND Common";
    caps.connector = "Mini-USB (ST-LINK / Power), Micro-USB (OTG FS), 2x 25-pin Expansion Headers";
    caps.supported_protocols = {"UART/USART", "SPI", "I2C", "CAN 2.0B", "USB OTG", "SWD"};
    caps.features = {
        "ARM Cortex-M4F 32-bit RISC core @ 168 MHz",
        "1024 KB Flash, 192 KB SRAM",
        "Hardware Floating Point Unit (FPU)",
        "Onboard LIS302DL/LIS3DSH 3-axis MEMS accelerometer",
        "Onboard CS43L22 audio DAC with class D speaker driver",
        "4 user LEDs (Orange, Green, Red, Blue) + 1 user pushbutton"
    };
    caps.signal_ownership_notes =
        "Host acts as USB master. Target board acts as USB CDC device. GPIOs are 3.3V CMOS compatible (5V tolerant pins available). Level shifter required if interfacing with 1.8V or 5.0V non-tolerant systems.";
    caps.is_debug_interface = false;
    return caps;
}

std::string McuAdapter::get_status_string() const {
    switch (status_) {
        case DeviceStatus::CONNECTED: return "CONNECTED";
        case DeviceStatus::CONNECTING: return "CONNECTING";
        case DeviceStatus::ERROR_TIMEOUT: return "ERROR_TIMEOUT";
        case DeviceStatus::ERROR_COMMUNICATION: return "ERROR_COMMUNICATION";
        case DeviceStatus::DISCONNECTED:
        default: return "DISCONNECTED";
    }
}

bool McuAdapter::connect(std::string& error_out) {
    if (fault_injection_) {
        status_ = DeviceStatus::ERROR_TIMEOUT;
        error_out = "MCU connection failed: Handshake timeout on COM4 after 2000 ms. (Simulated Fault)";
        return false;
    }

    status_ = DeviceStatus::CONNECTED;
    connect_time_ = std::chrono::steady_clock::now();
    error_out.clear();
    return true;
}

bool McuAdapter::disconnect() {
    status_ = DeviceStatus::DISCONNECTED;
    return true;
}

DeviceTelemetry McuAdapter::read_telemetry() {
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

    // Realistic simulated values
    telem.voltage_v = 3.295 + 0.01 * std::sin(elapsed * 0.1);
    telem.current_ma = 48.2 + 2.5 * std::cos(elapsed * 0.2);
    telem.temperature_c = 34.5 + 0.5 * std::sin(elapsed * 0.05);
    telem.status_message = "Nominal operating status. RTOS scheduler active.";

    telem.metrics = {
        {"Core Frequency", "168.0 MHz"},
        {"Supply Voltage VDD", "3.30 V"},
        {"Internal Die Temp", std::to_string(telem.temperature_c).substr(0, 5) + " °C"},
        {"Free Heap", "148,224 bytes / 196,608 bytes"},
        {"ADC Channel 1 (Potentiometer)", "2,048 (1.65 V)"},
        {"LED State", (elapsed % 2 == 0) ? "LD4 GREEN ON" : "LD3 ORANGE ON"},
        {"Packets Received / Sent", std::to_string(elapsed * 10) + " / " + std::to_string(elapsed * 10)}
    };

    return telem;
}

void McuAdapter::reset_state() {
    status_ = DeviceStatus::DISCONNECTED;
    fault_injection_ = false;
}

void McuAdapter::set_fault_injection(bool enable_timeout) {
    fault_injection_ = enable_timeout;
}

} // namespace workbench
