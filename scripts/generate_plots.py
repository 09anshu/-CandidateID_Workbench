import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import os

os.makedirs("results/plots", exist_ok=True)
plt.style.use('seaborn-v0_8-whitegrid' if 'seaborn-v0_8-whitegrid' in plt.style.available else 'default')

# ---------------------------------------------------------------------------
# 1. Thermal Temperature Distribution
# ---------------------------------------------------------------------------
df_therm = pd.read_csv("results/csv/thermal_solution_N40.csv", comment='#')
df_nodes = df_therm[df_therm['node_index'].notnull()]

plt.figure(figsize=(8, 5), dpi=300)
plt.plot(df_nodes['x_m'], df_nodes['temp_fem_C'], 'b-o', markersize=4, label='FEM Solution (N=40)', linewidth=1.5)
plt.plot(df_nodes['x_m'], df_nodes['temp_analytical_C'], 'r--', label='Analytical Exact', linewidth=1.5)
plt.title("1D Steady-State Thermal Conduction with Heat Generation\nUniform Rod (L=1.0m, k=10 W/m·K, q=10,000 W/m³)", fontsize=12, fontweight='bold')
plt.xlabel("Position along Rod x (m)", fontsize=11)
plt.ylabel("Temperature (°C)", fontsize=11)
plt.legend(frameon=True, facecolor='white', framealpha=0.9)
plt.grid(True, linestyle='--', alpha=0.6)
plt.tight_layout()
plt.savefig("results/plots/thermal_temperature_distribution.png")
plt.close()
print("Saved thermal_temperature_distribution.png")

# ---------------------------------------------------------------------------
# 2. Thermal Mesh Convergence (Log-Log)
# ---------------------------------------------------------------------------
df_conv = pd.read_csv("results/csv/thermal_mesh_convergence.csv", comment='#')

plt.figure(figsize=(7, 5), dpi=300)
plt.loglog(df_conv['elements_N'], df_conv['max_midpoint_error_C'], 'bo-', linewidth=2, markersize=8, label='Observed Midpoint Error')
# Reference O(h^2) line
ref_x = df_conv['elements_N']
ref_y = df_conv['max_midpoint_error_C'].iloc[0] * (df_conv['elements_N'].iloc[0] / ref_x)**2
plt.loglog(ref_x, ref_y, 'k--', linewidth=1.5, label='Theoretical Quadratic Slope O(h²)')

plt.title("Mesh Convergence Study (Midpoint Interpolation Error)", fontsize=12, fontweight='bold')
plt.xlabel("Number of Elements N (log scale)", fontsize=11)
plt.ylabel("Maximum Absolute Midpoint Error (°C, log scale)", fontsize=11)
plt.xticks(df_conv['elements_N'], [f"N={n}" for n in df_conv['elements_N']])
plt.legend(frameon=True, facecolor='white', framealpha=0.9)
plt.grid(True, which="both", linestyle='--', alpha=0.6)
plt.tight_layout()
plt.savefig("results/plots/thermal_mesh_convergence.png")
plt.close()
print("Saved thermal_mesh_convergence.png")

# ---------------------------------------------------------------------------
# 3. Multicore Speedup and Efficiency
# ---------------------------------------------------------------------------
df_bench = pd.read_csv("results/benchmarks/multicore_benchmark_results.csv", comment='#')

fig, ax1 = plt.subplots(figsize=(7, 5), dpi=300)
workers = df_bench['worker_count']
speedup = df_bench['speedup']
efficiency = df_bench['efficiency'] * 100

color = 'tab:blue'
ax1.set_xlabel('Worker Count p (threads)', fontsize=11)
ax1.set_ylabel('Speedup (T1 / Tp)', color=color, fontsize=11)
line1 = ax1.plot(workers, speedup, 'o-', color=color, linewidth=2.5, markersize=7, label='Measured Speedup')
line2 = ax1.plot(workers, workers, '--', color='gray', linewidth=1.5, label='Ideal Linear Speedup')
ax1.tick_params(axis='y', labelcolor=color)
ax1.set_xticks(workers)

ax2 = ax1.twinx()
color = 'tab:green'
ax2.set_ylabel('Efficiency (%)', color=color, fontsize=11)
line3 = ax2.plot(workers, efficiency, 's--', color=color, linewidth=2, markersize=7, label='Efficiency (%)')
ax2.tick_params(axis='y', labelcolor=color)
ax2.set_ylim(0, 110)

lines = line1 + line2 + line3
labels = [l.get_label() for l in lines]
ax1.legend(lines, labels, loc='center right', frameon=True, facecolor='white', framealpha=0.9)
plt.title("Multicore CPU Parallel Speedup & Efficiency\n(Intel Core i5-10300H @ 2.50GHz, 500 cases, mesh N=40)", fontsize=11, fontweight='bold')
plt.tight_layout()
plt.savefig("results/plots/multicore_speedup_efficiency.png")
plt.close()
print("Saved multicore_speedup_efficiency.png")

# ---------------------------------------------------------------------------
# 4. Backpropagation Training Loss
# ---------------------------------------------------------------------------
df_train = pd.read_csv("results/csv/backpropagation_training_100steps.csv", comment='#')

plt.figure(figsize=(7, 4.5), dpi=300)
plt.plot(df_train['step'], df_train['loss'], 'r-', linewidth=2, label='Training Loss J = 0.5(ŷ - y)²')
plt.title("Neural Network Backpropagation Convergence (100 Steps)\nx=1.0, y=2.0, Learning Rate η=0.1", fontsize=12, fontweight='bold')
plt.xlabel("Iteration Step", fontsize=11)
plt.ylabel("Loss J", fontsize=11)
plt.yscale('log')
plt.grid(True, which="both", linestyle='--', alpha=0.6)
plt.legend(frameon=True, facecolor='white', framealpha=0.9)
plt.tight_layout()
plt.savefig("results/plots/backpropagation_loss_curve.png")
plt.close()
print("Saved backpropagation_loss_curve.png")
