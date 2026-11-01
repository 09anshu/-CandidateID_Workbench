#pragma once

#include <chrono>
#include <string>
#include <vector>

#include "../thermal/thermal_solver.h"

namespace workbench {

/// Configuration for a batch of thermal solves
struct BatchConfig {
    int num_cases = 500;            // Number of independent solves
    int mesh_size = 40;             // Mesh elements per solve
    unsigned int random_seed = 42;  // Fixed seed for strict reproducibility
    double k_min = 5.0;             // Min conductivity W/(m*K)
    double k_max = 50.0;            // Max conductivity W/(m*K)
    double q_min = 1000.0;          // Min volumetric heat gen W/m^3
    double q_max = 50000.0;         // Max volumetric heat gen W/m^3
};

/// Workload case parameters
struct ThermalCase {
    int case_id = 0;
    ThermalParams params;
    int mesh_size = 40;
};

/// Hardware environment specification
struct HardwareEnvironment {
    std::string cpu_model = "Unknown CPU";
    int physical_cores = 0;
    int logical_cpus = 0;
    double total_ram_gb = 0.0;
    std::string compiler = "GCC 16.2.0";
    std::string optimization_flags = "-O3";
};

/// Benchmark results for a specific worker count p
struct WorkerBenchmark {
    int worker_count = 1;
    std::vector<double> repetition_times_ms; // 5 repetitions
    double median_time_ms = 0.0;
    double min_time_ms = 0.0;
    double max_time_ms = 0.0;
    double speedup = 1.0;                    // T_1 / T_p
    double efficiency = 1.0;                 // (T_1 / T_p) / p
};

/// Full parallel benchmark report
struct PerformanceBenchmarkReport {
    HardwareEnvironment hardware;
    BatchConfig config;
    double warmup_time_ms = 0.0;
    WorkerBenchmark serial_benchmark;
    std::vector<WorkerBenchmark> parallel_benchmarks; // for 1, 2, 4 workers
    double max_serial_parallel_diff = 0.0;            // Numerical agreement check
    bool agreement_passed = false;                    // <= 1e-8 °C
    std::string notes;
};

/// Detect host hardware specifications
HardwareEnvironment detect_hardware();

/// Generate a reproducible batch of thermal cases using a fixed random seed
std::vector<ThermalCase> generate_thermal_batch(const BatchConfig& config);

/// Execute batch serially
std::vector<ThermalResult> solve_batch_serial(const std::vector<ThermalCase>& cases);

/// Execute batch in parallel using a bounded worker pool with p threads
/// Results are preserved strictly in deterministic input order (case_id order)
std::vector<ThermalResult> solve_batch_parallel(const std::vector<ThermalCase>& cases, int num_workers);

/// Run complete multi-core performance benchmark with 5 repetitions per worker count
PerformanceBenchmarkReport run_performance_benchmark(const BatchConfig& config, const std::vector<int>& worker_counts = {1, 2, 4});

/// Export benchmark results to CSV format
std::string export_benchmark_csv(const PerformanceBenchmarkReport& report);

} // namespace workbench

