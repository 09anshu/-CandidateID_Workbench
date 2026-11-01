#pragma once

#include <string>
#include <vector>

#include "../core/types.h"

namespace workbench {

/// Physical parameters for the 1D steady-state thermal problem
struct ThermalParams {
    double length = 1.0;          // Rod length L (m)
    double area = 1e-4;           // Cross-sectional area A (m^2)
    double conductivity = 10.0;   // Thermal conductivity k (W/(m*K))
    double heat_generation = 10000.0; // Volumetric heat generation q (W/m^3)
    double temp_left = 100.0;     // Boundary temperature T(0) (°C)
    double temp_right = 20.0;     // Boundary temperature T(L) (°C)
};

/// Midpoint error evaluation for an element
struct MidpointEvaluation {
    int element_index = 0;
    double x_mid = 0.0;
    double temp_fem = 0.0;
    double temp_analytical = 0.0;
    double absolute_error = 0.0;
};

/// Results from a single FEM solve
struct ThermalResult {
    int num_elements = 0;
    double h = 0.0; // element size
    std::vector<double> node_x;
    std::vector<double> node_temp;
    std::vector<MidpointEvaluation> midpoints;
    double max_midpoint_error = 0.0;
    double l2_norm_error = 0.0;
};

/// Convergence study across multiple mesh configurations (e.g., N=10, 20, 40, 80)
struct ConvergenceCase {
    int num_elements = 0;
    double h = 0.0;
    double max_midpoint_error = 0.0;
    double l2_error = 0.0;
    double convergence_rate = 0.0; // Empirical rate p where Error ~ C * h^p
};

struct ConvergenceStudyResult {
    ThermalParams params;
    std::vector<ThermalResult> mesh_results;
    std::vector<ConvergenceCase> summary;
};

/// Validate physical parameters
Result<bool> validate_thermal_params(const ThermalParams& p, int num_elements);

/// Compute the exact analytical temperature at position x
double analytical_temperature(const ThermalParams& p, double x);

/// Solve the 1D steady-state thermal FEA using linear two-node elements
Result<ThermalResult> solve_thermal_1d(const ThermalParams& p, int num_elements);

/// Run convergence study for meshes N = 10, 20, 40, 80
Result<ConvergenceStudyResult> run_convergence_study(const ThermalParams& p, const std::vector<int>& mesh_sizes = {10, 20, 40, 80});

/// Generate CSV string for export
std::string export_thermal_csv(const ThermalResult& result, const ThermalParams& params);

/// Generate multi-mesh comparison CSV string
std::string export_convergence_csv(const ConvergenceStudyResult& study);

} // namespace workbench

