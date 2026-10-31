# ASCII STL Geometry Dataset Documentation

---

## 1. Provenance & Attribution

- **File**: `sample_cube.stl`
- **Source**: Self-created standard test geometry for PASS SCIENCE WORKS R&D Workbench.
- **License**: Creative Commons Zero (CC0) / Public Domain.
- **Format**: ASCII StereoLithography (STL).

---

## 2. Geometry Specifications

- **Solid Name**: `cube`
- **Dimensions**: $1.0\text{ m} \times 1.0\text{ m} \times 1.0\text{ m}$ (Unit Cube)
- **Bounding Box**:
  - $X \in [0.0, 1.0]\text{ m}$
  - $Y \in [0.0, 1.0]\text{ m}$
  - $Z \in [0.0, 1.0]\text{ m}$
- **Facet Count**: 12 planar triangular facets (2 triangles per each of the 6 cube faces).
- **Normals**: Right-hand rule outward-pointing unit normal vectors.
- **Vertex Ordering**: Counter-clockwise winding when viewed from exterior.

---

## 3. Assumptions & Limitations

- The parser strictly requires ASCII STL (`solid ... endsolid`).
- Binary STL files are detected and rejected with an explicit actionable error message advising the user to export as ASCII STL.
- Units are assumed to be meters (m) for engineering consistency with the thermal analysis module.
