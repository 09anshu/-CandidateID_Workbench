#pragma once

#include <array>
#include <string>
#include <vector>

#include "../core/types.h"

namespace workbench {

/// Parsed STL model
struct StlModel {
    std::string name;
    std::vector<Triangle> triangles;
    std::array<double, 3> bounds_min{0.0, 0.0, 0.0};
    std::array<double, 3> bounds_max{0.0, 0.0, 0.0};

    int triangle_count() const { return static_cast<int>(triangles.size()); }
};

/// Result of STL parsing
struct StlParseResult {
    bool success = false;
    std::string error;
    StlModel model;
};

/// Parse an ASCII STL file from a string
StlParseResult parse_stl(const std::string& content);

/// Parse an ASCII STL file from a file path
StlParseResult parse_stl_file(const std::string& filepath);

} // namespace workbench
