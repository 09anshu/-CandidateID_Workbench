#include "thermal_solver.h"

#include <cmath>
#include <iomanip>
#include <sstream>

#include "../core/linear_algebra.h"

namespace workbench {

Result<bool> validate_thermal_params(const ThermalParams& p, int num_elements) {
    if (p.length <= 0.0 || !is_finite(p.length)) {
        return Result<bool>::fail("Invalid rod length L: must be positive and finite.");
    }
    if (p.area <= 0.0 || !is_finite(p.area)) {
        return Result<bool>::fail("Invalid cross-sectional area A: must be positive and finite.");
    }
    if (p.conductivity <= 0.0 || !is_finite(p.conductivity)) {
        return Result<bool>::fail("Invalid thermal conductivity k: must be positive and finite.");
    }
    if (!is_finite(p.heat_generation)) {
        return Result<bool>::fail("Invalid volumetric heat generation q: must be finite.");
    }
    if (!is_finite(p.temp_left) || !is_finite(p.temp_right)) {
        return Result<bool>::fail("Invalid boundary temperatures: must be finite.");
    }
    if (num_elements < 1 || num_elements > 100000) {
        return Result<bool>::fail("Invalid element count N: must be between 1 and 100,000.");
    }
    return Result<bool>::ok(true);
}

double analytical_temperature(const ThermalParams& p, double x) {
    // T_exact(x) = T(0) - (T(0) - T(L))*(x/L) + [q / (2*k)] * x * (L - x)
    double linear_part = p.temp_left - (p.temp_left - p.temp_right) * (x / p.length);
    double quadratic_part = (p.heat_generation / (2.0 * p.conductivity)) * x * (p.length - x);
    return linear_part + quadratic_part;
}

Result<ThermalResult> solve_thermal_1d(const ThermalParams& p, int num_elements) {
    auto val = validate_thermal_params(p, num_elements);
    if (!val.success) {
        return Result<ThermalResult>::fail(val.error);
    }

    const int n_nodes = num_elements + 1;
    const double h = p.length / static_cast<double>(num_elements);

    // 1D Two-node linear element matrices:
    // K_e = (k * A / h) * [ 1, -1; -1, 1 ]
    // f_e = (q * A * h / 2) * [ 1; 1 ]
    const double k_elem = (p.conductivity * p.area) / h;
    const double f_elem = (p.heat_generation * p.area * h) / 2.0;

    // We assemble the tridiagonal system:
    // a: sub-diagonal (size n_nodes - 1, indices 0..n_nodes-2)
    // b: main diagonal (size n_nodes)
    // c: super-diagonal (size n_nodes - 1, indices 0..n_nodes-2)
    // d: load vector (size n_nodes)
    std::vector<double> a(n_nodes - 1, 0.0);
    std::vector<double> b(n_nodes, 0.0);
    std::vector<double> c(n_nodes - 1, 0.0);
    std::vector<double> d(n_nodes, 0.0);

    // Assembly loop over elements
    for (int e = 0; e < num_elements; ++e) {
        int n1 = e;
        int n2 = e + 1;

        // Node n1
        b[n1] += k_elem;
        d[n1] += f_elem;

        // Off-diagonal connection
        c[n1] -= k_elem;
        a[n1] -= k_elem;

        // Node n2
        b[n2] += k_elem;
        d[n2] += f_elem;
    }

    // Apply Dirichlet Boundary Conditions:
    // T(0) = T_left at node 0
    // T(L) = T_right at node N
    //
    // For row 0: T[0] = T_left
    // Modify row 1 to maintain tridiagonal form: d[1] -= a[0] * T_left; a[0] = 0.0;
    d[1] -= a[0] * p.temp_left;
    a[0] = 0.0;
    b[0] = 1.0;
    c[0] = 0.0;
    d[0] = p.temp_left;

    // For row N: T[N] = T_right
    // Modify row N-1: d[N-1] -= c[N-2] * T_right; c[N-2] = 0.0;
    d[n_nodes - 2] -= c[n_nodes - 2] * p.temp_right;
    c[n_nodes - 2] = 0.0;
    b[n_nodes - 1] = 1.0;
    a[n_nodes - 2] = 0.0;
    d[n_nodes - 1] = p.temp_right;

    // Solve tridiagonal system via Thomas algorithm
    auto solve_res = solve_tridiagonal(a, b, c, d);
    if (!solve_res.success) {
        return Result<ThermalResult>::fail("Thermal FEA solve failed: " + solve_res.error);
    }

    ThermalResult result;
    result.num_elements = num_elements;
    result.h = h;
    result.node_temp = std::move(solve_res.value);
    result.node_x.resize(n_nodes);

    for (int i = 0; i < n_nodes; ++i) {
        result.node_x[i] = i * h;
    }

    // Evaluate error at element midpoints using FE linear interpolation
    result.midpoints.resize(num_elements);
    double max_err = 0.0;
    double sum_sq_err = 0.0;

    for (int e = 0; e < num_elements; ++e) {
        double x_m = (result.node_x[e] + result.node_x[e + 1]) / 2.0;
        // Linear interpolation inside element e at midpoint
        double T_fem_mid = 0.5 * (result.node_temp[e] + result.node_temp[e + 1]);
        double T_exact_mid = analytical_temperature(p, x_m);
        double err = std::abs(T_fem_mid - T_exact_mid);

        result.midpoints[e].element_index = e;
        result.midpoints[e].x_mid = x_m;
        result.midpoints[e].temp_fem = T_fem_mid;
        result.midpoints[e].temp_analytical = T_exact_mid;
        result.midpoints[e].absolute_error = err;

        if (err > max_err) {
            max_err = err;
        }
        sum_sq_err += err * err;
    }

    result.max_midpoint_error = max_err;
    result.l2_norm_error = std::sqrt(sum_sq_err / static_cast<double>(num_elements));

    return Result<ThermalResult>::ok(std::move(result));
}

Result<ConvergenceStudyResult> run_convergence_study(const ThermalParams& p, const std::vector<int>& mesh_sizes) {
    ConvergenceStudyResult study;
    study.params = p;

    for (size_t i = 0; i < mesh_sizes.size(); ++i) {
        int N = mesh_sizes[i];
        auto res = solve_thermal_1d(p, N);
        if (!res.success) {
            return Result<ConvergenceStudyResult>::fail("Convergence study failed at N=" + std::to_string(N) + ": " + res.error);
        }

        ConvergenceCase c_case;
        c_case.num_elements = N;
        c_case.h = res.value.h;
        c_case.max_midpoint_error = res.value.max_midpoint_error;
        c_case.l2_error = res.value.l2_norm_error;

        if (i > 0) {
            double h_prev = study.summary[i - 1].h;
            double err_prev = study.summary[i - 1].max_midpoint_error;
            double h_curr = c_case.h;
            double err_curr = c_case.max_midpoint_error;
            if (err_curr > 0.0 && err_prev > 0.0) {
                c_case.convergence_rate = std::log(err_prev / err_curr) / std::log(h_prev / h_curr);
            }
        }

        study.summary.push_back(c_case);
        study.mesh_results.push_back(std::move(res.value));
    }

    return Result<ConvergenceStudyResult>::ok(std::move(study));
}

std::string export_thermal_csv(const ThermalResult& result, const ThermalParams& params) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(8);
    ss << "# PASS SCIENCE WORKS - 1D Thermal FEA Results\n";
    ss << "# Length (m): " << params.length << "\n";
    ss << "# Area (m^2): " << params.area << "\n";
    ss << "# Conductivity k (W/(m*K)): " << params.conductivity << "\n";
    ss << "# Heat Generation q (W/m^3): " << params.heat_generation << "\n";
    ss << "# T(0) (°C): " << params.temp_left << ", T(L) (°C): " << params.temp_right << "\n";
    ss << "# Number of Elements: " << result.num_elements << "\n";
    ss << "# Max Midpoint Error (°C): " << result.max_midpoint_error << "\n";
    ss << "node_index,x_m,temp_fem_C,temp_analytical_C,error_C\n";

    for (size_t i = 0; i < result.node_x.size(); ++i) {
        double x = result.node_x[i];
        double t_fem = result.node_temp[i];
        double t_exact = analytical_temperature(params, x);
        double err = std::abs(t_fem - t_exact);
        ss << i << "," << x << "," << t_fem << "," << t_exact << "," << err << "\n";
    }

    ss << "\n# Midpoint Evaluations\n";
    ss << "element_index,x_mid_m,temp_fem_mid_C,temp_analytical_mid_C,midpoint_error_C\n";
    for (const auto& mid : result.midpoints) {
        ss << mid.element_index << "," << mid.x_mid << "," << mid.temp_fem << "," << mid.temp_analytical << "," << mid.absolute_error << "\n";
    }

    return ss.str();
}

std::string export_convergence_csv(const ConvergenceStudyResult& study) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(8);
    ss << "# PASS SCIENCE WORKS - Thermal Mesh Convergence Study\n";
    ss << "elements_N,mesh_size_h_m,max_midpoint_error_C,l2_error_C,empirical_convergence_rate\n";
    for (const auto& c : study.summary) {
        ss << c.num_elements << "," << c.h << "," << c.max_midpoint_error << "," << c.l2_error << "," << c.convergence_rate << "\n";
    }
    return ss.str();
}

} // namespace workbench
