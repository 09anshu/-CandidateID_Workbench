# docs/critical_issues.md — Critical Issues, Defect Log & Engineering Resolutions

> **Project:** PASS SCIENCE WORKS — Unified Engineering Workbench  
> **Log Standard:** Strict Engineering Audit & Defect Tracking  

---

## 1. Defect & Resolution Log

### Issue 1: Controlled Numeric Input "Snap-Back" in React Form Fields
- **Severity:** High (User Experience & Data Entry)
- **Symptom:** In the Performance and Thermal tabs, attempting to clear or backspace input values (`num_cases`, `mesh_size`, `random_seed`, `length`, etc.) caused the input field to immediately snap back to its default value (`100`, `20`, `42`). Typing `0` was impossible because `0` was falsy.
- **Root Cause:** React controlled inputs were written with `parseInt(e.target.value) || DEFAULT_VALUE`. When a user cleared the input to type a new number, `e.target.value` became `""`, which parsed to `NaN`. In JavaScript, `NaN || 100` evaluated to `100`, forcing the state back to the default on the keystroke.
- **Resolution:** Modified `src/frontend/app/page.tsx` so inputs bind directly to raw keystroke text (`e.target.value`). Form submission parsers (`handleRunBenchmark`, `handleRunThermal`) sanitize and parse to numbers only when the user explicitly triggers computation.
- **Status:** **RESOLVED & VERIFIED**.

---

### Issue 2: Legacy MinGW GCC 6.3.0 Incompatibility with Modern C++17
- **Severity:** Critical (Build Blocker)
- **Symptom:** Existing `C:\MinGW\bin\g++.exe` failed with DLL relocation memory errors and lacked C++17 headers (`<optional>`, `<charconv>`, filesystem).
- **Root Cause:** Legacy 32-bit MinGW toolchain from 2017 installed in root system.
- **Resolution:** Installed MSYS2 toolchain with GCC 16.2.0, CMake 4.4.3, and Make 4.4.1 in `C:\msys64\mingw64\bin`.
- **Status:** **RESOLVED & VERIFIED**.

---

### Issue 3: C++ Line-Continuation Comment Bug in `json_helper.h`
- **Severity:** High (Compilation Error)
- **Symptom:** Compiler reported syntax errors in `json_helper.h` regarding undeclared variables.
- **Root Cause:** Line 52 contained `// Unescape \n, \r, \", \\` where the trailing double-backslash was treated by GCC as a preprocessor line continuation character, commenting out the following line (`std::string unescaped;`).
- **Resolution:** Removed the trailing backslash from the comment.
- **Status:** **RESOLVED & VERIFIED**.

---

### Issue 4: PowerShell `Compress-Archive` Headless Console Buffer Error
- **Severity:** Medium (Packaging Blocker)
- **Symptom:** Running `Compress-Archive` in PowerShell threw `Write-Progress : Win32 internal error "Access is denied" 0x5 occurred while reading console output buffer`.
- **Root Cause:** Headless execution environments without a Win32 console buffer fail when PowerShellcmdlets attempt to render interactive progress bars.
- **Resolution:** Authored `scripts/package_zip.py` using Python's standard `zipfile` module, producing `CandidateID_Workbench.zip` in seconds with reproducible deflate compression.
- **Status:** **RESOLVED & VERIFIED**.

---

## 2. Unresolved Critical Issues

- **None.** All identified defects have been systematically diagnosed, resolved, verified, and re-tested.

