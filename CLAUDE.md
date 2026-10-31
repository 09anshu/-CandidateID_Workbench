# CLAUDE.md — Claude-Specific Instructions

> **Read [AGENTS.md](./AGENTS.md) first.** It is the canonical project reference.
> This file contains Claude-specific guidance only.

---

## Quick Orientation

1. **Understand the project** → Read [AGENTS.md](./AGENTS.md) for architecture, conventions, and rules.
2. **What's been done?** → Read [progress.md](./progress.md) for latest status.
3. **What's next?** → Read [tasks.md](./tasks.md) for the full checklist.
4. **Why decisions were made** → Read [findings.md](./findings.md) for rationale and math.
5. **Phased plan** → Read [task_plan.md](./task_plan.md) for implementation order and dependencies.

---

## Build Commands

### C++ Backend

```bash
cd src/backend
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

On Windows with MSYS2/MinGW:

```bash
cd src/backend
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Run C++ Tests

```bash
cd src/backend/build
cmake --build . --target run_tests
# or directly:
./workbench_tests.exe
```

### Frontend

```bash
cd src/frontend
npm install
npm run dev            # Dev server → http://localhost:3000
```

### Electron Packaging

```bash
cd src/frontend
npm run electron-build
```

---

## Key Files to Inspect Before Working

| File | Purpose |
|------|---------|
| `tasks.md` | Current task status — find what to work on next |
| `progress.md` | What's been done and when |
| `findings.md` | Architecture decisions, math derivations, debugging notes |
| `task_plan.md` | Implementation phases and dependencies |
| `AGENTS.md` | Full project spec, conventions, directory structure |

---

## Constraints (from AGENTS.md)

1. **All computation in C++.** Frontend is display-only.
2. **Never hardcode or fabricate results.**
3. **Label synthetic/simulated data** clearly.
4. **C++17 standard** with snake_case naming.
5. **TypeScript** with camelCase naming.
6. **Custom test framework** — no Google Test or Catch2.
7. **One-click launch** — `Launch.exe` must work without admin, terminal, or installers.
8. **Relative paths only** — no absolute paths in code.

---

## Claude-Specific Notes

### Working Style

- **Read before writing.** Always check `tasks.md` and `progress.md` before starting work.
- **Prefer targeted edits** over full file rewrites to keep Git diffs clean.
- **Build and test** after every significant change.
- **Update documentation** (`tasks.md`, `progress.md`, `findings.md`) after completing work.
- **Log AI usage** in `docs/ai_usage_log.md`.

### Code Quality Expectations

- Follow the coding conventions in [AGENTS.md § 6](./AGENTS.md).
- Write meaningful comments — explain *why*, not *what*.
- Every public function needs a Doxygen-style `///` docstring.
- Handle errors gracefully; never silently swallow exceptions.

### Common Pitfalls

- Do not add Google Test, Catch2, or any external test framework. Use the custom
  lightweight framework in `test_framework.h`.
- Do not perform calculations in TypeScript/frontend code. All computation is C++.
- Do not use `std::filesystem` without checking compiler support — some MinGW
  versions have issues.
- Remember: Windows line endings in PowerShell, Unix in MSYS2.

---

## Common Workflows

### Adding a New C++ Module

1. Create `src/<module>/<name>.h` and `src/<module>/<name>.cpp`.
2. Add source to `src/backend/CMakeLists.txt`.
3. Create test file `src/backend/tests/test_<name>.cpp`.
4. Add test source to CMakeLists.txt test target.
5. Build and run tests.
6. Add API route in `src/backend/routes/`.
7. Update `tasks.md`, `progress.md`.

### Adding a New Frontend View

1. Create `src/frontend/app/(views)/<name>/page.tsx`.
2. Add navigation link in layout.
3. Create components in `src/frontend/components/`.
4. Add API client function in `src/frontend/lib/api.ts`.
5. Test with dev server.

---

## Environment Info

| Tool | Version | Notes |
|------|---------|-------|
| Node.js | v22.20.0 | Installed |
| Git | 2.50.0 | Installed |
| C++ Compiler | Needs MSYS2 GCC 13+ or MSVC | Must support C++17 |
| CMake | ≥ 3.16 | Required for backend build |
| npm | Bundled with Node.js | For frontend |

---

## Priority Order

Focus on **shortlisting gates** first:

1. **C++ backend** (≥ 6/12) — core computation must work
2. **Thermal FEA** (≥ 7/14) — solver + analytical comparison + plots
3. **Delivery/report** (≥ 3/6) — Launch.exe + README + documentation
4. Then: visualization, parallel, embedded, backprop, UI polish
