# launch.ps1 — Helper Launch Script for Development & Verification
$rootDir = (Get-Item $PSScriptRoot).Parent.FullName
$launchExe = Join-Path $rootDir "Launch.exe"

if (!(Test-Path $launchExe)) {
    Write-Host "Launch.exe not found! Running build first..." -ForegroundColor Yellow
    & (Join-Path $PSScriptRoot "build.ps1")
}

Write-Host "Starting PASS SCIENCE WORKS Workbench via Launch.exe..." -ForegroundColor Cyan
Start-Process -FilePath $launchExe
