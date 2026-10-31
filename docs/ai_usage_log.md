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

---

## 3. Deliberate Challenge Test & Outcome

To rigorously validate model reasoning under subtle edge cases, a deliberate challenge test was conducted:
- **Challenge**: "If the right boundary condition is changed from Dirichlet $T(L) = 20^\circ\text{C}$ to an insulated Neumann condition $\frac{dT}{dx}(L) = 0$, how does the global stiffness matrix change?"
- **Outcome**: The AI correctly identified that the boundary row $N$ is no longer eliminated or set to identity; instead, the natural boundary condition $-k A \frac{dT}{dx}(L) = 0$ contributes zero flux to $f_N$, leaving $K_{N,N} = \frac{kA}{h}$ and $F_N = \frac{qAh}{2}$.
