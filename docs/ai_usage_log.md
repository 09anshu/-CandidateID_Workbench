# AI Assistance & Verification Log

> **PASS SCIENCE WORKS — R&D Internship 2026-2027**  
> Mandatory AI Assistance Disclosure & Verification Evidence

---

## 1. Overview & Policy Compliance

All AI interactions were conducted under strict engineering oversight. Every mathematical derivation, numerical formulation, and software algorithm was independently checked against first principles and verified with automated test suites.

| AI Tool / Model | Version / Provider | Role |
|:---|:---|:---|
| **Antigravity / Gemini / Claude** | Google DeepMind / Anthropic | Code generation, architectural review, documentation drafting |

---

## 2. Representative Interactions & Prompt Log

### Interaction 1: Thermal FEA Solver Formulation
- **Prompt Summary**: "Formulate the 1-D steady-state finite element stiffness matrix and load vector for $-d/dx(kA dT/dx) = qA$, and verify the exact analytical solution."
- **Model Output**: Provided element equations $K^e = \frac{kA}{h} [1, -1; -1, 1]$ and $f^e = \frac{qAh}{2} [1; 1]$, suggesting error be evaluated at mesh nodes.
- **Critical Assessment & Correction**:
  - *Identified Flaw*: In 1-D linear elements with uniform heat generation, Green's function reproduces quadratic polynomials **exactly at the mesh nodes** (nodal superconvergence). If error is computed only at nodes, the observed error is numerically zero ($< 10^{-14}$), falsely suggesting zero discretization error!
  - *Correction Applied*: Evaluated error at element midpoints using linear finite-element shape function interpolation:
    $$T_{\text{FEM}}(x_m) = \frac{T_i + T_{i+1}}{2}$$
    The difference between the parabolic exact curve and linear secant is analytically proven to be $\frac{q}{8k} h^2$, rigorously exposing the true $O(h^2)$ truncation error.

### Interaction 2: C++ Header Comment Syntax Bug
- **Prompt Summary**: "Create JSON string extraction helper for REST request parsing in `json_helper.h`."
- **Model Output**: Generated helper function with comment: `// Unescape \n, \r, \", \\` followed immediately on the next line by `std::string unescaped;`.
- **Compiler Failure**: GCC threw error: `'unescaped' was not declared in this scope`.
- **Critical Assessment & Resolution**:
  - In C++, a trailing backslash `\` at the end of a single-line comment `//` acts as a line continuation character! It caused the compiler to treat the subsequent line (`std::string unescaped;`) as part of the comment.
  - Corrected by removing trailing backslash from the comment.

### Interaction 3: Backpropagation Chain Rule & Update Order
- **Prompt Summary**: "Derive gradients for $h = \tanh(wx + b), \hat{y} = vh + c, J = 0.5(\hat{y} - y)^2$ and perform gradient descent update."
- **Model Output**: Derived gradients and suggested updating $w$, then using the updated $w$ to compute the update for $b$.
- **Critical Assessment & Correction**:
  - *Identified Flaw*: Sequential updates violate standard gradient descent (and the specific prompt instruction). Updating $w$ sequentially alters subsequent gradients.
  - *Correction Applied*: Enforced simultaneous gradient descent update using gradients computed strictly from the **original** initial parameters $\theta_0 = (w_0, b_0, v_0, c_0)$. Verified using central finite differences with $\varepsilon = 10^{-6}$, achieving error $< 10^{-7}$.

### Interaction 4: GoogleTest & CTest Automated Testing Suite Integration
- **Prompt Summary**: "Implement an automated testing module using GoogleTest and CTest with backend execution endpoint and frontend dashboard."
- **Model Output**: Created `tests/gtest_suite.cpp`, CMake GoogleTest integration, backend test runner with atomic concurrency lock, and Next.js testing dashboard with live metrics and console terminal.
- **Critical Assessment & Resolution**:
  - *Identified Flaw*: Dynamically linked MSYS2 GoogleTest DLLs (`libgtest.dll`, `libgtest_main.dll`) were initially missing when running `workbench_gtests.exe` outside MSYS2 environment.
  - *Correction Applied*: Copied runtime DLLs to application root and build output folders, verified standalone zero-dependency execution, and registered CTest integration achieving 100% test pass rate (28/28 tests passed).

### Interaction 5: Form Input Freeform Numeric Editing & Video Integration
- **Prompt Summary**: "Fix numeric input freezing / stepper snapping in multicore benchmark and thermal views, add demonstration video to README, and prepare GitHub push."
- **Model Output**: Refactored inputs to `type="text" inputMode="numeric"` with CSS spinner removal, updated `README.md` with official demonstration video link, and validated repository structure.
- **Critical Assessment & Resolution**:
  - Validated that unconstrained text inputs allow freeform backspacing, negative numbers, and arbitrary values while eliminating browser mouse-wheel scroll hijacking.

### Interaction 6: Assignment Extension — 3D Heatsink Multi-Solver Comparison
- **Prompt Summary**: "Complete assignment extension comparing three solvers (Eigen SparseLU, PCG with Incomplete Cholesky, AMG-PCG via AMGCL) on a 3D Aluminium Finned Heatsink across four mesh resolutions with 12 figures and a comprehensive PDF report."
- **Model Output**: Designed 3D heatsink procedural geometry (10 fins, 100x60x8 mm base), conforming 5-tet Kuhn volume mesher, closed-form 3D element conduction matrix assembly, and wrapper classes for all three solvers. Generated 12 scientific figures in `docs/figures_heatsink/` and compiled 12-page report `Heatsink_Thermal_Solver_Comparison.pdf`.
- **Critical Assessment & Resolution**:
  - *Identified Flaw 1*: In initial 5-tet subdivision, one tetrahedron had a negative determinant orientation ($\det(C) = -2.0$). Corrected vertex ordering to $(2, 0, 5, 7)$ ensuring strictly positive Jacobian determinants across all elements.
  - *Identified Flaw 2*: Uncompressed PDF generation produced an unnecessarily large 234 MB file due to high-resolution raster image streams. Applied `doc.save(..., deflate=True, garbage=4, clean=True)` to compress stream objects, reducing file size to 15.6 MB while preserving 100% vector typography and visual fidelity.
  - *Verification*: Confirmed that all three solvers solve the exact same assembled FEM system with cross-solver solution agreement $< 4.86 \times 10^{-7\ \circ}\text{C}$ and $0.00\%$ thermodynamic energy imbalance.

### Interaction 7: Comprehensive Project Audit, PDF Text Truncation Fix & Full Frontend/Backend Extension Integration
- **Prompt Summary**: "Execute an exhaustive 18-phase audit and completion of the Unified Engineering Workbench: fix all frontend views, link the 3D heatsink solver into the main backend REST API, eliminate PDF report indentation and text-clipping issues, render and visually inspect all 12 pages, and re-validate entire test suite."
- **Model Output & Architectural Actions**:
  - *Identified Root Cause of Missing PDF Text*: In `scripts/generate_heatsink_report_pdf.py`, hardcoded bounding box heights in PyMuPDF's `page.insert_textbox(rect, ...)` caused textboxes on pages 2, 4, 5, 6, 7, 8, 9, 10, 11, 12 to return negative values (deficiency), which PyMuPDF handled by silently dropping the entire text of those sections (e.g. Page 5 only showed headings with 351 characters).
  - *Resolution Applied*: Implemented dynamic auto-fitting `draw_text_box` with bottom-limit bounding and adaptive font scaling. Total character counts increased from ~800 to >3,400 on Page 5, restoring all mathematical weak form formulations, analytical zero-quadrature tetrahedral stiffness integrals, and energy conservation proofs.
  - *Backend Integration*: Linked `heatsink_mesh.cpp` and `heatsink_solver.cpp` directly into `workbench_backend` CMake target, added REST endpoints (`POST /api/v1/heatsink/solve`, `GET /api/v1/heatsink/results`, `GET /api/v1/heatsink/agreement`, `GET /api/v1/heatsink/report/pdf`), and resolved `Point3D` struct duplicate definition between `src/core/types.h` and `src/thermal/heatsink_mesh.h`.
  - *Frontend Extension*: Added dedicated "3D Heatsink (3 Solvers)" tab to Next.js dashboard with mesh resolution selector (Coarse to Very Fine), solver selector (SparseLU, PCG, AMG-PCG), live solve execution, real-time metrics, cross-solver agreement banner, and 5-run median benchmark table.
  - *Verification*: Executed both test suites (43/43 custom tests passed, 28/28 GoogleTest cases passed), validated all REST endpoints live, and rendered all 12 PDF pages to 150 DPI PNGs confirming zero text clipping and professional engineering formatting.

### Interaction 8: PDF Visual Formatting Corrections & Word Deliverable Authoring
- **Prompt Summary**: "Correct formatting defects in `Heatsink_Thermal_Solver_Comparison.pdf` (Table 6 text collisions, Table 4 header collisions, Page 1 metadata overlaps, question-mark character corruption, contour caption overlap) and deliver the complete report in Microsoft Word format (.docx)."
- **Model Output & Architectural Actions**:
  - *Identified Flaw 1 (Question Mark Glyph Corruption)*: PyMuPDF's built-in Type 1 Helvetica font (`helv`) uses Standard 14 / WinAnsi encoding. Unicode characters like em-dash (`—`), bullets (`•`), and math symbols were substituted with question marks (`?`). Corrected by integrating Windows TrueType Arial fonts (`arial.ttf`, `arialbd.ttf`, `ariali.ttf`) and normalizing ASCII text replacements, completely eliminating all `?` artifacts (verified with 0 question marks across the entire report).
  - *Identified Flaw 2 (Table Cell Collisions & Word-Wrapping)*: `draw_table` wrote single unclipped lines. Table 6 descriptions in columns 4 & 5 exceeded column widths and severely overlapped. Implemented a robust word-wrapping table drawer (`draw_table`) that calculates cell wrapping, adapts row heights dynamically, and centers text vertically.
  - *Identified Flaw 3 (Header Overlap & Spacing)*: Resized Table 4 headers ("Max Temp Diff (L_inf)" and "RMS Temp Diff (L2)") and adjusted column widths to ensure ample clearance. Restructured Page 1 metadata into a clean 3-row layout, completely eliminating overlap between `Windows 11 x64` and `Backend Standard:`.
  - *Identified Flaw 4 (Figure 5/6/7 Clearance)*: Reduced contour heights to 160 pt and added a 16 pt vertical clearance gap between Figure 5/6 captions and Figure 7, preventing any caption overlap.
  - *Word Deliverable Creation*: Authored `scripts/generate_heatsink_report_docx.py` using `python-docx` to generate `Heatsink_Thermal_Solver_Comparison.docx` (in root and in `docs/`), containing all 26 sections, 9 styled tables, 12 embedded figures, and 4,770 words.
  - *Verification*: Automated spatial collision test (`scripts/verify_pdf_layout.py`) verified 0 word collisions and 0 image collisions across all 12 pages. Rebuilt `CandidateID_Workbench.zip` (85.43 MB) containing both verified deliverables.

### Interaction 9: Figure 8 Bottom Box Restoration, Figure 5-7 Title Centering & Seal Removal
- **Prompt Summary**: "Fix Figure 8 flowchart where bottom boxes were cropped, fix Figure 6 & 7 titles where 'Figure' was clipped on the left, and remove the final deliverable verification box at the end of the documents."
- **Model Output & Architectural Actions**:
  - *Identified Root Cause (Figure 8 Box Cropping)*: In `scripts/generate_heatsink_visualizations.py`, `ax.set_ylim([0, 8.5])` truncated the terminal box whose rounded bottom boundary extended down to $y = -0.30$. Expanded axes limits to `ylim=[-0.8, 8.8]` and applied `bbox_inches="tight"`, providing 116 px of clean margin below the terminal boxes with fully rounded, closed borders.
  - *Identified Root Cause (Figure 6 & 7 Title Clipping)*: Matplotlib 3D axes titles were positioned using `ax.set_title` with long text strings that extended past the left figure boundary ($x < 0$). Replaced with centered `fig.suptitle(..., y=0.95, weight="bold")` and `bbox_inches="tight"`, leaving >100 px of margin on both sides and completely eliminating left title clipping.
  - *Deliverable Verification Seal Removal*: Removed the final verification box from the end of Section 26 in both `generate_heatsink_report_pdf.py` and `generate_heatsink_report_docx.py`.
  - *Re-compilation & Packaging*: Re-compiled `Heatsink_Thermal_Solver_Comparison.pdf` and `Heatsink_Thermal_Solver_Comparison.docx`, verified zero collisions, and repackaged `CandidateID_Workbench.zip` (76.23 MB).

---

## 3. Deliberate Challenge Test & Outcome

To rigorously validate model reasoning under subtle edge cases, a deliberate challenge test was conducted:
- **Challenge**: "If the right boundary condition is changed from Dirichlet $T(L) = 20^\circ\text{C}$ to an insulated Neumann condition $\frac{dT}{dx}(L) = 0$, how does the global stiffness matrix change?"
- **Outcome**: The AI correctly identified that the boundary row $N$ is no longer eliminated or set to identity; instead, the natural boundary condition $-k A \frac{dT}{dx}(L) = 0$ contributes zero flux to $f_N$, leaving $K_{N,N} = \frac{kA}{h}$ and $F_N = \frac{qAh}{2}$.

