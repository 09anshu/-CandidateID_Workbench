#pragma once

#include <cmath>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include <array>

namespace workbench {

/// Simple matrix using row-major vector of vectors
struct Matrix {
    std::vector<std::vector<double>> data;
    int rows = 0;
    int cols = 0;

    Matrix() = default;
    Matrix(int r, int c, double val = 0.0)
        : data(r, std::vector<double>(c, val)), rows(r), cols(c) {}

    std::vector<double>& operator[](int i) { return data[i]; }
    const std::vector<double>& operator[](int i) const { return data[i]; }

    void resize(int r, int c, double val = 0.0) {
        rows = r;
        cols = c;
        data.assign(r, std::vector<double>(c, val));
    }
};

/// Result type for operations that can succeed or fail
template<typename T>
struct Result {
    T value;
    bool success = false;
    std::string error;

    static Result ok(const T& val) {
        Result r;
        r.value = val;
        r.success = true;
        return r;
    }
    static Result ok(T&& val) {
        Result r;
        r.value = std::move(val);
        r.success = true;
        return r;
    }
    static Result fail(const std::string& err) {
        Result r;
        r.success = false;
        r.error = err;
        return r;
    }
};

/// 3D Point
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

/// Triangle facet for STL geometry
struct Triangle {
    std::array<double, 3> normal{0.0, 0.0, 0.0};
    std::array<double, 3> v1{0.0, 0.0, 0.0};
    std::array<double, 3> v2{0.0, 0.0, 0.0};
    std::array<double, 3> v3{0.0, 0.0, 0.0};
};

/// Nodal temperature point from CSV
struct TemperatureNode {
    int node_id = 0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double temperature = 0.0;
};

/// 3D Bounding Box
struct BoundingBox {
    std::array<double, 3> min_pt{0.0, 0.0, 0.0};
    std::array<double, 3> max_pt{0.0, 0.0, 0.0};
};

/// Physical and simulation constants
constexpr double PI = 3.14159265358979323846;
constexpr double TOLERANCE = 1e-10;

/// Check if a value is finite (not NaN, not Inf)
inline bool is_finite(double v) {
    return std::isfinite(v);
}

/// Helper to serialize doubles to precise string
inline std::string to_string_precise(double v, int precision = 8) {
    std::ostringstream oss;
    oss.precision(precision);
    oss << v;
    return oss.str();
}

} // namespace workbench
