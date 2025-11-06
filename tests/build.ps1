#!/usr/bin/env pwsh
# Wolf Tests Build Script for Windows
# Requires: Clang++, Boost, Catch2

param(
    [string]$BoostDir = $env:BOOST_DIR,
    [string]$Catch2Dir = $env:CATCH2_DIR,
    [switch]$Clean,
    [switch]$Verbose
)

$ErrorActionPreference = "Stop"

# Color output helpers
function Write-ColorOutput {
    param([string]$Message, [string]$Color = "White")
    Write-Host $Message -ForegroundColor $Color
}

function Write-Header {
    param([string]$Message)
    Write-Host ""
    Write-ColorOutput "========================================" "Cyan"
    Write-ColorOutput $Message "Cyan"
    Write-ColorOutput "========================================" "Cyan"
}

function Write-Success {
    param([string]$Message)
    Write-ColorOutput "✓ $Message" "Green"
}

function Write-Error {
    param([string]$Message)
    Write-ColorOutput "✗ $Message" "Red"
}

function Write-Info {
    param([string]$Message)
    Write-ColorOutput "→ $Message" "Yellow"
}

Write-Header "Wolf Tests Build Script (Windows)"

# Clean build directory if requested
if ($Clean -and (Test-Path "build")) {
    Write-Info "Cleaning build directory..."
    Remove-Item -Path "build" -Recurse -Force
    Write-Success "Build directory cleaned"
}

# Create build directory
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
    Write-Success "Created build directory"
}

# Check for BOOST_DIR
if (-not $BoostDir) {
    Write-Error "BOOST_DIR environment variable is not set and -BoostDir not provided"
    Write-Info "Please set BOOST_DIR or use -BoostDir parameter"
    Write-Info "Example: `$env:BOOST_DIR = 'c:\libraries\boost'"
    Write-Info "Or: .\build.ps1 -BoostDir 'c:\libraries\boost'"
    exit 1
}

if (-not (Test-Path $BoostDir)) {
    Write-Error "Boost directory not found: $BoostDir"
    exit 1
}

$BoostInclude = Join-Path $BoostDir "include"
$BoostLib = Join-Path $BoostDir "lib"

if (-not (Test-Path $BoostInclude)) {
    Write-Error "Boost include directory not found: $BoostInclude"
    exit 1
}

if (-not (Test-Path $BoostLib)) {
    Write-Error "Boost lib directory not found: $BoostLib"
    exit 1
}

Write-Info "BOOST_DIR: $BoostDir"
Write-Info "Boost headers: $BoostInclude"
Write-Info "Boost libraries: $BoostLib"

# Check for CATCH2_DIR
if (-not $Catch2Dir) {
    Write-Error "CATCH2_DIR environment variable is not set and -Catch2Dir not provided"
    Write-Info "Please set CATCH2_DIR or use -Catch2Dir parameter"
    Write-Info "Example: `$env:CATCH2_DIR = 'c:\libraries\Catch2'"
    Write-Info "Or: .\build.ps1 -Catch2Dir 'c:\libraries\Catch2'"
    exit 1
}

if (-not (Test-Path $Catch2Dir)) {
    Write-Error "Catch2 directory not found: $Catch2Dir"
    exit 1
}

$Catch2Include = Join-Path $Catch2Dir "include"
$Catch2Lib = Join-Path $Catch2Dir "lib"

if (-not (Test-Path $Catch2Include)) {
    Write-Error "Catch2 include directory not found: $Catch2Include"
    Write-Info "Expected structure: CATCH2_DIR/include/catch2/"
    exit 1
}

if (-not (Test-Path $Catch2Lib)) {
    Write-Error "Catch2 lib directory not found: $Catch2Lib"
    Write-Info "Expected structure: CATCH2_DIR/lib/*.lib"
    exit 1
}

Write-Info "CATCH2_DIR: $Catch2Dir"
Write-Info "Catch2 headers: $Catch2Include"
Write-Info "Catch2 libraries: $Catch2Lib"

# Check for clang++
try {
    $clangVersion = & clang++ --version 2>&1 | Select-Object -First 1
    Write-Success "Compiler: clang++"
    Write-Info "$clangVersion"
} catch {
    Write-Error "clang++ not found in PATH"
    Write-Info "Please install LLVM: winget install LLVM.LLVM"
    exit 1
}

Write-Host ""
Write-Info "Building tests..."
Write-Host ""

# Auto-detect Boost library names
$boostJsonLib = Get-ChildItem -Path $BoostLib -Filter "*boost_json*.lib" | Select-Object -First 1
$boostUrlLib = Get-ChildItem -Path $BoostLib -Filter "*boost_url*.lib" | Select-Object -First 1

if (-not $boostJsonLib) {
    Write-Error "Boost.JSON library not found in $BoostLib"
    exit 1
}

if (-not $boostUrlLib) {
    Write-Error "Boost.URL library not found in $BoostLib"
    exit 1
}

$boostJsonLibName = $boostJsonLib.BaseName
$boostUrlLibName = $boostUrlLib.BaseName

# Clang's -l flag adds "lib" prefix when searching, so we need to remove it
# For libboost_json-*.lib, use -lboost_json-* (linker searches for libboost_json-*.lib)
# For boost_json-*.lib, use -lboost_json-* (linker searches for libboost_json-*.lib)
if ($boostJsonLibName -match '^lib(.+)$') {
    $boostJsonLibName = $matches[1]
}
if ($boostUrlLibName -match '^lib(.+)$') {
    $boostUrlLibName = $matches[1]
}

Write-Success "Found: $($boostJsonLib.Name)"
Write-Success "Found: $($boostUrlLib.Name)"

# Auto-detect Catch2 library names (need both Catch2 and Catch2Main)
$catch2MainLibFile = Get-ChildItem -Path $Catch2Lib -Filter "*Catch2Main*.lib" | Select-Object -First 1

if (-not $catch2MainLibFile) {
    # Try .a files (Clang on Windows produces these)
    $catch2MainLibFile = Get-ChildItem -Path $Catch2Lib -Filter "*Catch2Main*.a" | Select-Object -First 1
}

if (-not $catch2MainLibFile) {
    Write-Error "Catch2Main library not found in $Catch2Lib"
    Write-Info "Expected: libCatch2Main.lib or libCatch2Main.a"
    Write-Info "Available files:"
    Get-ChildItem -Path $Catch2Lib | Select-Object -First 10 | ForEach-Object { Write-Info "  $($_.Name)" }
    exit 1
}

# Find the main Catch2 library (not Catch2Main)
$catch2CoreLibFile = Get-ChildItem -Path $Catch2Lib -Filter "libCatch2.lib" | Where-Object { $_.Name -notmatch "Main" } | Select-Object -First 1

if (-not $catch2CoreLibFile) {
    # Try .a files
    $catch2CoreLibFile = Get-ChildItem -Path $Catch2Lib -Filter "libCatch2.a" | Where-Object { $_.Name -notmatch "Main" } | Select-Object -First 1
}

if (-not $catch2CoreLibFile) {
    Write-Error "Catch2 core library not found in $Catch2Lib"
    Write-Info "Expected: libCatch2.lib or libCatch2.a (without 'Main' in the name)"
    Write-Info "Available files:"
    Get-ChildItem -Path $Catch2Lib | Select-Object -First 10 | ForEach-Object { Write-Info "  $($_.Name)" }
    exit 1
}

# Get library names for linking
$catch2MainLibName = $catch2MainLibFile.BaseName
$catch2CoreLibName = $catch2CoreLibFile.BaseName

# Clang's -l flag adds "lib" prefix when searching
if ($catch2MainLibName -match '^lib(.+)$') {
    $catch2MainLibName = $matches[1]
}
if ($catch2CoreLibName -match '^lib(.+)$') {
    $catch2CoreLibName = $matches[1]
}

Write-Success "Found: $($catch2CoreLibFile.Name)"
Write-Success "Found: $($catch2MainLibFile.Name)"
Write-Info "Linker will use: -l$catch2CoreLibName -l$catch2MainLibName"

# Common compile flags
# Note: For libraries with complex names (dashes, etc.), use full path instead of -l
$commonFlags = @(
    "-std=c++23",
    "-I$BoostInclude",
    "-I$Catch2Include",
    "-I..",
    "$($boostJsonLib.FullName)",
    "$($boostUrlLib.FullName)",
    "-L$Catch2Lib",
    "-l$catch2CoreLibName",
    "-l$catch2MainLibName",
    "-lws2_32",
    "-lwsock32"
)

if ($Verbose) {
    $commonFlags += "-v"
}

# Build router_test
Write-Host ""
Write-Info "[1/2] Building router_test..."

$routerArgs = @("router_test.cpp", "-o", "build\router_test.exe") + $commonFlags

if ($Verbose) {
    Write-Info "Command: clang++ $($routerArgs -join ' ')"
}

try {
    & clang++ $routerArgs
    if ($LASTEXITCODE -eq 0) {
        $size = (Get-Item "build\router_test.exe").Length
        $sizeKB = [math]::Round($size / 1KB, 1)
        Write-Success "[1/2] router_test.exe - OK ($sizeKB KB)"
    } else {
        Write-Error "[1/2] router_test.exe - FAILED"
        exit 1
    }
} catch {
    Write-Error "[1/2] Failed to compile router_test.cpp"
    Write-Error $_.Exception.Message
    exit 1
}

# Build web_server_test
Write-Host ""
Write-Info "[2/2] Building web_server_test..."

$webServerArgs = @("web_server_test.cpp", "-o", "build\web_server_test.exe") + $commonFlags

if ($Verbose) {
    Write-Info "Command: clang++ $($webServerArgs -join ' ')"
}

try {
    & clang++ $webServerArgs
    if ($LASTEXITCODE -eq 0) {
        $size = (Get-Item "build\web_server_test.exe").Length
        $sizeKB = [math]::Round($size / 1KB, 1)
        Write-Success "[2/2] web_server_test.exe - OK ($sizeKB KB)"
    } else {
        Write-Error "[2/2] web_server_test.exe - FAILED"
        exit 1
    }
} catch {
    Write-Error "[2/2] Failed to compile web_server_test.cpp"
    Write-Error $_.Exception.Message
    exit 1
}

Write-Host ""
Write-Header "Build completed successfully!"
Write-Host ""
Write-Info "Test executables:"
Write-Success "  build\router_test.exe"
Write-Success "  build\web_server_test.exe"
Write-Host ""
Write-Info "Run tests:"
Write-ColorOutput "  cd build" "White"
Write-ColorOutput "  .\router_test.exe" "White"
Write-ColorOutput "  .\web_server_test.exe" "White"
Write-Host ""
Write-Info "Run specific test:"
Write-ColorOutput "  .\router_test.exe [http_router]" "White"
Write-ColorOutput "  .\web_server_test.exe [http_request]" "White"
Write-Host ""
