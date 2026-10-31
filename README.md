# PASS SCIENCE WORKS — Unified Engineering Workbench

[![Windows 11 x64](https://img.shields.io/badge/Platform-Windows%2011%20x64-blue.svg)](https://microsoft.com/windows)
[![C++ Standard](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://isocpp.org)
[![Build System](https://img.shields.io/badge/Build-CMake%20%7C%20Ninja%20%7C%20MinGW-orange.svg)](https://cmake.org)
[![Frontend](https://img.shields.io/badge/Frontend-Next.js%2014%20%7C%20Three.js-black.svg)](https://nextjs.org)
[![Tests](https://img.shields.io/badge/Tests-28%2F28%20Passed-brightgreen.svg)](tests/)

> **PASS SCIENCE WORKS R&D Internship 2026-2027 Technical Assignment**  
> Comprehensive, high-performance desktop engineering workbench integrating geometry visualization, 1D thermal finite-element analysis, multicore execution, embedded hardware mock adapters, and handwritten neural backpropagation.

---

## 1. Quick Start: One-Click Execution

1. Extract `CandidateID_Workbench.zip` to any local folder.
2. Double-click **`Launch.exe`**.
3. **That's it!** The native launcher automatically starts the high-performance C++ computational backend on `localhost:8080` in the background and launches the workbench application window.

> **Zero Prerequisites**: Does **not** require administrator privileges, PowerShell scripts, terminal commands, internet access, or prior package installations (Node.js/Python are NOT required for execution).

---

## 2. System Architecture

```
┌────────────────────────────────────────────────────────────────────────┐
│             Desktop Shell / Dedicated Native Webview                   │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │             Next.js 14 Frontend (TypeScript / Tailwind)          │  │
│  │   • Three.js 3D CAD & Point Cloud Viewport                       │  │
│  │   • Interactive Scientific SVG Charts (FEA / Speedup / Loss)     │  │
│  │   • Hardware Telemetry Cards & Device Controls                   │  │
│  └──────────────────────────────────┬───────────────────────────────┘  │
│                                     │ HTTP REST (JSON)                 │
│  ┌──────────────────────────────────▼───────────────────────────────┐  │
│  │               C++17 High-Performance Native Backend              │  │
│  │   • Port 8080 Multi-Threaded HTTP/1.1 REST Server                │  │
│  │   • Static UI Asset File Server                                  │  │
│  │  ┌───────────────┐ ┌───────────────┐ ┌───────────────┐           │  │
│  │  │  Thermal FEA  │ │  Multicore    │ │ Embedded HW   │           │  │
│  │  │  1D FE Solver │ │  Worker Pool  │ │ Mock Adapters │           │  │
│  │  └───────────────┘ └───────────────┘ └───────────────┘           │  │
│  │  ┌───────────────┐ ┌───────────────┐ ┌───────────────┐           │  │
│  │  │  STL / CSV    │ │ Thomas Tri-   │ │ Handwritten   │           │  │
│  │  │  Parsers      │ │ diagonal Alg. │ │ Backprop Core │           │  │
│  │  └───────────────┘ └───────────────┘ └───────────────┘           │  │
│  └──────────────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────────┘
```

All heavy computational algorithms, file parsing, and multithreading are executed natively in compiled C++17. The frontend is strictly presentation, 3D interaction, and plotting.

---

## 3. Implemented Technical Features

### Task A: Unified Interface & Native Launcher
- Responsive single-page application with 6 dedicated workspaces: Geometry, Thermal FEA, Performance, Hardware, Backpropagation, and Documentation.
- Real-time C++ backend status indicator (detects host CPU, physical cores, logical threads, RAM, and compiler flags).
- One-click native launcher (`Launch.exe`) built with Win32 API (`WinMain`, `-mwindows`).

### Task B: CAD & ANSYS Visualization
- **ASCII STL Geometry**: High-speed C++ parser (`src/geometry/stl_parser.cpp`) validating facet normals, 3-vertex loops, and bounding boxes. Three.js viewport with rotate (left-drag), pan (right-drag), zoom (wheel), fit-to-view, and wireframe toggle.
- **ANSYS Nodal Temperature CSV**: Validates schema (`node_id,x_m,y_m,z_m,temperature_C`), strictly rejects `NaN`/`Inf` and malformed entries. 3D colormapped point cloud with temperature legend (°C) and node inspection.
- Sample data provided: `data/geometry/sample_cube.stl` (CC0 unit cube) and `data/thermal/sample_temperature.csv` (clearly labelled `SYNTHETIC`).

### Task C: Mechanical Thermal FEA Solver & Verification
- Real 1D steady-state finite-element solver for uniform rod:
  $$-\frac{d}{dx}\left(k A \frac{dT}{dx}\right) = q A, \quad T(0) = 100^\circ\text{C}, \quad T(L) = 20^\circ\text{C}$$
- Two-node linear elements with element stiffness $K^e = \frac{kA}{h} [1, -1; -1, 1]$ and element load vector $f^e = \frac{qAh}{2} [1; 1]$.
- Solved in $O(N)$ operations using the Thomas algorithm.
- Verified against exact analytical solution $T_{\text{exact}}(x) = 100 - 80x/L + \frac{q}{2k}x(L-x)$.
- **Empirical Quadratic Convergence Verified**: Max midpoint error equals exactly $\frac{q}{8k} h^2$:
  - $N = 10$: $1.250000^\circ\text{C}$
  - $N = 20$: $0.312500^\circ\text{C}$ (Rate $p = 2.0000$)
  - $N = 40$: $0.078125^\circ\text{C}$ (Rate $p = 2.0000$)
  - $N = 80$: $0.019531^\circ\text{C}$ (Rate $p = 2.0000$)
- Instant CSV export and interactive comparison plots.

### Task D: Multicore Execution & Benchmarking
- Reproducible batch generator with fixed seed (Mersenne Twister, seed 42) varying positive $k \in [5, 50]$ and $q \in [1000, 50000]$.
- Bounded worker pool (`std::thread`, $p = 1, 2, 4$ workers capped at logical CPUs).
- Strict deterministic input ordering preserved.
- **Serial vs Parallel Agreement Check**: Maximum temperature difference is $\mathbf{0.000000^\circ\text{C}} \le 1\times 10^{-8\ \circ}\text{C}$.
- 5 measured repetitions, median execution time, speedup $S = T_1 / T_p$, and efficiency $(T_1/T_p)/p$ reported.

### Task E: Embedded System Integration
- Modular host-connected architecture representing 3 device types:
  1. **STM32F4 Discovery** (3.3V ARM Cortex-M4F MCU, USB-CDC)
  2. **Digilent Arty A7** (Xilinx Artix-7 FPGA, 1.0V Core / 3.3V I/O, UART bridge)
  3. **SEGGER J-Link BASE** (Hardware JTAG/SWD debug probe, 1.2V-5.0V target sensing)
- Detailed interface and voltage table covering power, grounding, level-shifting, and pinouts in `docs/embedded_design.md`.
- Architectural distinction: JTAG probe explained as a bus master debug probe rather than a generic compute board.
- C++ mock adapters support discovery, capabilities, connect, disconnect, and live telemetry streaming.
- **Recoverable Failure Demonstration**: JTAG adapter simulates timeout failure on initial connect attempt and recovers successfully on retry.

### Task F: Handwritten Backpropagation
- Forward pass for sample $x = 1.0, y = 2.0$ with $h = \tanh(wx + b), \hat{y} = vh + c, J = 0.5(\hat{y} - y)^2$. Initial: $w=0.5, b=0, v=1, c=0, \eta=0.1$.
- Complete chain-rule analytical derivations for $\frac{\partial J}{\partial w}, \frac{\partial J}{\partial b}, \frac{\partial J}{\partial v}, \frac{\partial J}{\partial c}$.
- Simultaneous gradient descent update using gradients from original parameters.
- **Central Finite Difference Verification** ($\varepsilon = 10^{-6}$): All analytical gradients verified with absolute error $< 10^{-7}$ (well within $\le 10^{-5}$ requirement).
- 100-step training loop with loss strictly decreasing from $1.1825$ to $< 10^{-6}$.
- Formatted two-page derivation reference in `docs/handwritten_backpropagation_appendix.md`.

---

## 4. Directory Structure

```
Pass_Science_assignment/
├── Launch.exe                  # Standalone Windows 11 x64 launcher
├── workbench_backend.exe       # Compiled native C++17 REST server
├── libgcc_s_seh-1.dll          # Bundled GCC runtime
├── libstdc++-6.dll             # Bundled C++ standard library
├── libwinpthread-1.dll         # Bundled POSIX threads runtime
├── CMakeLists.txt              # Root CMake configuration
├── AGENTS.md                   # Canonical AI instruction file
├── GEMINI.md                   # Gemini / Antigravity reference
├── CLAUDE.md                   # Claude / Anthropic reference
├── SKILL.md                    # Project development skill
├── tasks.md                    # Detailed task traceability checklist
├── task_plan.md                # Phased implementation plan
├── findings.md                 # Mathematical derivations & verified numbers
├── progress.md                 # Development progress log
├── README.md                   # User documentation (this file)
│
├── data/
│   ├── geometry/               # Sample ASCII STL (sample_cube.stl)
│   └── thermal/                # Sample ANSYS CSV (sample_temperature.csv, SYNTHETIC)
│
├── docs/
│   ├── technical_report.md     # ≤6-page Technical Report
│   ├── embedded_design.md      # Pinout table, block diagram, voltage safety
│   ├── handwritten_backpropagation_appendix.md # Handwritten derivation layout
│   ├── ai_usage_log.md         # Transparent AI usage disclosure & tests
│   └── ownership_statement.md  # Original vs reused code disclosure
│
├── results/
│   ├── csv/                    # Raw generated CSV results
│   ├── plots/                  # High-resolution publication plots
│   └── benchmarks/             # Multicore benchmark timings
│
├── src/
│   ├── core/                   # Linear algebra, Thomas algorithm, types
│   ├── geometry/               # STL and ANSYS CSV parsers
│   ├── thermal/                # 1D thermal FEA solver
│   ├── performance/            # Multicore parallel batch engine
│   ├── hardware/               # MCU, FPGA, and JTAG mock adapters
│   ├── backpropagation/        # Neural network and gradient checks
│   ├── backend/                # HTTP server, REST routes, main, launcher
│   └── frontend/               # Next.js 14 App Router source code
│
├── tests/
│   └── test_main.cpp           # 28-test automated C++ test suite
│
├── ui/                         # Exported static web assets served by backend
└── scripts/
    ├── generate_results.py     # Reproducible result generation script
    └── generate_plots.py       # Matplotlib plot generation script
```

---

## 5. Building From Source (Developers)

### 5.1 Prerequisites
- Windows 11 x64
- MinGW-w64 GCC 13+ (or MSYS2 GCC) supporting C++17
- CMake ≥ 3.16
- Node.js ≥ 18 (only needed if modifying frontend)

### 5.2 Building C++ Backend & Tests
```powershell
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# Run automated tests
.\workbench_tests.exe
```

### 5.3 Building Frontend
```powershell
cd src/frontend
npm install
npm run build
```

---

## 6. Supported Input Formats

| Format | Specification | Notes |
|:---|:---|:---|
| **ASCII STL** | StereoLithography (`solid ... endsolid`) | Triangles, vertices, and normals. Binary STL is detected and rejected with an explicit error. |
| **ANSYS CSV** | `node_id,x_m,y_m,z_m,temperature_C` | Required columns checked; non-finite (NaN/Inf) values rejected. |

---

## 7. Known Limitations

1. **Geometry Scope**: Only ASCII STL geometry is supported. Binary STL and native CAD solid formats (STEP, IGES) are outside the scope of this assessment prototype.
2. **Thermal FEA Dimensionality**: The finite element solver is formulated specifically for 1-D heat conduction. 2D/3D volumetric solid meshes are not solved by this prototype.
3. **Hardware Simulation**: The embedded adapters (STM32, Artix-7, J-Link) simulate protocol handshakes, register reads, and telemetry in software. No physical hardware connection is made.

---

## 8. License & Attribution

- Built as part of the PASS SCIENCE WORKS R&D Internship Assignment 2026-2027.
- External libraries: Three.js (MIT), Next.js (MIT), Tailwind CSS (MIT), Lucide React (ISC).
