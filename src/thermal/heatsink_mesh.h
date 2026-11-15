#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <memory>
#include <array>

namespace workbench {

struct Point3D {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    Point3D() = default;
    Point3D(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    Point3D operator-(const Point3D& o) const {
        return {x - o.x, y - o.y, z - o.z};
    }
    Point3D operator+(const Point3D& o) const {
        return {x + o.x, y + o.y, z + o.z};
    }
    Point3D operator*(double s) const {
        return {x * s, y * s, z * s};
    }
    double dot(const Point3D& o) const {
        return x * o.x + y * o.y + z * o.z;
    }
    Point3D cross(const Point3D& o) const {
        return {
            y * o.z - z * o.y,
            z * o.x - x * o.z,
            x * o.y - y * o.x
        };
    }
    double norm() const {
        return std::sqrt(x * x + y * y + z * z);
    }
};

struct TetElement {
    int v[4]; // 4 node indices
    double volume = 0.0;
};

enum class BoundaryType {
    Internal = 0,
    Convection = 1, // Exposed heat sink surfaces
    HeatFlux = 2    // Base heat source
};

struct BoundaryFace {
    int v[3]; // 3 node indices (ordered for outward normal)
    BoundaryType type = BoundaryType::Convection;
    double area = 0.0;
    Point3D normal;
    Point3D centroid;
};

enum class MeshResolution {
    Coarse,
    Medium,
    Fine,
    VeryFine
};

struct MeshStats {
    int node_count = 0;
    int element_count = 0;
    int boundary_face_count = 0;
    int convection_face_count = 0;
    int heat_flux_face_count = 0;
    double total_volume = 0.0;
    double total_area = 0.0;
    double heat_source_area = 0.0;
    double convection_area = 0.0;
    int matrix_dimension = 0;
    int estimated_nonzeros = 0;
};

class HeatsinkMesh {
public:
    HeatsinkMesh() = default;

    // Dimensions in meters
    double base_length = 0.100;  // 100 mm (X)
    double base_width  = 0.060;  // 60 mm (Y)
    double base_height = 0.008;  // 8 mm (Z)
    int    num_fins    = 10;
    double fin_height  = 0.030;  // 30 mm (Z: 0.008 to 0.038)
    double fin_thick   = 0.002;  // 2 mm

    // Heat source region on bottom base (Z = 0)
    double source_x_min = 0.030; // 30 mm to 70 mm (40 mm width)
    double source_x_max = 0.070;
    double source_y_min = 0.015; // 15 mm to 45 mm (30 mm height)
    double source_y_max = 0.045;

    std::vector<Point3D> nodes;
    std::vector<TetElement> elements;
    std::vector<BoundaryFace> boundary_faces;
    MeshStats stats;

    int num_nodes() const { return static_cast<int>(nodes.size()); }
    int num_elements() const { return static_cast<int>(elements.size()); }

    // Generate mesh according to specified resolution
    bool generate(MeshResolution res);

    // Export surface to ASCII STL file
    bool export_stl(const std::string& filepath) const;

    // Export volume mesh with temperatures to VTK legacy format
    bool export_vtk(const std::string& filepath, const std::vector<double>& temperatures) const;

    // Export nodal coordinates and temperatures to CSV
    bool export_csv(const std::string& filepath, const std::vector<double>& temperatures) const;

private:
    void compute_mesh_stats();
};

} // namespace workbench
