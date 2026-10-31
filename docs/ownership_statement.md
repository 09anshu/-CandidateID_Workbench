# Candidate Ownership Statement

> **PASS SCIENCE WORKS — R&D Internship 2026-2027**  
> Technical Assignment Ownership & Attribution Declaration

---

## 1. Candidate Declaration

I hereby declare that the work submitted in this repository and archive (`CandidateID_Workbench.zip`) represents my authentic engineering implementation and technical analysis. All sources of code, mathematical derivations, external libraries, and artificial intelligence tools have been transparently disclosed below in full compliance with the submission brief.

---

## 2. Categorization of Work

### 2.1 Original Work (Authored for this Project)
- **1-D Thermal FEA Core**: Formulation, stiffness matrix assembly, tridiagonal Thomas algorithm linear solver, and midpoint error evaluation (`src/thermal/thermal_solver.cpp`, `src/core/linear_algebra.cpp`).
- **Geometry Parsers**: ASCII STL parser and ANSYS CSV schema validator with error diagnostics (`src/geometry/stl_parser.cpp`, `src/geometry/csv_parser.cpp`).
- **Multicore Parallel Engine**: Bounded worker pool implementation using `std::thread` and atomic dynamic indexing (`src/performance/parallel_solver.cpp`).
- **Embedded Mock Adapters**: Device state machines, capability descriptors, telemetry generators, and recoverable timeout demonstration for STM32, Artix-7, and J-Link (`src/hardware/`).
- **Handwritten Backpropagation Core**: Forward pass, analytical chain-rule gradients, simultaneous update, and central finite difference verification (`src/backpropagation/backprop.cpp`).
- **Native HTTP/1.1 REST Server & Win32 Launcher**: Winsock2 HTTP server with CORS support and Win32 `Launch.exe` desktop launcher (`src/backend/`).
- **Documentation & Technical Report**: Technical report, embedded system design document, AI usage log, and test suites.

### 2.2 Reused Libraries & Open-Source Components

| Component | License | Purpose | Source / Attribution |
|:---|:---|:---|:---|
| **Next.js 14** | MIT | React frontend application framework | Vercel Inc. |
| **React 18** | MIT | UI component rendering library | Meta Platforms Inc. |
| **Three.js** | MIT | 3D WebGL rendering engine | Ricardo Cabello (Mr.doob) |
| **Tailwind CSS** | MIT | Utility-first CSS styling framework | Tailwind Labs Inc. |
| **Lucide React** | ISC | UI vector iconography | Lucide Contributors |
| **Matplotlib** | BSD-compatible | Scientific plot generation script | John D. Hunter et al. |
| **MSYS2 MinGW-w64** | Public Domain / GPL | C++17 Windows compiler toolchain | MinGW-w64 Project |

### 2.3 Synthetic Test Data
- `data/geometry/sample_cube.stl`: Self-created 12-triangle unit cube ASCII STL (Public Domain / CC0).
- `data/thermal/sample_temperature.csv`: Generated numerically using the exact thermal equation $T(x) = 100 - 80x + 500x(1-x)$ across 51 nodes. Clearly marked `SYNTHETIC` and not represented as proprietary ANSYS output.

---

## 3. License Agreement
The custom source code authored for this submission is licensed under the MIT License for evaluation and review by the PASS SCIENCE WORKS R&D Internship Committee.

