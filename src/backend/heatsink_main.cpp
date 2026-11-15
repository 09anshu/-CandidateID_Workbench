#include "thermal/heatsink_mesh.h"
#include "thermal/heatsink_solver.h"

#include <iostream>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <cmath>

int main() {
    using namespace workbench;

    std::cout << "===========================================================" << std::endl;
    std::cout << "  PASS SCIENCE WORKS - 3D HEATSINK THERMAL SOLVER BENCHMARK" << std::endl;
    std::cout << "  Comparing: SparseLU, Incomplete Cholesky PCG, AMG-PCG" << std::endl;
    std::cout << "===========================================================" << std::endl;

    // Create output directories
    std::filesystem::create_directories("data/heatsink");
    std::filesystem::create_directories("results/heatsink");

    HeatsinkSolver solver;

    // 1. Export STL geometry files for visual inspection and UI
    std::cout << "\n[Step 1/4] Generating 3D CAD STL geometry files..." << std::endl;
    {
        HeatsinkMesh coarse_mesh;
        coarse_mesh.generate(MeshResolution::Coarse);
        coarse_mesh.export_stl("data/heatsink/heatsink_coarse.stl");
        std::cout << "  Exported data/heatsink/heatsink_coarse.stl" << std::endl;

        HeatsinkMesh medium_mesh;
        medium_mesh.generate(MeshResolution::Medium);
        medium_mesh.export_stl("data/heatsink/heatsink_medium.stl");
        std::cout << "  Exported data/heatsink/heatsink_medium.stl" << std::endl;

        HeatsinkMesh fine_mesh;
        fine_mesh.generate(MeshResolution::Fine);
        fine_mesh.export_stl("data/heatsink/heatsink_fine.stl");
        std::cout << "  Exported data/heatsink/heatsink_fine.stl" << std::endl;

        HeatsinkMesh vfine_mesh;
        vfine_mesh.generate(MeshResolution::VeryFine);
        vfine_mesh.export_stl("data/heatsink/heatsink_very_fine.stl");
        std::cout << "  Exported data/heatsink/heatsink_very_fine.stl" << std::endl;
    }

    // 2. Run full benchmarking suite across 4 mesh resolutions
    std::cout << "\n[Step 2/4] Executing benchmark across 4 resolutions (5 repetitions each)..." << std::endl;
    auto benchmark_runs = solver.run_benchmark(5);

    // 3. Export benchmark results CSV
    std::string csv_path = "results/heatsink/benchmark_summary.csv";
    solver.export_benchmark_csv(csv_path, benchmark_runs);
    std::cout << "\n[Step 3/4] Exported benchmark metrics to " << csv_path << std::endl;

    // 4. Export detailed solution fields and verify solver consistency
    std::cout << "\n[Step 4/4] Comparing solver consistency on Fine mesh..." << std::endl;
    {
        HeatsinkMesh fine_mesh;
        fine_mesh.generate(MeshResolution::Fine);

        Eigen::SparseMatrix<double, Eigen::RowMajor> K;
        Eigen::VectorXd b;
        solver.assemble_system(fine_mesh, K, b);

        std::cout << "  Solving with Eigen SparseLU..." << std::endl;
        auto res_lu = solver.solve_sparse_lu(fine_mesh, K, b);

        std::cout << "  Solving with Eigen PCG..." << std::endl;
        auto res_pcg = solver.solve_pcg(fine_mesh, K, b);

        std::cout << "  Solving with AMG-PCG..." << std::endl;
        auto res_amg = solver.solve_amg_pcg(fine_mesh, K, b);

        // Export temperature fields to CSV
        fine_mesh.export_csv("results/heatsink/temperature_field_sparse_lu.csv", res_lu.temperatures);
        fine_mesh.export_csv("results/heatsink/temperature_field_pcg.csv", res_pcg.temperatures);
        fine_mesh.export_csv("results/heatsink/temperature_field_amg_pcg.csv", res_amg.temperatures);

        // Export VTK files
        fine_mesh.export_vtk("results/heatsink/solution_sparse_lu.vtk", res_lu.temperatures);
        fine_mesh.export_vtk("results/heatsink/solution_pcg.vtk", res_pcg.temperatures);
        fine_mesh.export_vtk("results/heatsink/solution_amg_pcg.vtk", res_amg.temperatures);

        // Compute pointwise maximum absolute temperature differences
        double max_diff_lu_pcg = 0.0;
        double max_diff_lu_amg = 0.0;
        double max_diff_pcg_amg = 0.0;
        double l2_diff_lu_pcg = 0.0;
        double l2_diff_lu_amg = 0.0;

        for (size_t i = 0; i < res_lu.temperatures.size(); ++i) {
            double diff1 = std::abs(res_lu.temperatures[i] - res_pcg.temperatures[i]);
            double diff2 = std::abs(res_lu.temperatures[i] - res_amg.temperatures[i]);
            double diff3 = std::abs(res_pcg.temperatures[i] - res_amg.temperatures[i]);

            max_diff_lu_pcg = std::max(max_diff_lu_pcg, diff1);
            max_diff_lu_amg = std::max(max_diff_lu_amg, diff2);
            max_diff_pcg_amg = std::max(max_diff_pcg_amg, diff3);

            l2_diff_lu_pcg += diff1 * diff1;
            l2_diff_lu_amg += diff2 * diff2;
        }

        l2_diff_lu_pcg = std::sqrt(l2_diff_lu_pcg / res_lu.temperatures.size());
        l2_diff_lu_amg = std::sqrt(l2_diff_lu_amg / res_lu.temperatures.size());

        std::cout << "\n-----------------------------------------------------------" << std::endl;
        std::cout << "  SOLVER SOLUTION AGREEMENT (Fine Mesh, " << fine_mesh.stats.node_count << " nodes)" << std::endl;
        std::cout << "-----------------------------------------------------------" << std::endl;
        std::cout << "  Max |T_SparseLU - T_PCG|:     " << std::scientific << std::setprecision(4) << max_diff_lu_pcg << " °C (RMS: " << l2_diff_lu_pcg << " °C)" << std::endl;
        std::cout << "  Max |T_SparseLU - T_AMG-PCG|: " << max_diff_lu_amg << " °C (RMS: " << l2_diff_lu_amg << " °C)" << std::endl;
        std::cout << "  Max |T_PCG - T_AMG-PCG|:      " << max_diff_pcg_amg << " °C" << std::endl;
        std::cout << "-----------------------------------------------------------" << std::endl;

        // Save solution comparison summary to file
        std::ofstream comp_out("results/heatsink/solution_agreement.txt");
        comp_out << "SOLVER SOLUTION AGREEMENT SUMMARY\n"
                 << "Nodes: " << fine_mesh.stats.node_count << "\n"
                 << "Max |T_SparseLU - T_PCG|: " << max_diff_lu_pcg << " °C\n"
                 << "RMS |T_SparseLU - T_PCG|: " << l2_diff_lu_pcg << " °C\n"
                 << "Max |T_SparseLU - T_AMG-PCG|: " << max_diff_lu_amg << " °C\n"
                 << "RMS |T_SparseLU - T_AMG-PCG|: " << l2_diff_lu_amg << " °C\n"
                 << "Max |T_PCG - T_AMG-PCG|: " << max_diff_pcg_amg << " °C\n";
    }

    std::cout << "\n>>> BENCHMARK COMPLETED SUCCESSFULLY! <<<\n" << std::endl;
    return 0;
}
