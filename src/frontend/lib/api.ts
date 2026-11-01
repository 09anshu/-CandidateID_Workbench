const API_BASE = "http://localhost:8080/api/v1";

export interface SystemInfo {
  success: boolean;
  cpu_model: string;
  physical_cores: number;
  logical_cpus: number;
  total_ram_gb: number;
  compiler: string;
  optimization_flags: string;
}

export interface StlParseResult {
  success: boolean;
  model_name: string;
  triangle_count: number;
  bounds: { min: [number, number, number]; max: [number, number, number] };
  triangles: Array<{
    normal: [number, number, number];
    v1: [number, number, number];
    v2: [number, number, number];
    v3: [number, number, number];
  }>;
  error?: string;
}

export interface CsvParseResult {
  success: boolean;
  node_count: number;
  temp_min: number;
  temp_max: number;
  bounds: { min: [number, number, number]; max: [number, number, number] };
  nodes: Array<{
    id: number;
    x: number;
    y: number;
    z: number;
    temperature: number;
  }>;
  error?: string;
}

export interface ThermalParamsInput {
  length: number;
  area: number;
  conductivity: number;
  heat_generation: number;
  temp_left: number;
  temp_right: number;
  num_elements: number;
}

export interface ThermalSolveResult {
  success: boolean;
  num_elements: number;
  element_size_h: number;
  max_midpoint_error: number;
  l2_norm_error: number;
  nodes: Array<{
    index: number;
    x: number;
    temp_fem: number;
    temp_analytical: number;
    error: number;
  }>;
  midpoints: Array<{
    element_index: number;
    x_mid: number;
    temp_fem: number;
    temp_analytical: number;
    absolute_error: number;
  }>;
  error?: string;
}

export interface ConvergenceStudyResult {
  success: boolean;
  cases: Array<{
    num_elements: number;
    h: number;
    max_midpoint_error: number;
    l2_error: number;
    convergence_rate: number;
  }>;
  mesh_solutions: Array<{
    num_elements: number;
    h: number;
    max_midpoint_error: number;
    nodes: Array<{ x: number; temp: number }>;
  }>;
  error?: string;
}

export interface BenchmarkReport {
  success: boolean;
  cpu_model: string;
  physical_cores: number;
  logical_cpus: number;
  total_ram_gb: number;
  compiler: string;
  optimization_flags: string;
  num_cases: number;
  mesh_size: number;
  random_seed: number;
  warmup_time_ms: number;
  max_temperature_diff: number;
  agreement_passed: boolean;
  benchmarks: Array<{
    worker_count: number;
    median_time_ms: number;
    min_time_ms: number;
    max_time_ms: number;
    speedup: number;
    efficiency: number;
    repetition_times_ms: number[];
  }>;
  error?: string;
}

export interface DeviceSummary {
  id: string;
  name: string;
  type: string;
  status: string;
  is_connected: boolean;
  is_debug_interface: boolean;
}

export interface DeviceCapabilities {
  success: boolean;
  device_name: string;
  model: string;
  manufacturer: string;
  device_type: string;
  communication_interface: string;
  voltage_domain: string;
  connector: string;
  signal_ownership_notes: string;
  is_debug_interface: boolean;
  supported_protocols: string[];
  features: string[];
  error?: string;
}

export interface DeviceTelemetry {
  success: boolean;
  device_id: string;
  device_type: string;
  timestamp: string;
  voltage_v: number;
  current_ma: number;
  temperature_c: number;
  uptime_seconds: number;
  status_message: string;
  is_simulated: boolean;
  metrics: Record<string, string>;
  error?: string;
}

export interface BackpropComputeResult {
  success: boolean;
  initial: {
    w: number; b: number; v: number; c: number;
    z: number; h: number; y_hat: number; error: number; loss: number;
  };
  gradients: {
    dJ_dw: number; dJ_db: number; dJ_dv: number; dJ_dc: number;
  };
  verification: {
    numerical_dJ_dw: number; numerical_dJ_db: number;
    numerical_dJ_dv: number; numerical_dJ_dc: number;
    abs_error_w: number; abs_error_b: number; abs_error_v: number; abs_error_c: number;
    all_passed: boolean;
  };
  updated_step1: {
    w: number; b: number; v: number; c: number;
  };
  explanation: string;
  error?: string;
}

export interface BackpropTrainResult {
  success: boolean;
  final_loss: number;
  steps: Array<{
    step: number;
    loss: number;
    y_hat: number;
    w: number;
    b: number;
    v: number;
    c: number;
  }>;
  error?: string;
}

export interface TestCaseResult {
  suite: string;
  name: string;
  status: "PASSED" | "FAILED" | "SKIPPED";
  duration_ms: number;
  failure_message: string;
}

export interface TestSuiteRunResult {
  success: boolean;
  executed: boolean;
  exit_code: number;
  total: number;
  passed: number;
  failed: number;
  skipped: number;
  duration_ms: number;
  timestamp: string;
  xml_path: string;
  log_path: string;
  error?: string;
  console_output: string;
  tests: TestCaseResult[];
}

// ---------------------------------------------------------------------------
// HTTP API Client Implementation
// ---------------------------------------------------------------------------

async function fetchJson<T>(url: string, options?: RequestInit): Promise<T> {
  const res = await fetch(url, options);
  if (!res.ok) {
    const errorBody = await res.text();
    try {
      const parsed = JSON.parse(errorBody);
      throw new Error(parsed.error || `HTTP error ${res.status}`);
    } catch {
      throw new Error(errorBody || `HTTP error ${res.status}`);
    }
  }
  return res.json() as Promise<T>;
}

export const api = {
  checkHealth: () => fetchJson<{ success: boolean; status: string }>(`${API_BASE}/health`),
  getSystemInfo: () => fetchJson<SystemInfo>(`${API_BASE}/system/info`),

  parseStl: (content: string) =>
    fetchJson<StlParseResult>(`${API_BASE}/geometry/stl/parse`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ content }),
    }),

  parseCsv: (content: string) =>
    fetchJson<CsvParseResult>(`${API_BASE}/geometry/csv/parse`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ content }),
    }),

  solveThermal: (params: ThermalParamsInput) =>
    fetchJson<ThermalSolveResult>(`${API_BASE}/thermal/solve`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(params),
    }),

  runConvergence: (params: Partial<ThermalParamsInput>) =>
    fetchJson<ConvergenceStudyResult>(`${API_BASE}/thermal/convergence`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(params),
    }),

  exportThermalCsvUrl: () => `${API_BASE}/thermal/export/csv`,

  runBenchmark: (numCases: number, meshSize: number, seed: number) =>
    fetchJson<BenchmarkReport>(`${API_BASE}/performance/benchmark`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ num_cases: numCases, mesh_size: meshSize, random_seed: seed }),
    }),

  discoverHardware: () => fetchJson<{ success: boolean; devices: DeviceSummary[] }>(`${API_BASE}/hardware/discover`),

  getHardwareCapabilities: (id: string) =>
    fetchJson<DeviceCapabilities>(`${API_BASE}/hardware/capabilities?id=${encodeURIComponent(id)}`),

  connectHardware: (deviceId: string) =>
    fetchJson<{ success: boolean; message: string }>(`${API_BASE}/hardware/connect`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ device_id: deviceId }),
    }),

  disconnectHardware: (deviceId: string) =>
    fetchJson<{ success: boolean; message: string }>(`${API_BASE}/hardware/disconnect`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ device_id: deviceId }),
    }),

  getHardwareTelemetry: (id: string) =>
    fetchJson<DeviceTelemetry>(`${API_BASE}/hardware/telemetry?id=${encodeURIComponent(id)}`),

  resetHardware: () =>
    fetchJson<{ success: boolean; message: string }>(`${API_BASE}/hardware/reset`, {
      method: "POST",
    }),

  computeBackprop: () =>
    fetchJson<BackpropComputeResult>(`${API_BASE}/backprop/compute`, {
      method: "POST",
    }),

  trainBackprop: () =>
    fetchJson<BackpropTrainResult>(`${API_BASE}/backprop/train`, {
      method: "POST",
    }),

  exportBackpropCsvUrl: () => `${API_BASE}/backprop/export/csv`,

  runTests: () =>
    fetchJson<TestSuiteRunResult>(`${API_BASE}/tests/run`, {
      method: "POST",
    }),

  getLatestTests: () => fetchJson<TestSuiteRunResult>(`${API_BASE}/tests/latest`),

  exportTestXmlUrl: () => `${API_BASE}/tests/export/xml`,
  exportTestLogUrl: () => `${API_BASE}/tests/export/log`,
};

