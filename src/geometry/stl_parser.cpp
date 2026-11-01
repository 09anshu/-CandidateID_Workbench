#include "stl_parser.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>

namespace workbench {

namespace {

std::string trim(const std::string& str) {
    auto first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    auto last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

bool is_binary_stl(const std::string& content) {
    if (content.size() < 6) return false;
    // If the file contains null bytes in the first 512 bytes, it's likely binary
    size_t check_len = std::min(content.size(), static_cast<size_t>(512));
    for (size_t i = 0; i < check_len; ++i) {
        if (content[i] == '\0') {
            return true;
        }
    }
    // Check if it doesn't start with "solid"
    std::string start = content.substr(0, std::min(content.size(), static_cast<size_t>(100)));
    std::istringstream iss(start);
    std::string first_token;
    iss >> first_token;
    std::transform(first_token.begin(), first_token.end(), first_token.begin(), ::tolower);
    if (first_token != "solid") {
        return true;
    }
    return false;
}

} // anonymous namespace

StlParseResult parse_stl(const std::string& content) {
    StlParseResult res;
    if (content.empty()) {
        res.success = false;
        res.error = "STL parsing error: file is empty.";
        return res;
    }

    if (is_binary_stl(content)) {
        res.success = false;
        res.error = "STL parsing error: binary STL format detected. Only ASCII STL geometry files are supported by this workbench.";
        return res;
    }

    std::istringstream stream(content);
    std::string line;
    std::string solid_name;
    bool found_solid = false;
    bool in_facet = false;
    bool in_loop = false;
    int vertex_count = 0;
    Triangle current_triangle;

    double min_x = std::numeric_limits<double>::infinity();
    double min_y = std::numeric_limits<double>::infinity();
    double min_z = std::numeric_limits<double>::infinity();
    double max_x = -std::numeric_limits<double>::infinity();
    double max_y = -std::numeric_limits<double>::infinity();
    double max_z = -std::numeric_limits<double>::infinity();

    int line_num = 0;

    while (std::getline(stream, line)) {
        ++line_num;
        std::string trimmed = trim(line);
        if (trimmed.empty()) continue;

        std::istringstream line_stream(trimmed);
        std::string token;
        line_stream >> token;

        std::string lower_token = token;
        std::transform(lower_token.begin(), lower_token.end(), lower_token.begin(), ::tolower);

        if (lower_token == "solid") {
            found_solid = true;
            std::getline(line_stream, solid_name);
            solid_name = trim(solid_name);
            if (solid_name.empty()) solid_name = "unnamed_solid";
            res.model.name = solid_name;
        } else if (lower_token == "facet") {
            if (!found_solid) {
                res.success = false;
                res.error = "STL parsing error at line " + std::to_string(line_num) + ": 'facet' encountered before 'solid'.";
                return res;
            }
            std::string normal_token;
            line_stream >> normal_token;
            double nx, ny, nz;
            if (!(line_stream >> nx >> ny >> nz) || !std::isfinite(nx) || !std::isfinite(ny) || !std::isfinite(nz)) {
                res.success = false;
                res.error = "STL parsing error at line " + std::to_string(line_num) + ": malformed or non-finite normal vector.";
                return res;
            }
            current_triangle.normal = {nx, ny, nz};
            in_facet = true;
            vertex_count = 0;
        } else if (lower_token == "outer") {
            std::string loop_token;
            line_stream >> loop_token;
            in_loop = true;
            vertex_count = 0;
        } else if (lower_token == "vertex") {
            if (!in_facet || !in_loop) {
                res.success = false;
                res.error = "STL parsing error at line " + std::to_string(line_num) + ": 'vertex' outside loop/facet.";
                return res;
            }
            double vx, vy, vz;
            if (!(line_stream >> vx >> vy >> vz) || !std::isfinite(vx) || !std::isfinite(vy) || !std::isfinite(vz)) {
                res.success = false;
                res.error = "STL parsing error at line " + std::to_string(line_num) + ": malformed or non-finite vertex coordinates.";
                return res;
            }
            if (vertex_count == 0) current_triangle.v1 = {vx, vy, vz};
            else if (vertex_count == 1) current_triangle.v2 = {vx, vy, vz};
            else if (vertex_count == 2) current_triangle.v3 = {vx, vy, vz};
            else {
                res.success = false;
                res.error = "STL parsing error at line " + std::to_string(line_num) + ": facet has more than 3 vertices.";
                return res;
            }

            min_x = std::min(min_x, vx);
            min_y = std::min(min_y, vy);
            min_z = std::min(min_z, vz);
            max_x = std::max(max_x, vx);
            max_y = std::max(max_y, vy);
            max_z = std::max(max_z, vz);

            ++vertex_count;
        } else if (lower_token == "endloop") {
            if (vertex_count != 3) {
                res.success = false;
                res.error = "STL parsing error at line " + std::to_string(line_num) + ": facet loop must contain exactly 3 vertices, found " + std::to_string(vertex_count);
                return res;
            }
            in_loop = false;
        } else if (lower_token == "endfacet") {
            if (!in_facet) {
                res.success = false;
                res.error = "STL parsing error at line " + std::to_string(line_num) + ": unexpected 'endfacet'.";
                return res;
            }
            res.model.triangles.push_back(current_triangle);
            in_facet = false;
        } else if (lower_token == "endsolid") {
            // End of solid
            break;
        }
    }

    if (!found_solid) {
        res.success = false;
        res.error = "STL parsing error: missing 'solid' keyword. File is not a valid ASCII STL.";
        return res;
    }

    if (res.model.triangles.empty()) {
        res.success = false;
        res.error = "STL parsing error: file contains solid definition but no valid triangle facets.";
        return res;
    }

    res.model.bounds_min = {min_x, min_y, min_z};
    res.model.bounds_max = {max_x, max_y, max_z};
    res.success = true;
    return res;
}

StlParseResult parse_stl_file(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        StlParseResult res;
        res.success = false;
        res.error = "Cannot open file: " + filepath;
        return res;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    return parse_stl(content);
}

} // namespace workbench

