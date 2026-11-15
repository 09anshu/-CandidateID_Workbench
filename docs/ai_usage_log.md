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

---

## 3. Deliberate Challenge Test & Outcome

To rigorously validate model reasoning under subtle edge cases, a deliberate challenge test was conducted:
- **Challenge**: "If the right boundary condition is changed from Dirichlet $T(L) = 20^\circ\text{C}$ to an insulated Neumann condition $\frac{dT}{dx}(L) = 0$, how does the global stiffness matrix change?"
- **Outcome**: The AI correctly identified that the boundary row $N$ is no longer eliminated or set to identity; instead, the natural boundary condition $-k A \frac{dT}{dx}(L) = 0$ contributes zero flux to $f_N$, leaving $K_{N,N} = \frac{kA}{h}$ and $F_N = \frac{qAh}{2}$.

