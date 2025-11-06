# PowerShell Build Script for Wolf Examples
# Requires: BOOST_DIR environment variable or will use default
# Requires: clang++ in PATH

param(
    [string]$BoostDir = $env:BOOST_DIR
)

# Use default if not set
if (-not $BoostDir) {
    $BoostDir = "c:\libraries\boost"
    Write-Host "BOOST_DIR not set, using default: $BoostDir" -ForegroundColor Yellow
}

# Set environment variable for this session and child processes
$env:BOOST_DIR = $BoostDir

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Wolf Examples Build Script (Windows)" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Verify Boost directory
if (-not (Test-Path "$BoostDir\include\boost")) {
    Write-Host "ERROR: Boost headers not found at $BoostDir\include\boost" -ForegroundColor Red
    Write-Host "Please ensure Boost is properly installed." -ForegroundColor Red
    exit 1
}

if (-not (Test-Path "$BoostDir\lib")) {
    Write-Host "ERROR: Boost libraries not found at $BoostDir\lib" -ForegroundColor Red
    exit 1
}

Write-Host "BOOST_DIR: $BoostDir" -ForegroundColor Green
Write-Host "Boost headers: $BoostDir\include" -ForegroundColor Gray
Write-Host "Boost libraries: $BoostDir\lib" -ForegroundColor Gray
Write-Host ""

# Check for clang++
if (-not (Get-Command clang++ -ErrorAction SilentlyContinue)) {
    Write-Host "ERROR: clang++ not found in PATH" -ForegroundColor Red
    Write-Host "Please install LLVM/Clang and add it to your PATH" -ForegroundColor Red
    Write-Host "Download from: https://github.com/llvm/llvm-project/releases" -ForegroundColor Red
    exit 1
}

Write-Host "Compiler: clang++" -ForegroundColor Green
$clangVersion = (clang++ --version | Select-String "clang version").ToString()
Write-Host $clangVersion -ForegroundColor Gray
Write-Host ""

# Create build directory
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

Set-Location build

Write-Host "Building examples..." -ForegroundColor Cyan
Write-Host ""

# Compiler flags
$cxxFlags = "-std=c++23", "-Wall", "-Wextra", "-Wpedantic", "-O2"
$includeFlags = "-I$BoostDir\include", "-I..\.."
$linkFlags = "-L$BoostDir\lib", "-lws2_32", "-lwsock32"

# Auto-detect Boost library names
$boostJsonLib = Get-ChildItem "$BoostDir\lib" -Filter "*boost_json*.lib" | 
    Where-Object { $_.Name -notmatch '-d-x64' } | 
    Select-Object -First 1

$boostUrlLib = Get-ChildItem "$BoostDir\lib" -Filter "*boost_url*.lib" | 
    Where-Object { $_.Name -notmatch '-d-x64' } | 
    Select-Object -First 1

if ($boostJsonLib) {
    $jsonLibName = $boostJsonLib.BaseName
    Write-Host "Found: $($boostJsonLib.Name)" -ForegroundColor Green
    $linkFlags += "-l$jsonLibName"
} else {
    Write-Host "WARNING: boost_json library not found" -ForegroundColor Yellow
    $linkFlags += "-lboost_json"
}

if ($boostUrlLib) {
    $urlLibName = $boostUrlLib.BaseName
    Write-Host "Found: $($boostUrlLib.Name)" -ForegroundColor Green
    $linkFlags += "-l$urlLibName"
} else {
    Write-Host "WARNING: boost_url library not found" -ForegroundColor Yellow
    $linkFlags += "-lboost_url"
}

Write-Host ""

# Build function
function Build-Example {
    param($name, $source, $number, $total)
    
    Write-Host "[$number/$total] Building $name..." -ForegroundColor Cyan
    
    $allFlags = $cxxFlags + $includeFlags + $source + "-o" + "$name.exe" + $linkFlags
    
    # Debug: show the command
    Write-Host "Command: clang++ $($allFlags -join ' ')" -ForegroundColor DarkGray
    
    & clang++ $allFlags 2>&1 | ForEach-Object {
        if ($_ -match "error:") {
            Write-Host $_ -ForegroundColor Red
        } elseif ($_ -match "warning:") {
            Write-Host $_ -ForegroundColor Yellow
        } else {
            Write-Host $_ -ForegroundColor Gray
        }
    }
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Failed to build $name" -ForegroundColor Red
        Set-Location ..
        exit 1
    }
    
    Write-Host "[$number/$total] $name.exe - OK" -ForegroundColor Green
    Write-Host ""
}

# Build all examples
Build-Example "example_router" "..\example_router.cpp" 1 3
Build-Example "example_web" "..\example_web.cpp" 2 3
Build-Example "test_client" "..\test_client.cpp" 3 3

Set-Location ..

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Build completed successfully!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Executables are in the 'build' directory:" -ForegroundColor White
Write-Host "  - build\example_router.exe" -ForegroundColor Gray
Write-Host "  - build\example_web.exe" -ForegroundColor Gray
Write-Host "  - build\test_client.exe" -ForegroundColor Gray
Write-Host ""
Write-Host "To run the web server:" -ForegroundColor White
Write-Host "  cd build" -ForegroundColor Gray
Write-Host "  .\example_web.exe" -ForegroundColor Gray
Write-Host ""
