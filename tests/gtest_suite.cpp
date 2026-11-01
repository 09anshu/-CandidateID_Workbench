#include <gtest/gtest.h>
#include <cmath>
#include <string>
#include <vector>
#include <limits>

#include "../src/core/linear_algebra.h"
#include "../src/core/types.h"
#include "../src/thermal/thermal_solver.h"
#include "../src/geometry/stl_parser.h"
#include "../src/geometry/csv_parser.h"
#include "../src/performance/parallel_solver.h"
#include "../src/hardware/device_manager.h"
#include "../src/backpropagation/backprop.h"

using namespace workbench;

// ===========================================================================
// 1. Geometry & STL Parser Tests (Task B)
// ===========================================================================

TEST(StlParserTest, AsciiStl_ValidParsing) {
    std::string valid_stl =
        "solid test_triangle\n"
        "  facet normal 0.0 0.0 1.0\n"
        "    outer loop\n"
        "      vertex 0.0 0.0 0.0\n"
        "      vertex 1.0 0.0 0.0\n"
        "      vertex 0.0 1.0 0.0\n"
        "    endloop\n"
        "  endfacet\n"
        "endsolid test_triangle\n";

    auto res = parse_stl(valid_stl);
    ASSERT_TRUE(res.success) << "Failed to parse valid ASCII STL: " << res.error;
    EXPECT_EQ(res.model.triangle_count(), 1);
    EXPECT_EQ(res.model.name, "test_triangle");

    // Bounds check
    EXPECT_DOUBLE_EQ(res.model.bounds_min[0], 0.0);
    EXPECT_DOUBLE_EQ(res.model.bounds_max[0], 1.0);
    EXPECT_DOUBLE_EQ(res.model.bounds_min[1], 0.0);
    EXPECT_DOUBLE_EQ(res.model.bounds_max[1], 1.0);
    EXPECT_DOUBLE_EQ(res.model.bounds_min[2], 0.0);
    EXPECT_DOUBLE_EQ(res.model.bounds_max[2], 0.0);
}

TEST(StlParserTest, AsciiStl_EmptyFileRejection) {
    auto res = parse_stl("");
    EXPECT_FALSE(res.success);
    EXPECT_NE(res.error.find("empty"), std::string::npos);
}

TEST(StlParserTest, BinaryStl_DetectionAndRejection) {
    // Binary STL has 80-byte header with null bytes followed by 4-byte triangle count
    std::string binary_stl = "XYZ\0\0\0dummy binary header" + std::string(100, '\0');
    auto res = parse_stl(binary_stl);
    EXPECT_FALSE(res.success);
    EXPECT_NE(res.error.find("binary"), std::string::npos);
}

TEST(StlParserTest, AsciiStl_MalformedFacetRejection) {
    // Only 2 vertices in loop instead of 3
    std::string malformed_stl =
        "solid bad_loop\n"
        "  facet normal 0 0 1\n"
        "    outer loop\n"
        "      vertex 0 0 0\n"
        "      vertex 1 0 0\n"
        "    endloop\n"
        "  endfacet\n"
        "endsolid bad_loop\n";

    auto res = parse_stl(malformed_stl);
    EXPECT_FALSE(res.success);
    EXPECT_NE(res.error.find("3 vertices"), std::string::npos);
}

TEST(StlParserTest, AsciiStl_ZeroFacetsRejection) {
    std::string zero_facet_stl = "solid empty_mesh\nendsolid empty_mesh\n";
    auto res = parse_stl(zero_facet_stl);
    EXPECT_FALSE(res.success);
    EXPECT_NE(res.error.find("no valid triangle facets"), std::string::npos);
}

TEST(GeometryValidationTest, NonNumericNormalsRejection) {
    std::string bad_norm_stl =
        "solid bad_norm\n"
        "  facet normal abc def ghi\n"
        "    outer loop\n"
        "      vertex 0 0 0\n"
        "      vertex 1 0 0\n"
        "      vertex 0 1 0\n"
        "    endloop\n"
        "  endfacet\n"
        "endsolid bad_norm\n";

    auto res = parse_stl(bad_norm_stl);
    EXPECT_FALSE(res.success);
}

// ===========================================================================
// 2. CSV Temperature Parser Tests (Task B)
// ===========================================================================

TEST(CsvParserTest, AnsysCsv_ValidParsing) {
    std::string valid_csv =
        "node_id,x_m,y_m,z_m,temperature_C\n"
        "1,0.0,0.0,0.0,100.0\n"
        "2,0.5,0.0,0.0,125.0\n"
        "3,1.0,0.0,0.0,20.0\n";

    auto res = parse_temperature_csv(valid_csv);
    ASSERT_TRUE(res.success) << "Failed to parse valid CSV: " << res.error;
    EXPECT_EQ(res.data.node_count(), 3);
    EXPECT_DOUBLE_EQ(res.data.temp_min, 20.0);
    EXPECT_DOUBLE_EQ(res.data.temp_max, 125.0);
    EXPECT_DOUBLE_EQ(res.data.nodes[0].temperature, 100.0);
    EXPECT_DOUBLE_EQ(res.data.nodes[1].temperature, 125.0);
    EXPECT_DOUBLE_EQ(res.data.nodes[2].temperature, 20.0);
}

TEST(CsvParserTest, AnsysCsv_MissingColumnsRejection) {
    std::string missing_col = "node_id,x_m,y_m\n1,0,0\n";
    auto res = parse_temperature_csv(missing_col);
    EXPECT_FALSE(res.success);
    EXPECT_NE(res.error.find("missing required columns"), std::string::npos);
}

TEST(CsvParserTest, AnsysCsv_NonFiniteValuesRejection) {
    std::string nan_csv = "node_id,x_m,y_m,z_m,temperature_C\n1,0.0,0.0,0.0,NaN\n";
    auto res = parse_temperature_csv(nan_csv);
    EXPECT_FALSE(res.success);
    EXPECT_NE(res.error.find("non-finite"), std::string::npos);

    std::string inf_csv = "node_id,x_m,y_m,z_m,temperature_C\n1,0.0,0.0,0.0,Infinity\n";
    EXPECT_FALSE(parse_temperature_csv(inf_csv).success);
}

TEST(CsvParserTest, AnsysCsv_HeaderOnlyRejection) {
    std::string header_only = "node_id,x_m,y_m,z_m,temperature_C\n";
    auto res = parse_temperature_csv(header_only);
    EXPECT_FALSE(res.success);
    EXPECT_NE(res.error.find("no data rows"), std::string::npos);
}

// ===========================================================================
// 3. Thermal FEA Solver & Convergence Tests (Task C)
// ===========================================================================

class ThermalFemFixture : public ::testing::Test {
protected:
    ThermalParams params;

    void SetUp() override {
        params.length = 1.0;
        params.area = 1e-4;
        params.conductivity = 10.0;
        params.heat_generation = 10000.0;
        params.temp_left = 100.0;
        params.temp_right = 20.0;
    }
};

TEST_F(ThermalFemFixture, BoundaryConditions_EnforcedExactly) {
    auto res = solve_thermal_1d(params, 10);
    ASSERT_TRUE(res.success) << res.error;
    EXPECT_NEAR(res.value.node_temp.front(), 100.0, 1e-9);
    EXPECT_NEAR(res.value.node_temp.back(), 20.0, 1e-9);
}

TEST_F(ThermalFemFixture, AnalyticalSolution_NodalAccuracy) {
    auto res = solve_thermal_1d(params, 20);
    ASSERT_TRUE(res.success);

    // Green's function reproducing quadratic polynomial at nodes
    for (size_t i = 0; i < res.value.node_x.size(); ++i) {
        double x = res.value.node_x[i];
        double exact = analytical_temperature(params, x);
        EXPECT_NEAR(res.value.node_temp[i], exact, 1e-7)
            << "Nodal mismatch at node " << i << " (x = " << x << ")";
    }
}

TEST_F(ThermalFemFixture, MeshConvergence_QuadraticRate) {
    // Test N = 10, 20, 40, 80
    auto study = run_convergence_study(params, {10, 20, 40, 80});
    ASSERT_TRUE(study.success) << study.error;
    ASSERT_EQ(study.value.summary.size(), 4);

    // Midpoint theoretical errors: E(h) = (q / 8k) * h^2 = 125 * h^2
    // N=10 (h=0.1)   -> 1.25 C
    // N=20 (h=0.05)  -> 0.3125 C
    // N=40 (h=0.025) -> 0.078125 C
    // N=80 (h=0.0125)-> 0.01953125 C
    EXPECT_NEAR(study.value.summary[0].max_midpoint_error, 1.250000, 1e-5);
    EXPECT_NEAR(study.value.summary[1].max_midpoint_error, 0.312500, 1e-5);
    EXPECT_NEAR(study.value.summary[2].max_midpoint_error, 0.078125, 1e-5);
    EXPECT_NEAR(study.value.summary[3].max_midpoint_error, 0.019531, 1e-5);

    // Convergence rate p = ln(E1/E2) / ln(h1/h2) must equal 2.0 (quadratic O(h^2))
    for (size_t i = 1; i < study.value.summary.size(); ++i) {
        EXPECT_NEAR(study.value.summary[i].convergence_rate, 2.0000, 0.02)
            << "Rate at step " << i << " did not match O(h^2)";
    }
}

TEST_F(ThermalFemFixture, PhysicalParameter_Validation) {
    ThermalParams bad_p = params;
    bad_p.conductivity = -1.0;
    EXPECT_FALSE(solve_thermal_1d(bad_p, 10).success);

    bad_p = params;
    bad_p.length = -0.5;
    EXPECT_FALSE(solve_thermal_1d(bad_p, 10).success);

    bad_p = params;
    bad_p.area = 0.0;
    EXPECT_FALSE(solve_thermal_1d(bad_p, 10).success);

    EXPECT_FALSE(solve_thermal_1d(params, 0).success);
    EXPECT_FALSE(solve_thermal_1d(params, -5).success);
}

TEST_F(ThermalFemFixture, StressParameters_MicroAndMacroScale) {
    // Micro-scale
    ThermalParams micro;
    micro.length = 1e-6;
    micro.area = 1e-10;
    micro.conductivity = 1e-3;
    micro.heat_generation = 0.1;
    micro.temp_left = 10.0;
    micro.temp_right = 5.0;
    auto res_micro = solve_thermal_1d(micro, 10);
    EXPECT_TRUE(res_micro.success);
    EXPECT_FALSE(res_micro.value.node_temp.empty());

    // Macro-scale
    ThermalParams macro;
    macro.length = 1000.0;
    macro.area = 100.0;
    macro.conductivity = 5000.0;
    macro.heat_generation = 1e7;
    macro.temp_left = 500.0;
    macro.temp_right = 100.0;
    auto res_macro = solve_thermal_1d(macro, 20);
    EXPECT_TRUE(res_macro.success);
    EXPECT_FALSE(res_macro.value.node_temp.empty());
}

// ===========================================================================
// 4. Multicore Parallel Worker Pool Tests (Task D)
// ===========================================================================

TEST(MulticoreParallelTest, SerialVersusParallel_NumericalConsistency) {
    BatchConfig cfg;
    cfg.num_cases = 100;
    cfg.mesh_size = 20;
    cfg.random_seed = 42;

    auto cases = generate_thermal_batch(cfg);
    ASSERT_EQ(cases.size(), 100);

    auto serial_res = solve_batch_serial(cases);
    auto parallel_res = solve_batch_parallel(cases, 4);

    ASSERT_EQ(serial_res.size(), parallel_res.size());

    double max_diff = 0.0;
    for (size_t c = 0; c < serial_res.size(); ++c) {
        ASSERT_EQ(serial_res[c].node_temp.size(), parallel_res[c].node_temp.size());
        for (size_t n = 0; n < serial_res[c].node_temp.size(); ++n) {
            double diff = std::abs(serial_res[c].node_temp[n] - parallel_res[c].node_temp[n]);
            if (diff > max_diff) max_diff = diff;
        }
    }

    // Must be <= 1e-8 C per specification
    EXPECT_LE(max_diff, 1e-8);
    EXPECT_DOUBLE_EQ(max_diff, 0.0);
}

TEST(MulticoreParallelTest, DeterministicBatchOrdering) {
    BatchConfig cfg;
    cfg.num_cases = 50;
    cfg.mesh_size = 15;
    cfg.random_seed = 9999;

    auto run1_cases = generate_thermal_batch(cfg);
    auto run2_cases = generate_thermal_batch(cfg);

    ASSERT_EQ(run1_cases.size(), run2_cases.size());
    for (size_t i = 0; i < run1_cases.size(); ++i) {
        EXPECT_EQ(run1_cases[i].case_id, static_cast<int>(i));
        EXPECT_DOUBLE_EQ(run1_cases[i].params.conductivity, run2_cases[i].params.conductivity);
        EXPECT_DOUBLE_EQ(run1_cases[i].params.heat_generation, run2_cases[i].params.heat_generation);
    }
}

// ===========================================================================
// 5. Embedded Hardware Adapters Tests (Task E)
// ===========================================================================

TEST(EmbeddedHardwareTest, DeviceDiscovery) {
    DeviceManager mgr;
    auto devs = mgr.discover_devices();
    ASSERT_EQ(devs.size(), 3);
    EXPECT_EQ(devs[0].device_id, "MCU-STM32F4-01");
    EXPECT_EQ(devs[1].device_id, "FPGA-ARTIX7-02");
    EXPECT_EQ(devs[2].device_id, "JTAG-JLINK-03");
}

TEST(EmbeddedHardwareTest, McuConnectionAndTelemetry) {
    DeviceManager mgr;
    auto conn = mgr.connect_device("MCU-STM32F4-01");
    EXPECT_TRUE(conn.success);

    auto telem = mgr.get_telemetry("MCU-STM32F4-01");
    ASSERT_TRUE(telem.success);
    EXPECT_GT(telem.value.voltage_v, 3.0);
    EXPECT_GT(telem.value.temperature_c, 20.0);

    auto disc = mgr.disconnect_device("MCU-STM32F4-01");
    EXPECT_TRUE(disc.success);
}

TEST(EmbeddedHardwareTest, FpgaConnectionAndTelemetry) {
    DeviceManager mgr;
    auto conn = mgr.connect_device("FPGA-ARTIX7-02");
    EXPECT_TRUE(conn.success);

    auto telem = mgr.get_telemetry("FPGA-ARTIX7-02");
    ASSERT_TRUE(telem.success);
    EXPECT_FALSE(telem.value.metrics.empty());

    mgr.disconnect_device("FPGA-ARTIX7-02");
}

TEST(EmbeddedHardwareTest, JtagTimeoutAndRecoveryLifecycle) {
    DeviceManager mgr;

    // Attempt 1: Simulates recoverable timeout
    auto attempt1 = mgr.connect_device("JTAG-JLINK-03");
    EXPECT_FALSE(attempt1.success);
    EXPECT_NE(attempt1.error.find("timed out"), std::string::npos);

    // Attempt 2: Reconnection succeeds
    auto attempt2 = mgr.connect_device("JTAG-JLINK-03");
    EXPECT_TRUE(attempt2.success);

    auto telem = mgr.get_telemetry("JTAG-JLINK-03");
    EXPECT_TRUE(telem.success);

    mgr.disconnect_device("JTAG-JLINK-03");
}

TEST(EmbeddedHardwareTest, UnknownDeviceRejection) {
    DeviceManager mgr;
    EXPECT_FALSE(mgr.connect_device("UNKNOWN-99").success);
    EXPECT_FALSE(mgr.get_telemetry("UNKNOWN-99").success);
}

// ===========================================================================
// 6. Neural Backpropagation Tests (Task F)
// ===========================================================================

TEST(BackpropagationTest, ForwardPass_ExactValues) {
    NetworkParams p;
    p.w = 0.5; p.b = 0.0; p.v = 1.0; p.c = 0.0;
    double x = 1.0; double y = 2.0;

    auto fwd = forward_pass(p, x, y);
    EXPECT_DOUBLE_EQ(fwd.z, 0.5);
    EXPECT_NEAR(fwd.h, std::tanh(0.5), 1e-9);
    EXPECT_NEAR(fwd.y_hat, std::tanh(0.5), 1e-9);
    EXPECT_NEAR(fwd.loss, 0.5 * std::pow(std::tanh(0.5) - 2.0, 2.0), 1e-9);
}

TEST(BackpropagationTest, GradientVerification_CentralDifferences) {
    NetworkParams p;
    p.w = 0.5; p.b = 0.0; p.v = 1.0; p.c = 0.0;
    double x = 1.0; double y = 2.0;

    auto ver = verify_gradients_finite_difference(p, x, y, 1e-6, 1e-5);
    EXPECT_TRUE(ver.all_passed);
    EXPECT_LE(ver.absolute_error.dJ_dw, 1e-5);
    EXPECT_LE(ver.absolute_error.dJ_db, 1e-5);
    EXPECT_LE(ver.absolute_error.dJ_dv, 1e-5);
    EXPECT_LE(ver.absolute_error.dJ_dc, 1e-5);
}

TEST(BackpropagationTest, SimultaneousParameterUpdate_Step1) {
    auto analysis = run_backpropagation_analysis(1.0, 2.0, 0.1, 100);
    auto u = analysis.updated_params_step1;

    // Verified analytical step-1 updates:
    // w1 = 0.620946, b1 = 0.120946, v1 = 1.071068, c1 = 0.153788
    EXPECT_NEAR(u.w, 0.620946, 1e-5);
    EXPECT_NEAR(u.b, 0.120946, 1e-5);
    EXPECT_NEAR(u.v, 1.071068, 1e-5);
    EXPECT_NEAR(u.c, 0.153788, 1e-5);
}

TEST(BackpropagationTest, TrainingDescent_100Steps) {
    auto analysis = run_backpropagation_analysis(1.0, 2.0, 0.1, 100);
    EXPECT_LT(analysis.final_loss, 1e-4);
    EXPECT_LT(analysis.final_loss, analysis.initial_forward.loss);
    // History includes initial step 0 + 100 training steps = 101 records
    EXPECT_EQ(analysis.training_history.size(), 101);
    EXPECT_EQ(analysis.training_history.back().step, 100);
}

// ===========================================================================
// 7. Core Linear Algebra Tests
// ===========================================================================

TEST(LinearAlgebraTest, ThomasAlgorithm_TridiagonalSystem) {
    // 3x3 system:
    // [ 2 -1  0 ] [x0]   [1]
    // [-1  2 -1 ] [x1] = [0]  ==> Solution: x = [1, 1, 1]
    // [ 0 -1  2 ] [x2]   [1]
    std::vector<double> a = {-1.0, -1.0};
    std::vector<double> b = {2.0, 2.0, 2.0};
    std::vector<double> c = {-1.0, -1.0};
    std::vector<double> d = {1.0, 0.0, 1.0};

    auto res = solve_tridiagonal(a, b, c, d);
    ASSERT_TRUE(res.success);
    ASSERT_EQ(res.value.size(), 3);
    EXPECT_NEAR(res.value[0], 1.0, 1e-9);
    EXPECT_NEAR(res.value[1], 1.0, 1e-9);
    EXPECT_NEAR(res.value[2], 1.0, 1e-9);
}

TEST(LinearAlgebraTest, ThomasAlgorithm_ZeroPivotDetection) {
    std::vector<double> a = {-1.0};
    std::vector<double> b = {0.0, 2.0}; // Leading zero pivot
    std::vector<double> c = {-1.0};
    std::vector<double> d = {1.0, 1.0};

    auto res = solve_tridiagonal(a, b, c, d);
    EXPECT_FALSE(res.success);
    EXPECT_NE(res.error.find("zero pivot"), std::string::npos);
}
