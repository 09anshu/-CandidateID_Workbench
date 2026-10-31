#include "server_routes.h"

#include <iomanip>
#include <sstream>

#include "../backpropagation/backprop.h"
#include "../geometry/csv_parser.h"
#include "../geometry/stl_parser.h"
#include "../performance/parallel_solver.h"
#include "../thermal/thermal_solver.h"
#include "json_helper.h"

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
}

} // namespace workbench
