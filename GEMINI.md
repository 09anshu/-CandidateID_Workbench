# GEMINI.md — Gemini / Antigravity Instructions

> **Read [AGENTS.md](./AGENTS.md) first.** It is the canonical project reference.
> This file contains Gemini/Antigravity-specific guidance only.

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

## Workspace Layout

```
Pass_Science_assignment/
├── src/backend/          # C++17 HTTP server + all computation modules
├── src/frontend/         # Next.js + Electron frontend
├── data/                 # Example STL and CSV files
├── docs/                 # Reports, AI usage log, ownership statement
└── scripts/              # Build and launch scripts
```

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

## Antigravity-Specific Notes

- When using Antigravity's workspace tools, prefer editing files in-place rather than
  rewriting entire files to preserve Git history cleanliness.
- Use the workspace terminal for build verification after changes.
- When creating new source files, follow the directory structure in [AGENTS.md § 4](./AGENTS.md).
- Always update `progress.md` and `tasks.md` after completing work.
- Log AI interactions in `docs/ai_usage_log.md`.

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
