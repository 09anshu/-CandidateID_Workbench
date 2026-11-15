#include "heatsink_mesh.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <algorithm>
#include <cassert>

namespace workbench {

namespace {

// Hash for sorted 3-node face
struct FaceKey {
    int v[3];
    bool operator==(const FaceKey& o) const {
        return v[0] == o.v[0] && v[1] == o.v[1] && v[2] == o.v[2];
    }
};

struct FaceKeyHash {
    std::size_t operator()(const FaceKey& k) const {
        std::size_t h1 = std::hash<int>()(k.v[0]);
        std::size_t h2 = std::hash<int>()(k.v[1]);
        std::size_t h3 = std::hash<int>()(k.v[2]);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

FaceKey make_face_key(int a, int b, int c) {
    int s[3] = {a, b, c};
    std::sort(s, s + 3);
    return {s[0], s[1], s[2]};
}

} // anonymous namespace

bool HeatsinkMesh::generate(MeshResolution res) {
    nodes.clear();
    elements.clear();
    boundary_faces.clear();

    // 1. Grid resolution parameters
    int nx = 12;
    int nz_base = 2;
    int nz_fin = 4;
    int sub_margin = 1;
    int sub_fin = 1;
    int sub_gap = 1;

    switch (res) {
        case MeshResolution::Coarse:
            nx = 12;
            nz_base = 2;
            nz_fin = 4;
            sub_margin = 1;
            sub_fin = 1;
            sub_gap = 1;
            break;
        case MeshResolution::Medium:
            nx = 24;
            nz_base = 3;
            nz_fin = 8;
            sub_margin = 1;
            sub_fin = 1;
            sub_gap = 2;
            break;
        case MeshResolution::Fine:
            nx = 36;
            nz_base = 4;
            nz_fin = 12;
            sub_margin = 2;
            sub_fin = 2;
            sub_gap = 3;
            break;
        case MeshResolution::VeryFine:
            nx = 48;
            nz_base = 5;
            nz_fin = 16;
            sub_margin = 2;
            sub_fin = 2;
            sub_gap = 4;
            break;
    }

    // 2. Build coordinate partitions
    // X coordinates: [0, base_length]
    std::vector<double> x_coords(nx + 1);
    for (int i = 0; i <= nx; ++i) {
        x_coords[i] = (static_cast<double>(i) / nx) * base_length;
    }

    // Y coordinates: margins (2mm), 10 fins (2mm each), 9 gaps (4mm each)
    // Structure: Margin1, Fin0, Gap0, Fin1, Gap1, ..., Fin9, Margin2 (total 21 segments)
    struct YSegment {
        double y_start;
        double y_end;
        bool is_fin;
        int num_sub;
    };

    std::vector<YSegment> y_segments;
    double cur_y = 0.0;
    const double margin_w = 0.002;
    const double fin_w    = 0.002;
    const double gap_w    = 0.004;

    // Margin 1
    y_segments.push_back({cur_y, cur_y + margin_w, false, sub_margin});
    cur_y += margin_w;

    for (int f = 0; f < num_fins; ++f) {
        // Fin f
        y_segments.push_back({cur_y, cur_y + fin_w, true, sub_fin});
        cur_y += fin_w;

        // Gap after fin (except after last fin)
        if (f < num_fins - 1) {
            y_segments.push_back({cur_y, cur_y + gap_w, false, sub_gap});
            cur_y += gap_w;
        }
    }

    // Margin 2
    y_segments.push_back({cur_y, cur_y + margin_w, false, sub_margin});
    cur_y += margin_w;

    // Build flattened Y coordinates and track which intervals belong to fins
    std::vector<double> y_coords;
    std::vector<bool> y_interval_is_fin;

    y_coords.push_back(y_segments[0].y_start);
    for (const auto& seg : y_segments) {
        double dy = (seg.y_end - seg.y_start) / seg.num_sub;
        for (int s = 1; s <= seg.num_sub; ++s) {
            y_coords.push_back(seg.y_start + s * dy);
            y_interval_is_fin.push_back(seg.is_fin);
        }
    }
    int ny = static_cast<int>(y_interval_is_fin.size());

    // Z coordinates
    // Base Z: [0, base_height]
    std::vector<double> z_coords_base(nz_base + 1);
    for (int k = 0; k <= nz_base; ++k) {
        z_coords_base[k] = (static_cast<double>(k) / nz_base) * base_height;
    }

    // Fin Z: [base_height, base_height + fin_height]
    std::vector<double> z_coords_fin(nz_fin + 1);
    for (int k = 0; k <= nz_fin; ++k) {
        z_coords_fin[k] = base_height + (static_cast<double>(k) / nz_fin) * fin_height;
    }

    // 3. Generate Nodes with unique indexing
    // Base nodes: (i, j, k) for i in [0, nx], j in [0, ny], k in [0, nz_base]
    std::vector<std::vector<std::vector<int>>> base_node_idx(
        nx + 1, std::vector<std::vector<int>>(ny + 1, std::vector<int>(nz_base + 1, -1)));

    for (int i = 0; i <= nx; ++i) {
        for (int j = 0; j <= ny; ++j) {
            for (int k = 0; k <= nz_base; ++k) {
                base_node_idx[i][j][k] = static_cast<int>(nodes.size());
                nodes.push_back(Point3D(x_coords[i], y_coords[j], z_coords_base[k]));
            }
        }
    }

    // Fin nodes: (i, j, k) for i in [0, nx], j in fin ranges, k in [1, nz_fin]
    // Note: at k = 0 in fin Z, it reuses the base top node at k = nz_base!
    std::vector<std::vector<std::vector<int>>> fin_node_idx(
        nx + 1, std::vector<std::vector<int>>(ny + 1, std::vector<int>(nz_fin + 1, -1)));

    for (int i = 0; i <= nx; ++i) {
        for (int j = 0; j <= ny; ++j) {
            // Check if this j node touches any fin interval
            bool node_in_fin = false;
            if (j > 0 && y_interval_is_fin[j - 1]) node_in_fin = true;
            if (j < ny && y_interval_is_fin[j]) node_in_fin = true;

            if (!node_in_fin) continue;

            // At k = 0, reuse base top node
            fin_node_idx[i][j][0] = base_node_idx[i][j][nz_base];

            // For k >= 1, create new node
            for (int k = 1; k <= nz_fin; ++k) {
                fin_node_idx[i][j][k] = static_cast<int>(nodes.size());
                nodes.push_back(Point3D(x_coords[i], y_coords[j], z_coords_fin[k]));
            }
        }
    }

    // Helper lambda to decompose a hex cell into 5 tetrahedra
    auto add_hex_tets = [&](const int c[8], int parity) {
        // c contains local corners:
        // 0: (x0, y0, z0), 1: (x1, y0, z0), 2: (x1, y1, z0), 3: (x0, y1, z0)
        // 4: (x0, y0, z1), 5: (x1, y0, z1), 6: (x1, y1, z1), 7: (x0, y1, z1)
        int tets[5][4];
        if (parity == 0) {
            tets[0][0] = c[0]; tets[0][1] = c[1]; tets[0][2] = c[3]; tets[0][3] = c[4];
            tets[1][0] = c[1]; tets[1][1] = c[2]; tets[1][2] = c[3]; tets[1][3] = c[6];
            tets[2][0] = c[1]; tets[2][1] = c[4]; tets[2][2] = c[5]; tets[2][3] = c[6];
            tets[3][0] = c[3]; tets[3][1] = c[4]; tets[3][2] = c[6]; tets[3][3] = c[7];
            tets[4][0] = c[1]; tets[4][1] = c[3]; tets[4][2] = c[4]; tets[4][3] = c[6];
        } else {
            tets[0][0] = c[0]; tets[0][1] = c[1]; tets[0][2] = c[2]; tets[0][3] = c[5];
            tets[1][0] = c[0]; tets[1][1] = c[2]; tets[1][2] = c[3]; tets[1][3] = c[7];
            tets[2][0] = c[0]; tets[2][1] = c[4]; tets[2][2] = c[5]; tets[2][3] = c[7];
            tets[3][0] = c[2]; tets[3][1] = c[5]; tets[3][2] = c[6]; tets[3][3] = c[7];
            tets[4][0] = c[2]; tets[4][1] = c[0]; tets[4][2] = c[5]; tets[4][3] = c[7];
        }

        for (int t = 0; t < 5; ++t) {
            TetElement tet;
            tet.v[0] = tets[t][0];
            tet.v[1] = tets[t][1];
            tet.v[2] = tets[t][2];
            tet.v[3] = tets[t][3];

            // Compute volume
            const Point3D& p0 = nodes[tet.v[0]];
            const Point3D& p1 = nodes[tet.v[1]];
            const Point3D& p2 = nodes[tet.v[2]];
            const Point3D& p3 = nodes[tet.v[3]];
            Point3D e1 = p1 - p0;
            Point3D e2 = p2 - p0;
            Point3D e3 = p3 - p0;
            double vol = (e1.dot(e2.cross(e3))) / 6.0;
            tet.volume = std::abs(vol);
            elements.push_back(tet);
        }
    };

    // 4. Generate Base Elements
    for (int i = 0; i < nx; ++i) {
        for (int j = 0; j < ny; ++j) {
            for (int k = 0; k < nz_base; ++k) {
                int c[8] = {
                    base_node_idx[i][j][k],
                    base_node_idx[i+1][j][k],
                    base_node_idx[i+1][j+1][k],
                    base_node_idx[i][j+1][k],
                    base_node_idx[i][j][k+1],
                    base_node_idx[i+1][j][k+1],
                    base_node_idx[i+1][j+1][k+1],
                    base_node_idx[i][j+1][k+1]
                };
                int parity = (i + j + k) % 2;
                add_hex_tets(c, parity);
            }
        }
    }

    // 5. Generate Fin Elements
    for (int i = 0; i < nx; ++i) {
        for (int j = 0; j < ny; ++j) {
            if (!y_interval_is_fin[j]) continue;

            for (int k = 0; k < nz_fin; ++k) {
                int c[8] = {
                    fin_node_idx[i][j][k],
                    fin_node_idx[i+1][j][k],
                    fin_node_idx[i+1][j+1][k],
                    fin_node_idx[i][j+1][k],
                    fin_node_idx[i][j][k+1],
                    fin_node_idx[i+1][j][k+1],
                    fin_node_idx[i+1][j+1][k+1],
                    fin_node_idx[i][j+1][k+1]
                };
                int parity = (i + j + (nz_base + k)) % 2;
                add_hex_tets(c, parity);
            }
        }
    }

    // 6. Extract boundary faces using Face hash table
    struct FaceUsage {
        int count = 0;
        int tet_idx = -1;
        int v[3]; // Oriented vertices
    };

    std::unordered_map<FaceKey, FaceUsage, FaceKeyHash> face_map;

    // Standard local face definitions of a tet (ordered counter-clockwise from outside)
    const int tet_faces[4][3] = {
        {1, 2, 3}, // Opposite node 0
        {0, 3, 2}, // Opposite node 1
        {0, 1, 3}, // Opposite node 2
        {0, 2, 1}  // Opposite node 3
    };

    for (size_t t = 0; t < elements.size(); ++t) {
        const auto& el = elements[t];
        for (int f = 0; f < 4; ++f) {
            int n0 = el.v[tet_faces[f][0]];
            int n1 = el.v[tet_faces[f][1]];
            int n2 = el.v[tet_faces[f][2]];

            FaceKey key = make_face_key(n0, n1, n2);
            auto& usage = face_map[key];
            usage.count++;
            usage.tet_idx = static_cast<int>(t);
            usage.v[0] = n0;
            usage.v[1] = n1;
            usage.v[2] = n2;
        }
    }

    // 7. Filter and classify boundary faces (count == 1)
    for (const auto& kv : face_map) {
        if (kv.second.count != 1) continue; // Internal face

        BoundaryFace bf;
        bf.v[0] = kv.second.v[0];
        bf.v[1] = kv.second.v[1];
        bf.v[2] = kv.second.v[2];

        const Point3D& p0 = nodes[bf.v[0]];
        const Point3D& p1 = nodes[bf.v[1]];
        const Point3D& p2 = nodes[bf.v[2]];

        bf.centroid = (p0 + p1 + p2) * (1.0 / 3.0);
        Point3D cross = (p1 - p0).cross(p2 - p0);
        double area2 = cross.norm();
        bf.area = 0.5 * area2;
        if (area2 > 1e-14) {
            bf.normal = cross * (1.0 / area2);
        }

        // Verify normal points away from the 4th node of the parent tet
        const auto& parent_el = elements[kv.second.tet_idx];
        int fourth_node = -1;
        for (int n : parent_el.v) {
            if (n != bf.v[0] && n != bf.v[1] && n != bf.v[2]) {
                fourth_node = n;
                break;
            }
        }
        if (fourth_node != -1) {
            Point3D to_centroid = bf.centroid - nodes[fourth_node];
            if (bf.normal.dot(to_centroid) < 0.0) {
                // Flip orientation to guarantee outward normal
                std::swap(bf.v[1], bf.v[2]);
                bf.normal = bf.normal * (-1.0);
            }
        }

        // Classify boundary type
        if (bf.centroid.z < 1e-5) {
            // Bottom face at Z = 0
            if (bf.centroid.x >= source_x_min && bf.centroid.x <= source_x_max &&
                bf.centroid.y >= source_y_min && bf.centroid.y <= source_y_max) {
                bf.type = BoundaryType::HeatFlux;
            } else {
                bf.type = BoundaryType::Convection;
            }
        } else {
            // All other exposed surfaces (fins, sides, gaps)
            bf.type = BoundaryType::Convection;
        }

        boundary_faces.push_back(bf);
    }

    compute_mesh_stats();
    return true;
}

void HeatsinkMesh::compute_mesh_stats() {
    stats.node_count = static_cast<int>(nodes.size());
    stats.element_count = static_cast<int>(elements.size());
    stats.boundary_face_count = static_cast<int>(boundary_faces.size());
    stats.matrix_dimension = stats.node_count;

    stats.total_volume = 0.0;
    for (const auto& el : elements) {
        stats.total_volume += el.volume;
    }

    stats.total_area = 0.0;
    stats.heat_source_area = 0.0;
    stats.convection_area = 0.0;
    stats.heat_flux_face_count = 0;
    stats.convection_face_count = 0;

    for (const auto& bf : boundary_faces) {
        stats.total_area += bf.area;
        if (bf.type == BoundaryType::HeatFlux) {
            stats.heat_source_area += bf.area;
            stats.heat_flux_face_count++;
        } else {
            stats.convection_area += bf.area;
            stats.convection_face_count++;
        }
    }

    // Estimate nonzeros: on average each node in a 3D tet mesh connects to ~14-16 neighbors
    // We will compute exact nonzeros during assembly
    stats.estimated_nonzeros = stats.node_count * 15;
}

bool HeatsinkMesh::export_stl(const std::string& filepath) const {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << "solid Heatsink_3D\n";
    for (const auto& bf : boundary_faces) {
        out << "  facet normal " << std::scientific << std::setprecision(6)
            << bf.normal.x << " " << bf.normal.y << " " << bf.normal.z << "\n";
        out << "    outer loop\n";
        for (int i = 0; i < 3; ++i) {
            const auto& p = nodes[bf.v[i]];
            out << "      vertex " << p.x << " " << p.y << " " << p.z << "\n";
        }
        out << "    endloop\n";
        out << "  endfacet\n";
    }
    out << "endsolid Heatsink_3D\n";
    return true;
}

bool HeatsinkMesh::export_vtk(const std::string& filepath, const std::vector<double>& temperatures) const {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << "# vtk DataFile Version 3.0\n";
    out << "Heatsink 3D Thermal Simulation\n";
    out << "ASCII\n";
    out << "DATASET UNSTRUCTURED_GRID\n";

    // Points
    out << "POINTS " << nodes.size() << " double\n";
    for (const auto& p : nodes) {
        out << p.x << " " << p.y << " " << p.z << "\n";
    }

    // Cells: 4 nodes per tet + count = 5 integers per cell
    size_t total_ints = elements.size() * 5;
    out << "CELLS " << elements.size() << " " << total_ints << "\n";
    for (const auto& el : elements) {
        out << "4 " << el.v[0] << " " << el.v[1] << " " << el.v[2] << " " << el.v[3] << "\n";
    }

    // Cell types: 10 = VTK_TETRA
    out << "CELL_TYPES " << elements.size() << "\n";
    for (size_t i = 0; i < elements.size(); ++i) {
        out << "10\n";
    }

    // Point Data: Temperature
    if (temperatures.size() == nodes.size()) {
        out << "POINT_DATA " << nodes.size() << "\n";
        out << "SCALARS Temperature double 1\n";
        out << "LOOKUP_TABLE default\n";
        for (double t : temperatures) {
            out << t << "\n";
        }
    }

    return true;
}

bool HeatsinkMesh::export_csv(const std::string& filepath, const std::vector<double>& temperatures) const {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << "node_id,x,y,z,temperature\n";
    out << std::fixed << std::setprecision(6);
    for (size_t i = 0; i < nodes.size(); ++i) {
        double t = (i < temperatures.size()) ? temperatures[i] : 0.0;
        out << i << "," << nodes[i].x << "," << nodes[i].y << "," << nodes[i].z << "," << t << "\n";
    }
    return true;
}

} // namespace workbench
