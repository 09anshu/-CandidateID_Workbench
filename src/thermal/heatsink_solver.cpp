#include "heatsink_solver.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <cmath>
#include <algorithm>

#include <Eigen/Dense>
#include <Eigen/SparseLU>
#include <Eigen/IterativeLinearSolvers>

#ifndef AMGCL_NO_BOOST
#define AMGCL_NO_BOOST
#endif
#include <amgcl/adapter/eigen.hpp>
#include <amgcl/backend/builtin.hpp>
#include <amgcl/make_solver.hpp>
#include <amgcl/solver/cg.hpp>
#include <amgcl/amg.hpp>
#include <amgcl/coarsening/smoothed_aggregation.hpp>
#include <amgcl/relaxation/spai0.hpp>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

namespace workbench {

namespace {

size_t get_current_process_memory() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
#endif
    return 0;
}

double compute_median(std::vector<double>& vals) {
    if (vals.empty()) return 0.0;
    std::sort(vals.begin(), vals.end());
    size_t mid = vals.size() / 2;
    if (vals.size() % 2 == 0) {
        return (vals[mid - 1] + vals[mid]) * 0.5;
    }
    return vals[mid];
}

} // anonymous namespace

HeatsinkSolver::HeatsinkSolver()
    : params_{} {}

HeatsinkSolver::HeatsinkSolver(const ThermalParams& params)
    : params_(params) {}

bool HeatsinkSolver::assemble_system(const HeatsinkMesh& mesh,
                                    Eigen::SparseMatrix<double, Eigen::RowMajor>& K,
                                    Eigen::VectorXd& b) {
    const int num_nodes = mesh.num_nodes();
    if (num_nodes == 0 || mesh.num_elements() == 0) return false;

    K.resize(num_nodes, num_nodes);
    b = Eigen::VectorXd::Zero(num_nodes);

    // Reserve estimated triplet storage (15 entries per node average)
    std::vector<Eigen::Triplet<double>> triplets;
    triplets.reserve(mesh.elements.size() * 16 + mesh.boundary_faces.size() * 9);

    // 1. Internal Conduction Stiffness (k * integral(grad N_i . grad N_j dV))
    for (const auto& el : mesh.elements) {
        const Point3D& p0 = mesh.nodes[el.v[0]];
        const Point3D& p1 = mesh.nodes[el.v[1]];
        const Point3D& p2 = mesh.nodes[el.v[2]];
        const Point3D& p3 = mesh.nodes[el.v[3]];

        Eigen::Matrix4d C;
        C << 1.0, p0.x, p0.y, p0.z,
             1.0, p1.x, p1.y, p1.z,
             1.0, p2.x, p2.y, p2.z,
             1.0, p3.x, p3.y, p3.z;

        double det = C.determinant();
        double vol = std::abs(det) / 6.0;
        if (vol < 1e-16) continue;

        Eigen::Matrix4d invC = C.inverse();

        // Gradients of linear shape functions are rows 1, 2, 3 of invC
        Eigen::Vector3d gradN[4];
        for (int i = 0; i < 4; ++i) {
            gradN[i] = Eigen::Vector3d(invC(1, i), invC(2, i), invC(3, i));
        }

        // Add element conduction matrix
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                double kij = params_.thermal_conductivity * vol * gradN[i].dot(gradN[j]);
                triplets.emplace_back(el.v[i], el.v[j], kij);
            }
        }
    }

    // 2. Compute heat flux input per unit area
    double total_heat_src_area = 0.0;
    for (const auto& bf : mesh.boundary_faces) {
        if (bf.type == BoundaryType::HeatFlux) {
            total_heat_src_area += bf.area;
        }
    }
    double heat_flux = (total_heat_src_area > 1e-12) ?
                       (params_.heat_source_power / total_heat_src_area) : 0.0;

    // 3. Boundary Convection and Heat Flux terms
    for (const auto& bf : mesh.boundary_faces) {
        if (bf.type == BoundaryType::Convection) {
            // Convection stiffness contribution: (h * Area / 12) * [2 1 1; 1 2 1; 1 1 2]
            double h_area_12 = (params_.convection_coeff * bf.area) / 12.0;
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    double weight = (i == j) ? 2.0 : 1.0;
                    triplets.emplace_back(bf.v[i], bf.v[j], weight * h_area_12);
                }
            }

            // Convection load contribution: (h * T_amb * Area / 3) * [1; 1; 1]
            double f_conv = (params_.convection_coeff * params_.ambient_temperature * bf.area) / 3.0;
            b[bf.v[0]] += f_conv;
            b[bf.v[1]] += f_conv;
            b[bf.v[2]] += f_conv;
        } else if (bf.type == BoundaryType::HeatFlux) {
            // Heat flux load contribution: (q_in * Area / 3) * [1; 1; 1]
            double f_flux = (heat_flux * bf.area) / 3.0;
            b[bf.v[0]] += f_flux;
            b[bf.v[1]] += f_flux;
            b[bf.v[2]] += f_flux;
        }
    }

    // Assemble and compress matrix
    K.setFromTriplets(triplets.begin(), triplets.end());
    K.makeCompressed();

    return true;
}

SolverResult HeatsinkSolver::solve_sparse_lu(const HeatsinkMesh& mesh,
                                            const Eigen::SparseMatrix<double, Eigen::RowMajor>& K,
                                            const Eigen::VectorXd& b) {
    SolverResult res;
    res.type = SolverType::DirectSparseLU;
    res.solver_name = "Eigen SparseLU (Direct)";
    res.node_count = mesh.num_nodes();
    res.element_count = mesh.num_elements();
    res.nonzero_count = static_cast<int>(K.nonZeros());

    size_t mem_before = get_current_process_memory();

    // Setup: Pattern analysis + Factorization
    auto t0 = std::chrono::high_resolution_clock::now();
    Eigen::SparseLU<Eigen::SparseMatrix<double, Eigen::RowMajor>> solver;
    solver.analyzePattern(K);
    solver.factorize(K);
    auto t1 = std::chrono::high_resolution_clock::now();

    res.setup_time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    if (solver.info() != Eigen::Success) {
        std::cerr << "SparseLU factorization failed!" << std::endl;
        return res;
    }

    // Solve: Forward/Backward substitution
    auto t2 = std::chrono::high_resolution_clock::now();
    Eigen::VectorXd x = solver.solve(b);
    auto t3 = std::chrono::high_resolution_clock::now();

    res.solve_time_ms = std::chrono::duration<double, std::milli>(t3 - t2).count();
    res.total_time_ms = res.setup_time_ms + res.solve_time_ms;

    size_t mem_after = get_current_process_memory();
    res.peak_memory_bytes = (mem_after > mem_before) ? (mem_after - mem_before) : sizeof(double) * K.nonZeros() * 3;

    // Residual: ||K*x - b|| / ||b||
    Eigen::VectorXd residual = K * x - b;
    res.relative_residual = residual.norm() / b.norm();
    res.iterations = 1; // Direct solver completes in 1 step

    // Temperature field statistics
    res.temperatures.resize(x.size());
    for (int i = 0; i < x.size(); ++i) {
        res.temperatures[i] = x[i];
    }
    res.min_temperature = *std::min_element(res.temperatures.begin(), res.temperatures.end());
    res.max_temperature = *std::max_element(res.temperatures.begin(), res.temperatures.end());
    res.avg_temperature = std::accumulate(res.temperatures.begin(), res.temperatures.end(), 0.0) / res.temperatures.size();

    res.energy_imbalance_percent = check_energy_balance(mesh, res.temperatures);

    return res;
}

SolverResult HeatsinkSolver::solve_pcg(const HeatsinkMesh& mesh,
                                      const Eigen::SparseMatrix<double, Eigen::RowMajor>& K,
                                      const Eigen::VectorXd& b,
                                      double tol,
                                      int max_iters) {
    SolverResult res;
    res.type = SolverType::PreconditionedCG;
    res.solver_name = "Eigen PCG (Incomplete Cholesky)";
    res.node_count = mesh.num_nodes();
    res.element_count = mesh.num_elements();
    res.nonzero_count = static_cast<int>(K.nonZeros());

    size_t mem_before = get_current_process_memory();

    // Setup: Incomplete Cholesky preconditioner compute
    auto t0 = std::chrono::high_resolution_clock::now();
    Eigen::ConjugateGradient<Eigen::SparseMatrix<double, Eigen::RowMajor>,
                             Eigen::Lower | Eigen::Upper,
                             Eigen::IncompleteCholesky<double>> solver;
    solver.setTolerance(tol);
    solver.setMaxIterations(max_iters);
    solver.compute(K);
    auto t1 = std::chrono::high_resolution_clock::now();

    res.setup_time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    if (solver.info() != Eigen::Success) {
        std::cerr << "Incomplete Cholesky preconditioner failed!" << std::endl;
        return res;
    }

    // Solve: Preconditioned CG iterations with ambient temperature initial guess
    Eigen::VectorXd x0 = Eigen::VectorXd::Constant(b.size(), params_.ambient_temperature);
    auto t2 = std::chrono::high_resolution_clock::now();
    Eigen::VectorXd x = solver.solveWithGuess(b, x0);
    auto t3 = std::chrono::high_resolution_clock::now();

    res.solve_time_ms = std::chrono::duration<double, std::milli>(t3 - t2).count();
    res.total_time_ms = res.setup_time_ms + res.solve_time_ms;

    size_t mem_after = get_current_process_memory();
    res.peak_memory_bytes = (mem_after > mem_before) ? (mem_after - mem_before) : sizeof(double) * K.nonZeros() * 2;

    res.iterations = static_cast<int>(solver.iterations());
    Eigen::VectorXd residual = K * x - b;
    res.relative_residual = residual.norm() / b.norm();

    // Temperature field statistics
    res.temperatures.resize(x.size());
    for (int i = 0; i < x.size(); ++i) {
        res.temperatures[i] = x[i];
    }
    res.min_temperature = *std::min_element(res.temperatures.begin(), res.temperatures.end());
    res.max_temperature = *std::max_element(res.temperatures.begin(), res.temperatures.end());
    res.avg_temperature = std::accumulate(res.temperatures.begin(), res.temperatures.end(), 0.0) / res.temperatures.size();

    res.energy_imbalance_percent = check_energy_balance(mesh, res.temperatures);

    return res;
}

SolverResult HeatsinkSolver::solve_amg_pcg(const HeatsinkMesh& mesh,
                                          const Eigen::SparseMatrix<double, Eigen::RowMajor>& K,
                                          const Eigen::VectorXd& b,
                                          double tol,
                                          int max_iters) {
    SolverResult res;
    res.type = SolverType::AMG_PreconditionedCG;
    res.solver_name = "AMG-PCG (AMGCL Smoothed Aggregation)";
    res.node_count = mesh.num_nodes();
    res.element_count = mesh.num_elements();
    res.nonzero_count = static_cast<int>(K.nonZeros());

    size_t mem_before = get_current_process_memory();

    typedef amgcl::backend::builtin<double> Backend;
    typedef amgcl::make_solver<
        amgcl::amg<
            Backend,
            amgcl::coarsening::smoothed_aggregation,
            amgcl::relaxation::spai0
        >,
        amgcl::solver::cg<Backend>
    > AMG_Solver;

    AMG_Solver::params prm;
    prm.solver.tol = tol;
    prm.solver.maxiter = max_iters;

    // Setup: Build Multigrid Hierarchy + Coarse Solvers
    auto t0 = std::chrono::high_resolution_clock::now();
    AMG_Solver solver(K, prm);
    auto t1 = std::chrono::high_resolution_clock::now();

    res.setup_time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // Prepare RHS and solution vectors
    std::vector<double> rhs(b.data(), b.data() + b.size());
    std::vector<double> x(b.size(), params_.ambient_temperature); // Initial guess

    // Solve: Multigrid preconditioned CG V-cycles
    auto t2 = std::chrono::high_resolution_clock::now();
    int iters = 0;
    double error = 0.0;
    std::tie(iters, error) = solver(rhs, x);
    auto t3 = std::chrono::high_resolution_clock::now();

    res.solve_time_ms = std::chrono::duration<double, std::milli>(t3 - t2).count();
    res.total_time_ms = res.setup_time_ms + res.solve_time_ms;

    size_t mem_after = get_current_process_memory();
    res.peak_memory_bytes = (mem_after > mem_before) ? (mem_after - mem_before) : sizeof(double) * K.nonZeros() * 3;

    res.iterations = iters;

    // Compute exact residual ||K*x - b|| / ||b||
    Eigen::Map<Eigen::VectorXd> x_vec(x.data(), x.size());
    Eigen::VectorXd residual = K * x_vec - b;
    res.relative_residual = residual.norm() / b.norm();

    // Temperature field statistics
    res.temperatures = x;
    res.min_temperature = *std::min_element(res.temperatures.begin(), res.temperatures.end());
    res.max_temperature = *std::max_element(res.temperatures.begin(), res.temperatures.end());
    res.avg_temperature = std::accumulate(res.temperatures.begin(), res.temperatures.end(), 0.0) / res.temperatures.size();

    res.energy_imbalance_percent = check_energy_balance(mesh, res.temperatures);

    return res;
}

double HeatsinkSolver::check_energy_balance(const HeatsinkMesh& mesh,
                                           const std::vector<double>& temperatures) const {
    double q_in = params_.heat_source_power; // Known input power in Watts

    // Total convection heat dissipation: integral of h * (T - T_amb) dA over all convection faces
    double q_out = 0.0;
    for (const auto& bf : mesh.boundary_faces) {
        if (bf.type == BoundaryType::Convection) {
            double t0 = temperatures[bf.v[0]];
            double t1 = temperatures[bf.v[1]];
            double t2 = temperatures[bf.v[2]];
            double avg_face_temp = (t0 + t1 + t2) / 3.0;
            q_out += params_.convection_coeff * (avg_face_temp - params_.ambient_temperature) * bf.area;
        }
    }

    if (q_in <= 1e-12) return 0.0;
    return (std::abs(q_in - q_out) / q_in) * 100.0; // Percentage error
}

std::vector<BenchmarkRun> HeatsinkSolver::run_benchmark(int num_repetitions) {
    std::vector<BenchmarkRun> runs;

    struct MeshConfig {
        std::string name;
        MeshResolution res;
    };

    std::vector<MeshConfig> configs = {
        {"Coarse", MeshResolution::Coarse},
        {"Medium", MeshResolution::Medium},
        {"Fine", MeshResolution::Fine},
        {"VeryFine", MeshResolution::VeryFine}
    };

    for (const auto& cfg : configs) {
        std::cout << "\n==========================================" << std::endl;
        std::cout << "Benchmarking Mesh: " << cfg.name << " (" << num_repetitions << " repetitions)" << std::endl;
        std::cout << "==========================================" << std::endl;

        HeatsinkMesh mesh;
        mesh.generate(cfg.res);

        std::cout << "Nodes: " << mesh.stats.node_count
                  << ", Elements: " << mesh.stats.element_count
                  << ", Boundary faces: " << mesh.stats.boundary_face_count << std::endl;

        Eigen::SparseMatrix<double, Eigen::RowMajor> K;
        Eigen::VectorXd b;
        assemble_system(mesh, K, b);

        std::cout << "Matrix assembled: " << K.rows() << " x " << K.cols()
                  << ", Nonzeros: " << K.nonZeros() << std::endl;

        BenchmarkRun run;
        run.mesh_name = cfg.name;
        run.resolution = cfg.res;

        // Repeat measurements for each solver and collect medians
        for (int solver_id = 0; solver_id < 3; ++solver_id) {
            std::vector<double> setup_times;
            std::vector<double> solve_times;
            std::vector<double> total_times;
            std::vector<size_t> mem_usages;
            SolverResult last_res;

            for (int rep = 0; rep < num_repetitions; ++rep) {
                SolverResult current_res;
                if (solver_id == 0) {
                    current_res = solve_sparse_lu(mesh, K, b);
                } else if (solver_id == 1) {
                    current_res = solve_pcg(mesh, K, b);
                } else {
                    current_res = solve_amg_pcg(mesh, K, b);
                }

                setup_times.push_back(current_res.setup_time_ms);
                solve_times.push_back(current_res.solve_time_ms);
                total_times.push_back(current_res.total_time_ms);
                mem_usages.push_back(current_res.peak_memory_bytes);
                last_res = std::move(current_res);
            }

            // Assign median times to reported result
            last_res.mesh_name = cfg.name;
            last_res.setup_time_ms = compute_median(setup_times);
            last_res.solve_time_ms = compute_median(solve_times);
            last_res.total_time_ms = compute_median(total_times);

            std::cout << "  [" << last_res.solver_name << "]"
                      << " Setup: " << std::fixed << std::setprecision(2) << last_res.setup_time_ms << " ms"
                      << ", Solve: " << last_res.solve_time_ms << " ms"
                      << ", Total: " << last_res.total_time_ms << " ms"
                      << ", Iters: " << last_res.iterations
                      << ", Res: " << std::scientific << std::setprecision(2) << last_res.relative_residual
                      << ", T_max: " << std::fixed << std::setprecision(2) << last_res.max_temperature << " C"
                      << ", Imbalance: " << last_res.energy_imbalance_percent << "%"
                      << std::endl;

            run.solver_results.push_back(last_res);
        }

        runs.push_back(run);
    }

    return runs;
}

bool HeatsinkSolver::export_benchmark_csv(const std::string& filepath,
                                         const std::vector<BenchmarkRun>& runs) {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << "Mesh,Solver,Nodes,Elements,Nonzeros,SetupTimeMs,SolveTimeMs,TotalTimeMs,Iterations,RelativeResidual,MinTempC,MaxTempC,AvgTempC,EnergyImbalancePercent\n";
    out << std::fixed << std::setprecision(4);

    for (const auto& run : runs) {
        for (const auto& sr : run.solver_results) {
            out << run.mesh_name << ","
                << "\"" << sr.solver_name << "\","
                << sr.node_count << ","
                << sr.element_count << ","
                << sr.nonzero_count << ","
                << sr.setup_time_ms << ","
                << sr.solve_time_ms << ","
                << sr.total_time_ms << ","
                << sr.iterations << ","
                << std::scientific << sr.relative_residual << std::fixed << ","
                << sr.min_temperature << ","
                << sr.max_temperature << ","
                << sr.avg_temperature << ","
                << sr.energy_imbalance_percent << "\n";
        }
    }

    return true;
}

} // namespace workbench
