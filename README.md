# PASS SCIENCE WORKS — Unified Engineering Workbench

[![Platform](https://img.shields.io/badge/Platform-Windows%2011%20x64-blue.svg)](https://microsoft.com/windows)
[![C++ Standard](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://isocpp.org)
[![Build System](https://img.shields.io/badge/Build-CMake%20%7C%20MinGW-orange.svg)](https://cmake.org)
[![Frontend](https://img.shields.io/badge/Frontend-Next.js%2014%20%7C%20Three.js-black.svg)](https://nextjs.org)
[![Tests](https://img.shields.io/badge/Tests-43%2F43%20Passed-brightgreen.svg)](tests/)

---

## 1. Project Overview

The **Unified Engineering Workbench** is a high-performance desktop engineering application developed for the **PASS SCIENCE WORKS R&D Internship 2026–2027**. It unifies scientific computing, mechanical CAD visualization, thermal simulation, parallel performance benchmarks, embedded instrumentation, and machine learning into a single cohesive workbench.

### Primary Capabilities:
1. **CAD & ANSYS Visualization (Task B)**: 3D ASCII STL mesh rendering with orbit controls and 3D ANSYS nodal temperature point clouds with interactive Turbo colormap.
2. **1D Thermal FEA Solver (Task C)**: Linear two-node finite element solver for steady-state Poisson heat conduction, Thomas $O(N)$ tridiagonal algorithm, and rigorous $O(h^2)$ midpoint convergence verification.
3. **Multicore Worker Pool (Task D)**: Bounded `std::thread` pool with atomic work stealing, deterministic case ordering, 5-repetition median timing, and serial/parallel agreement verification ($\le 10^{-8\ \circ}\text{C}$).
4. **Embedded Instrumentation (Task E)**: Architecture and C++ mock adapters for MCU (STM32F4), FPGA (Artix-7), and JTAG debug probe (SEGGER J-Link) with recoverable timeout recovery.
5. **Neural Backpropagation (Task F)**: Analytical handwritten chain-rule derivatives for single-neuron regression, verified against central finite differences ($\varepsilon=10^{-6}$, error $< 3\times 10^{-10}$), and 100-step training loop.

---

## 2. Prerequisites

### For End Users (One-Click Launch):
- **Operating System**: Windows 11 x64 (or Windows 10 x64)
- **Privileges**: Standard user permissions (**No Administrator rights required**)
- **Internet**: **Zero network access required** (works 100% offline)
- **Dependencies**: **None** (all required C++ runtime DLLs and pre-built static UI assets are bundled)

### For Developers (Building from Source):
- MinGW-w64 GCC 13+ (or MSYS2 MinGW64 GCC 16.2.0) supporting C++17
- CMake $\ge 3.16$
- Node.js $\ge 18$ & npm (only required if recompiling the frontend)
- Python 3.9+ (optional, for plot/PDF generation scripts)

---

## 3. One-Click Launch Instructions

1. Extract `CandidateID_Workbench.zip` to any local directory.
2. Double-click **`Launch.exe`** in the extracted root folder.
3. **Execution Behavior**:
   - The native Win32 launcher silently starts `workbench_backend.exe` on `localhost:8080`.
   - It polls the local loopback socket until the REST server is ready.
   - It automatically opens your default web browser to `http://localhost:8080`.
   - **Zero console windows, zero PowerShell prompts, and zero background installers.**

---

## 4. System Architecture

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

- **Separation of Concerns**: The C++ computational engine is strictly decoupled from presentation. All engineering calculations occur in C++; the frontend is purely display and interaction.
- **Transport**: Standard HTTP REST with structured JSON bodies. Local socket communication on `127.0.0.1:8080`.

---

## 5. Supported File Formats

| Format | File Extension | Expected Schema / Specification | Provenance & Units |
| :--- | :--- | :--- | :--- |
| **ASCII STL** | `.stl` | Standard ASCII StereoLithography (`solid ... endsolid`) containing triangular facets with outward normal vectors and 3-vertex loops. Binary STL is detected and rejected. | Example: `data/geometry/sample_cube.stl` (12-facet cantilever cube bracket). Units: millimeters (mm). Openly licensed CC0/Public Domain. |
| **ANSYS CSV** | `.csv` | 5 required columns: `node_id,x_m,y_m,z_m,temperature_C`. Rejects non-finite values (NaN, Inf), empty files, or missing columns. | Example: `data/thermal/sample_temperature.csv` (8-node grid, $20.0^\circ\text{C}$ to $125.0^\circ\text{C}$). Units: meters (m) and $^\circ\text{C}$. Clearly labelled **SYNTHETIC**. |

---

## 6. Build Instructions (From Source)

### 6.1 Compiling C++ Backend & Automated Test Suite
From the repository root:
```powershell
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```
This produces `workbench_backend.exe`, `workbench_tests.exe`, and `Launch.exe`.

### 6.2 Compiling Next.js Frontend
```powershell
cd src/frontend
npm install
npm run build
```
The static HTML/JS/CSS output is generated into `src/frontend/out/` and copied to `ui/`.

---

## 7. Test Instructions

### 7.1 Running Automated C++ Test Suite
The test suite requires zero external testing libraries (Google Test/Catch2 not needed):
```powershell
# From root directory:
.\workbench_tests.exe
```
**Test Coverage (43 Automated Tests)**:
- Linear algebra: Tridiagonal Thomas algorithm exact solution, zero-pivot detection.
- Thermal FEA: Dirichlet boundary conditions, nodal superconvergence, midpoint quadratic convergence ($N=10, 20, 40, 80$, $p=2.0000$), micro/macro parameter stability, negative parameter rejection.
- Geometry parsers: Valid ASCII STL parsing, empty STL rejection, binary STL rejection, malformed facet loop rejection, ANSYS CSV 5-column validation, NaN/Inf rejection.
- Multicore parallel execution: Deterministic batch workload, serial vs. parallel agreement ($0.0^\circ\text{C} \le 10^{-8\ \circ}\text{C}$), fixed-seed reproducibility.
- Embedded hardware mocks: Lifecycle discovery, MCU/FPGA telemetry, JTAG recoverable timeout on attempt 1, connection recovery on retry.
- Neural backpropagation: Forward pass, analytical chain rule vs. central finite differences ($\varepsilon=10^{-6}$, error $< 3\times 10^{-10}$), 100-step training loss reduction.

**Expected Output**:
```text
========================================================
 TEST SUMMARY: 43 / 43 PASSED (100% Pass Rate)
========================================================
```

---

## 8. Known Limitations & Attribution

1. **Geometry Scope**: Only ASCII STL geometry is supported. Binary STL and native proprietary CAD solid formats (STEP, IGES, Parasolid) are outside the prototype scope.
2. **Thermal Dimensionality**: The FEA solver is formulated for 1D steady-state conduction. 2D planar and 3D volumetric solid FEA meshes are planned for future iterations.
3. **Hardware Simulation**: The embedded instrumentation adapters (STM32F4, Arty A7, SEGGER J-Link) are software mock adapters simulating bus handshakes, register reads, and telemetry. No physical USB connection is established.
4. **Licensing & Dependencies**:
   - `nlohmann/json`: MIT License.
   - `Three.js`: MIT License.
   - `Lucide Icons`: ISC License.
   - All numerical solvers, parsers, and application code are original work. See `docs/ownership_statement.md`.
