# Embedded System Integration Design Document

> **PASS SCIENCE WORKS — R&D Internship 2026-2027**  
> Task E Deliverable: Modular Host-Connected Workbench Architecture

---

## 1. System Overview & Architecture

The embedded subsystem establishes a modular host-connected engineering workbench integrating three heterogeneous hardware targets:
1. **Target Microcontroller (MCU)**: STMicroelectronics STM32F407VG Discovery (ARM Cortex-M4F)
2. **Target Reconfigurable Logic (FPGA)**: Digilent Arty A7-35T (AMD/Xilinx Artix-7 XC7A35T)
3. **Debug & Trace Probe**: SEGGER J-Link BASE (Hardware Debugger)

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                                     HOST WORKSTATION                                    │
│                     PASS SCIENCE WORKS Unified Engineering Workbench                    │
└───────┬─────────────────────────────────┬─────────────────────────────────────┬─────────┘
        │ USB 2.0 (CDC-ACM VCP)           │ USB 2.0 (FTDI UART Bridge)          │ USB 2.0 High-Speed
        │ 115200 Baud, 8N1                │ 921600 Baud, 8N1                    │ 480 Mbps
┌───────▼─────────────────────────┐┌──────▼─────────────────────────────┐┌──────▼─────────────────────────┐
│     TARGET MCU (STM32F407)      ││     TARGET FPGA (ARTIX-7 A7)       ││    DEBUG PROBE (J-LINK BASE)    │
│  • ARM Cortex-M4F @ 168 MHz     ││  • 33,280 Logic Cells (XC7A35T)    ││  • Hardware In-Circuit Debugger │
│  • 1024 KB Flash, 192 KB SRAM   ││  • 1.0V VCCINT Core Power          ││  • Bus Master for JTAG/SWD TAP  │
│  • 3.3V LVCMOS I/O Domain       ││  • 3.3V LVCMOS I/O Domain          ││  • Auto-Sensing VTref: 1.2-5.0V │
└───────────────▲─────────────────┘└────────────────────────────────────┘└────────────┬────────────────────┘
                │                                                                     │
                │                           ARM 10-pin / 20-pin SWD/JTAG Bus          │
                └─────────────────────────────────────────────────────────────────────┘
                     [ Pin 1: VTref (3.3V sensed by J-Link to set driver levels) ]
                     [ Pin 2: SWDIO / TMS (Bidirectional Data)                   ]
                     [ Pin 4: SWCLK / TCK (Clock driven by J-Link Master)        ]
                     [ Pin 6: SWO / TDO (Trace Data out from Target MCU)         ]
                     [ Pin 7: nRESET (Active-Low Hardware Reset)                ]
                     [ Pin 9: GND (Common Reference)                             ]
```

---

## 2. Comprehensive Interface & Pin-Voltage Table

| Parameter / Signal | STM32F4 Discovery (MCU) | Digilent Arty A7 (FPGA) | SEGGER J-Link BASE (Debug Probe) |
|:---|:---|:---|:---|
| **Primary Chip Model** | STM32F407VGT6 | XC7A35T-1CSG324C | Segger Custom Controller |
| **Manufacturer** | STMicroelectronics | Digilent / AMD Xilinx | SEGGER Microcontroller GmbH |
| **Target Function** | Embedded Application Controller | Hardware Accelerator / Coprocessor | Hardware Debug Bus Master |
| **Host Interface** | Mini-USB (ST-LINK) / Micro-USB (OTG) | Micro-USB (FT2232HQ Bridge) | USB 2.0 Type-B Female |
| **Host Communication** | USB Virtual COM Port (115200 8N1) | FTDI Virtual COM Port (921600 8N1) | WinUSB / J-Link USB Driver |
| **Internal Core Voltage** | 1.20 V (Internal LDO) | **1.00 V (VCCINT)** switching reg | 3.30 V Internal Logic |
| **I/O Voltage Domain** | **3.30 V LVCMOS** (3.0V – 3.6V) | **3.30 V LVCMOS** (Banks 0, 14, 15, 34) | Auto-sensing: **1.20 V – 5.00 V** |
| **5V Tolerance** | Yes (Select 5V-tolerant FT GPIOs) | **NO! (Direct 5V will destroy FPGA)** | Yes (Input buffers up to 5.5 V) |
| **External Connectors** | 2x 25-pin 0.1" Extension Headers | 4x 12-pin Pmod, Arduino Shield Header | Standard 20-pin 0.1" IDC Header |
| **Target Power Draw** | ~50 mA @ 3.3V (165 mW) | ~145 mA @ 1.0V + 45 mA @ 3.3V | Powered from Host USB (~12 mA) |
| **Common Ground** | Shared Host USB & Pin Header GND | Shared Host USB & Pmod GND | Target GND connected to Pin 9/20 |

---

## 3. Signal Ownership & Bus Arbitration

### Architectural Distinction: Why JTAG is NOT an Interchangeable Compute Board
A critical distinction in embedded systems engineering:
- **Compute Boards (MCU & FPGA)**: Act as autonomous computational devices executing user application code or hardware state machines. They possess onboard memories, ALUs, and peripheral controllers.
- **JTAG Debug Adapter (SEGGER J-Link)**: Is a **bus master test interface**, not a computing peer.
  - It does **not** process application workloads.
  - It exercises IEEE 1149.1 boundary-scan or ARM Serial Wire Debug (SWD) protocols.
  - **Signal Ownership**: The JTAG probe is the unconditional master of the test clock (`TCK`/`SWCLK`) and test mode select (`TMS`/`SWDIO`). It drives signals into the target TAP (Test Access Port) state machine to halt CPU execution, read internal core registers, step instructions, and program non-volatile Flash memory.
  - **Passive Sensing**: Pin 1 (`VTref`) of the JTAG connector draws negligible current (<10 µA); it is solely used by the probe's internal level-shifter DACs to track target supply rail variations, guaranteeing matching output drive voltages.

---

## 4. Voltage Compatibility & Safe Level-Shifting Plan

1. **FPGA Protection**:
   - The Artix-7 I/O banks operate at 3.3V LVCMOS. Connecting standard 5.0V TTL/CMOS signals directly to FPGA I/O pins triggers the internal clamp diodes, causing thermal runaway or gate oxide punch-through.
   - **Mitigation**: All external peripheral interfaces running at 5V must pass through bidirectional voltage level translators (such as TI TXS0108E or SN74LVC8T245).
2. **Ground Loops & Common Mode Voltage**:
   - When all three boards are connected to the same host via USB hubs, common ground is maintained through the host USB chassis ground.
   - For isolated test benches, USB galvanic isolators (e.g., ADuM4160) must be interposed between host and target devices to prevent ground loops that corrupt low-level ADC and differential signals.
3. **Power Sequencing**:
   - FPGA core voltage ($V_{\text{CCINT}} = 1.0\text{V}$) must be brought up before or concurrently with auxiliary ($V_{\text{CCAUX}} = 1.8\text{V}$) and I/O ($V_{\text{CCO}} = 3.3\text{V}$) rails to avoid high inrush currents during configuration. The onboard TI regulators on the Arty A7 guarantee proper monotonic ramp times (<10 ms).

