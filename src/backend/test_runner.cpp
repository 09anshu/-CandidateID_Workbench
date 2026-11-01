#include "test_runner.h"
#include "json_helper.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <mutex>
#include <regex>

#ifdef _WIN32
#include <io.h>
#define popen_cmd _popen
#define pclose_cmd _pclose
#else
#include <unistd.h>
#define popen_cmd popen
#define pclose_cmd pclose
#endif

namespace workbench {

static std::atomic<bool> s_running{false};
static std::mutex s_cache_mutex;
static TestSuiteRunResult s_cached_result;

bool is_test_run_in_progress() {
    return s_running.load();
}

static std::string get_iso_timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &tt);
#else
    localtime_r(&tt, &tm_buf);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
    return std::string(buf);
}

static bool file_exists(const std::string& path) {
    std::ifstream f(path.c_str());
    return f.good();
}

static std::string locate_test_binary() {
    std::vector<std::string> candidates = {
        "workbench_gtests.exe",
        "build/workbench_gtests.exe",
        "workbench_gtests",
        "build/workbench_gtests",
        "workbench_tests.exe",
        "build/workbench_tests.exe",
        "workbench_tests"
    };

    for (const auto& path : candidates) {
        if (file_exists(path)) {
            return path;
        }
    }
    return "";
}

// Simple XML attribute reader: extracts value of attr="value" from a tag string
static std::string extract_xml_attr(const std::string& tag, const std::string& attr) {
    std::string needle = attr + "=\"";
    size_t pos = tag.find(needle);
    if (pos == std::string::npos) {
        needle = attr + "='";
        pos = tag.find(needle);
        if (pos == std::string::npos) return "";
    }
    size_t val_start = pos + needle.size();
    char quote = tag[val_start - 1];
    size_t val_end = tag.find(quote, val_start);
    if (val_end == std::string::npos) return "";
    return tag.substr(val_start, val_end - val_start);
}

static void parse_gtest_xml(const std::string& xml_content, TestSuiteRunResult& result) {
    result.tests.clear();

    // Find all <testcase ...> tags and any nested <failure ...>
    size_t pos = 0;
    while ((pos = xml_content.find("<testcase", pos)) != std::string::npos) {
        size_t tag_end = xml_content.find(">", pos);
        if (tag_end == std::string::npos) break;

        std::string tag = xml_content.substr(pos, tag_end - pos + 1);
        TestCaseResult tc;
        tc.test_name = extract_xml_attr(tag, "name");
        tc.suite_name = extract_xml_attr(tag, "classname");
        if (tc.suite_name.empty()) {
            tc.suite_name = extract_xml_attr(tag, "suite");
        }

        std::string time_str = extract_xml_attr(tag, "time");
        if (!time_str.empty()) {
            try {
                tc.duration_ms = std::stod(time_str) * 1000.0;
            } catch (...) {
                tc.duration_ms = 0.0;
            }
        }

        // Check if self-closing or contains child elements
        if (tag.rfind("/>") == tag.size() - 2) {
            tc.status = "PASSED";
            pos = tag_end + 1;
        } else {
            size_t close_pos = xml_content.find("</testcase>", tag_end);
            if (close_pos != std::string::npos) {
                std::string body = xml_content.substr(tag_end + 1, close_pos - tag_end - 1);
                size_t fail_pos = body.find("<failure");
                if (fail_pos != std::string::npos) {
                    tc.status = "FAILED";
                    size_t fail_end = body.find(">", fail_pos);
                    if (fail_end != std::string::npos) {
                        std::string fail_tag = body.substr(fail_pos, fail_end - fail_pos + 1);
                        tc.failure_message = extract_xml_attr(fail_tag, "message");
                        size_t fail_close = body.find("</failure>", fail_end);
                        if (fail_close != std::string::npos) {
                            std::string text_fail = body.substr(fail_end + 1, fail_close - fail_end - 1);
                            if (!text_fail.empty()) {
                                if (!tc.failure_message.empty()) tc.failure_message += "\n";
                                tc.failure_message += text_fail;
                            }
                        }
                    }
                } else if (body.find("<skipped") != std::string::npos || body.find("<disabled") != std::string::npos) {
                    tc.status = "SKIPPED";
                } else {
                    tc.status = "PASSED";
                }
                pos = close_pos + 11;
            } else {
                tc.status = "PASSED";
                pos = tag_end + 1;
            }
        }

        result.tests.push_back(tc);
    }

    // Re-count from parsed tests
    if (!result.tests.empty()) {
        result.total = static_cast<int>(result.tests.size());
        result.passed = 0;
        result.failed = 0;
        result.skipped = 0;
        for (const auto& tc : result.tests) {
            if (tc.status == "PASSED") result.passed++;
            else if (tc.status == "FAILED") result.failed++;
            else if (tc.status == "SKIPPED") result.skipped++;
        }
    }
}

static void parse_console_fallback(const std::string& console, TestSuiteRunResult& result) {
    std::istringstream iss(console);
    std::string line;
    result.tests.clear();
    result.total = 0;
    result.passed = 0;
    result.failed = 0;
    result.skipped = 0;

    std::string current_suite = "General";
    while (std::getline(iss, line)) {
        if (line.find("[----------] ") != std::string::npos && line.find("tests from ") != std::string::npos) {
            size_t s = line.find("tests from ");
            if (s != std::string::npos) {
                current_suite = line.substr(s + 11);
                size_t space = current_suite.find(" ");
                if (space != std::string::npos) current_suite = current_suite.substr(0, space);
            }
        } else if (line.find("[       OK ] ") != std::string::npos) {
            size_t s = line.find("[       OK ] ");
            std::string tname = line.substr(s + 13);
            size_t space = tname.find(" ");
            if (space != std::string::npos) tname = tname.substr(0, space);
            TestCaseResult tc;
            size_t dot = tname.find(".");
            if (dot != std::string::npos) {
                tc.suite_name = tname.substr(0, dot);
                tc.test_name = tname.substr(dot + 1);
            } else {
                tc.suite_name = current_suite;
                tc.test_name = tname;
            }
            tc.status = "PASSED";
            result.tests.push_back(tc);
            result.passed++;
        } else if (line.find("[  FAILED  ] ") != std::string::npos && line.find("test, listed below") == std::string::npos && line.find("FAILED TEST") == std::string::npos) {
            size_t s = line.find("[  FAILED  ] ");
            std::string tname = line.substr(s + 13);
            size_t space = tname.find(" ");
            if (space != std::string::npos) tname = tname.substr(0, space);
            TestCaseResult tc;
            size_t dot = tname.find(".");
            if (dot != std::string::npos) {
                tc.suite_name = tname.substr(0, dot);
                tc.test_name = tname.substr(dot + 1);
            } else {
                tc.suite_name = current_suite;
                tc.test_name = tname;
            }
            tc.status = "FAILED";
            result.tests.push_back(tc);
            result.failed++;
        } else if (line.find("[PASS] ") != std::string::npos) {
            TestCaseResult tc;
            tc.suite_name = "CustomSuite";
            tc.test_name = line.substr(line.find("[PASS] ") + 7);
            tc.status = "PASSED";
            result.tests.push_back(tc);
            result.passed++;
        } else if (line.find("[FAIL] ") != std::string::npos) {
            TestCaseResult tc;
            tc.suite_name = "CustomSuite";
            tc.test_name = line.substr(line.find("[FAIL] ") + 7);
            tc.status = "FAILED";
            result.tests.push_back(tc);
            result.failed++;
        }
    }
    result.total = static_cast<int>(result.tests.size());
}

TestSuiteRunResult execute_test_suite() {
    // Concurrency Guard: prevent concurrent test executions
    bool expected = false;
    if (!s_running.compare_exchange_strong(expected, true)) {
        TestSuiteRunResult busy_res;
        busy_res.executed = false;
        busy_res.error = "Test execution already in progress. Please wait for the current run to finish.";
        return busy_res;
    }

    struct RunGuard {
        ~RunGuard() { s_running.store(false); }
    } guard;

    TestSuiteRunResult result;
    result.timestamp = get_iso_timestamp();

    std::string bin_path = locate_test_binary();
    if (bin_path.empty()) {
        result.executed = false;
        result.error = "Test binary 'workbench_gtests.exe' or 'workbench_tests.exe' not found. Please compile the test suite first.";
        return result;
    }

    std::string xml_rel = "results/test_report.xml";
    std::string log_rel = "results/test_execution.log";
    result.xml_path = xml_rel;
    result.log_path = log_rel;

    // Secure command execution: fixed binary path with fixed argument. Zero user shell injection.
    std::string command;
    if (bin_path.find("gtests") != std::string::npos) {
        command = bin_path + " --gtest_output=xml:" + xml_rel + " 2>&1";
    } else {
        command = bin_path + " 2>&1";
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    FILE* pipe = popen_cmd(command.c_str(), "r");
    if (!pipe) {
        result.executed = false;
        result.error = "Failed to launch test executable process.";
        return result;
    }

    std::ostringstream console_ss;
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        console_ss << buffer;
    }

    int exit_code = pclose_cmd(pipe);
    auto end_time = std::chrono::high_resolution_clock::now();

    result.executed = true;
    result.exit_code = exit_code;
    result.duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    result.console_output = console_ss.str();

    // Save console output to results/test_execution.log
    {
        std::ofstream log_file(log_rel);
        if (log_file.is_open()) {
            log_file << "=================================================================\n";
            log_file << "PASS SCIENCE WORKS — Automated Test Suite Execution Log\n";
            log_file << "Timestamp: " << result.timestamp << "\n";
            log_file << "Binary: " << bin_path << "\n";
            log_file << "Exit Code: " << exit_code << "\n";
            log_file << "Execution Time: " << std::fixed << std::setprecision(2) << result.duration_ms << " ms\n";
            log_file << "=================================================================\n\n";
            log_file << result.console_output;
        }
    }

    // Attempt to parse XML report if present
    bool parsed_xml = false;
    if (file_exists(xml_rel)) {
        std::ifstream xml_file(xml_rel);
        if (xml_file.is_open()) {
            std::string xml_str((std::istreambuf_iterator<char>(xml_file)), std::istreambuf_iterator<char>());
            parse_gtest_xml(xml_str, result);
            if (!result.tests.empty()) {
                parsed_xml = true;
            }
        }
    }

    // If XML was missing or failed, parse console output
    if (!parsed_xml) {
        parse_console_fallback(result.console_output, result);
    }

    // Cache the result
    {
        std::lock_guard<std::mutex> lk(s_cache_mutex);
        s_cached_result = result;
    }

    return result;
}

TestSuiteRunResult load_latest_test_results() {
    {
        std::lock_guard<std::mutex> lk(s_cache_mutex);
        if (s_cached_result.executed && !s_cached_result.tests.empty()) {
            return s_cached_result;
        }
    }

    TestSuiteRunResult result;
    result.xml_path = "results/test_report.xml";
    result.log_path = "results/test_execution.log";

    if (file_exists(result.xml_path)) {
        std::ifstream xml_file(result.xml_path);
        if (xml_file.is_open()) {
            std::string xml_str((std::istreambuf_iterator<char>(xml_file)), std::istreambuf_iterator<char>());
            parse_gtest_xml(xml_str, result);
            result.executed = true;
        }
    }

    if (file_exists(result.log_path)) {
        std::ifstream log_file(result.log_path);
        if (log_file.is_open()) {
            result.console_output.assign((std::istreambuf_iterator<char>(log_file)), std::istreambuf_iterator<char>());
        }
    }

    if (result.executed) {
        result.timestamp = get_iso_timestamp();
    }

    return result;
}

std::string test_results_to_json(const TestSuiteRunResult& res) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "{"
       << "\"success\":" << (res.executed ? "true" : "false") << ","
       << "\"executed\":" << (res.executed ? "true" : "false") << ","
       << "\"exit_code\":" << res.exit_code << ","
       << "\"total\":" << res.total << ","
       << "\"passed\":" << res.passed << ","
       << "\"failed\":" << res.failed << ","
       << "\"skipped\":" << res.skipped << ","
       << "\"duration_ms\":" << res.duration_ms << ","
       << "\"timestamp\":\"" << json_escape(res.timestamp) << "\","
       << "\"xml_path\":\"" << json_escape(res.xml_path) << "\","
       << "\"log_path\":\"" << json_escape(res.log_path) << "\","
       << "\"error\":\"" << json_escape(res.error) << "\","
       << "\"console_output\":\"" << json_escape(res.console_output) << "\","
       << "\"tests\":[";

    for (size_t i = 0; i < res.tests.size(); ++i) {
        const auto& tc = res.tests[i];
        if (i > 0) ss << ",";
        ss << "{"
           << "\"suite\":\"" << json_escape(tc.suite_name) << "\","
           << "\"name\":\"" << json_escape(tc.test_name) << "\","
           << "\"status\":\"" << json_escape(tc.status) << "\","
           << "\"duration_ms\":" << tc.duration_ms << ","
           << "\"failure_message\":\"" << json_escape(tc.failure_message) << "\""
           << "}";
    }
    ss << "]}";

    return ss.str();
}

} // namespace workbench

