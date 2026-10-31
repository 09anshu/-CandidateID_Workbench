#pragma once

#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace workbench {

/// Escapes a string for valid JSON output
inline std::string json_escape(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        switch (c) {
            case '"': o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\b': o << "\\b"; break;
            case '\f': o << "\\f"; break;
            case '\n': o << "\\n"; break;
            case '\r': o << "\\r"; break;
            case '\t': o << "\\t"; break;
            default:
                if ('\x00' <= c && c <= '\x1f') {
                    o << "\\u00" << std::hex << (int)c;
                } else {
                    o << c;
                }
        }
    }
    return o.str();
}

/// Simple JSON parser for basic key-value extraction
inline std::string extract_json_string(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) return "";

    pos = json.find(':', pos + pattern.size());
    if (pos == std::string::npos) return "";

    pos = json.find_first_not_of(" \t\r\n", pos + 1);
    if (pos == std::string::npos) return "";

    if (json[pos] == '"') {
        size_t end = json.find('"', pos + 1);
        while (end != std::string::npos && json[end - 1] == '\\') {
            end = json.find('"', end + 1);
        }
        if (end != std::string::npos) {
            std::string raw = json.substr(pos + 1, end - (pos + 1));
            // Unescape characters
            std::string unescaped;
            for (size_t i = 0; i < raw.size(); ++i) {
                if (raw[i] == '\\' && i + 1 < raw.size()) {
                    if (raw[i + 1] == 'n') { unescaped += '\n'; ++i; }
                    else if (raw[i + 1] == 'r') { unescaped += '\r'; ++i; }
                    else if (raw[i + 1] == 't') { unescaped += '\t'; ++i; }
                    else if (raw[i + 1] == '"') { unescaped += '"'; ++i; }
                    else if (raw[i + 1] == '\\') { unescaped += '\\'; ++i; }
                    else { unescaped += raw[i]; }
                } else {
                    unescaped += raw[i];
                }
            }
            return unescaped;
        }
    }
    return "";
}

inline double extract_json_double(const std::string& json, const std::string& key, double default_val = 0.0) {
    std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) return default_val;

    pos = json.find(':', pos + pattern.size());
    if (pos == std::string::npos) return default_val;

    pos = json.find_first_not_of(" \t\r\n", pos + 1);
    if (pos == std::string::npos) return default_val;

    size_t end = json.find_first_of(",}\n\r", pos);
    std::string val_str = (end == std::string::npos) ? json.substr(pos) : json.substr(pos, end - pos);

    try {
        return std::stod(val_str);
    } catch (...) {
        return default_val;
    }
}

inline int extract_json_int(const std::string& json, const std::string& key, int default_val = 0) {
    return static_cast<int>(extract_json_double(json, key, default_val));
}

} // namespace workbench
