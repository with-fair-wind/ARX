<#
.SYNOPSIS
  Generate compile_commands.json in the build directory.

.DESCRIPTION
  1. Create the build folder if it does not exist.
  2. Locate clang-build.ps1.
  3. Clear old kJsonCompilationDbPath variables.
  4. Switch to build directory and invoke clang-build.ps1 to generate JSON.
#>

param(
    [string]$ProjectRoot = (Split-Path -Parent $MyInvocation.MyCommand.Definition),
    [string]$BuildDir = "$(Split-Path -Parent $MyInvocation.MyCommand.Definition)\build",
    [string]$Config = ""
)

Write-Host "PowerShell version:" $PSVersionTable.PSVersion
Write-Host "Current script path:" $PSCommandPath
Write-Host "Current PowerShell process path:" (Get-Process -Id $PID).Path
Write-Host "Project root: $ProjectRoot"
Write-Host "Build dir:    $BuildDir"

# 1) Create build directory
if (-not (Test-Path $BuildDir)) {
    Write-Host "> Creating directory $BuildDir"
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# Prefer scanning the CMake build directory for solutions/projects.
# Scanning the project root can unintentionally pick up 3rdparty SDK sample .vcxproj files
# (often using older toolsets like v141), which may fail on machines without those toolsets.
Write-Host "> Detecting solution scan directory..."
$scanDir = $BuildDir
$sln = Get-ChildItem -Path $BuildDir -Filter "*.sln*" -File -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $sln) {
    Write-Warning "No .sln/.slnx found in build dir. Falling back to project root scan."
    $scanDir = $ProjectRoot
}
Write-Host "Scan dir:     $scanDir"

# 2) Locate clang-build.ps1
Write-Host "> Locating clang-build.ps1..."
$clangBuild = Get-ChildItem `
    -Path "$env:LOCALAPPDATA\Microsoft\VisualStudio" `
    -Filter "clang-build.ps1" -Recurse -ErrorAction SilentlyContinue |
Select-Object -First 1

if (-not $clangBuild) {
    Write-Error "clang-build.ps1 not found. Please install Clang Power Tools first."
    exit 1
}
Write-Host "Found script: $($clangBuild.FullName)"

# 3) Switch to build directory and execute (JSON will be output here)
Push-Location $BuildDir
try {
    Write-Host "> Clearing old kJsonCompilationDbPath variables..."
    Remove-Variable -Name kJsonCompilationDbPath -Scope Script -Force -ErrorAction SilentlyContinue
    Remove-Variable -Name kJsonCompilationDbPath -Scope Global -Force -ErrorAction SilentlyContinue

    Write-Host "> Generating compile_commands.json..."
    & $clangBuild.FullName `
        -dir $scanDir `
        -active-config "$Config|x64" `
        -export-jsondb `
        -parallel

    $outInScanDir = Join-Path $scanDir 'compile_commands.json'
    $outInBuildDir = Join-Path $BuildDir 'compile_commands.json'

    if (Test-Path $outInScanDir) {
        if ($scanDir -ne $BuildDir) {
            Move-Item -Force $outInScanDir $outInBuildDir
            Write-Host "> Moved compile_commands.json to $BuildDir"
        }
        else {
            Write-Host "> compile_commands.json generated in $BuildDir"
        }
    }
    else {
        Write-Warning "Operation completed, but compile_commands.json was not found in $scanDir"
    }

    # If a stale root-level compile_commands.json exists, remove it to avoid confusion.
    $staleRoot = Join-Path $ProjectRoot 'compile_commands.json'
    if ((Test-Path $staleRoot) -and ($staleRoot -ne $outInBuildDir)) {
        Remove-Item -Force $staleRoot -ErrorAction SilentlyContinue
    }
}
finally {
    Pop-Location
}