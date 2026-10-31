#pragma once

#include <array>
#include <string>
#include <vector>

#include "../core/types.h"

namespace workbench {

/// Parsed CSV temperature data
struct CsvData {
    std::vector<TemperatureNode> nodes;
    double temp_min = 0.0;
    double temp_max = 0.0;
    std::array<double, 3> bounds_min{0.0, 0.0, 0.0};
    std::array<double, 3> bounds_max{0.0, 0.0, 0.0};

    int node_count() const { return static_cast<int>(nodes.size()); }
};

/// Result of CSV parsing
struct CsvParseResult {
    bool success = false;
    std::string error;
    CsvData data;
};

/// Parse ANSYS nodal temperature CSV from string content
/// Expected columns: node_id,x_m,y_m,z_m,temperature_C
CsvParseResult parse_temperature_csv(const std::string& content);

/// Parse from file path
CsvParseResult parse_temperature_csv_file(const std::string& filepath);

} // namespace workbench
