#include "csv_parser.h"

#include <algorithm>
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

std::vector<std::string> split_csv_line(const std::string& line) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream token_stream(line);
    while (std::getline(token_stream, token, ',')) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

} // anonymous namespace

CsvParseResult parse_temperature_csv(const std::string& content) {
    CsvParseResult res;
    if (content.empty()) {
        res.success = false;
        res.error = "CSV parsing error: file is empty.";
        return res;
    }

    std::istringstream stream(content);
    std::string line;

    // Read header line
    if (!std::getline(stream, line)) {
        res.success = false;
        res.error = "CSV parsing error: unable to read header line.";
        return res;
    }

    std::string trimmed_header = trim(line);
    if (trimmed_header.empty()) {
        res.success = false;
        res.error = "CSV parsing error: header line is blank.";
        return res;
    }

    auto headers = split_csv_line(trimmed_header);
    // Find expected column indices
    int idx_node = -1, idx_x = -1, idx_y = -1, idx_z = -1, idx_temp = -1;
    for (size_t i = 0; i < headers.size(); ++i) {
        std::string col = headers[i];
        std::transform(col.begin(), col.end(), col.begin(), ::tolower);
        if (col == "node_id" || col == "nodeid" || col == "id") idx_node = static_cast<int>(i);
        else if (col == "x_m" || col == "x" || col == "x (m)") idx_x = static_cast<int>(i);
        else if (col == "y_m" || col == "y" || col == "y (m)") idx_y = static_cast<int>(i);
        else if (col == "z_m" || col == "z" || col == "z (m)") idx_z = static_cast<int>(i);
        else if (col == "temperature_c" || col == "temperature" || col == "temp_c" || col == "t_c") idx_temp = static_cast<int>(i);
    }

    if (idx_node == -1 || idx_x == -1 || idx_y == -1 || idx_z == -1 || idx_temp == -1) {
        res.success = false;
        res.error = "CSV parsing error: missing required columns. Required schema: node_id,x_m,y_m,z_m,temperature_C";
        return res;
    }

    double min_x = std::numeric_limits<double>::infinity();
    double min_y = std::numeric_limits<double>::infinity();
    double min_z = std::numeric_limits<double>::infinity();
    double max_x = -std::numeric_limits<double>::infinity();
    double max_y = -std::numeric_limits<double>::infinity();
    double max_z = -std::numeric_limits<double>::infinity();
    double min_t = std::numeric_limits<double>::infinity();
    double max_t = -std::numeric_limits<double>::infinity();

    int line_num = 1;
    while (std::getline(stream, line)) {
        ++line_num;
        std::string trimmed = trim(line);
        if (trimmed.empty()) continue;

        auto tokens = split_csv_line(trimmed);
        int max_req_idx = std::max({idx_node, idx_x, idx_y, idx_z, idx_temp});
        if (static_cast<int>(tokens.size()) <= max_req_idx) {
            res.success = false;
            res.error = "CSV parsing error at line " + std::to_string(line_num) + ": insufficient columns (expected at least " + std::to_string(max_req_idx + 1) + ", got " + std::to_string(tokens.size()) + ").";
            return res;
        }

        try {
            int node_id = std::stoi(tokens[idx_node]);
            double x = std::stod(tokens[idx_x]);
            double y = std::stod(tokens[idx_y]);
            double z = std::stod(tokens[idx_z]);
            double temp = std::stod(tokens[idx_temp]);

            if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || !std::isfinite(temp)) {
                res.success = false;
                res.error = "CSV parsing error at line " + std::to_string(line_num) + ": non-finite value encountered (NaN or Inf).";
                return res;
            }

            TemperatureNode node;
            node.node_id = node_id;
            node.x = x;
            node.y = y;
            node.z = z;
            node.temperature = temp;

            min_x = std::min(min_x, x);
            min_y = std::min(min_y, y);
            min_z = std::min(min_z, z);
            max_x = std::max(max_x, x);
            max_y = std::max(max_y, y);
            max_z = std::max(max_z, z);
            min_t = std::min(min_t, temp);
            max_t = std::max(max_t, temp);

            res.data.nodes.push_back(node);
        } catch (const std::exception& e) {
            res.success = false;
            res.error = "CSV parsing error at line " + std::to_string(line_num) + ": invalid numerical record (" + e.what() + ").";
            return res;
        }
    }

    if (res.data.nodes.empty()) {
        res.success = false;
        res.error = "CSV parsing error: file contains header but no data rows.";
        return res;
    }

    res.data.bounds_min = {min_x, min_y, min_z};
    res.data.bounds_max = {max_x, max_y, max_z};
    res.data.temp_min = min_t;
    res.data.temp_max = max_t;
    res.success = true;
    return res;
}

CsvParseResult parse_temperature_csv_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        CsvParseResult res;
        res.success = false;
        res.error = "Cannot open CSV file: " + filepath;
        return res;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    return parse_temperature_csv(content);
}

} // namespace workbench
