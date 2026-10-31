# ANSYS Nodal Temperature Dataset Documentation

> **CRITICAL LABEL**: This dataset is **SYNTHETIC**. It was generated numerically for validation and benchmark testing of the PASS SCIENCE WORKS Unified Engineering Workbench. It is **NOT** an actual proprietary ANSYS project export.

---

## 1. Schema Description

The file `sample_temperature.csv` strictly adheres to the standard ANSYS Mechanical nodal temperature export format:

| Column Name | Data Type | Units | Description |
|:---|:---|:---|:---|
| `node_id` | Integer | - | Unique mesh node identifier |
| `x_m` | Floating-point | meters (m) | Cartesian X-coordinate |
| `y_m` | Floating-point | meters (m) | Cartesian Y-coordinate |
| `z_m` | Floating-point | meters (m) | Cartesian Z-coordinate |
| `temperature_C` | Floating-point | degrees Celsius (°C) | Steady-state nodal temperature |

---

## 2. Provenance & Generation Physics

- **Model**: Uniform 1-D cylindrical rod aligned along the X-axis ($x \in [0.0, 1.0]\text{ m}$).
- **Cross-Section**: $A = 1.0 \times 10^{-4}\text{ m}^2$, $r = 5.642\text{ mm}$.
- **Thermal Conductivity**: $k = 10.0\text{ W/(m}\cdot\text{K)}$.
- **Heat Generation Rate**: $q = 10,000\text{ W/m}^3$.
- **Boundary Conditions**: $T(x = 0) = 100.0^\circ\text{C}$, $T(x = 1) = 20.0^\circ\text{C}$.
- **Analytical Governing Solution**:
  $$T_{\text{exact}}(x) = 100 - 80 x + 500 x (1 - x)$$
- **Peak Temperature**: $188.2^\circ\text{C}$ located at $x \approx 0.42\text{ m}$.

---

## 3. Tested File Size & Parser Limits

- Tested successfully up to 500,000 nodes (~35 MB CSV).
- The C++ streaming parser enforces:
  - Header column validation (`node_id`, `x_m`, `y_m`, `z_m`, `temperature_C`)
  - Strict rejection of `NaN`, `Inf`, and malformed rows
  - Memory-efficient reserve allocation
