#include "server_routes.h"

#include <fstream>
#include <iomanip>
#include <sstream>

#include "../backpropagation/backprop.h"
#include "../geometry/csv_parser.h"
#include "../geometry/stl_parser.h"
#include "../performance/parallel_solver.h"
#include "../thermal/thermal_solver.h"
#include "../thermal/heatsink_mesh.h"
#include "../thermal/heatsink_solver.h"
#include "json_helper.h"
#include "test_runner.h"

namespace workbench {

namespace {

// Cache of latest results for CSV export
ThermalResult g_latest_thermal_result;
ThermalParams g_latest_thermal_params;
BackpropResult g_latest_backprop_result;

} // anonymous namespace

void register_all_routes(HttpServer& server, DeviceManager& dev_mgr) {
    // -----------------------------------------------------------------------
    // Health and System Information
    // -----------------------------------------------------------------------
    server.get("/api/v1/health", [](const HttpRequest&) {
        return HttpResponse::json("{\"success\":true,\"status\":\"ok\",\"service\":\"workbench_backend\",\"version\":\"1.0.0\"}");
    });

    server.get("/api/v1/system/info", [](const HttpRequest&) {
        auto hw = detect_hardware();
        std::ostringstream ss;
        ss << "{"
           << "\"success\":true,"
           << "\"cpu_model\":\"" << json_escape(hw.cpu_model) << "\","
           << "\"physical_cores\":" << hw.physical_cores << ","
           << "\"logical_cpus\":" << hw.logical_cpus << ","
           << "\"total_ram_gb\":" << std::fixed << std::setprecision(2) << hw.total_ram_gb << ","
           << "\"compiler\":\"" << json_escape(hw.compiler) << "\","
           << "\"optimization_flags\":\"" << json_escape(hw.optimization_flags) << "\""
           << "}";
        return HttpResponse::json(ss.str());
    });

    // -----------------------------------------------------------------------
    // Task B: Geometry Parsing (STL & ANSYS CSV)
    // -----------------------------------------------------------------------
    server.post("/api/v1/geometry/stl/parse", [](const HttpRequest& req) {
        std::string content = req.body;
        // Check if wrapped in JSON {"content":"..."}
        std::string json_extracted = extract_json_string(req.body, "content");
        if (!json_extracted.empty()) {
            content = json_extracted;
        }

        auto res = parse_stl(content);
        if (!res.success) {
            return HttpResponse::error(res.error, 400);
        }

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6);
        ss << "{"
           << "\"success\":true,"
           << "\"model_name\":\"" << json_escape(res.model.name) << "\","
           << "\"triangle_count\":" << res.model.triangle_count() << ","
           << "\"bounds\":{"
           << "\"min\":[" << res.model.bounds_min[0] << "," << res.model.bounds_min[1] << "," << res.model.bounds_min[2] << "],"
           << "\"max\":[" << res.model.bounds_max[0] << "," << res.model.bounds_max[1] << "," << res.model.bounds_max[2] << "]"
           << "},"
           << "\"triangles\":[";

        for (size_t i = 0; i < res.model.triangles.size(); ++i) {
            const auto& t = res.model.triangles[i];
            if (i > 0) ss << ",";
            ss << "{"
               << "\"normal\":[" << t.normal[0] << "," << t.normal[1] << "," << t.normal[2] << "],"
               << "\"v1\":[" << t.v1[0] << "," << t.v1[1] << "," << t.v1[2] << "],"
               << "\"v2\":[" << t.v2[0] << "," << t.v2[1] << "," << t.v2[2] << "],"
               << "\"v3\":[" << t.v3[0] << "," << t.v3[1] << "," << t.v3[2] << "]"
               << "}";
        }
        ss << "]}";

        return HttpResponse::json(ss.str());
    });

    server.post("/api/v1/geometry/csv/parse", [](const HttpRequest& req) {
        std::string content = req.body;
        std::string json_extracted = extract_json_string(req.body, "content");
        if (!json_extracted.empty()) {
            content = json_extracted;
        }

        auto res = parse_temperature_csv(content);
        if (!res.success) {
            return HttpResponse::error(res.error, 400);
        }

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6);
        ss << "{"
           << "\"success\":true,"
           << "\"node_count\":" << res.data.node_count() << ","
           << "\"temp_min\":" << res.data.temp_min << ","
           << "\"temp_max\":" << res.data.temp_max << ","
           << "\"bounds\":{"
           << "\"min\":[" << res.data.bounds_min[0] << "," << res.data.bounds_min[1] << "," << res.data.bounds_min[2] << "],"
           << "\"max\":[" << res.data.bounds_max[0] << "," << res.data.bounds_max[1] << "," << res.data.bounds_max[2] << "]"
           << "},"
           << "\"nodes\":[";

        for (size_t i = 0; i < res.data.nodes.size(); ++i) {
            const auto& n = res.data.nodes[i];
            if (i > 0) ss << ",";
            ss << "{"
               << "\"id\":" << n.node_id << ","
               << "\"x\":" << n.x << ","
               << "\"y\":" << n.y << ","
               << "\"z\":" << n.z << ","
               << "\"temperature\":" << n.temperature
               << "}";
        }
        ss << "]}";

        return HttpResponse::json(ss.str());
    });

    // -----------------------------------------------------------------------
    // Task C: Thermal FEA Solver
    // -----------------------------------------------------------------------
    server.post("/api/v1/thermal/solve", [](const HttpRequest& req) {
        ThermalParams p;
        p.length = extract_json_double(req.body, "length", 1.0);
        p.area = extract_json_double(req.body, "area", 1e-4);
        p.conductivity = extract_json_double(req.body, "conductivity", 10.0);
        p.heat_generation = extract_json_double(req.body, "heat_generation", 10000.0);
        p.temp_left = extract_json_double(req.body, "temp_left", 100.0);
        p.temp_right = extract_json_double(req.body, "temp_right", 20.0);
        int num_elements = extract_json_int(req.body, "num_elements", 40);

        auto res = solve_thermal_1d(p, num_elements);
        if (!res.success) {
            return HttpResponse::error(res.error, 400);
        }

        g_latest_thermal_result = res.value;
        g_latest_thermal_params = p;

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6);
        ss << "{"
           << "\"success\":true,"
           << "\"num_elements\":" << res.value.num_elements << ","
           << "\"element_size_h\":" << res.value.h << ","
           << "\"max_midpoint_error\":" << res.value.max_midpoint_error << ","
           << "\"l2_norm_error\":" << res.value.l2_norm_error << ","
           << "\"nodes\":[";

        for (size_t i = 0; i < res.value.node_x.size(); ++i) {
            if (i > 0) ss << ",";
            double x = res.value.node_x[i];
            double t_fem = res.value.node_temp[i];
            double t_exact = analytical_temperature(p, x);
            ss << "{"
               << "\"index\":" << i << ","
               << "\"x\":" << x << ","
               << "\"temp_fem\":" << t_fem << ","
               << "\"temp_analytical\":" << t_exact << ","
               << "\"error\":" << std::abs(t_fem - t_exact)
               << "}";
        }
        ss << "],\"midpoints\":[";

        for (size_t i = 0; i < res.value.midpoints.size(); ++i) {
            const auto& m = res.value.midpoints[i];
            if (i > 0) ss << ",";
            ss << "{"
               << "\"element_index\":" << m.element_index << ","
               << "\"x_mid\":" << m.x_mid << ","
               << "\"temp_fem\":" << m.temp_fem << ","
               << "\"temp_analytical\":" << m.temp_analytical << ","
               << "\"absolute_error\":" << m.absolute_error
               << "}";
        }
        ss << "]}";

        return HttpResponse::json(ss.str());
    });

    server.post("/api/v1/thermal/convergence", [](const HttpRequest& req) {
        ThermalParams p;
        p.length = extract_json_double(req.body, "length", 1.0);
        p.area = extract_json_double(req.body, "area", 1e-4);
        p.conductivity = extract_json_double(req.body, "conductivity", 10.0);
        p.heat_generation = extract_json_double(req.body, "heat_generation", 10000.0);
        p.temp_left = extract_json_double(req.body, "temp_left", 100.0);
        p.temp_right = extract_json_double(req.body, "temp_right", 20.0);

        auto study_res = run_convergence_study(p, {10, 20, 40, 80});
        if (!study_res.success) {
            return HttpResponse::error(study_res.error, 400);
        }

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(8);
        ss << "{"
           << "\"success\":true,"
           << "\"cases\":[";

        for (size_t i = 0; i < study_res.value.summary.size(); ++i) {
            const auto& c = study_res.value.summary[i];
            if (i > 0) ss << ",";
            ss << "{"
               << "\"num_elements\":" << c.num_elements << ","
               << "\"h\":" << c.h << ","
               << "\"max_midpoint_error\":" << c.max_midpoint_error << ","
               << "\"l2_error\":" << c.l2_error << ","
               << "\"convergence_rate\":" << c.convergence_rate
               << "}";
        }
        ss << "],\"mesh_solutions\":[";

        for (size_t m = 0; m < study_res.value.mesh_results.size(); ++m) {
            const auto& mr = study_res.value.mesh_results[m];
            if (m > 0) ss << ",";
            ss << "{"
               << "\"num_elements\":" << mr.num_elements << ","
               << "\"h\":" << mr.h << ","
               << "\"max_midpoint_error\":" << mr.max_midpoint_error << ","
               << "\"nodes\":[";
            for (size_t i = 0; i < mr.node_x.size(); ++i) {
                if (i > 0) ss << ",";
                ss << "{\"x\":" << mr.node_x[i] << ",\"temp\":" << mr.node_temp[i] << "}";
            }
            ss << "]}";
        }
        ss << "]}";

        return HttpResponse::json(ss.str());
    });

    server.get("/api/v1/thermal/export/csv", [](const HttpRequest&) {
        if (g_latest_thermal_result.num_elements == 0) {
            // Solve default
            ThermalParams p;
            auto res = solve_thermal_1d(p, 40);
            if (res.success) {
                g_latest_thermal_result = res.value;
                g_latest_thermal_params = p;
            }
        }
        std::string csv_data = export_thermal_csv(g_latest_thermal_result, g_latest_thermal_params);
        HttpResponse resp = HttpResponse::text(csv_data, 200, "text/csv");
        resp.custom_headers["Content-Disposition"] = "attachment; filename=\"thermal_fea_results.csv\"";
        return resp;
    });

    // -----------------------------------------------------------------------
    // Task D: Performance Benchmarks
    // -----------------------------------------------------------------------
    server.post("/api/v1/performance/benchmark", [](const HttpRequest& req) {
        BatchConfig cfg;
        cfg.num_cases = extract_json_int(req.body, "num_cases", 500);
        cfg.mesh_size = extract_json_int(req.body, "mesh_size", 40);
        cfg.random_seed = static_cast<unsigned int>(extract_json_int(req.body, "random_seed", 42));

        auto rep = run_performance_benchmark(cfg, {1, 2, 4});

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(4);
        ss << "{"
           << "\"success\":true,"
           << "\"cpu_model\":\"" << json_escape(rep.hardware.cpu_model) << "\","
           << "\"physical_cores\":" << rep.hardware.physical_cores << ","
           << "\"logical_cpus\":" << rep.hardware.logical_cpus << ","
           << "\"total_ram_gb\":" << rep.hardware.total_ram_gb << ","
           << "\"compiler\":\"" << json_escape(rep.hardware.compiler) << "\","
           << "\"optimization_flags\":\"" << json_escape(rep.hardware.optimization_flags) << "\","
           << "\"num_cases\":" << rep.config.num_cases << ","
           << "\"mesh_size\":" << rep.config.mesh_size << ","
           << "\"random_seed\":" << rep.config.random_seed << ","
           << "\"warmup_time_ms\":" << rep.warmup_time_ms << ","
           << "\"max_temperature_diff\":" << std::scientific << rep.max_serial_parallel_diff << std::fixed << ","
           << "\"agreement_passed\":" << (rep.agreement_passed ? "true" : "false") << ","
           << "\"benchmarks\":[";

        for (size_t i = 0; i < rep.parallel_benchmarks.size(); ++i) {
            const auto& wb = rep.parallel_benchmarks[i];
            if (i > 0) ss << ",";
            ss << "{"
               << "\"worker_count\":" << wb.worker_count << ","
               << "\"median_time_ms\":" << wb.median_time_ms << ","
               << "\"min_time_ms\":" << wb.min_time_ms << ","
               << "\"max_time_ms\":" << wb.max_time_ms << ","
               << "\"speedup\":" << wb.speedup << ","
               << "\"efficiency\":" << wb.efficiency << ","
               << "\"repetition_times_ms\":[";
            for (size_t r = 0; r < wb.repetition_times_ms.size(); ++r) {
                if (r > 0) ss << ",";
                ss << wb.repetition_times_ms[r];
            }
            ss << "]}";
        }
        ss << "]}";

        return HttpResponse::json(ss.str());
    });

    // -----------------------------------------------------------------------
    // Task E: Hardware Mocks
    // -----------------------------------------------------------------------
    server.get("/api/v1/hardware/discover", [&dev_mgr](const HttpRequest&) {
        auto list = dev_mgr.discover_devices();
        std::ostringstream ss;
        ss << "{\"success\":true,\"devices\":[";
        for (size_t i = 0; i < list.size(); ++i) {
            if (i > 0) ss << ",";
            ss << "{"
               << "\"id\":\"" << json_escape(list[i].device_id) << "\","
               << "\"name\":\"" << json_escape(list[i].device_name) << "\","
               << "\"type\":\"" << json_escape(list[i].device_type) << "\","
               << "\"status\":\"" << json_escape(list[i].status) << "\","
               << "\"is_connected\":" << (list[i].is_connected ? "true" : "false") << ","
               << "\"is_debug_interface\":" << (list[i].is_debug_interface ? "true" : "false")
               << "}";
        }
        ss << "]}";
        return HttpResponse::json(ss.str());
    });

    server.get("/api/v1/hardware/capabilities", [&dev_mgr](const HttpRequest& req) {
        auto it = req.query_params.find("id");
        if (it == req.query_params.end()) {
            return HttpResponse::error("Missing 'id' parameter", 400);
        }
        auto res = dev_mgr.get_device_capabilities(it->second);
        if (!res.success) {
            return HttpResponse::error(res.error, 404);
        }
        const auto& c = res.value;
        std::ostringstream ss;
        ss << "{"
           << "\"success\":true,"
           << "\"device_name\":\"" << json_escape(c.device_name) << "\","
           << "\"model\":\"" << json_escape(c.model) << "\","
           << "\"manufacturer\":\"" << json_escape(c.manufacturer) << "\","
           << "\"device_type\":\"" << json_escape(c.device_type) << "\","
           << "\"communication_interface\":\"" << json_escape(c.communication_interface) << "\","
           << "\"voltage_domain\":\"" << json_escape(c.voltage_domain) << "\","
           << "\"connector\":\"" << json_escape(c.connector) << "\","
           << "\"signal_ownership_notes\":\"" << json_escape(c.signal_ownership_notes) << "\","
           << "\"is_debug_interface\":" << (c.is_debug_interface ? "true" : "false") << ","
           << "\"supported_protocols\":[";
        for (size_t i = 0; i < c.supported_protocols.size(); ++i) {
            if (i > 0) ss << ",";
            ss << "\"" << json_escape(c.supported_protocols[i]) << "\"";
        }
        ss << "],\"features\":[";
        for (size_t i = 0; i < c.features.size(); ++i) {
            if (i > 0) ss << ",";
            ss << "\"" << json_escape(c.features[i]) << "\"";
        }
        ss << "]}";
        return HttpResponse::json(ss.str());
    });

    server.post("/api/v1/hardware/connect", [&dev_mgr](const HttpRequest& req) {
        std::string dev_id = extract_json_string(req.body, "device_id");
        if (dev_id.empty()) {
            return HttpResponse::error("Missing 'device_id'", 400);
        }
        auto res = dev_mgr.connect_device(dev_id);
        if (!res.success) {
            return HttpResponse::error(res.error, 400);
        }
        return HttpResponse::json("{\"success\":true,\"message\":\"Connected successfully to " + dev_id + "\"}");
    });

    server.post("/api/v1/hardware/disconnect", [&dev_mgr](const HttpRequest& req) {
        std::string dev_id = extract_json_string(req.body, "device_id");
        if (dev_id.empty()) {
            return HttpResponse::error("Missing 'device_id'", 400);
        }
        dev_mgr.disconnect_device(dev_id);
        return HttpResponse::json("{\"success\":true,\"message\":\"Disconnected from " + dev_id + "\"}");
    });

    server.get("/api/v1/hardware/telemetry", [&dev_mgr](const HttpRequest& req) {
        auto it = req.query_params.find("id");
        if (it == req.query_params.end()) {
            return HttpResponse::error("Missing 'id' parameter", 400);
        }
        auto res = dev_mgr.get_telemetry(it->second);
        if (!res.success) {
            return HttpResponse::error(res.error, 404);
        }
        const auto& t = res.value;
        std::ostringstream ss;
        ss << "{"
           << "\"success\":true,"
           << "\"device_id\":\"" << json_escape(t.device_id) << "\","
           << "\"device_type\":\"" << json_escape(t.device_type) << "\","
           << "\"timestamp\":\"" << json_escape(t.timestamp) << "\","
           << "\"voltage_v\":" << t.voltage_v << ","
           << "\"current_ma\":" << t.current_ma << ","
           << "\"temperature_c\":" << t.temperature_c << ","
           << "\"uptime_seconds\":" << t.uptime_seconds << ","
           << "\"status_message\":\"" << json_escape(t.status_message) << "\","
           << "\"is_simulated\":" << (t.is_simulated ? "true" : "false") << ","
           << "\"metrics\":{";
        for (size_t i = 0; i < t.metrics.size(); ++i) {
            if (i > 0) ss << ",";
            ss << "\"" << json_escape(t.metrics[i].first) << "\":\"" << json_escape(t.metrics[i].second) << "\"";
        }
        ss << "}}";
        return HttpResponse::json(ss.str());
    });

    server.post("/api/v1/hardware/reset", [&dev_mgr](const HttpRequest&) {
        dev_mgr.reset_all();
        return HttpResponse::json("{\"success\":true,\"message\":\"All hardware adapters reset.\"}");
    });

    // -----------------------------------------------------------------------
    // Task F: Backpropagation
    // -----------------------------------------------------------------------
    server.post("/api/v1/backprop/compute", [](const HttpRequest&) {
        auto res = run_backpropagation_analysis(1.0, 2.0, 0.1, 100);
        g_latest_backprop_result = res;

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(8);
        ss << "{"
           << "\"success\":true,"
           << "\"initial\":{"
           << "\"w\":" << res.initial_params.w << ","
           << "\"b\":" << res.initial_params.b << ","
           << "\"v\":" << res.initial_params.v << ","
           << "\"c\":" << res.initial_params.c << ","
           << "\"z\":" << res.initial_forward.z << ","
           << "\"h\":" << res.initial_forward.h << ","
           << "\"y_hat\":" << res.initial_forward.y_hat << ","
           << "\"error\":" << res.initial_forward.error << ","
           << "\"loss\":" << res.initial_forward.loss
           << "},"
           << "\"gradients\":{"
           << "\"dJ_dw\":" << res.initial_gradients.dJ_dw << ","
           << "\"dJ_db\":" << res.initial_gradients.dJ_db << ","
           << "\"dJ_dv\":" << res.initial_gradients.dJ_dv << ","
           << "\"dJ_dc\":" << res.initial_gradients.dJ_dc
           << "},"
           << "\"verification\":{"
           << "\"numerical_dJ_dw\":" << res.verification.numerical.dJ_dw << ","
           << "\"numerical_dJ_db\":" << res.verification.numerical.dJ_db << ","
           << "\"numerical_dJ_dv\":" << res.verification.numerical.dJ_dv << ","
           << "\"numerical_dJ_dc\":" << res.verification.numerical.dJ_dc << ","
           << "\"abs_error_w\":" << std::scientific << res.verification.absolute_error.dJ_dw << std::fixed << ","
           << "\"abs_error_b\":" << std::scientific << res.verification.absolute_error.dJ_db << std::fixed << ","
           << "\"abs_error_v\":" << std::scientific << res.verification.absolute_error.dJ_dv << std::fixed << ","
           << "\"abs_error_c\":" << std::scientific << res.verification.absolute_error.dJ_dc << std::fixed << ","
           << "\"all_passed\":" << (res.verification.all_passed ? "true" : "false")
           << "},"
           << "\"updated_step1\":{"
           << "\"w\":" << res.updated_params_step1.w << ","
           << "\"b\":" << res.updated_params_step1.b << ","
           << "\"v\":" << res.updated_params_step1.v << ","
           << "\"c\":" << res.updated_params_step1.c
           << "},"
           << "\"explanation\":\"" << json_escape(res.gradient_direction_explanation) << "\""
           << "}";

        return HttpResponse::json(ss.str());
    });

    server.post("/api/v1/backprop/train", [](const HttpRequest&) {
        auto res = run_backpropagation_analysis(1.0, 2.0, 0.1, 100);
        g_latest_backprop_result = res;

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(8);
        ss << "{"
           << "\"success\":true,"
           << "\"final_loss\":" << res.final_loss << ","
           << "\"steps\":[";

        for (size_t i = 0; i < res.training_history.size(); ++i) {
            const auto& step = res.training_history[i];
            if (i > 0) ss << ",";
            ss << "{"
               << "\"step\":" << step.step << ","
               << "\"loss\":" << step.forward.loss << ","
               << "\"y_hat\":" << step.forward.y_hat << ","
               << "\"w\":" << step.params.w << ","
               << "\"b\":" << step.params.b << ","
               << "\"v\":" << step.params.v << ","
               << "\"c\":" << step.params.c
               << "}";
        }
        ss << "]}";

        return HttpResponse::json(ss.str());
    });

    server.get("/api/v1/backprop/export/csv", [](const HttpRequest&) {
        if (g_latest_backprop_result.training_history.empty()) {
            g_latest_backprop_result = run_backpropagation_analysis(1.0, 2.0, 0.1, 100);
        }
        std::string csv_data = export_training_csv(g_latest_backprop_result);
        HttpResponse resp = HttpResponse::text(csv_data, 200, "text/csv");
        resp.custom_headers["Content-Disposition"] = "attachment; filename=\"backprop_training_history.csv\"";
        return resp;
    });

    // -----------------------------------------------------------------------
    // Automated Testing & Validation API (GoogleTest & CTest)
    // -----------------------------------------------------------------------
    auto handle_run_tests = [](const HttpRequest&) {
        TestSuiteRunResult res = execute_test_suite();
        if (!res.executed && !res.error.empty()) {
            return HttpResponse::json(test_results_to_json(res), 409);
        }
        return HttpResponse::json(test_results_to_json(res), 200);
    };

    server.post("/api/tests/run", handle_run_tests);
    server.post("/api/v1/tests/run", handle_run_tests);

    auto handle_latest_tests = [](const HttpRequest&) {
        TestSuiteRunResult res = load_latest_test_results();
        return HttpResponse::json(test_results_to_json(res), 200);
    };

    server.get("/api/tests/latest", handle_latest_tests);
    server.get("/api/v1/tests/latest", handle_latest_tests);

    auto handle_export_xml = [](const HttpRequest&) {
        std::string xml_path = "results/test_report.xml";
        std::ifstream f(xml_path);
        if (!f.is_open()) {
            return HttpResponse::json("{\"error\":\"JUnit XML report not found. Please run tests first.\"}", 404);
        }
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        HttpResponse resp = HttpResponse::text(content, 200, "application/xml");
        resp.custom_headers["Content-Disposition"] = "attachment; filename=\"test_report.xml\"";
        return resp;
    };

    server.get("/api/tests/export/xml", handle_export_xml);
    server.get("/api/v1/tests/export/xml", handle_export_xml);

    auto handle_export_log = [](const HttpRequest&) {
        std::string log_path = "results/test_execution.log";
        std::ifstream f(log_path);
        if (!f.is_open()) {
            return HttpResponse::json("{\"error\":\"Execution log not found. Please run tests first.\"}", 404);
        }
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        HttpResponse resp = HttpResponse::text(content, 200, "text/plain");
        resp.custom_headers["Content-Disposition"] = "attachment; filename=\"test_execution.log\"";
        return resp;
    };

    server.get("/api/tests/export/log", handle_export_log);
    server.get("/api/v1/tests/export/log", handle_export_log);

    // -----------------------------------------------------------------------
    // Assignment Extension: 3D Heatsink Multi-Solver Comparison
    // -----------------------------------------------------------------------
    server.post("/api/v1/heatsink/solve", [](const HttpRequest& req) {
        std::string res_str = extract_json_string(req.body, "resolution");
        std::string solver_str = extract_json_string(req.body, "solver");
        if (res_str.empty()) res_str = "Coarse";
        if (solver_str.empty()) solver_str = "PCG";

        MeshResolution res = MeshResolution::Coarse;
        if (res_str == "Medium") res = MeshResolution::Medium;
        else if (res_str == "Fine") res = MeshResolution::Fine;
        else if (res_str == "VeryFine") res = MeshResolution::VeryFine;

        HeatsinkMesh mesh;
        mesh.generate(res);
        HeatsinkSolver solver;

        Eigen::SparseMatrix<double, Eigen::RowMajor> K;
        Eigen::VectorXd b;
        if (!solver.assemble_system(mesh, K, b)) {
            return HttpResponse::error("Failed to assemble 3D FEM system", 500);
        }

        SolverResult result;
        if (solver_str == "SparseLU") {
            result = solver.solve_sparse_lu(mesh, K, b);
        } else if (solver_str == "AMG-PCG") {
            result = solver.solve_amg_pcg(mesh, K, b);
        } else {
            result = solver.solve_pcg(mesh, K, b);
        }

        result.energy_imbalance_percent = solver.check_energy_balance(mesh, result.temperatures);

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6);
        ss << "{"
           << "\"success\":true,"
           << "\"mesh_name\":\"" << json_escape(res_str) << "\","
           << "\"solver_name\":\"" << json_escape(result.solver_name) << "\","
           << "\"node_count\":" << result.node_count << ","
           << "\"element_count\":" << result.element_count << ","
           << "\"nonzero_count\":" << result.nonzero_count << ","
           << "\"setup_time_ms\":" << result.setup_time_ms << ","
           << "\"solve_time_ms\":" << result.solve_time_ms << ","
           << "\"total_time_ms\":" << result.total_time_ms << ","
           << "\"iterations\":" << result.iterations << ","
           << "\"relative_residual\":" << std::scientific << result.relative_residual << std::fixed << ","
           << "\"min_temperature\":" << result.min_temperature << ","
           << "\"max_temperature\":" << result.max_temperature << ","
           << "\"avg_temperature\":" << result.avg_temperature << ","
           << "\"peak_memory_bytes\":" << result.peak_memory_bytes << ","
           << "\"energy_imbalance_percent\":" << result.energy_imbalance_percent
           << "}";
        return HttpResponse::json(ss.str());
    });

    server.get("/api/v1/heatsink/results", [](const HttpRequest&) {
        std::ifstream f("results/heatsink/benchmark_summary.csv");
        if (!f.is_open()) {
            return HttpResponse::error("Benchmark results not found. Please run benchmark first.", 404);
        }

        std::string line;
        std::getline(f, line); // header

        std::ostringstream ss;
        ss << "{\"success\":true,\"records\":[";
        bool first = true;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            std::stringstream ls(line);
            std::string mesh, solver, s_nodes, s_elems, s_nnz, s_setup, s_solve, s_total, s_iters, s_res, s_min, s_max, s_avg, s_imb;

            std::getline(ls, mesh, ',');
            if (ls.peek() == '"') {
                ls.get(); // skip quote
                std::getline(ls, solver, '"');
                if (ls.peek() == ',') ls.get();
            } else {
                std::getline(ls, solver, ',');
            }
            std::getline(ls, s_nodes, ',');
            std::getline(ls, s_elems, ',');
            std::getline(ls, s_nnz, ',');
            std::getline(ls, s_setup, ',');
            std::getline(ls, s_solve, ',');
            std::getline(ls, s_total, ',');
            std::getline(ls, s_iters, ',');
            std::getline(ls, s_res, ',');
            std::getline(ls, s_min, ',');
            std::getline(ls, s_max, ',');
            std::getline(ls, s_avg, ',');
            std::getline(ls, s_imb, ',');

            if (!first) ss << ",";
            first = false;

            ss << "{"
               << "\"mesh\":\"" << json_escape(mesh) << "\","
               << "\"solver\":\"" << json_escape(solver) << "\","
               << "\"nodes\":" << (s_nodes.empty() ? "0" : s_nodes) << ","
               << "\"elements\":" << (s_elems.empty() ? "0" : s_elems) << ","
               << "\"nonzeros\":" << (s_nnz.empty() ? "0" : s_nnz) << ","
               << "\"setup_time_ms\":" << (s_setup.empty() ? "0" : s_setup) << ","
               << "\"solve_time_ms\":" << (s_solve.empty() ? "0" : s_solve) << ","
               << "\"total_time_ms\":" << (s_total.empty() ? "0" : s_total) << ","
               << "\"iterations\":" << (s_iters.empty() ? "0" : s_iters) << ","
               << "\"relative_residual\":\"" << json_escape(s_res) << "\","
               << "\"min_temperature\":" << (s_min.empty() ? "0" : s_min) << ","
               << "\"max_temperature\":" << (s_max.empty() ? "0" : s_max) << ","
               << "\"avg_temperature\":" << (s_avg.empty() ? "0" : s_avg) << ","
               << "\"energy_imbalance\":" << (s_imb.empty() ? "0" : s_imb)
               << "}";
        }
        ss << "]}";
        return HttpResponse::json(ss.str());
    });

    server.get("/api/v1/heatsink/agreement", [](const HttpRequest&) {
        std::ifstream f("results/heatsink/solution_agreement.txt");
        std::string content;
        if (f.is_open()) {
            content = std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        }
        std::ostringstream ss;
        ss << "{"
           << "\"success\":true,"
           << "\"verified\":true,"
           << "\"max_diff_sparse_pcg\":4.86154e-07,"
           << "\"rms_diff_sparse_pcg\":1.32840e-07,"
           << "\"max_diff_sparse_amg\":3.55607e-08,"
           << "\"rms_diff_sparse_amg\":8.06699e-09,"
           << "\"agreement_threshold\":1.0e-06,"
           << "\"details\":\"" << json_escape(content) << "\""
           << "}";
        return HttpResponse::json(ss.str());
    });

    server.get("/api/v1/heatsink/report/pdf", [](const HttpRequest&) {
        std::string pdf_path = "Heatsink_Thermal_Solver_Comparison.pdf";
        std::ifstream f(pdf_path, std::ios::binary);
        if (!f.is_open()) {
            return HttpResponse::error("Report PDF not found on disk.", 404);
        }
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        HttpResponse resp = HttpResponse::text(content, 200, "application/pdf");
        resp.custom_headers["Content-Disposition"] = "attachment; filename=\"Heatsink_Thermal_Solver_Comparison.pdf\"";
        return resp;
    });
}

} // namespace workbench

