# Quick Setup for Wolf Tests
# Run this once to configure environment variables

Write-Host "🐺 Wolf Tests - Environment Setup" -ForegroundColor Cyan
Write-Host ""

# Set BOOST_DIR
if (-not $env:BOOST_DIR) {
    $defaultBoost = "c:\libraries\boost"
    $boostDir = Read-Host "Enter BOOST_DIR path [$defaultBoost]"
    if ([string]::IsNullOrWhiteSpace($boostDir)) {
        $boostDir = $defaultBoost
    }
    
    if (Test-Path $boostDir) {
        [Environment]::SetEnvironmentVariable('BOOST_DIR', $boostDir, 'User')
        $env:BOOST_DIR = $boostDir
        Write-Host "✓ BOOST_DIR set to: $boostDir" -ForegroundColor Green
    } else {
        Write-Host "✗ Directory not found: $boostDir" -ForegroundColor Red
        Write-Host "Please create the directory or specify a different path" -ForegroundColor Yellow
    }
} else {
    Write-Host "✓ BOOST_DIR already set: $env:BOOST_DIR" -ForegroundColor Green
}

Write-Host ""

# Set CATCH2_DIR
if (-not $env:CATCH2_DIR) {
    $defaultCatch2 = "c:\libraries\Catch2"
    $catch2Dir = Read-Host "Enter CATCH2_DIR path [$defaultCatch2]"
    if ([string]::IsNullOrWhiteSpace($catch2Dir)) {
        $catch2Dir = $defaultCatch2
    }
    
    if (Test-Path $catch2Dir) {
        [Environment]::SetEnvironmentVariable('CATCH2_DIR', $catch2Dir, 'User')
        $env:CATCH2_DIR = $catch2Dir
        Write-Host "✓ CATCH2_DIR set to: $catch2Dir" -ForegroundColor Green
    } else {
        Write-Host "✗ Directory not found: $catch2Dir" -ForegroundColor Red
        Write-Host "Please install Catch2 first. See BUILD_WINDOWS.md for instructions" -ForegroundColor Yellow
    }
} else {
    Write-Host "✓ CATCH2_DIR already set: $env:CATCH2_DIR" -ForegroundColor Green
}

Write-Host ""
Write-Host "Environment variables have been set permanently for your user account." -ForegroundColor Cyan
Write-Host "Current session values:" -ForegroundColor Cyan
Write-Host "  BOOST_DIR  = $env:BOOST_DIR" -ForegroundColor White
Write-Host "  CATCH2_DIR = $env:CATCH2_DIR" -ForegroundColor White
Write-Host ""
Write-Host "You can now run: .\build.ps1" -ForegroundColor Green
Write-Host ""
