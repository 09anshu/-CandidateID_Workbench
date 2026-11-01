#pragma once

#include <string>
#include <vector>
#include <atomic>

namespace workbench {

/// Individual test case result
struct TestCaseResult {
    std::string suite_name;
    std::string test_name;
    double duration_ms = 0.0;
    std::string status = "PASSED"; // "PASSED", "FAILED", "SKIPPED"
    std::string failure_message;
};

/// Aggregated test suite run result
struct TestSuiteRunResult {
    bool executed = false;
    int exit_code = 0;
    int total = 0;
    int passed = 0;
    int failed = 0;
    int skipped = 0;
    double duration_ms = 0.0;
    std::string timestamp;
    std::string console_output;
    std::string xml_path = "results/test_report.xml";
    std::string log_path = "results/test_execution.log";
    std::vector<TestCaseResult> tests;
    std::string error;
};

/// Check if tests are currently running
bool is_test_run_in_progress();

/// Execute the compiled test suite (thread-safe, non-concurrent, zero arbitrary shell injection)
TestSuiteRunResult execute_test_suite();

/// Get the cached or latest test results from disk
TestSuiteRunResult load_latest_test_results();

/// Convert test results to JSON
std::string test_results_to_json(const TestSuiteRunResult& res);

} // namespace workbench

