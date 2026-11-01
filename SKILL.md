---
name: pass-science-workbench
description: Development skill and operating handbook for the PASS SCIENCE WORKS Unified Engineering Workbench
---

# PASS SCIENCE WORKS — Unified Engineering Workbench Developer Skill

> Operating instructions and engineering workflow guide for any contributor or AI model maintaining this codebase.

---

## 1. Quick Reference Commands

### 1.1 Building C++ Backend & Tests
```powershell
$env:PATH = "C:\msys64\mingw64\bin;" + $env:PATH
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### 1.2 Running C++ Tests (28 Automated Tests)
```powershell
cd build
.\workbench_tests.exe
```

### 1.3 Running Next.js Frontend Dev Server
```powershell
cd src/frontend
npm run dev
# Accessible at http://localhost:3000
```

### 1.4 Building Frontend Static Production Export
```powershell
cd src/frontend
npm run build
# Generates static assets in src/frontend/out/
```

### 1.5 Building One-Click Native Launcher
```powershell
$env:PATH = "C:\msys64\mingw64\bin;" + $env:PATH
g++.exe -O3 -mwindows src/backend/launcher.cpp -o Launch.exe
```

---

## 2. Architecture Principles

1. **Strict Backend Separation**: All numerical computing (FEA, linear algebra, multithreading, geometry parsing, embedded telemetry, and backpropagation) MUST be written in C++17. The frontend is presentation and interaction only.
2. **Deterministic Computations**: Workload generation must use a fixed seed (`std::mt19937_64(seed)`).
3. **No Hidden Failures**: All operations return `workbench::Result<T>` or HTTP error codes with human-readable explanations.
4. **Zero Administrative Elevation**: `Launch.exe` runs in standard user mode without requiring admin rights or PowerShell script execution privileges.

---

## 3. How to Add a New C++ Numerical Module

1. Define data structures and return types in `src/<module>/<name>.h`.
2. Implement computational logic in `src/<module>/<name>.cpp`.
3. Add unit tests in `tests/test_main.cpp`.
4. Register the new source file in `CMakeLists.txt` under `CORE_SOURCES`.
5. Expose REST API endpoint in `src/backend/server_routes.cpp`.
6. Add corresponding TypeScript API client function in `src/frontend/lib/api.ts`.
7. Rebuild backend and run tests to verify zero regressions.

