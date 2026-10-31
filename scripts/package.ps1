# package.ps1 — Submissions Packaging Script
# Creates CandidateID_Workbench.zip satisfying all assignment delivery requirements

$ErrorActionPreference = "Stop"
$rootDir = (Get-Item $PSScriptRoot).Parent.FullName
$zipName = "CandidateID_Workbench.zip"
$zipPath = Join-Path $rootDir $zipName

Write-Host "Creating submission archive: $zipName..." -ForegroundColor Cyan

if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}

# Create staging directory
$stageDir = Join-Path $rootDir "staging_pkg"
if (Test-Path $stageDir) { Remove-Item $stageDir -Recurse -Force }
New-Item -ItemType Directory -Path $stageDir | Out-Null

$itemsToCopy = @(
    "Launch.exe",
    "workbench_backend.exe",
    "libgcc_s_seh-1.dll",
    "libstdc++-6.dll",
    "libwinpthread-1.dll",
    "CMakeLists.txt",
    "README.md",
    "AGENTS.md",
    "GEMINI.md",
    "CLAUDE.md",
    "SKILL.md",
    "tasks.md",
    "task_plan.md",
    "findings.md",
    "progress.md",
    ".gitignore",
    ".vscode",
    "data",
    "docs",
    "results",
    "scripts",
    "src",
    "tests",
    "ui"
)

foreach ($item in $itemsToCopy) {
    $srcPath = Join-Path $rootDir $item
    if (Test-Path $srcPath) {
        Copy-Item -Path $srcPath -Destination $stageDir -Recurse -Force
    }
}

# Exclude large development artifacts from staging
$excludePaths = @(
    (Join-Path $stageDir "src\frontend\node_modules"),
    (Join-Path $stageDir "src\frontend\.next")
)

foreach ($p in $excludePaths) {
    if (Test-Path $p) {
        Remove-Item -Path $p -Recurse -Force
    }
}

# Compress staging directory
Compress-Archive -Path (Join-Path $stageDir "*") -DestinationPath $zipPath -CompressionLevel Optimal

# Cleanup staging
Remove-Item $stageDir -Recurse -Force

$zipInfo = Get-Item $zipPath
Write-Host "Successfully generated: $($zipInfo.FullName) ($([math]::Round($zipInfo.Length / 1MB, 2)) MB)" -ForegroundColor Green

