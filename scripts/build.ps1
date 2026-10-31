# build.ps1 — Complete Automated Build and Verification Script
# PASS SCIENCE WORKS — Unified Engineering Workbench

$ErrorActionPreference = "Stop"
$rootDir = (Get-Item $PSScriptRoot).Parent.FullName
Write-Host "========================================================" -ForegroundColor Cyan
Write-Host " PASS SCIENCE WORKS - Full Workbench Build Pipeline" -ForegroundColor Cyan
Write-Host "========================================================" -ForegroundColor Cyan

# 1. Setup Toolchain Environment
$msysBin = "C:\msys64\mingw64\bin"
if (Test-Path $msysBin) {
    $env:PATH = "$msysBin;" + $env:PATH
}

# 2. Build C++ Backend & Tests
Write-Host "`n[1/5] Building C++17 Backend and Test Suite..." -ForegroundColor Yellow
$buildDir = Join-Path $rootDir "build"
if (!(Test-Path $buildDir)) { New-Item -ItemType Directory -Path $buildDir | Out-Null }
Set-Location $buildDir

cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# 3. Execute Automated Tests
Write-Host "`n[2/5] Executing 28-Test Automated Suite..." -ForegroundColor Yellow
$testExe = Join-Path $buildDir "workbench_tests.exe"
& $testExe
if ($LASTEXITCODE -ne 0) {
    Write-Error "Test suite failed! Aborting build."
}

# 4. Build Next.js Frontend
Write-Host "`n[3/5] Building Next.js Static Frontend..." -ForegroundColor Yellow
$frontendDir = Join-Path $rootDir "src\frontend"
Set-Location $frontendDir
& "C:\Program Files\nodejs\npm.cmd" run build

# Copy UI export to root and build directory
$outDir = Join-Path $frontendDir "out"
$rootUiDir = Join-Path $rootDir "ui"
if (Test-Path $rootUiDir) { Remove-Item $rootUiDir -Recurse -Force }
Copy-Item $outDir -Destination $rootUiDir -Recurse -Force

# 5. Compile Native Launcher
Write-Host "`n[4/5] Compiling Native Win32 Launcher (Launch.exe)..." -ForegroundColor Yellow
Set-Location $rootDir
$launcherSrc = Join-Path $rootDir "src\backend\launcher.cpp"
$launcherOut = Join-Path $rootDir "Launch.exe"
g++.exe -O3 -mwindows $launcherSrc -o $launcherOut

# Copy runtime binaries to root
Copy-Item (Join-Path $buildDir "workbench_backend.exe") $rootDir -Force
Copy-Item (Join-Path $buildDir "libgcc_s_seh-1.dll") $rootDir -Force -ErrorAction SilentlyContinue
Copy-Item (Join-Path $buildDir "libstdc++-6.dll") $rootDir -Force -ErrorAction SilentlyContinue
Copy-Item (Join-Path $buildDir "libwinpthread-1.dll") $rootDir -Force -ErrorAction SilentlyContinue

# 6. Generate Results & Plots
Write-Host "`n[5/5] Generating Empirical Results and Plots..." -ForegroundColor Yellow
$backendProc = Start-Process -FilePath (Join-Path $rootDir "workbench_backend.exe") -ArgumentList "8080" -PassThru
Start-Sleep -Seconds 1
try {
    python (Join-Path $rootDir "scripts\generate_results.py")
    python (Join-Path $rootDir "scripts\generate_plots.py")
} finally {
    Stop-Process -Id $backendProc.Id -Force -ErrorAction SilentlyContinue
}

Write-Host "`n========================================================" -ForegroundColor Green
Write-Host " BUILD & VERIFICATION PIPELINE COMPLETE: SUCCESS!" -ForegroundColor Green
Write-Host " Double-click Launch.exe to start the application." -ForegroundColor Green
Write-Host "========================================================" -ForegroundColor Green
