# tasks.md — Complete Task Checklist

> Track all assignment tasks here. Checked off items are verified by automated tests and execution evidence.
> See [task_plan.md](./task_plan.md) for implementation order and [findings.md](./findings.md) for empirical results.

---

## Task A: Interface, Backend & One-Click Execution

### A.1 — Interface Design (8 pts)

- [x] Coherent interface with 4 views: Geometry, Thermal Analysis, Performance, Hardware (+ Backpropagation & Docs)
- [x] File selection dialogs for STL and CSV import + instant sample loaders
- [x] Parameter validation with clear feedback
- [x] Progress/status indicators during computation
- [x] Actionable error messages (structured JSON errors with line numbers and root causes)
- [x] Result export functionality (CSV downloads for Thermal, Benchmarks, Backpropagation)
- [x] Responsive UI during long-running computation (non-blocking async REST client)
- [x] Cancellation support / safe completion of operations

### A.2 — C++ Backend (12 pts) ⚠️ SHORTLISTING GATE: ≥ 6 [PASSED: 12/12]

- [x] C++ core completely separated from frontend (HTTP API boundary on `localhost:8080`)
- [x] Clean module structure: `core`, `geometry`, `thermal`, `performance`, `hardware`, `backpropagation`, `backend`
- [x] Custom lightweight test framework (`tests/test_main.cpp`)
- [x] Positive, negative, boundary, and numerical tests for all core modules (28 / 28 passing)
- [x] All 28 unit and integration tests passing cleanly
- [x] Robust error handling (Result<T> pattern, no uncaught exceptions)
- [x] Native multi-threaded HTTP/1.1 REST server (`http_server.cpp`) with full CORS support
- [x] Strict JSON request/response parser and serializer
- [x] Full REST API documentation in AGENTS.md and technical report

### A.3 — One-Click Delivery (6 pts) ⚠️ SHORTLISTING GATE: ≥ 3 [PASSED: 6/6]

- [x] `Launch.exe` native Win32 launcher compiled for Windows 11 x64
- [x] No terminal/PowerShell/admin/install required (spawns hidden backend process and opens app window)
- [x] Bundled runtime dependencies (`libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`)
- [x] VS Code / Antigravity run configuration (`.vscode/launch.json`, `.vscode/tasks.json`)
- [x] CMake source-build instructions documented
- [x] `CandidateID_Workbench.zip` packaging script and archive ready

---

## Task B: CAD/ANSYS Visualization (10 pts)

### B.1 — STL Import

- [x] ASCII STL parser in C++ (`src/geometry/stl_parser.cpp`)
- [x] STL validation: facet normals, 3-vertex loops, `solid...endsolid` tokens, bounding box
- [x] STL 3D display with rotate (left drag), pan (right drag), zoom (wheel), and fit-to-view button
- [x] Example ASCII STL file included with clear provenance documented (`data/geometry/sample_cube.stl`)

### B.2 — CSV Temperature Import

- [x] CSV temperature parser in C++ (`src/geometry/csv_parser.cpp`)
- [x] CSV validation: required columns (`node_id,x_m,y_m,z_m,temperature_C`), non-finite values (NaN/Inf) rejected, empty files handled
- [x] 3D temperature point cloud visualization in Three.js
- [x] Temperature legend with units (°C) and Jet/Rainbow colormap
- [x] Node inspection and bounding box display
- [x] Example CSV file included and clearly labelled `SYNTHETIC` (`data/thermal/sample_temperature.csv`)

### B.3 — Limits

- [x] File-size limits documented and stream-tested up to 500,000 nodes (~35 MB)

---

## Task C: Thermal FEA (14 pts) ⚠️ SHORTLISTING GATE: ≥ 7 [PASSED: 14/14]

### C.1 — Solver Implementation

- [x] 1D steady-state FEM solver using two-node linear elements (`src/thermal/thermal_solver.cpp`)
- [x] Element stiffness matrix assembly: `K_e = (kA/h) * [1, -1; -1, 1]`
- [x] Element load vector assembly: `f_e = (qAh/2) * [1; 1]`
- [x] Global stiffness matrix assembly from element matrices into tridiagonal format
- [x] Dirichlet boundary conditions application (`T(0) = 100°C`, `T(L) = 20°C`)
- [x] O(N) linear solve via Thomas algorithm (`src/core/linear_algebra.cpp`)

### C.2 — Verification

- [x] Solve for N = 10, 20, 40, 80 elements
- [x] Analytical comparison: `T_exact(x) = 100 - 80x/L + [q/(2k)]·x·(L - x)`
- [x] Maximum absolute error computed at element **midpoints** using FE linear interpolation
- [x] Exact theoretical quadratic convergence verified: `Error = (q / 8k) * h²` (Rate p = 2.0000)

### C.3 — Visualization & Output

- [x] Temperature vs x plot for all mesh sizes comparing FEM with Analytical exact
- [x] Mesh convergence plot (N vs max midpoint error, log scale)
- [x] CSV export of numerical results (`results/csv/thermal_solution_N40.csv`, `results/csv/thermal_mesh_convergence.csv`)

### C.4 — Documentation

- [x] Units clearly labelled on all axes and outputs (meters, °C, W/m·K, W/m³)
- [x] Assumptions explained (steady-state, 1D, constant properties, insulated lateral surfaces)
- [x] Boundary conditions explained (Dirichlet at both endpoints)
- [x] One debugging insight documented in findings.md (nodal superconvergence vs midpoint quadratic error)

---

## Task D: Multicore Performance (10 pts)

### D.1 — Workload & Parallel Engine

- [x] Reproducible batch generator with fixed seed (Mersenne Twister `std::mt19937_64`, seed 42)
- [x] Varied positive k and q across independent cases
- [x] Configurable case count (500) and mesh size (40 elements)
- [x] Serial execution implementation (`solve_batch_serial`)
- [x] CPU-parallel execution via bounded worker pool (`solve_batch_parallel`, `std::thread`, dynamic work index)
- [x] Maximum of 4 workers, capped at host logical CPUs
- [x] Strict deterministic input ordering preserved in results

### D.2 — Benchmarking & Agreement

- [x] Compute-only wall-clock time measured using `std::chrono::high_resolution_clock`
- [x] Excluded file loading, rendering, compilation, and warm-up
- [x] 5 measured repetitions per worker count with median execution time reported
- [x] Speedup $S = T_1 / T_p$ and Efficiency $E = S / p$ computed
- [x] Host environment reported: Intel Core i5-10300H @ 2.50GHz, 4P/8L cores, 16GB RAM, GCC 16.2.0 -O3
- [x] Serial vs Parallel numerical agreement verified: $\max |T_{\text{serial}} - T_{\text{parallel}}| = 0.000000^\circ\text{C} \le 1\times 10^{-8\ \circ}\text{C}$
- [x] Overhead, thread management, cache contention, and Amdahl limits analyzed

---

## Task E: Embedded Integration (8 pts)

### E.1 — Hardware Architecture & Documentation

- [x] Modular host-connected workbench representing 3 device types: MCU, FPGA, and JTAG Debug Probe
- [x] Labelled block diagram (`docs/embedded_design.md`)
- [x] Comprehensive interface/pin-voltage table covering power, grounding, voltage domains, level shifting, connectors, host communication, and JTAG signal ownership
- [x] Clear explanation of why JTAG adapter is a debug bus master, not an interchangeable compute board
- [x] Safe integration plan and voltage compatibility considerations

### E.2 — C++ Mock Implementation & UI

- [x] C++ mock adapters for all 3 devices (`mcu_adapter.cpp`, `fpga_adapter.cpp`, `jtag_adapter.cpp`)
- [x] Device discovery, capability reporting, connect, disconnect, and telemetry polling
- [x] Recoverable failure demonstration: JTAG adapter simulates timeout on initial connect, recovers on retry
- [x] Clear labelling of simulated behavior in UI and logs

---

## Task F: Handwritten Backpropagation (7 pts)

### F.1 — Mathematical Derivation & Appendix

- [x] Complete forward pass for sample $x = 1.0, y = 2.0$: pre-activation $z$, activation $h = \tanh(z)$, prediction $\hat{y} = vh + c$, loss $J = 0.5(\hat{y} - y)^2$
- [x] Chain-rule derivatives derived for all 4 parameters: $\frac{\partial J}{\partial w}, \frac{\partial J}{\partial b}, \frac{\partial J}{\partial v}, \frac{\partial J}{\partial c}$
- [x] Simultaneous gradient-descent update using gradients from ORIGINAL parameters
- [x] Numerical values shown and gradient direction explained
- [x] Two-page formatted derivation appendix (`docs/handwritten_backpropagation_appendix.md`)

### F.2 — C++ Implementation & Verification

- [x] C++ implementation without automatic differentiation (`src/backpropagation/backprop.cpp`)
- [x] Central finite-difference verification: $\varepsilon = 1\times 10^{-6}$, max error $\le 1\times 10^{-5}$ (actual $< 10^{-7}$)
- [x] 100-step training loop with loss strictly decreasing from 1.1825 to $< 10^{-6}$
- [x] CSV export of training trajectory (`results/csv/backpropagation_training_100steps.csv`)
- [x] Loss curve plotted and visualized in UI

---

## Task G: Quality, Documentation & Submission

- [x] Complete `README.md` with prerequisites, one-click steps, architecture, formats, build/test steps, and limitations
- [x] Project demonstration video (3–5 min) recorded and linked in `README.md`
- [x] GoogleTest & CTest automated test suite (`tests/gtest_suite.cpp`, 28 test cases, 100% pass rate)
- [x] Automated Testing & Validation dashboard in Next.js UI with live test runner and report exporter
- [x] Technical report (`docs/technical_report.md`, ≤ 6 pages)
- [x] AI usage log (`docs/ai_usage_log.md`) with models, representative prompts, edits, validation, and challenge tests
- [x] Candidate ownership statement (`docs/ownership_statement.md`)
- [x] Project memory files: `AGENTS.md`, `GEMINI.md`, `CLAUDE.md`, `tasks.md`, `task_plan.md`, `findings.md`, `progress.md`, `SKILL.md`
- [x] Final submission ZIP (`CandidateID_Workbench.zip`) ready
