#pragma once

#include "heatsink_mesh.h"

#include <vector>
#include <string>
#include <chrono>
#include <memory>
#include <Eigen/Sparse>

namespace workbench {

enum class SolverType {
    DirectSparseLU,
    PreconditionedCG,
    AMG_PreconditionedCG
};

struct ThermalParams {
    double thermal_conductivity = 205.0; // k = 205 W/(m*K) (Aluminium 6061)
    double convection_coeff = 25.0;      // h = 25 W/(m^2*K) (Air convection)
    double ambient_temperature = 25.0;   // T_amb = 25.0 °C
    double heat_source_power = 75.0;     // P_in = 75 W
};

struct SolverResult {
    SolverType type = SolverType::DirectSparseLU;
    std::string solver_name;
    std::string mesh_name;
    int node_count = 0;
    int element_count = 0;
    int nonzero_count = 0;

    double setup_time_ms = 0.0;
    double solve_time_ms = 0.0;
    double total_time_ms = 0.0;

    int iterations = 0;
    double relative_residual = 0.0;
    double min_temperature = 0.0;
    double max_temperature = 0.0;
    double avg_temperature = 0.0;

    size_t peak_memory_bytes = 0;
    double energy_imbalance_percent = 0.0;

    std::vector<double> temperatures;
    std::vector<double> convergence_history; // Relative residual per iteration if available
};

struct BenchmarkRun {
    std::string mesh_name;
    MeshResolution resolution;
    std::vector<SolverResult> solver_results; // [DirectLU, PCG, AMG-PCG]
};

class HeatsinkSolver {
public:
    HeatsinkSolver();
    explicit HeatsinkSolver(const ThermalParams& params);

    void set_params(const ThermalParams& p) { params_ = p; }
    const ThermalParams& params() const { return params_; }

    // Assemble FEM global stiffness matrix and load vector
    bool assemble_system(const HeatsinkMesh& mesh,
                         Eigen::SparseMatrix<double, Eigen::RowMajor>& K,
                         Eigen::VectorXd& b);

    // Solve using Solver 1: Direct Sparse LU
    SolverResult solve_sparse_lu(const HeatsinkMesh& mesh,
                                 const Eigen::SparseMatrix<double, Eigen::RowMajor>& K,
                                 const Eigen::VectorXd& b);

    // Solve using Solver 2: PCG with Incomplete Cholesky
    SolverResult solve_pcg(const HeatsinkMesh& mesh,
                           const Eigen::SparseMatrix<double, Eigen::RowMajor>& K,
                           const Eigen::VectorXd& b,
                           double tol = 1e-7,
                           int max_iters = 1000);

    // Solve using Solver 3: AMG-PCG (AMGCL)
    SolverResult solve_amg_pcg(const HeatsinkMesh& mesh,
                               const Eigen::SparseMatrix<double, Eigen::RowMajor>& K,
                               const Eigen::VectorXd& b,
                               double tol = 1e-7,
                               int max_iters = 1000);

    // Run full benchmark: 4 mesh resolutions, 3 solvers each, 5 repetitions
    std::vector<BenchmarkRun> run_benchmark(int num_repetitions = 5);

    // Verify thermal energy balance (heat in vs heat dissipated via convection)
    double check_energy_balance(const HeatsinkMesh& mesh,
                                const std::vector<double>& temperatures) const;

    // Export benchmark summary to CSV
    static bool export_benchmark_csv(const std::string& filepath,
                                     const std::vector<BenchmarkRun>& runs);

private:
    ThermalParams params_;
};

} // namespace workbench
