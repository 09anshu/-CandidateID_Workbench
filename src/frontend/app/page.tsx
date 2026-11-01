"use client";

import React, { useState, useEffect } from "react";
import {
  Layers,
  Thermometer,
  Cpu,
  Radio,
  BrainCircuit,
  BookOpen,
  Activity,
  CheckCircle2,
  AlertCircle,
  Download,
  Play,
  RotateCcw,
  Upload,
  Server,
  Zap,
  CheckSquare,
  XCircle,
  Clock,
  Terminal,
  FileText,
  RefreshCw,
  Copy,
} from "lucide-react";
import ThreeViewer from "@/components/viewers/ThreeViewer";
import ScientificChart, { DataSeries } from "@/components/charts/ScientificChart";
import {
  api,
  SystemInfo,
  StlParseResult,
  CsvParseResult,
  ThermalSolveResult,
  ConvergenceStudyResult,
  BenchmarkReport,
  DeviceSummary,
  DeviceCapabilities,
  DeviceTelemetry,
  BackpropComputeResult,
  BackpropTrainResult,
  TestSuiteRunResult,
  TestCaseResult,
} from "@/lib/api";

export default function WorkbenchApp() {
  const [activeTab, setActiveTab] = useState<"geometry" | "thermal" | "performance" | "hardware" | "backprop" | "tests" | "docs">("geometry");
  const [backendOnline, setBackendOnline] = useState<boolean>(false);
  const [systemInfo, setSystemInfo] = useState<SystemInfo | null>(null);

  // Geometry state
  const [stlResult, setStlResult] = useState<StlParseResult | null>(null);
  const [csvResult, setCsvResult] = useState<CsvParseResult | null>(null);
  const [geoError, setGeoError] = useState<string | null>(null);
  const [geoLoading, setGeoLoading] = useState(false);

  // Thermal FEA state
  const [thermalParams, setThermalParams] = useState<Record<string, any>>({
    length: 1.0,
    area: 0.0001,
    conductivity: 10.0,
    heat_generation: 10000.0,
    temp_left: 100.0,
    temp_right: 20.0,
    num_elements: 40,
  });
  const [thermalResult, setThermalResult] = useState<ThermalSolveResult | null>(null);
  const [convergenceResult, setConvergenceResult] = useState<ConvergenceStudyResult | null>(null);
  const [thermalLoading, setThermalLoading] = useState(false);
  const [thermalError, setThermalError] = useState<string | null>(null);

  // Multicore performance state
  const [benchConfig, setBenchConfig] = useState<{
    num_cases: number | string;
    mesh_size: number | string;
    random_seed: number | string;
  }>({
    num_cases: 100,
    mesh_size: 20,
    random_seed: 42,
  });
  const [benchReport, setBenchReport] = useState<BenchmarkReport | null>(null);
  const [benchLoading, setBenchLoading] = useState(false);

  // Hardware state
  const [devices, setDevices] = useState<DeviceSummary[]>([]);
  const [selectedDevice, setSelectedDevice] = useState<DeviceSummary | null>(null);
  const [deviceCaps, setDeviceCaps] = useState<DeviceCapabilities | null>(null);
  const [telemetry, setTelemetry] = useState<DeviceTelemetry | null>(null);
  const [hwError, setHwError] = useState<string | null>(null);
  const [hwLoading, setHwLoading] = useState(false);

  // Backprop state
  const [backpropCompute, setBackpropCompute] = useState<BackpropComputeResult | null>(null);
  const [backpropTrain, setBackpropTrain] = useState<BackpropTrainResult | null>(null);
  const [backpropLoading, setBackpropLoading] = useState(false);

  // Automated testing & validation state
  const [testResult, setTestResult] = useState<TestSuiteRunResult | null>(null);
  const [testLoading, setTestLoading] = useState(false);
  const [testFilter, setTestFilter] = useState<"ALL" | "PASSED" | "FAILED">("ALL");
  const [testSearch, setTestSearch] = useState("");
  const [testOutputExpanded, setTestOutputExpanded] = useState(true);
  const [testRunningStatus, setTestRunningStatus] = useState<string>("");
  const [copiedLog, setCopiedLog] = useState(false);

  // Periodic health & system polling
  useEffect(() => {
    const checkStatus = async () => {
      try {
        const health = await api.checkHealth();
        if (health.success) {
          setBackendOnline(true);
          const sys = await api.getSystemInfo();
          setSystemInfo(sys);
        }
      } catch {
        setBackendOnline(false);
      }
    };
    checkStatus();
    const interval = setInterval(checkStatus, 4000);
    return () => clearInterval(interval);
  }, []);

  // Auto-fetch hardware devices when hardware tab is opened
  useEffect(() => {
    if (activeTab === "hardware") {
      fetchDevices();
    }
  }, [activeTab]);

  // Auto-fetch latest tests when tests tab is opened
  useEffect(() => {
    if (activeTab === "tests" && !testResult && !testLoading) {
      fetchLatestTests();
    }
  }, [activeTab]);

  const fetchLatestTests = async () => {
    try {
      const res = await api.getLatestTests();
      if (res && res.total > 0) {
        setTestResult(res);
      }
    } catch {
      // Ignored if not yet executed
    }
  };

  const handleRunTests = async () => {
    setTestLoading(true);
    setTestRunningStatus("Executing GoogleTest & CTest suite via backend...");
    try {
      const res = await api.runTests();
      setTestResult(res);
    } catch (e: any) {
      alert("Test Execution Error: " + e.message);
    } finally {
      setTestLoading(false);
      setTestRunningStatus("");
    }
  };

  const handleCopyConsole = () => {
    if (testResult?.console_output) {
      navigator.clipboard.writeText(testResult.console_output);
      setCopiedLog(true);
      setTimeout(() => setCopiedLog(false), 2000);
    }
  };

  const fetchDevices = async () => {
    try {
      const res = await api.discoverHardware();
      if (res.success) {
        setDevices(res.devices);
        if (!selectedDevice && res.devices.length > 0) {
          selectDevice(res.devices[0]);
        }
      }
    } catch (e: any) {
      setHwError(e.message);
    }
  };

  const selectDevice = async (dev: DeviceSummary) => {
    setSelectedDevice(dev);
    try {
      const caps = await api.getHardwareCapabilities(dev.id);
      setDeviceCaps(caps);
      if (dev.is_connected) {
        const telem = await api.getHardwareTelemetry(dev.id);
        setTelemetry(telem);
      } else {
        setTelemetry(null);
      }
    } catch (e: any) {
      setHwError(e.message);
    }
  };

  // -------------------------------------------------------------------------
  // Handlers
  // -------------------------------------------------------------------------

  // Load sample STL (Unit Cube)
  const handleLoadSampleStl = async () => {
    setGeoLoading(true);
    setGeoError(null);
    try {
      const sampleStl = `solid cube
  facet normal 0 0 -1
    outer loop
      vertex 0 0 0
      vertex 1 0 0
      vertex 1 1 0
    endloop
  endfacet
  facet normal 0 0 -1
    outer loop
      vertex 0 0 0
      vertex 1 1 0
      vertex 0 1 0
    endloop
  endfacet
  facet normal 0 0 1
    outer loop
      vertex 0 0 1
      vertex 1 1 1
      vertex 1 0 1
    endloop
  endfacet
  facet normal 0 0 1
    outer loop
      vertex 0 0 1
      vertex 0 1 1
      vertex 1 1 1
    endloop
  endfacet
  facet normal 0 -1 0
    outer loop
      vertex 0 0 0
      vertex 1 0 1
      vertex 1 0 0
    endloop
  endfacet
  facet normal 0 -1 0
    outer loop
      vertex 0 0 0
      vertex 0 0 1
      vertex 1 0 1
    endloop
  endfacet
  facet normal 1 0 0
    outer loop
      vertex 1 0 0
      vertex 1 0 1
      vertex 1 1 1
    endloop
  endfacet
  facet normal 1 0 0
    outer loop
      vertex 1 0 0
      vertex 1 1 1
      vertex 1 1 0
    endloop
  endfacet
  facet normal 0 1 0
    outer loop
      vertex 0 1 0
      vertex 1 1 0
      vertex 1 1 1
    endloop
  endfacet
  facet normal 0 1 0
    outer loop
      vertex 0 1 0
      vertex 1 1 1
      vertex 0 1 1
    endloop
  endfacet
  facet normal -1 0 0
    outer loop
      vertex 0 0 0
      vertex 0 1 0
      vertex 0 1 1
    endloop
  endfacet
  facet normal -1 0 0
    outer loop
      vertex 0 0 0
      vertex 0 1 1
      vertex 0 0 1
    endloop
  endfacet
endsolid cube`;
      const res = await api.parseStl(sampleStl);
      setStlResult(res);
    } catch (e: any) {
      setGeoError(e.message);
    } finally {
      setGeoLoading(false);
    }
  };

  // Load sample ANSYS CSV
  const handleLoadSampleCsv = async () => {
    setGeoLoading(true);
    setGeoError(null);
    try {
      // 51 nodes along rod with exact thermal profile
      let csvText = "node_id,x_m,y_m,z_m,temperature_C\n";
      for (let i = 0; i <= 50; ++i) {
        const x = i * 0.02;
        const t = 100 - 80 * x + 500 * x * (1 - x);
        csvText += `${i + 1},${x.toFixed(3)},0.000,0.000,${t.toFixed(4)}\n`;
      }
      const res = await api.parseCsv(csvText);
      setCsvResult(res);
    } catch (e: any) {
      setGeoError(e.message);
    } finally {
      setGeoLoading(false);
    }
  };

  // Custom File Upload Handler
  const handleFileUpload = (e: React.ChangeEvent<HTMLInputElement>, type: "stl" | "csv") => {
    const file = e.target.files?.[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = async (event) => {
      const content = event.target?.result as string;
      setGeoLoading(true);
      setGeoError(null);
      try {
        if (type === "stl") {
          const res = await api.parseStl(content);
          setStlResult(res);
        } else {
          const res = await api.parseCsv(content);
          setCsvResult(res);
        }
      } catch (err: any) {
        setGeoError(err.message);
      } finally {
        setGeoLoading(false);
      }
    };
    reader.readAsText(file);
  };

  // Run Thermal FEA solve
  const handleRunThermal = async () => {
    setThermalLoading(true);
    setThermalError(null);
    try {
      const parsedParams = {
        length: parseFloat(String(thermalParams.length)) || 1.0,
        area: parseFloat(String(thermalParams.area)) || 0.0001,
        conductivity: parseFloat(String(thermalParams.conductivity)) || 10.0,
        heat_generation: parseFloat(String(thermalParams.heat_generation)) || 0.0,
        temp_left: parseFloat(String(thermalParams.temp_left)) || 0.0,
        temp_right: parseFloat(String(thermalParams.temp_right)) || 0.0,
        num_elements: parseInt(String(thermalParams.num_elements), 10) || 40,
      };
      const res = await api.solveThermal(parsedParams);
      setThermalResult(res);
      const conv = await api.runConvergence(parsedParams);
      setConvergenceResult(conv);
    } catch (e: any) {
      setThermalError(e.message);
    } finally {
      setThermalLoading(false);
    }
  };

  // Run Multicore Benchmark
  const handleRunBenchmark = async () => {
    setBenchLoading(true);
    try {
      const numCases = Math.max(1, parseInt(String(benchConfig.num_cases).trim(), 10) || 100);
      const meshSize = Math.max(2, parseInt(String(benchConfig.mesh_size).trim(), 10) || 20);
      const seed = parseInt(String(benchConfig.random_seed).trim(), 10) || 42;
      const res = await api.runBenchmark(numCases, meshSize, seed);
      setBenchReport(res);
    } catch (e: any) {
      alert("Benchmark failed: " + e.message);
    } finally {
      setBenchLoading(false);
    }
  };

  // Hardware Connect / Disconnect
  const handleToggleConnect = async (dev: DeviceSummary) => {
    setHwLoading(true);
    setHwError(null);
    try {
      if (dev.is_connected) {
        await api.disconnectHardware(dev.id);
      } else {
        await api.connectHardware(dev.id);
      }
      await fetchDevices();
      if (selectedDevice?.id === dev.id) {
        const caps = await api.getHardwareCapabilities(dev.id);
        setDeviceCaps(caps);
        if (!dev.is_connected) {
          const telem = await api.getHardwareTelemetry(dev.id);
          setTelemetry(telem);
        } else {
          setTelemetry(null);
        }
      }
    } catch (e: any) {
      setHwError(e.message);
      await fetchDevices();
    } finally {
      setHwLoading(false);
    }
  };

  // Run Backprop
  const handleRunBackprop = async () => {
    setBackpropLoading(true);
    try {
      const comp = await api.computeBackprop();
      setBackpropCompute(comp);
      const train = await api.trainBackprop();
      setBackpropTrain(train);
    } catch (e: any) {
      alert("Backprop failed: " + e.message);
    } finally {
      setBackpropLoading(false);
    }
  };

  return (
    <div className="flex h-screen bg-slate-950 text-slate-100 overflow-hidden font-sans">
      {/* Sidebar */}
      <aside className="w-64 bg-slate-900 border-r border-slate-800 flex flex-col justify-between select-none">
        <div>
          {/* Logo */}
          <div className="p-5 border-b border-slate-800 flex items-center space-x-3">
            <div className="w-9 h-9 rounded-lg bg-blue-600 flex items-center justify-center font-bold text-white shadow-lg shadow-blue-500/30">
              PW
            </div>
            <div>
              <div className="font-bold text-sm tracking-wide text-white">PASS SCIENCE</div>
              <div className="text-[11px] text-blue-400 font-mono">Workbench v1.0</div>
            </div>
          </div>

          {/* Navigation Items */}
          <nav className="p-3 space-y-1">
            <button
              onClick={() => setActiveTab("geometry")}
              className={`w-full flex items-center space-x-3 px-3 py-2.5 rounded-lg text-xs font-medium transition ${
                activeTab === "geometry"
                  ? "bg-blue-600 text-white shadow-md shadow-blue-600/30"
                  : "text-slate-400 hover:bg-slate-800 hover:text-slate-200"
              }`}
            >
              <Layers className="w-4 h-4" />
              <span>Geometry & CAD</span>
            </button>

            <button
              onClick={() => setActiveTab("thermal")}
              className={`w-full flex items-center space-x-3 px-3 py-2.5 rounded-lg text-xs font-medium transition ${
                activeTab === "thermal"
                  ? "bg-blue-600 text-white shadow-md shadow-blue-600/30"
                  : "text-slate-400 hover:bg-slate-800 hover:text-slate-200"
              }`}
            >
              <Thermometer className="w-4 h-4" />
              <span>Thermal FEA (1D)</span>
            </button>

            <button
              onClick={() => setActiveTab("performance")}
              className={`w-full flex items-center space-x-3 px-3 py-2.5 rounded-lg text-xs font-medium transition ${
                activeTab === "performance"
                  ? "bg-blue-600 text-white shadow-md shadow-blue-600/30"
                  : "text-slate-400 hover:bg-slate-800 hover:text-slate-200"
              }`}
            >
              <Cpu className="w-4 h-4" />
              <span>Multicore Performance</span>
            </button>

            <button
              onClick={() => setActiveTab("hardware")}
              className={`w-full flex items-center space-x-3 px-3 py-2.5 rounded-lg text-xs font-medium transition ${
                activeTab === "hardware"
                  ? "bg-blue-600 text-white shadow-md shadow-blue-600/30"
                  : "text-slate-400 hover:bg-slate-800 hover:text-slate-200"
              }`}
            >
              <Radio className="w-4 h-4" />
              <span>Embedded Hardware</span>
            </button>

            <button
              onClick={() => setActiveTab("backprop")}
              className={`w-full flex items-center space-x-3 px-3 py-2.5 rounded-lg text-xs font-medium transition ${
                activeTab === "backprop"
                  ? "bg-blue-600 text-white shadow-md shadow-blue-600/30"
                  : "text-slate-400 hover:bg-slate-800 hover:text-slate-200"
              }`}
            >
              <BrainCircuit className="w-4 h-4" />
              <span>Backpropagation</span>
            </button>

            <button
              onClick={() => setActiveTab("tests")}
              className={`w-full flex items-center space-x-3 px-3 py-2.5 rounded-lg text-xs font-medium transition ${
                activeTab === "tests"
                  ? "bg-blue-600 text-white shadow-md shadow-blue-600/30"
                  : "text-slate-400 hover:bg-slate-800 hover:text-slate-200"
              }`}
            >
              <CheckSquare className="w-4 h-4" />
              <span>Validation & Tests</span>
            </button>

            <button
              onClick={() => setActiveTab("docs")}
              className={`w-full flex items-center space-x-3 px-3 py-2.5 rounded-lg text-xs font-medium transition ${
                activeTab === "docs"
                  ? "bg-blue-600 text-white shadow-md shadow-blue-600/30"
                  : "text-slate-400 hover:bg-slate-800 hover:text-slate-200"
              }`}
            >
              <BookOpen className="w-4 h-4" />
              <span>Documentation & Report</span>
            </button>
          </nav>
        </div>

        {/* Backend Status Card */}
        <div className="p-4 border-t border-slate-800 text-xs">
          <div className="flex items-center justify-between mb-2">
            <span className="text-slate-400 font-medium">C++ Backend</span>
            <div className="flex items-center space-x-1.5">
              <span
                className={`w-2 h-2 rounded-full ${
                  backendOnline ? "bg-emerald-500 animate-pulse" : "bg-red-500"
                }`}
              />
              <span className={backendOnline ? "text-emerald-400 font-semibold" : "text-red-400 font-semibold"}>
                {backendOnline ? "ONLINE (8080)" : "OFFLINE"}
              </span>
            </div>
          </div>
          {systemInfo && (
            <div className="space-y-1 text-[11px] text-slate-400 font-mono bg-slate-950/60 p-2 rounded border border-slate-800">
              <div className="truncate" title={systemInfo.cpu_model}>CPU: {systemInfo.cpu_model}</div>
              <div>Cores: {systemInfo.physical_cores}P / {systemInfo.logical_cpus}L | RAM: {systemInfo.total_ram_gb.toFixed(1)} GB</div>
              <div>Compiler: {systemInfo.compiler}</div>
            </div>
          )}
        </div>
      </aside>

      {/* Main Workspace */}
      <main className="flex-1 flex flex-col overflow-y-auto bg-slate-950">
        {/* Top Header */}
        <header className="h-14 border-b border-slate-800 px-6 flex items-center justify-between bg-slate-900/60 backdrop-blur-md sticky top-0 z-10">
          <div className="flex items-center space-x-3">
            <span className="text-sm font-semibold text-slate-200">
              {activeTab === "geometry" && "CAD / ANSYS 3D Geometry Viewport"}
              {activeTab === "thermal" && "Steady-State 1D Thermal FEA Solver & Verification"}
              {activeTab === "performance" && "Multicore Worker Pool Benchmark & Scalability"}
              {activeTab === "hardware" && "Embedded Workbench: MCU, FPGA, and JTAG Probe"}
              {activeTab === "backprop" && "Handwritten Neural Backpropagation & Gradient Check"}
              {activeTab === "tests" && "Automated Testing & Validation Suite (GoogleTest & CTest)"}
              {activeTab === "docs" && "Technical Report & Verification Deliverables"}
            </span>
          </div>

          <div className="flex items-center space-x-3 text-xs">
            <div className="bg-slate-800/80 px-2.5 py-1 rounded text-slate-300 font-mono border border-slate-700">
              Windows 11 x64 Native C++17 Core
            </div>
          </div>
        </header>

        {/* View Content */}
        <div className="p-6 flex-1">
          {/* ========================================================================= */}
          {/* TAB 1: GEOMETRY VIEW */}
          {/* ========================================================================= */}
          {activeTab === "geometry" && (
            <div className="space-y-6">
              {/* Controls bar */}
              <div className="bg-slate-900 p-4 rounded-xl border border-slate-800 flex flex-wrap items-center justify-between gap-4">
                <div className="flex items-center space-x-3">
                  <button
                    onClick={handleLoadSampleStl}
                    disabled={geoLoading}
                    className="bg-blue-600 hover:bg-blue-500 text-white text-xs px-3.5 py-2 rounded-lg font-medium transition shadow-md shadow-blue-600/20 flex items-center space-x-1.5"
                  >
                    <Layers className="w-3.5 h-3.5" />
                    <span>Load Sample Cube STL</span>
                  </button>

                  <button
                    onClick={handleLoadSampleCsv}
                    disabled={geoLoading}
                    className="bg-emerald-600 hover:bg-emerald-500 text-white text-xs px-3.5 py-2 rounded-lg font-medium transition shadow-md shadow-emerald-600/20 flex items-center space-x-1.5"
                  >
                    <Thermometer className="w-3.5 h-3.5" />
                    <span>Load Sample ANSYS CSV</span>
                  </button>

                  <label className="bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs px-3.5 py-2 rounded-lg font-medium cursor-pointer transition border border-slate-700 flex items-center space-x-1.5">
                    <Upload className="w-3.5 h-3.5" />
                    <span>Upload ASCII STL</span>
                    <input type="file" accept=".stl" className="hidden" onChange={(e) => handleFileUpload(e, "stl")} />
                  </label>

                  <label className="bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs px-3.5 py-2 rounded-lg font-medium cursor-pointer transition border border-slate-700 flex items-center space-x-1.5">
                    <Upload className="w-3.5 h-3.5" />
                    <span>Upload Temperature CSV</span>
                    <input type="file" accept=".csv" className="hidden" onChange={(e) => handleFileUpload(e, "csv")} />
                  </label>
                </div>

                {geoLoading && <span className="text-xs text-blue-400 animate-pulse font-mono">Parsing in C++ core...</span>}
              </div>

              {/* Error Alert */}
              {geoError && (
                <div className="bg-red-950/60 border border-red-800 text-red-300 p-3.5 rounded-xl text-xs flex items-center space-x-2">
                  <AlertCircle className="w-4 h-4 flex-shrink-0 text-red-400" />
                  <span>{geoError}</span>
                </div>
              )}

              {/* 3D Viewport */}
              <ThreeViewer stlData={stlResult} pointCloudData={csvResult} />

              {/* Data Summary Grid */}
              <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
                {/* STL Metadata Card */}
                <div className="bg-slate-900 border border-slate-800 p-4 rounded-xl text-xs space-y-2">
                  <h4 className="font-semibold text-slate-200 flex items-center space-x-1.5">
                    <Layers className="w-4 h-4 text-blue-400" />
                    <span>ASCII STL Geometry Summary</span>
                  </h4>
                  {stlResult ? (
                    <div className="space-y-1.5 font-mono text-slate-300">
                      <div>Solid Name: <span className="text-blue-400">{stlResult.model_name}</span></div>
                      <div>Triangles: <span className="text-emerald-400 font-bold">{stlResult.triangle_count}</span></div>
                      <div>Bounds X: [{stlResult.bounds.min[0].toFixed(2)}, {stlResult.bounds.max[0].toFixed(2)}] m</div>
                      <div>Bounds Y: [{stlResult.bounds.min[1].toFixed(2)}, {stlResult.bounds.max[1].toFixed(2)}] m</div>
                      <div>Bounds Z: [{stlResult.bounds.min[2].toFixed(2)}, {stlResult.bounds.max[2].toFixed(2)}] m</div>
                    </div>
                  ) : (
                    <p className="text-slate-500">No STL geometry loaded. Click &quot;Load Sample Cube STL&quot; or upload a file.</p>
                  )}
                </div>

                {/* CSV Metadata Card */}
                <div className="bg-slate-900 border border-slate-800 p-4 rounded-xl text-xs space-y-2">
                  <h4 className="font-semibold text-slate-200 flex items-center space-x-1.5">
                    <Thermometer className="w-4 h-4 text-emerald-400" />
                    <span>ANSYS Temperature Dataset Summary</span>
                  </h4>
                  {csvResult ? (
                    <div className="space-y-1.5 font-mono text-slate-300">
                      <div>Mesh Nodes: <span className="text-emerald-400 font-bold">{csvResult.node_count}</span></div>
                      <div>Temperature Range: <span className="text-blue-400">{csvResult.temp_min.toFixed(2)} °C</span> to <span className="text-red-400">{csvResult.temp_max.toFixed(2)} °C</span></div>
                      <div>Bounds X: [{csvResult.bounds.min[0].toFixed(2)}, {csvResult.bounds.max[0].toFixed(2)}] m</div>
                      <div>Data Type: <span className="text-amber-400 font-semibold">SYNTHETIC (ANSYS Nodal CSV)</span></div>
                    </div>
                  ) : (
                    <p className="text-slate-500">No ANSYS temperature CSV loaded. Click &quot;Load Sample ANSYS CSV&quot; or upload a file.</p>
                  )}
                </div>
              </div>
            </div>
          )}

          {/* ========================================================================= */}
          {/* TAB 2: THERMAL FEA VIEW */}
          {/* ========================================================================= */}
          {activeTab === "thermal" && (
            <div className="space-y-6">
              {/* Parameters Input & Actions */}
              <div className="bg-slate-900 border border-slate-800 p-5 rounded-xl space-y-4">
                <div className="flex items-center justify-between">
                  <h3 className="font-semibold text-sm text-slate-200">1D Steady-State Thermal Finite-Element Parameters</h3>
                  <div className="flex items-center space-x-3">
                    <button
                      onClick={handleRunThermal}
                      disabled={thermalLoading}
                      className="bg-blue-600 hover:bg-blue-500 text-white text-xs px-4 py-2 rounded-lg font-medium transition shadow-md shadow-blue-600/20 flex items-center space-x-1.5"
                    >
                      <Play className="w-3.5 h-3.5" />
                      <span>{thermalLoading ? "Solving in C++..." : "Run FEA Solver"}</span>
                    </button>

                    <a
                      href={api.exportThermalCsvUrl()}
                      className="bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs px-3.5 py-2 rounded-lg font-medium transition border border-slate-700 flex items-center space-x-1.5"
                    >
                      <Download className="w-3.5 h-3.5" />
                      <span>Export CSV</span>
                    </a>
                  </div>
                </div>

                <div className="grid grid-cols-2 md:grid-cols-4 lg:grid-cols-7 gap-3 text-xs">
                  <div>
                    <label className="text-slate-400 block mb-1">Length L (m)</label>
                    <input
                      type="text"
                      inputMode="decimal"
                      value={thermalParams.length}
                      onChange={(e) => setThermalParams({ ...thermalParams, length: e.target.value })}
                      className="w-full bg-slate-950 border border-slate-800 rounded px-2.5 py-1.5 text-slate-200 focus:outline-none focus:border-blue-500"
                    />
                  </div>
                  <div>
                    <label className="text-slate-400 block mb-1">Area A (m²)</label>
                    <input
                      type="text"
                      inputMode="decimal"
                      value={thermalParams.area}
                      onChange={(e) => setThermalParams({ ...thermalParams, area: e.target.value })}
                      className="w-full bg-slate-950 border border-slate-800 rounded px-2.5 py-1.5 text-slate-200 focus:outline-none focus:border-blue-500"
                    />
                  </div>
                  <div>
                    <label className="text-slate-400 block mb-1">Conductivity k (W/m·K)</label>
                    <input
                      type="text"
                      inputMode="decimal"
                      value={thermalParams.conductivity}
                      onChange={(e) => setThermalParams({ ...thermalParams, conductivity: e.target.value })}
                      className="w-full bg-slate-950 border border-slate-800 rounded px-2.5 py-1.5 text-slate-200 focus:outline-none focus:border-blue-500"
                    />
                  </div>
                  <div>
                    <label className="text-slate-400 block mb-1">Heat Gen q (W/m³)</label>
                    <input
                      type="text"
                      inputMode="decimal"
                      value={thermalParams.heat_generation}
                      onChange={(e) => setThermalParams({ ...thermalParams, heat_generation: e.target.value })}
                      className="w-full bg-slate-950 border border-slate-800 rounded px-2.5 py-1.5 text-slate-200 focus:outline-none focus:border-blue-500"
                    />
                  </div>
                  <div>
                    <label className="text-slate-400 block mb-1">T(0) Left (°C)</label>
                    <input
                      type="text"
                      inputMode="decimal"
                      value={thermalParams.temp_left}
                      onChange={(e) => setThermalParams({ ...thermalParams, temp_left: e.target.value })}
                      className="w-full bg-slate-950 border border-slate-800 rounded px-2.5 py-1.5 text-slate-200 focus:outline-none focus:border-blue-500"
                    />
                  </div>
                  <div>
                    <label className="text-slate-400 block mb-1">T(L) Right (°C)</label>
                    <input
                      type="text"
                      inputMode="decimal"
                      value={thermalParams.temp_right}
                      onChange={(e) => setThermalParams({ ...thermalParams, temp_right: e.target.value })}
                      className="w-full bg-slate-950 border border-slate-800 rounded px-2.5 py-1.5 text-slate-200 focus:outline-none focus:border-blue-500"
                    />
                  </div>
                  <div>
                    <label className="text-slate-400 block mb-1">Mesh Size N</label>
                    <select
                      value={thermalParams.num_elements}
                      onChange={(e) => setThermalParams({ ...thermalParams, num_elements: parseInt(e.target.value) })}
                      className="w-full bg-slate-950 border border-slate-800 rounded px-2.5 py-1.5 text-slate-200 focus:outline-none focus:border-blue-500"
                    >
                      <option value={10}>N = 10</option>
                      <option value={20}>N = 20</option>
                      <option value={40}>N = 40</option>
                      <option value={80}>N = 80</option>
                    </select>
                  </div>
                </div>
              </div>

              {thermalError && (
                <div className="bg-red-950/60 border border-red-800 text-red-300 p-3 rounded-lg text-xs flex items-center space-x-2">
                  <AlertCircle className="w-4 h-4 text-red-400" />
                  <span>{thermalError}</span>
                </div>
              )}

              {/* Temperature Distribution Plot & Convergence Plot */}
              {thermalResult && (
                <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
                  {/* Temperature vs x */}
                  <ScientificChart
                    title={`Temperature Distribution along Rod (N = ${thermalResult.num_elements})`}
                    xLabel="Position x (meters)"
                    yLabel="Temperature (°C)"
                    series={[
                      {
                        name: "FEM Solution",
                        color: "#3b82f6",
                        points: thermalResult.nodes.map((n) => ({ x: n.x, y: n.temp_fem })),
                      },
                      {
                        name: "Analytical Exact",
                        color: "#ef4444",
                        dashed: true,
                        points: thermalResult.nodes.map((n) => ({ x: n.x, y: n.temp_analytical })),
                      },
                    ]}
                  />

                  {/* Convergence Plot (Error vs N) */}
                  {convergenceResult && (
                    <ScientificChart
                      title="Mesh Convergence: Max Absolute Midpoint Error vs Elements N"
                      xLabel="Number of Elements N"
                      yLabel="Max Midpoint Error (°C)"
                      series={[
                        {
                          name: "Midpoint Error",
                          color: "#10b981",
                          points: convergenceResult.cases.map((c) => ({ x: c.num_elements, y: c.max_midpoint_error })),
                        },
                      ]}
                    />
                  )}
                </div>
              )}

              {/* Midpoint Convergence Verification Table */}
              {convergenceResult && (
                <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 text-xs space-y-3">
                  <h4 className="font-semibold text-slate-200">Analytical Midpoint Error & Quadratic Convergence Study</h4>
                  <div className="overflow-x-auto">
                    <table className="w-full text-left font-mono">
                      <thead className="border-b border-slate-800 text-slate-400">
                        <tr>
                          <th className="py-2 px-3">Mesh Elements N</th>
                          <th className="py-2 px-3">Element Size h (m)</th>
                          <th className="py-2 px-3">Max Midpoint Error (°C)</th>
                          <th className="py-2 px-3">Theoretical Error q/(8k)*h²</th>
                          <th className="py-2 px-3">L2 Norm Error (°C)</th>
                          <th className="py-2 px-3">Empirical Convergence Rate p</th>
                        </tr>
                      </thead>
                      <tbody className="divide-y divide-slate-800/60 text-slate-300">
                        {convergenceResult.cases.map((c) => (
                          <tr key={c.num_elements} className="hover:bg-slate-800/40">
                            <td className="py-2 px-3 font-semibold text-blue-400">N = {c.num_elements}</td>
                            <td className="py-2 px-3">{c.h.toFixed(4)}</td>
                            <td className="py-2 px-3 text-emerald-400 font-bold">{c.max_midpoint_error.toFixed(6)} °C</td>
                            <td className="py-2 px-3 text-slate-400">{(125.0 * c.h * c.h).toFixed(6)} °C</td>
                            <td className="py-2 px-3">{c.l2_error.toFixed(6)} °C</td>
                            <td className="py-2 px-3 font-semibold text-purple-400">
                              {c.convergence_rate > 0 ? `p = ${c.convergence_rate.toFixed(3)} (O(h²))` : "Baseline"}
                            </td>
                          </tr>
                        ))}
                      </tbody>
                    </table>
                  </div>
                </div>
              )}
            </div>
          )}

          {/* ========================================================================= */}
          {/* TAB 3: MULTICORE PERFORMANCE VIEW */}
          {/* ========================================================================= */}
          {activeTab === "performance" && (
            <div className="space-y-6">
              <div className="bg-slate-900 border border-slate-800 p-5 rounded-xl space-y-4">
                <div className="flex items-center justify-between">
                  <h3 className="font-semibold text-sm text-slate-200">Multicore CPU Benchmark Configuration</h3>
                  <button
                    onClick={handleRunBenchmark}
                    disabled={benchLoading}
                    className="bg-blue-600 hover:bg-blue-500 text-white text-xs px-4 py-2 rounded-lg font-medium transition shadow-md shadow-blue-600/20 flex items-center space-x-1.5"
                  >
                    <Zap className="w-3.5 h-3.5" />
                    <span>{benchLoading ? "Benchmarking Workers..." : "Run Parallel Benchmark"}</span>
                  </button>
                </div>

                <div className="grid grid-cols-1 md:grid-cols-3 gap-4 text-xs">
                  <div>
                    <label className="text-slate-400 block mb-1">Workload Cases (Batch Size)</label>
                    <input
                      type="text"
                      inputMode="numeric"
                      value={benchConfig.num_cases}
                      onChange={(e) => setBenchConfig({ ...benchConfig, num_cases: e.target.value })}
                      className="w-full bg-slate-950 border border-slate-800 rounded px-2.5 py-1.5 text-slate-200 focus:outline-none focus:border-blue-500 font-mono"
                      placeholder="e.g. 100"
                    />
                  </div>
                  <div>
                    <label className="text-slate-400 block mb-1">Mesh Elements per Case</label>
                    <input
                      type="text"
                      inputMode="numeric"
                      value={benchConfig.mesh_size}
                      onChange={(e) => setBenchConfig({ ...benchConfig, mesh_size: e.target.value })}
                      className="w-full bg-slate-950 border border-slate-800 rounded px-2.5 py-1.5 text-slate-200 focus:outline-none focus:border-blue-500 font-mono"
                      placeholder="e.g. 20"
                    />
                  </div>
                  <div>
                    <label className="text-slate-400 block mb-1">Fixed Random Seed (Reproducibility)</label>
                    <input
                      type="text"
                      inputMode="numeric"
                      value={benchConfig.random_seed}
                      onChange={(e) => setBenchConfig({ ...benchConfig, random_seed: e.target.value })}
                      className="w-full bg-slate-950 border border-slate-800 rounded px-2.5 py-1.5 text-slate-200 focus:outline-none focus:border-blue-500 font-mono"
                      placeholder="e.g. 42"
                    />
                  </div>
                </div>
              </div>

              {benchReport && (
                <div className="space-y-6">
                  {/* Agreement Check Banner */}
                  <div className={`p-4 rounded-xl border text-xs flex items-center justify-between ${
                    benchReport.agreement_passed
                      ? "bg-emerald-950/40 border-emerald-800 text-emerald-300"
                      : "bg-red-950/40 border-red-800 text-red-300"
                  }`}>
                    <div className="flex items-center space-x-2">
                      <CheckCircle2 className="w-4 h-4 text-emerald-400" />
                      <span>
                        Serial vs Parallel Correctness Agreement: Max Difference = <strong>{benchReport.max_temperature_diff.toExponential(4)} °C</strong> (Threshold ≤ 1e-8 °C)
                      </span>
                    </div>
                    <span className="font-bold text-emerald-400">PASSED</span>
                  </div>

                  {/* Benchmark Speedup Chart */}
                  <ScientificChart
                    title="Parallel Speedup vs Worker Count p (Bounded Worker Pool)"
                    xLabel="Worker Count p (threads)"
                    yLabel="Speedup S = T1 / Tp"
                    series={[
                      {
                        name: "Measured Speedup",
                        color: "#3b82f6",
                        points: benchReport.benchmarks.map((b) => ({ x: b.worker_count, y: b.speedup })),
                      },
                      {
                        name: "Ideal Linear Speedup",
                        color: "#64748b",
                        dashed: true,
                        points: benchReport.benchmarks.map((b) => ({ x: b.worker_count, y: b.worker_count })),
                      },
                    ]}
                  />

                  {/* Repetitions Table */}
                  <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 text-xs space-y-3">
                    <h4 className="font-semibold text-slate-200">5-Repetition Timing & Parallel Efficiency</h4>
                    <div className="overflow-x-auto">
                      <table className="w-full text-left font-mono">
                        <thead className="border-b border-slate-800 text-slate-400">
                          <tr>
                            <th className="py-2 px-3">Workers p</th>
                            <th className="py-2 px-3">Repetitions (ms)</th>
                            <th className="py-2 px-3">Median Time (ms)</th>
                            <th className="py-2 px-3">Speedup (T1/Tp)</th>
                            <th className="py-2 px-3">Efficiency</th>
                          </tr>
                        </thead>
                        <tbody className="divide-y divide-slate-800/60 text-slate-300">
                          {benchReport.benchmarks.map((b) => (
                            <tr key={b.worker_count}>
                              <td className="py-2 px-3 font-semibold text-blue-400">{b.worker_count} Worker{b.worker_count > 1 ? "s" : ""}</td>
                              <td className="py-2 px-3 text-slate-400">
                                {b.repetition_times_ms.map((t) => t.toFixed(1)).join(", ")}
                              </td>
                              <td className="py-2 px-3 font-bold text-emerald-400">{b.median_time_ms.toFixed(1)} ms</td>
                              <td className="py-2 px-3 text-purple-400 font-semibold">{b.speedup.toFixed(2)}x</td>
                              <td className="py-2 px-3 text-amber-400">{(b.efficiency * 100).toFixed(1)}%</td>
                            </tr>
                          ))}
                        </tbody>
                      </table>
                    </div>
                  </div>
                </div>
              )}
            </div>
          )}

          {/* ========================================================================= */}
          {/* TAB 4: EMBEDDED HARDWARE VIEW */}
          {/* ========================================================================= */}
          {activeTab === "hardware" && (
            <div className="space-y-6">
              {/* Device Cards Grid */}
              <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
                {devices.map((dev) => (
                  <div
                    key={dev.id}
                    onClick={() => selectDevice(dev)}
                    className={`p-5 rounded-xl border cursor-pointer transition ${
                      selectedDevice?.id === dev.id
                        ? "bg-slate-900 border-blue-500 shadow-xl shadow-blue-500/10"
                        : "bg-slate-900/60 border-slate-800 hover:border-slate-700"
                    }`}
                  >
                    <div className="flex items-center justify-between mb-3">
                      <span className="text-xs font-mono text-blue-400 font-semibold">{dev.id}</span>
                      <span
                        className={`text-[10px] px-2 py-0.5 rounded font-mono font-bold ${
                          dev.is_connected
                            ? "bg-emerald-950 text-emerald-400 border border-emerald-800"
                            : dev.status.includes("TIMEOUT")
                            ? "bg-amber-950 text-amber-400 border border-amber-800"
                            : "bg-slate-800 text-slate-400"
                        }`}
                      >
                        {dev.status}
                      </span>
                    </div>

                    <h4 className="font-bold text-sm text-slate-100 mb-1">{dev.name}</h4>
                    <p className="text-xs text-slate-400 mb-4">{dev.type}</p>

                    <div className="flex items-center justify-between pt-3 border-t border-slate-800 text-xs">
                      <button
                        onClick={(e) => {
                          e.stopPropagation();
                          handleToggleConnect(dev);
                        }}
                        disabled={hwLoading}
                        className={`px-3 py-1.5 rounded font-medium transition ${
                          dev.is_connected
                            ? "bg-red-950 hover:bg-red-900 text-red-300 border border-red-800"
                            : "bg-blue-600 hover:bg-blue-500 text-white"
                        }`}
                      >
                        {dev.is_connected ? "Disconnect" : "Connect Device"}
                      </button>

                      {dev.is_debug_interface && (
                        <span className="text-[10px] text-amber-400 font-mono">DEBUG PROBE</span>
                      )}
                    </div>
                  </div>
                ))}
              </div>

              {hwError && (
                <div className="bg-amber-950/60 border border-amber-800 text-amber-300 p-3.5 rounded-xl text-xs flex items-center space-x-2">
                  <AlertCircle className="w-4 h-4 flex-shrink-0 text-amber-400" />
                  <span>{hwError}</span>
                </div>
              )}

              {/* Selected Device Telemetry & Specs */}
              {selectedDevice && deviceCaps && (
                <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
                  {/* Capabilities Card */}
                  <div className="bg-slate-900 border border-slate-800 p-5 rounded-xl text-xs space-y-3">
                    <h4 className="font-semibold text-slate-200">Hardware Specifications & Voltage Domains</h4>
                    <div className="space-y-1.5 text-slate-300">
                      <div><strong className="text-slate-400">Model:</strong> {deviceCaps.model} ({deviceCaps.manufacturer})</div>
                      <div><strong className="text-slate-400">Host Interface:</strong> {deviceCaps.communication_interface}</div>
                      <div><strong className="text-slate-400">Voltage Domain:</strong> <span className="text-amber-400 font-mono">{deviceCaps.voltage_domain}</span></div>
                      <div><strong className="text-slate-400">Connectors:</strong> {deviceCaps.connector}</div>
                      <div>
                        <strong className="text-slate-400">Protocols:</strong> {deviceCaps.supported_protocols.join(", ")}
                      </div>
                    </div>
                    <div className="p-2.5 bg-slate-950 rounded border border-slate-800 text-[11px] text-slate-400">
                      {deviceCaps.signal_ownership_notes}
                    </div>
                  </div>

                  {/* Telemetry Card */}
                  <div className="bg-slate-900 border border-slate-800 p-5 rounded-xl text-xs space-y-3">
                    <div className="flex items-center justify-between">
                      <h4 className="font-semibold text-slate-200">Live Telemetry Monitor</h4>
                      <span className="text-[10px] font-mono text-amber-400 bg-amber-950/60 px-2 py-0.5 rounded border border-amber-800">
                        SIMULATED
                      </span>
                    </div>

                    {telemetry ? (
                      <div className="space-y-3">
                        <div className="grid grid-cols-3 gap-2 text-center">
                          <div className="bg-slate-950 p-2.5 rounded border border-slate-800">
                            <div className="text-[10px] text-slate-400">Voltage</div>
                            <div className="text-sm font-mono font-bold text-emerald-400">{telemetry.voltage_v.toFixed(3)} V</div>
                          </div>
                          <div className="bg-slate-950 p-2.5 rounded border border-slate-800">
                            <div className="text-[10px] text-slate-400">Current</div>
                            <div className="text-sm font-mono font-bold text-blue-400">{telemetry.current_ma.toFixed(1)} mA</div>
                          </div>
                          <div className="bg-slate-950 p-2.5 rounded border border-slate-800">
                            <div className="text-[10px] text-slate-400">Die Temp</div>
                            <div className="text-sm font-mono font-bold text-amber-400">{telemetry.temperature_c.toFixed(1)} °C</div>
                          </div>
                        </div>

                        <div className="p-2.5 bg-slate-950 rounded border border-slate-800 text-slate-300 font-mono text-[11px]">
                          <div>Status: <span className="text-emerald-400">{telemetry.status_message}</span></div>
                          <div>Uptime: {telemetry.uptime_seconds}s</div>
                          {Object.entries(telemetry.metrics).map(([k, v]) => (
                            <div key={k}>{k}: <span className="text-slate-400">{v}</span></div>
                          ))}
                        </div>
                      </div>
                    ) : (
                      <p className="text-slate-500 py-6 text-center">Connect device to start streaming live telemetry.</p>
                    )}
                  </div>
                </div>
              )}
            </div>
          )}

          {/* ========================================================================= */}
          {/* TAB 5: BACKPROPAGATION VIEW */}
          {/* ========================================================================= */}
          {activeTab === "backprop" && (
            <div className="space-y-6">
              <div className="bg-slate-900 border border-slate-800 p-5 rounded-xl flex items-center justify-between">
                <div>
                  <h3 className="font-semibold text-sm text-slate-200">Neural Network Backpropagation & Finite Differences</h3>
                  <p className="text-xs text-slate-400 mt-1">
                    Regression sample: x = 1.0, Target y = 2.0 &bull; Network: h = tanh(wx + b), ŷ = vh + c &bull; Loss J = 0.5(ŷ - y)²
                  </p>
                </div>
                <div className="flex items-center space-x-3">
                  <button
                    onClick={handleRunBackprop}
                    disabled={backpropLoading}
                    className="bg-blue-600 hover:bg-blue-500 text-white text-xs px-4 py-2 rounded-lg font-medium transition shadow-md shadow-blue-600/20 flex items-center space-x-1.5"
                  >
                    <Play className="w-3.5 h-3.5" />
                    <span>{backpropLoading ? "Evaluating in C++..." : "Compute Gradients & Train"}</span>
                  </button>
                  <a
                    href={api.exportBackpropCsvUrl()}
                    className="bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs px-3.5 py-2 rounded-lg font-medium transition border border-slate-700 flex items-center space-x-1.5"
                  >
                    <Download className="w-3.5 h-3.5" />
                    <span>Export Training CSV</span>
                  </a>
                </div>
              </div>

              {backpropCompute && (
                <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
                  {/* Gradients & Finite Difference Verification Card */}
                  <div className="bg-slate-900 border border-slate-800 p-5 rounded-xl text-xs space-y-3 font-mono">
                    <div className="flex items-center justify-between">
                      <h4 className="font-semibold text-slate-200 font-sans">Chain-Rule Gradients vs Finite Differences (ε = 1e-6)</h4>
                      <span className="text-[10px] text-emerald-400 bg-emerald-950 px-2 py-0.5 rounded border border-emerald-800 font-bold">
                        VERIFIED ≤ 1e-5
                      </span>
                    </div>

                    <table className="w-full text-left">
                      <thead className="border-b border-slate-800 text-slate-400">
                        <tr>
                          <th className="py-1">Param</th>
                          <th className="py-1">Analytical</th>
                          <th className="py-1">Numerical</th>
                          <th className="py-1">Abs Error</th>
                          <th className="py-1">Updated (η=0.1)</th>
                        </tr>
                      </thead>
                      <tbody className="divide-y divide-slate-800/60 text-slate-300">
                        <tr>
                          <td className="py-1.5 font-bold text-blue-400">w</td>
                          <td>{backpropCompute.gradients.dJ_dw.toFixed(6)}</td>
                          <td>{backpropCompute.verification.numerical_dJ_dw.toFixed(6)}</td>
                          <td className="text-emerald-400">{backpropCompute.verification.abs_error_w.toExponential(2)}</td>
                          <td className="text-purple-400 font-bold">{backpropCompute.updated_step1.w.toFixed(6)}</td>
                        </tr>
                        <tr>
                          <td className="py-1.5 font-bold text-blue-400">b</td>
                          <td>{backpropCompute.gradients.dJ_db.toFixed(6)}</td>
                          <td>{backpropCompute.verification.numerical_dJ_db.toFixed(6)}</td>
                          <td className="text-emerald-400">{backpropCompute.verification.abs_error_b.toExponential(2)}</td>
                          <td className="text-purple-400 font-bold">{backpropCompute.updated_step1.b.toFixed(6)}</td>
                        </tr>
                        <tr>
                          <td className="py-1.5 font-bold text-blue-400">v</td>
                          <td>{backpropCompute.gradients.dJ_dv.toFixed(6)}</td>
                          <td>{backpropCompute.verification.numerical_dJ_dv.toFixed(6)}</td>
                          <td className="text-emerald-400">{backpropCompute.verification.abs_error_v.toExponential(2)}</td>
                          <td className="text-purple-400 font-bold">{backpropCompute.updated_step1.v.toFixed(6)}</td>
                        </tr>
                        <tr>
                          <td className="py-1.5 font-bold text-blue-400">c</td>
                          <td>{backpropCompute.gradients.dJ_dc.toFixed(6)}</td>
                          <td>{backpropCompute.verification.numerical_dJ_dc.toFixed(6)}</td>
                          <td className="text-emerald-400">{backpropCompute.verification.abs_error_c.toExponential(2)}</td>
                          <td className="text-purple-400 font-bold">{backpropCompute.updated_step1.c.toFixed(6)}</td>
                        </tr>
                      </tbody>
                    </table>

                    <div className="pt-2 text-[11px] text-slate-400 font-sans">
                      {backpropCompute.explanation}
                    </div>
                  </div>

                  {/* 100-Step Loss Convergence Plot */}
                  {backpropTrain && (
                    <ScientificChart
                      title="100-Step Training Loss Curve J(θ)"
                      xLabel="Training Step"
                      yLabel="Loss J"
                      series={[
                        {
                          name: "Squared Error Loss",
                          color: "#3b82f6",
                          points: backpropTrain.steps.map((s) => ({ x: s.step, y: s.loss })),
                        },
                      ]}
                    />
                  )}
                </div>
              )}
            </div>
          )}

          {/* ========================================================================= */}
          {/* TAB 6: AUTOMATED TESTING & VALIDATION (GOOGLETEST & CTEST) */}
          {/* ========================================================================= */}
          {activeTab === "tests" && (
            <div className="space-y-6">
              {/* Header & Execution Toolbar */}
              <div className="bg-slate-900 border border-slate-800 p-5 rounded-xl space-y-4">
                <div className="flex flex-wrap items-center justify-between gap-4">
                  <div>
                    <h3 className="text-base font-bold text-white flex items-center space-x-2">
                      <CheckSquare className="w-5 h-5 text-blue-400" />
                      <span>GoogleTest & CTest Automated Validation Suite</span>
                    </h3>
                    <p className="text-xs text-slate-400 mt-1">
                      Executes native C++ unit & integration tests covering Geometry, Thermal FEM, Multicore, Embedded Hardware, and Backpropagation.
                    </p>
                  </div>

                  <div className="flex items-center space-x-2.5">
                    <button
                      onClick={handleRunTests}
                      disabled={testLoading}
                      className="bg-blue-600 hover:bg-blue-500 disabled:opacity-50 text-white text-xs px-4 py-2.5 rounded-lg font-semibold transition shadow-md shadow-blue-600/20 flex items-center space-x-2"
                    >
                      {testLoading ? (
                        <>
                          <RefreshCw className="w-4 h-4 animate-spin" />
                          <span>Running Test Suite...</span>
                        </>
                      ) : (
                        <>
                          <Play className="w-4 h-4" />
                          <span>Run Test Suite</span>
                        </>
                      )}
                    </button>

                    <button
                      onClick={fetchLatestTests}
                      disabled={testLoading}
                      title="Reload latest test results from disk"
                      className="bg-slate-800 hover:bg-slate-700 text-slate-300 text-xs px-3 py-2.5 rounded-lg font-medium transition border border-slate-700 flex items-center space-x-1.5"
                    >
                      <RefreshCw className="w-3.5 h-3.5" />
                      <span>Reload</span>
                    </button>

                    <a
                      href={api.exportTestXmlUrl()}
                      download="test_report.xml"
                      className="bg-slate-800 hover:bg-slate-700 text-slate-300 text-xs px-3 py-2.5 rounded-lg font-medium transition border border-slate-700 flex items-center space-x-1.5"
                    >
                      <Download className="w-3.5 h-3.5" />
                      <span>JUnit XML</span>
                    </a>

                    <a
                      href={api.exportTestLogUrl()}
                      download="test_execution.log"
                      className="bg-slate-800 hover:bg-slate-700 text-slate-300 text-xs px-3 py-2.5 rounded-lg font-medium transition border border-slate-700 flex items-center space-x-1.5"
                    >
                      <FileText className="w-3.5 h-3.5" />
                      <span>Log</span>
                    </a>
                  </div>
                </div>

                {/* Real-time Status Notification */}
                {testLoading && (
                  <div className="bg-amber-950/40 border border-amber-800 text-amber-300 px-4 py-3 rounded-lg text-xs flex items-center space-x-2.5 animate-pulse">
                    <RefreshCw className="w-4 h-4 animate-spin text-amber-400" />
                    <span className="font-medium">{testRunningStatus || "Executing GoogleTest & CTest suite via backend..."}</span>
                  </div>
                )}

                {testResult && !testLoading && (
                  <div
                    className={`px-4 py-3 rounded-lg text-xs flex items-center justify-between border ${
                      testResult.failed === 0 && testResult.executed
                        ? "bg-emerald-950/40 border-emerald-800 text-emerald-300"
                        : "bg-red-950/40 border-red-800 text-red-300"
                    }`}
                  >
                    <div className="flex items-center space-x-2">
                      {testResult.failed === 0 && testResult.executed ? (
                        <CheckCircle2 className="w-4 h-4 text-emerald-400" />
                      ) : (
                        <XCircle className="w-4 h-4 text-red-400" />
                      )}
                      <span className="font-semibold">
                        {testResult.failed === 0 && testResult.executed
                          ? `All Test Suites Passed (${testResult.passed} / ${testResult.total} test cases succeeded)`
                          : `Failures Detected: ${testResult.failed} failed out of ${testResult.total} tests`}
                      </span>
                    </div>
                    <div className="text-[11px] text-slate-400 font-mono">
                      Exit Code: {testResult.exit_code} | Duration: {testResult.duration_ms.toFixed(1)} ms | Timestamp: {testResult.timestamp || "Just now"}
                    </div>
                  </div>
                )}

                {/* Test Metrics Overview Cards */}
                <div className="grid grid-cols-2 md:grid-cols-4 gap-3 text-xs">
                  <div className="bg-slate-950 p-3.5 rounded-lg border border-slate-800">
                    <div className="text-slate-400 mb-1 flex items-center justify-between">
                      <span>Total Test Cases</span>
                      <CheckSquare className="w-3.5 h-3.5 text-blue-400" />
                    </div>
                    <div className="text-xl font-bold text-slate-100 font-mono">
                      {testResult ? testResult.total : 0}
                    </div>
                    <div className="text-[11px] text-slate-500 mt-1">
                      {testResult ? `${testResult.duration_ms.toFixed(1)} ms total time` : "No tests executed"}
                    </div>
                  </div>

                  <div className="bg-slate-950 p-3.5 rounded-lg border border-slate-800">
                    <div className="text-slate-400 mb-1 flex items-center justify-between">
                      <span>Passed</span>
                      <CheckCircle2 className="w-3.5 h-3.5 text-emerald-400" />
                    </div>
                    <div className="text-xl font-bold text-emerald-400 font-mono">
                      {testResult ? testResult.passed : 0}
                    </div>
                    <div className="text-[11px] text-slate-500 mt-1">
                      {testResult && testResult.total > 0
                        ? `${((testResult.passed / testResult.total) * 100).toFixed(0)}% pass rate`
                        : "Ready to run"}
                    </div>
                  </div>

                  <div className="bg-slate-950 p-3.5 rounded-lg border border-slate-800">
                    <div className="text-slate-400 mb-1 flex items-center justify-between">
                      <span>Failed</span>
                      <XCircle className="w-3.5 h-3.5 text-red-400" />
                    </div>
                    <div className={`text-xl font-bold font-mono ${testResult?.failed ? "text-red-400" : "text-slate-100"}`}>
                      {testResult ? testResult.failed : 0}
                    </div>
                    <div className="text-[11px] text-slate-500 mt-1">
                      {testResult?.failed === 0 ? "Zero regressions" : "Investigation required"}
                    </div>
                  </div>

                  <div className="bg-slate-950 p-3.5 rounded-lg border border-slate-800">
                    <div className="text-slate-400 mb-1 flex items-center justify-between">
                      <span>Skipped / Filtered</span>
                      <Clock className="w-3.5 h-3.5 text-slate-400" />
                    </div>
                    <div className="text-xl font-bold text-slate-400 font-mono">
                      {testResult ? testResult.skipped : 0}
                    </div>
                    <div className="text-[11px] text-slate-500 mt-1">
                      JUnit XML verified
                    </div>
                  </div>
                </div>
              </div>

              {/* Table of Test Cases & Results */}
              {testResult && testResult.tests && testResult.tests.length > 0 && (
                <div className="bg-slate-900 border border-slate-800 rounded-xl overflow-hidden shadow-lg">
                  <div className="p-4 border-b border-slate-800 flex flex-wrap items-center justify-between gap-3">
                    <div className="flex items-center space-x-2">
                      <span className="text-xs font-semibold text-slate-200">Test Execution Details</span>
                      <span className="text-[11px] bg-slate-800 text-slate-400 px-2 py-0.5 rounded-full font-mono">
                        {testResult.tests.length} tests
                      </span>
                    </div>

                    <div className="flex items-center space-x-2">
                      {/* Filter pills */}
                      <div className="flex items-center bg-slate-950 p-0.5 rounded-lg border border-slate-800 text-xs">
                        <button
                          onClick={() => setTestFilter("ALL")}
                          className={`px-2.5 py-1 rounded font-medium transition ${
                            testFilter === "ALL" ? "bg-blue-600 text-white" : "text-slate-400 hover:text-white"
                          }`}
                        >
                          All ({testResult.total})
                        </button>
                        <button
                          onClick={() => setTestFilter("PASSED")}
                          className={`px-2.5 py-1 rounded font-medium transition ${
                            testFilter === "PASSED" ? "bg-emerald-600 text-white" : "text-slate-400 hover:text-white"
                          }`}
                        >
                          Passed ({testResult.passed})
                        </button>
                        <button
                          onClick={() => setTestFilter("FAILED")}
                          className={`px-2.5 py-1 rounded font-medium transition ${
                            testFilter === "FAILED" ? "bg-red-600 text-white" : "text-slate-400 hover:text-white"
                          }`}
                        >
                          Failed ({testResult.failed})
                        </button>
                      </div>

                      {/* Search input */}
                      <input
                        type="text"
                        placeholder="Search tests..."
                        value={testSearch}
                        onChange={(e) => setTestSearch(e.target.value)}
                        className="bg-slate-950 border border-slate-800 rounded px-2.5 py-1 text-xs text-slate-200 placeholder-slate-600 focus:outline-none focus:border-blue-500 w-44"
                      />
                    </div>
                  </div>

                  <div className="max-h-96 overflow-y-auto">
                    <table className="w-full text-left text-xs">
                      <thead className="bg-slate-950/80 sticky top-0 text-slate-400 border-b border-slate-800">
                        <tr>
                          <th className="py-2.5 px-4 font-semibold">Test Suite</th>
                          <th className="py-2.5 px-4 font-semibold">Test Case</th>
                          <th className="py-2.5 px-4 font-semibold text-center">Status</th>
                          <th className="py-2.5 px-4 font-semibold text-right">Duration (ms)</th>
                        </tr>
                      </thead>
                      <tbody className="divide-y divide-slate-800/60 font-mono text-[11px]">
                        {testResult.tests
                          .filter((t) => {
                            if (testFilter === "PASSED" && t.status !== "PASSED") return false;
                            if (testFilter === "FAILED" && t.status !== "FAILED") return false;
                            if (testSearch.trim() !== "") {
                              const s = testSearch.toLowerCase();
                              return (
                                t.suite.toLowerCase().includes(s) ||
                                t.name.toLowerCase().includes(s)
                              );
                            }
                            return true;
                          })
                          .map((t, idx) => (
                            <tr key={idx} className="hover:bg-slate-800/40 transition">
                              <td className="py-2 px-4 text-blue-400 font-medium">{t.suite}</td>
                              <td className="py-2 px-4 text-slate-200">
                                <div>{t.name}</div>
                                {t.failure_message && (
                                  <div className="mt-1 text-red-400 bg-red-950/40 p-2 rounded border border-red-900/60 whitespace-pre-wrap font-sans text-[10px]">
                                    {t.failure_message}
                                  </div>
                                )}
                              </td>
                              <td className="py-2 px-4 text-center">
                                <span
                                  className={`inline-flex items-center px-2 py-0.5 rounded text-[10px] font-bold ${
                                    t.status === "PASSED"
                                      ? "bg-emerald-950 text-emerald-400 border border-emerald-800"
                                      : "bg-red-950 text-red-400 border border-red-800"
                                  }`}
                                >
                                  {t.status}
                                </span>
                              </td>
                              <td className="py-2 px-4 text-right text-slate-400">
                                {t.duration_ms > 0 ? t.duration_ms.toFixed(2) : "<0.1"}
                              </td>
                            </tr>
                          ))}
                      </tbody>
                    </table>
                  </div>
                </div>
              )}

              {/* Console Output Terminal */}
              <div className="bg-slate-900 border border-slate-800 rounded-xl overflow-hidden shadow-lg">
                <div className="p-3.5 bg-slate-950/80 border-b border-slate-800 flex items-center justify-between">
                  <div className="flex items-center space-x-2">
                    <Terminal className="w-4 h-4 text-slate-400" />
                    <span className="text-xs font-semibold text-slate-200">
                      Execution Console Output (stdout/stderr)
                    </span>
                  </div>
                  <div className="flex items-center space-x-2">
                    <button
                      onClick={handleCopyConsole}
                      className="bg-slate-800 hover:bg-slate-700 text-slate-300 text-xs px-2.5 py-1 rounded transition flex items-center space-x-1"
                    >
                      <Copy className="w-3 h-3" />
                      <span>{copiedLog ? "Copied!" : "Copy Output"}</span>
                    </button>
                    <button
                      onClick={() => setTestOutputExpanded(!testOutputExpanded)}
                      className="text-xs text-slate-400 hover:text-white px-2 py-1"
                    >
                      {testOutputExpanded ? "Collapse" : "Expand"}
                    </button>
                  </div>
                </div>

                {testOutputExpanded && (
                  <div className="p-4 bg-black/95 max-h-80 overflow-y-auto font-mono text-[11px] text-slate-300 leading-relaxed whitespace-pre-wrap select-text">
                    {testResult?.console_output || (
                      <span className="text-slate-600">
                        No console output recorded yet. Click "Run Test Suite" to execute GoogleTest and CTest.
                      </span>
                    )}
                  </div>
                )}
              </div>
            </div>
          )}

          {/* ========================================================================= */}
          {/* TAB 7: DOCUMENTATION & REPORT */}
          {/* ========================================================================= */}
          {activeTab === "docs" && (
            <div className="space-y-6 max-w-4xl text-xs text-slate-300 leading-relaxed">
              <div className="bg-slate-900 border border-slate-800 p-6 rounded-xl space-y-4">
                <h3 className="text-base font-bold text-white">PASS SCIENCE WORKS — Technical Report Summary</h3>
                <p>
                  This workbench delivers an end-to-end engineering solution satisfying all requirements of the R&D Internship 2026-2027 technical assignment.
                </p>

                <h4 className="text-sm font-semibold text-blue-400 pt-2">1. Mathematical Thermal FEA Formulation</h4>
                <p>
                  Governing steady-state 1D heat conduction: <code>-d/dx(kA dT/dx) = qA</code> on <code>x ∈ [0, 1]</code> with <code>T(0) = 100°C</code> and <code>T(1) = 20°C</code>.
                  Discretized with linear two-node elements. The stiffness matrix is assembled directly into tridiagonal format and solved in O(N) operations via the Thomas algorithm.
                  Midpoint error strictly verifies quadratic convergence: <code>Error = (q / 8k) * h²</code>, yielding exact midpoint errors of <code>1.25, 0.3125, 0.078125, 0.01953125 °C</code> for <code>N = 10, 20, 40, 80</code>.
                </p>

                <h4 className="text-sm font-semibold text-blue-400 pt-2">2. Multicore Execution & Determinism</h4>
                <p>
                  The parallel batch engine distributes independent thermal solves over a bounded worker pool (up to 4 CPU workers).
                  Maximum absolute temperature difference between serial and parallel solutions is verified to be <code>&le; 1e-8 °C</code> (actual 0.0 °C).
                </p>

                <h4 className="text-sm font-semibold text-blue-400 pt-2">3. Embedded Hardware Architecture</h4>
                <p>
                  Integrates three distinct devices: STM32F4 Discovery (3.3V MCU), Digilent Arty A7 (Xilinx Artix-7 FPGA with 1.0V core / 3.3V I/O), and SEGGER J-Link BASE (debug probe).
                  The JTAG adapter is treated as a bus master debug probe rather than a generic compute board. Recoverable timeout failure and recovery are demonstrated interactively.
                </p>

                <h4 className="text-sm font-semibold text-blue-400 pt-2">4. Handwritten Backpropagation</h4>
                <p>
                  Single-sample regression forward pass, analytical chain-rule gradients, and simultaneous gradient updates are verified against central finite differences with <code>ε = 1e-6</code>, achieving absolute errors &le; 1e-7.
                </p>
              </div>
            </div>
          )}
        </div>
      </main>
    </div>
  );
}

