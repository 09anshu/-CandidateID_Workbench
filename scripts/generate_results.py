import urllib.request
import json
import os

os.makedirs("results/csv", exist_ok=True)
os.makedirs("results/benchmarks", exist_ok=True)

print("Connecting to backend at http://127.0.0.1:8080...")

# 1. Thermal Solve N=40
req = urllib.request.Request(
    'http://127.0.0.1:8080/api/v1/thermal/solve',
    data=b'{"num_elements":40}',
    headers={'Content-Type': 'application/json'}
)
resp = urllib.request.urlopen(req)
csv_resp = urllib.request.urlopen('http://127.0.0.1:8080/api/v1/thermal/export/csv')
with open('results/csv/thermal_solution_N40.csv', 'w', encoding='utf-8') as f:
    f.write(csv_resp.read().decode('utf-8'))
print("Successfully generated results/csv/thermal_solution_N40.csv")

# 2. Thermal Convergence N=10, 20, 40, 80
req = urllib.request.Request(
    'http://127.0.0.1:8080/api/v1/thermal/convergence',
    data=b'{}',
    headers={'Content-Type': 'application/json'}
)
resp = urllib.request.urlopen(req)
data = json.loads(resp.read().decode('utf-8'))
with open('results/csv/thermal_mesh_convergence.csv', 'w', encoding='utf-8') as f:
    f.write('# PASS SCIENCE WORKS - Thermal Mesh Convergence Study\n')
    f.write('elements_N,mesh_size_h_m,max_midpoint_error_C,l2_error_C,convergence_rate\n')
    for c in data['cases']:
        f.write(f"{c['num_elements']},{c['h']:.6f},{c['max_midpoint_error']:.8f},{c['l2_error']:.8f},{c['convergence_rate']:.4f}\n")
print("Successfully generated results/csv/thermal_mesh_convergence.csv")

# 3. Multicore Performance Benchmark (500 cases, mesh 40, seed 42)
print("Running multicore benchmark (5 repetitions)...")
req = urllib.request.Request(
    'http://127.0.0.1:8080/api/v1/performance/benchmark',
    data=b'{"num_cases":500,"mesh_size":40,"random_seed":42}',
    headers={'Content-Type': 'application/json'}
)
resp = urllib.request.urlopen(req)
bench_data = json.loads(resp.read().decode('utf-8'))
with open('results/benchmarks/multicore_benchmark_results.csv', 'w', encoding='utf-8') as f:
    f.write("# PASS SCIENCE WORKS - Multicore Execution Benchmark\n")
    f.write(f"# CPU: {bench_data['cpu_model']}, Physical Cores: {bench_data['physical_cores']}, Logical CPUs: {bench_data['logical_cpus']}, RAM: {bench_data['total_ram_gb']} GB\n")
    f.write(f"# Compiler: {bench_data['compiler']} ({bench_data['optimization_flags']}), Workload: {bench_data['num_cases']} cases, Mesh: {bench_data['mesh_size']} elements\n")
    f.write(f"# Max Serial-Parallel Diff: {bench_data['max_temperature_diff']} C, Passed: {bench_data['agreement_passed']}\n")
    f.write("worker_count,rep1_ms,rep2_ms,rep3_ms,rep4_ms,rep5_ms,median_ms,speedup,efficiency\n")
    for b in bench_data['benchmarks']:
        reps = ','.join(f"{t:.2f}" for t in b['repetition_times_ms'])
        f.write(f"{b['worker_count']},{reps},{b['median_time_ms']:.2f},{b['speedup']:.2f},{b['efficiency']:.2f}\n")
print("Successfully generated results/benchmarks/multicore_benchmark_results.csv")

# 4. Backpropagation Training History (100 steps)
req = urllib.request.Request(
    'http://127.0.0.1:8080/api/v1/backprop/compute',
    data=b'{}',
    headers={'Content-Type': 'application/json'}
)
resp = urllib.request.urlopen(req)
train_csv_resp = urllib.request.urlopen('http://127.0.0.1:8080/api/v1/backprop/export/csv')
with open('results/csv/backpropagation_training_100steps.csv', 'w', encoding='utf-8') as f:
    f.write(train_csv_resp.read().decode('utf-8'))
print("Successfully generated results/csv/backpropagation_training_100steps.csv")
