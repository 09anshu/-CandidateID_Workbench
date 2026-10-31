#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "../src/core/linear_algebra.h"
#include "../src/thermal/thermal_solver.h"
#include "../src/geometry/stl_parser.h"
#include "../src/geometry/csv_parser.h"
#include "../src/performance/parallel_solver.h"
#include "../src/hardware/device_manager.h"
#include "../src/backpropagation/backprop.h"

namespace {

int g_total_tests = 0;
int g_passed_tests = 0;
int g_failed_tests = 0;

void record_test(const std::string& name, bool passed, const std::string& details = "") {
    ++g_total_tests;
    if (passed) {
        ++g_passed_tests;
        std::cout << "  [PASS] " << name << "\n";
    } else {
        ++g_failed_tests;
        std::cout << "  [FAIL] " << name << " (" << details << ")\n";
    }
}

} // anonymous namespace

int main() {
    std::cout << "========================================================\n";
    std::cout << " PASS SCIENCE WORKS - C++ Test Suite Verification\n";
    std::cout << "========================================================\n\n";

    // -------------------------------------------------------------------
    // 1. Core Linear Algebra & Thomas Algorithm Tests
    // -------------------------------------------------------------------
    std::cout << "1. Running Linear Algebra & Tridiagonal Solver Tests...\n";
    {
        // Solve simple 3x3 tridiagonal system:
        // [ 2  -1   0 ] [ x0 ]   [ 1 ]
        // [-1   2  -1 ] [ x1 ] = [ 0 ]  --> Solution is x = [ 1, 1, 1 ]
        // [ 0  -1   2 ] [ x2 ]   [ 1 ]
        std::vector<double> a = {-1.0, -1.0};
        std::vector<double> b = {2.0, 2.0, 2.0};
        std::vector<double> c = {-1.0, -1.0};
        std::vector<double> d = {1.0, 0.0, 1.0};

        auto res = workbench::solve_tridiagonal(a, b, c, d);
        bool ok = res.success && res.value.size() == 3;
        if (ok) {
            ok = std::abs(res.value[0] - 1.0) < 1e-9 &&
                 std::abs(res.value[1] - 1.0) < 1e-9 &&
                 std::abs(res.value[2] - 1.0) < 1e-9;
        }
        record_test("Tridiagonal Thomas algorithm exact solution", ok);
    }

    // -------------------------------------------------------------------
    // 2. Thermal FEA Solver & Analytical Verification (Task C)
    // -------------------------------------------------------------------
    std::cout << "\n2. Running Thermal FEA Solver & Analytical Verification (Task C)...\n";
    {
        workbench::ThermalParams p;
        p.length = 1.0;
        p.area = 1e-4;
        p.conductivity = 10.0;
        p.heat_generation = 10000.0;
        p.temp_left = 100.0;
        p.temp_right = 20.0;

        // Boundary condition check
        auto res_10 = workbench::solve_thermal_1d(p, 10);
        bool bc_ok = res_10.success &&
                     std::abs(res_10.value.node_temp.front() - 100.0) < 1e-9 &&
                     std::abs(res_10.value.node_temp.back() - 20.0) < 1e-9;
        record_test("Dirichlet boundary conditions imposed correctly (T0=100, TL=20)", bc_ok);

        // Nodal superconvergence check: in 1D steady-state with linear elements, nodes match exact solution
        bool nodal_match = true;
        for (size_t i = 0; i < res_10.value.node_x.size(); ++i) {
            double x = res_10.value.node_x[i];
            double t_exact = workbench::analytical_temperature(p, x);
            if (std::abs(res_10.value.node_temp[i] - t_exact) > 1e-7) {
                nodal_match = false;
                break;
            }
        }
        record_test("Nodal temperatures agree with analytical solution within 1e-7 °C", nodal_match);

        // Convergence study across N = 10, 20, 40, 80
        auto conv = workbench::run_convergence_study(p, {10, 20, 40, 80});
        bool conv_ok = conv.success && conv.value.summary.size() == 4;

        if (conv_ok) {
            // Theoretical midpoint error is exactly (q / 8k) * h^2 = 125 * h^2
            // N=10 (h=0.1)   -> Error = 1.25 °C
            // N=20 (h=0.05)  -> Error = 0.3125 °C
            // N=40 (h=0.025) -> Error = 0.078125 °C
            // N=80 (h=0.0125)-> Error = 0.01953125 °C
            double err_10 = conv.value.summary[0].max_midpoint_error;
            double err_20 = conv.value.summary[1].max_midpoint_error;
            double err_40 = conv.value.summary[2].max_midpoint_error;
            double err_80 = conv.value.summary[3].max_midpoint_error;

            bool err_vals_match = std::abs(err_10 - 1.25) < 1e-6 &&
                                  std::abs(err_20 - 0.3125) < 1e-6 &&
                                  std::abs(err_40 - 0.078125) < 1e-6 &&
                                  std::abs(err_80 - 0.01953125) < 1e-6;
            record_test("Midpoint errors match theoretical quadratic error (1.25, 0.3125, 0.078125, 0.01953125 °C)", err_vals_match);

            // Convergence rate should be ~2.0 (O(h^2))
            double rate_20 = conv.value.summary[1].convergence_rate;
            double rate_40 = conv.value.summary[2].convergence_rate;
            double rate_80 = conv.value.summary[3].convergence_rate;
            bool rate_ok = std::abs(rate_20 - 2.0) < 0.05 &&
                           std::abs(rate_40 - 2.0) < 0.05 &&
                           std::abs(rate_80 - 2.0) < 0.05;
            record_test("Mesh convergence rate p ≈ 2.0 (quadratic O(h^2))", rate_ok);
        } else {
            record_test("Convergence study execution", false, conv.error);
        }

        // Invalid parameters validation
        workbench::ThermalParams p_bad = p;
        p_bad.conductivity = -5.0; // Negative conductivity
        auto bad_res = workbench::solve_thermal_1d(p_bad, 10);
        record_test("Negative conductivity rejection", !bad_res.success);

        p_bad = p;
        p_bad.length = -1.0;
        record_test("Negative length rejection", !workbench::solve_thermal_1d(p_bad, 10).success);
    }

    // -------------------------------------------------------------------
    // 3. Geometry Parsers (Task B)
    // -------------------------------------------------------------------
    std::cout << "\n3. Running Geometry Parsers Tests (Task B)...\n";
    {
        // Valid ASCII STL
        std::string valid_stl =
            "solid test_box\n"
            "  facet normal 0 0 1\n"
            "    outer loop\n"
            "      vertex 0 0 1\n"
            "      vertex 1 0 1\n"
            "      vertex 1 1 1\n"
            "    endloop\n"
            "  endfacet\n"
            "endsolid test_box\n";

        auto stl_res = workbench::parse_stl(valid_stl);
        record_test("ASCII STL valid parse", stl_res.success && stl_res.model.triangle_count() == 1);

        // Empty STL
        record_test("Empty STL file rejection", !workbench::parse_stl("").success);

        // Binary STL detection & rejection
        std::string binary_stl = "XYZ\0\0\0dummy binary header" + std::string(100, '\0');
        record_test("Binary STL rejection with informative error", !workbench::parse_stl(binary_stl).success);

        // Malformed STL
        std::string malformed_stl = "solid bad\nfacet normal 0 0 1\nouter loop\nvertex 0 0 0\nendloop\nendfacet\nendsolid bad";
        record_test("Malformed STL (insufficient vertices) rejection", !workbench::parse_stl(malformed_stl).success);

        // Valid ANSYS CSV
        std::string valid_csv =
            "node_id,x_m,y_m,z_m,temperature_C\n"
            "1,0.0,0.0,0.0,100.0\n"
            "2,0.5,0.0,0.0,120.0\n"
            "3,1.0,0.0,0.0,20.0\n";
        auto csv_res = workbench::parse_temperature_csv(valid_csv);
        record_test("ANSYS Temperature CSV valid parse", csv_res.success && csv_res.data.node_count() == 3);

        // CSV missing columns
        std::string missing_col_csv = "node_id,x_m,y_m\n1,0,0\n";
        record_test("CSV missing required columns rejection", !workbench::parse_temperature_csv(missing_col_csv).success);

        // CSV non-finite value (NaN)
        std::string nan_csv = "node_id,x_m,y_m,z_m,temperature_C\n1,0.0,0.0,0.0,NaN\n";
        record_test("CSV non-finite (NaN) rejection", !workbench::parse_temperature_csv(nan_csv).success);

        // Empty CSV
        record_test("Empty CSV file rejection", !workbench::parse_temperature_csv("").success);
    }

    // -------------------------------------------------------------------
    // 4. Multicore Parallel Execution & Agreement Check (Task D)
    // -------------------------------------------------------------------
    std::cout << "\n4. Running Multicore Parallel Execution Tests (Task D)...\n";
    {
        workbench::BatchConfig cfg;
        cfg.num_cases = 100;
        cfg.mesh_size = 20;
        cfg.random_seed = 42;

        auto cases = workbench::generate_thermal_batch(cfg);
        record_test("Batch generator produces exact requested case count", cases.size() == 100);

        auto serial_res = workbench::solve_batch_serial(cases);
        auto parallel_res = workbench::solve_batch_parallel(cases, 4);

        double max_diff = 0.0;
        for (size_t c = 0; c < serial_res.size(); ++c) {
            for (size_t n = 0; n < serial_res[c].node_temp.size(); ++n) {
                double diff = std::abs(serial_res[c].node_temp[n] - parallel_res[c].node_temp[n]);
                if (diff > max_diff) max_diff = diff;
            }
        }

        bool agreement_ok = (max_diff <= 1e-8);
        record_test("Serial vs Parallel agreement check (max diff <= 1e-8 °C, actual: " + std::to_string(max_diff) + ")", agreement_ok);
    }

    // -------------------------------------------------------------------
    // 5. Embedded System Mocks (Task E)
    // -------------------------------------------------------------------
    std::cout << "\n5. Running Embedded Hardware Mock Tests (Task E)...\n";
    {
        workbench::DeviceManager mgr;
        auto devs = mgr.discover_devices();
        record_test("Discovery finds all 3 devices (MCU, FPGA, JTAG)", devs.size() == 3);

        // MCU Connect and Telemetry
        auto mcu_conn = mgr.connect_device("MCU-STM32F4-01");
        record_test("MCU connection lifecycle", mcu_conn.success);

        auto mcu_telem = mgr.get_telemetry("MCU-STM32F4-01");
        record_test("MCU telemetry acquisition", mcu_telem.success && mcu_telem.value.voltage_v > 3.0);

        mgr.disconnect_device("MCU-STM32F4-01");

        // FPGA Connect and Telemetry
        auto fpga_conn = mgr.connect_device("FPGA-ARTIX7-02");
        record_test("FPGA connection lifecycle", fpga_conn.success);

        auto fpga_telem = mgr.get_telemetry("FPGA-ARTIX7-02");
        record_test("FPGA telemetry acquisition (LUT utilization)", fpga_telem.success && !fpga_telem.value.metrics.empty());

        mgr.disconnect_device("FPGA-ARTIX7-02");

        // JTAG Recoverable Failure Test:
        // Attempt 1 fails with timeout
        auto jtag_attempt1 = mgr.connect_device("JTAG-JLINK-03");
        bool timeout_simulated = !jtag_attempt1.success && (jtag_attempt1.error.find("timed out") != std::string::npos);
        record_test("JTAG recoverable timeout failure on attempt 1", timeout_simulated);

        // Attempt 2 succeeds
        auto jtag_attempt2 = mgr.connect_device("JTAG-JLINK-03");
        record_test("JTAG connection recovery on attempt 2 (retry)", jtag_attempt2.success);

        auto jtag_telem = mgr.get_telemetry("JTAG-JLINK-03");
        record_test("JTAG debug telemetry acquisition", jtag_telem.success);

        mgr.disconnect_device("JTAG-JLINK-03");
    }

    // -------------------------------------------------------------------
    // 6. Backpropagation & Finite Differences Verification (Task F)
    // -------------------------------------------------------------------
    std::cout << "\n6. Running Handwritten Backpropagation Tests (Task F)...\n";
    {
        workbench::NetworkParams p;
        p.w = 0.5; p.b = 0.0; p.v = 1.0; p.c = 0.0;
        double x = 1.0; double y = 2.0;

        // Forward pass check
        auto fwd = workbench::forward_pass(p, x, y);
        bool fwd_ok = std::abs(fwd.z - 0.5) < 1e-9 &&
                      std::abs(fwd.h - std::tanh(0.5)) < 1e-9 &&
                      std::abs(fwd.y_hat - std::tanh(0.5)) < 1e-9 &&
                      std::abs(fwd.loss - 0.5 * std::pow(std::tanh(0.5) - 2.0, 2.0)) < 1e-9;
        record_test("Backpropagation forward pass values (z, h, y_hat, loss)", fwd_ok);

        // Gradients finite difference check with epsilon = 1e-6
        auto ver = workbench::verify_gradients_finite_difference(p, x, y, 1e-6, 1e-5);
        record_test("Gradients vs Central Finite Difference check (error <= 1e-5 for all 4 parameters)", ver.all_passed);

        // Training loop: loss should strictly decrease
        auto res = workbench::run_backpropagation_analysis(x, y, 0.1, 100);
        bool loss_decreased = (res.final_loss < res.initial_forward.loss);
        record_test("100-step training loop converges (loss reduced from ~1.18 to " + std::to_string(res.final_loss) + ")", loss_decreased);
    }

    // -------------------------------------------------------------------
    // Summary
    // -------------------------------------------------------------------
    std::cout << "\n========================================================\n";
    std::cout << " TEST SUMMARY: " << g_passed_tests << " / " << g_total_tests << " PASSED\n";
    std::cout << "========================================================\n";

    return (g_failed_tests == 0) ? 0 : 1;
}
