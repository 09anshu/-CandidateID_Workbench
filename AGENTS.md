# AGENTS.md — PASS SCIENCE WORKS Unified Engineering Workbench

> **Canonical project instruction file.** All AI models and contributors should read
> this first. Model-specific files (GEMINI.md, CLAUDE.md) reference this document.

---

## 1  Project Overview

| Field | Value |
|-------|-------|
| **Assignment** | PASS SCIENCE WORKS R&D Internship 2026-2027 |
| **Title** | Unified Engineering Workbench |
| **Points** | 100 (see Evaluation Rubric below) |
| **Deadline** | 15 calendar days from start |
| **Delivery** | `CandidateID_Workbench.zip` — one-click `Launch.exe` on Windows 11 x64 |

### What It Does

A small, reliable engineering workbench that:

1. **Imports geometry** — ASCII STL files and ANSYS-format CSV temperature data.
2. **Solves a 1D thermal finite-element problem** — steady-state, two-node linear elements, Dirichlet BCs.
3. **Visualizes results** — 3D STL viewer, temperature point clouds, FEA plots, convergence charts.
4. **Runs parallel workloads** — bounded worker-pool batch solves with reproducible results.
5. **Represents embedded hardware** — mock adapters for MCU, FPGA, JTAG with discovery/connect/telemetry.
6. **Implements handwritten backpropagation** — forward pass, chain-rule gradients, numerical verification.

---

## 2  Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│                     Electron Shell (Desktop)                     │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │              Next.js Frontend (TypeScript)                 │  │
│  │   Tailwind CSS · shadcn/ui · Three.js/R3F · Plotly.js     │  │
│  └──────────────────────┬─────────────────────────────────────┘  │
│                         │ HTTP REST (JSON)                       │
│  ┌──────────────────────▼─────────────────────────────────────┐  │
│  │              C++17 Backend HTTP Server                      │  │
│  │   cpp-httplib · nlohmann/json · Eigen · spdlog             │  │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────┐  │  │
│  │  │ Geometry  │ │ Thermal  │ │Parallel  │ │  Hardware     │  │  │
│  │  │ Parsers   │ │ FEA      │ │Batch     │ │  Mock Adapters│  │  │
│  │  └──────────┘ └──────────┘ └──────────┘ └──────────────┘  │  │
│  │  ┌──────────────────┐                                      │  │
│  │  │ Backpropagation  │                                      │  │
│  │  └──────────────────┘                                      │  │
│  └────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────┘
```

### Communication

- **Frontend → Backend**: HTTP REST calls (JSON request/response bodies).
- **Backend binds** to `localhost:8080` (configurable).
- All heavy computation happens in C++. The frontend is purely for display and interaction.

---

## 3  Tech Stack

### 3.1  C++ Backend

| Library | Purpose | Version/Notes |
|---------|---------|---------------|
| **C++17** | Language standard | `-std=c++17` |
| **cpp-httplib** | Header-only HTTP server | Single header `httplib.h` |
| **nlohmann/json** | JSON serialization | Single header `json.hpp` |
| **Eigen** | Linear algebra (FEA solver) | Header-only |
| **spdlog** | Structured logging | Header-only |
| **CMake** | Build system | ≥ 3.16 |

### 3.2  Frontend

| Library | Purpose |
|---------|---------|
| **Next.js 14+** | React framework (App Router) |
| **TypeScript** | Type-safe JavaScript |
| **Tailwind CSS** | Utility-first styling |
| **shadcn/ui** | Accessible UI components |
| **Three.js / React Three Fiber** | 3D STL/point-cloud rendering |
| **Plotly.js** | 2D scientific charts |

### 3.3  Packaging

| Tool | Purpose |
|------|---------|
| **Electron** | Desktop application shell |
| **electron-builder** | Packaging to `.exe` / `.zip` |

---

## 4  Directory Structure

```
Pass_Science_assignment/
├── AGENTS.md                   # ← You are here
├── GEMINI.md                   # Gemini/Antigravity-specific instructions
├── CLAUDE.md                   # Claude-specific instructions
├── SKILL.md                    # AI skill descriptor
├── README.md                   # User-facing project README
├── tasks.md                    # Full task checklist
├── task_plan.md                # Implementation phases and strategy
├── findings.md                 # Architecture decisions & math notes
├── progress.md                 # Development progress log
│
├── docs/
│   ├── ai_usage_log.md         # AI assistance log (required for submission)
│   ├── ownership_statement.md  # Candidate ownership statement
│   └── technical_report.md     # Technical report (≤ 6 pages)
│
├── src/
│   ├── core/                   # Shared utilities, types, error handling
│   │   ├── types.h
│   │   ├── error.h
│   │   └── utils.h
│   │
│   ├── geometry/               # STL & CSV parsers
│   │   ├── stl_parser.h / .cpp
│   │   ├── csv_parser.h / .cpp
│   │   └── validators.h / .cpp
│   │
│   ├── thermal/                # 1D FEA solver
│   │   ├── solver.h / .cpp
│   │   ├── element.h / .cpp
│   │   ├── mesh.h / .cpp
│   │   └── analytical.h / .cpp
│   │
│   ├── performance/            # Parallel batch solver
│   │   ├── batch_runner.h / .cpp
│   │   ├── worker_pool.h / .cpp
│   │   └── benchmark.h / .cpp
│   │
│   ├── hardware/               # Embedded mock adapters
│   │   ├── device_types.h
│   │   ├── mcu_adapter.h / .cpp
│   │   ├── fpga_adapter.h / .cpp
│   │   ├── jtag_adapter.h / .cpp
│   │   └── device_manager.h / .cpp
│   │
│   ├── backpropagation/        # Neural net backprop
│   │   ├── network.h / .cpp
│   │   ├── gradient_check.h / .cpp
│   │   └── training.h / .cpp
│   │
│   └── backend/                # HTTP API server
│       ├── CMakeLists.txt      # CMake build configuration
│       ├── main.cpp            # Entry point
│       ├── server.h / .cpp     # Route definitions
│       ├── routes/
│       │   ├── geometry_routes.h / .cpp
│       │   ├── thermal_routes.h / .cpp
│       │   ├── performance_routes.h / .cpp
│       │   ├── hardware_routes.h / .cpp
│       │   └── backprop_routes.h / .cpp
│       ├── third_party/
│       │   ├── httplib.h
│       │   ├── json.hpp
│       │   └── eigen/          # Eigen headers
│       └── tests/
│           ├── test_main.cpp
│           ├── test_stl_parser.cpp
│           ├── test_csv_parser.cpp
│           ├── test_thermal.cpp
│           ├── test_backprop.cpp
│           └── test_framework.h  # Custom lightweight test framework
│
├── src/frontend/               # Next.js application
│   ├── package.json
│   ├── tsconfig.json
│   ├── tailwind.config.ts
│   ├── next.config.js
│   ├── electron/               # Electron main process
│   │   └── main.ts
│   ├── app/
│   │   ├── layout.tsx
│   │   ├── page.tsx
│   │   └── (views)/
│   │       ├── geometry/page.tsx
│   │       ├── thermal/page.tsx
│   │       ├── performance/page.tsx
│   │       └── hardware/page.tsx
│   ├── components/
│   │   ├── ui/                 # shadcn/ui components
│   │   ├── viewers/
│   │   │   ├── StlViewer.tsx
│   │   │   ├── PointCloudViewer.tsx
│   │   │   └── TemperatureLegend.tsx
│   │   ├── charts/
│   │   │   ├── TemperaturePlot.tsx
│   │   │   ├── ConvergencePlot.tsx
│   │   │   └── LossPlot.tsx
│   │   └── hardware/
│   │       ├── DevicePanel.tsx
│   │       └── BlockDiagram.tsx
│   └── lib/
│       ├── api.ts              # Backend HTTP client
│       └── types.ts            # Shared TypeScript types
│
├── data/
│   ├── example.stl             # Example ASCII STL (with provenance)
│   └── example_temperature.csv # SYNTHETIC temperature CSV
│
├── scripts/
│   ├── build.ps1               # Full build script
│   └── launch.ps1              # Dev launch helper
│
└── Launch.exe                  # One-click launcher (Electron output)
```

---

## 5  Build Instructions

### 5.1  C++ Backend

```bash
cd src/backend
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

The resulting binary is `workbench_backend.exe` (or `workbench_backend` on Linux).

### 5.2  Frontend

```bash
cd src/frontend
npm install
npm run dev          # Development server on localhost:3000
npm run build        # Production build
```

### 5.3  Electron Packaging

```bash
cd src/frontend
npm run electron-build   # Produces distributable in dist/
```

### 5.4  Full Build (Script)

```powershell
.\scripts\build.ps1      # Builds backend + frontend + packages Electron
```

---

## 6  Coding Conventions

### 6.1  C++ (Backend)

| Rule | Example |
|------|---------|
| Standard | C++17 (`-std=c++17`) |
| Naming — variables, functions | `snake_case` |
| Naming — classes, structs | `PascalCase` |
| Naming — constants, enums | `ALL_CAPS` |
| Headers | Use `#pragma once` |
| Error handling | Return `std::optional` or `Result<T>` types; throw only for unrecoverable errors |
| Includes | Group: standard library → third-party → project headers, alphabetical within groups |
| Comments | Doxygen-style `///` for public API; `//` for inline |
| Formatting | 4-space indent, `{}` on same line for functions, max 100 chars/line |

### 6.2  TypeScript (Frontend)

| Rule | Example |
|------|---------|
| Naming — variables, functions | `camelCase` |
| Naming — components, types | `PascalCase` |
| Naming — constants | `SCREAMING_SNAKE_CASE` |
| Imports | Absolute paths via `@/` alias |
| Components | Functional components with proper typing |
| Error handling | Try/catch with user-facing error messages |
| Formatting | 2-space indent, Prettier defaults |

---

## 7  Testing

### 7.1  C++ Tests

We use a **custom lightweight test framework** (`test_framework.h`) — no external
test library dependency. Features:

- `TEST(name)` macro to define test cases
- `ASSERT_TRUE(expr)`, `ASSERT_EQ(a, b)`, `ASSERT_NEAR(a, b, tol)` assertion macros
- Automatic test registration and summary
- Colored console output

```bash
cd src/backend/build
cmake --build . --target run_tests
# or
./workbench_tests
```

### 7.2  Frontend Tests

```bash
cd src/frontend
npm run test             # Jest / Vitest unit tests
```

### 7.3  What to Test

| Module | Must Test |
|--------|-----------|
| STL parser | Valid STL, malformed STL, empty file, binary STL rejection |
| CSV parser | Valid CSV, missing columns, non-finite values, empty file |
| Thermal solver | Known analytical solution comparison, boundary conditions, different mesh sizes |
| Backpropagation | Gradient check vs central finite differences (ε=1e-6, error ≤ 1e-5) |
| Parallel batch | Serial vs parallel result agreement (≤ 1e-8 °C) |
| Hardware adapters | Connect/disconnect lifecycle, failure recovery |

---

## 8  Critical Rules

> **These rules are non-negotiable for all contributors (human or AI).**

1. **Never hardcode results.** All numerical values must come from actual computation.
2. **Never fabricate data.** If a test hasn't been run, say so. Do not invent test outputs.
3. **Label synthetic data.** Any generated/simulated data must be clearly marked `SYNTHETIC` or `SIMULATED`.
4. **All computation in C++.** The frontend must not perform any engineering calculations. It only displays results received from the C++ backend via the HTTP API.
5. **Reproducibility.** Parallel batch runs must use fixed seeds. Results must be deterministic and ordered by input case index.
6. **No admin privileges.** The application must run without elevation, terminal, or installation.
7. **Relative paths only.** No absolute paths in source code or build outputs. No credentials or personal data.

---

## 9  Evaluation Rubric (100 Points)

| Category | Points | Shortlisting Gate? |
|----------|--------|--------------------|
| Interface / webpage design | 8 | No |
| C++ backend and testing | 12 | **Yes (≥ 6)** |
| Folder architecture | 6 | No |
| Code readability | 6 | No |
| Version control | 5 | No |
| Thermal FEA and adoption | 14 | **Yes (≥ 7)** |
| CAD/ANSYS visualization | 10 | No |
| Multicore performance | 10 | No |
| Embedded integration | 8 | No |
| Backpropagation | 7 | No |
| Prompt engineering / AI judgement | 8 | No |
| One-click delivery / report | 6 | **Yes (≥ 3)** |

### Shortlisting Gates (Must Pass)

- ≥ 6/12 on C++ backend
- ≥ 7/14 on thermal FEA
- ≥ 3/6 on delivery/report
- Working C++ backend binary
- Successful standalone launch (Launch.exe)

---

## 10  Documentation Update Rules

1. **tasks.md** — Check off tasks as completed. Never uncheck without explanation.
2. **progress.md** — Append a dated entry for each work session.
3. **findings.md** — Record all architecture decisions, debugging insights, and numerical results.
4. **docs/ai_usage_log.md** — Log every significant AI interaction (prompt summary, what was generated, what was modified).
5. Keep this file (`AGENTS.md`) updated if architecture or conventions change.

---

## 11  How to Continue Development

1. **Read** `tasks.md` to see what's done and what's next.
2. **Read** `progress.md` to understand recent activity.
3. **Read** `findings.md` for architectural decisions already made.
4. **Read** `task_plan.md` for the phased implementation strategy.
5. **Pick** the highest-priority unchecked task from `tasks.md`.
6. **Implement** following the conventions in this file.
7. **Test** thoroughly before checking off.
8. **Update** `tasks.md`, `progress.md`, `findings.md`, and `docs/ai_usage_log.md`.
9. **Commit** with a meaningful message describing what changed and why.
