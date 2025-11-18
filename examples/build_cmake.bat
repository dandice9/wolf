@echo off
REM CMake build script for Wolf examples on Windows
REM Requires: BOOST_ROOT environment variable to be set
REM Requires: CMake to be in PATH
REM Optional: Visual Studio or clang++ toolchain

setlocal EnableDelayedExpansion

echo ========================================
echo Wolf Examples CMake Build (Windows)
echo ========================================
echo.

REM Check if BOOST_ROOT is set
if not defined BOOST_ROOT (
    echo ERROR: BOOST_ROOT environment variable is not set
    echo Please set BOOST_ROOT to your Boost installation directory
    echo Example: set BOOST_ROOT=C:\boost_1_89_0
    exit /b 1
)

echo BOOST_ROOT: %BOOST_ROOT%
echo.

REM Check if CMake is available
where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake not found in PATH
    echo Please install CMake and add it to your PATH
    exit /b 1
)

echo CMake version:
cmake --version | findstr "cmake version"
echo.

REM Check for clang++ or Visual Studio
set USE_CLANG=0
where clang++ >nul 2>&1
if not errorlevel 1 (
    set USE_CLANG=1
    echo Using toolchain: LLVM/Clang
    clang++ --version | findstr "clang version"
) else (
    echo Using toolchain: Visual Studio
)
echo.

REM Create build directory if it doesn't exist
if not exist "build" mkdir build
cd build

echo Configuring CMake...
echo.

if !USE_CLANG! == 1 (
    REM Use Ninja with Clang if available
    where ninja >nul 2>&1
    if not errorlevel 1 (
        echo Using Ninja build system with Clang
        cmake -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DBOOST_ROOT="%BOOST_ROOT%" -DCMAKE_BUILD_TYPE=Release ..
    ) else (
        echo Using default build system with Clang
        cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DBOOST_ROOT="%BOOST_ROOT%" -DCMAKE_BUILD_TYPE=Release ..
    )
) else (
    REM Use Visual Studio
    cmake -DBOOST_ROOT="%BOOST_ROOT%" -DCMAKE_BUILD_TYPE=Release ..
)

if errorlevel 1 (
    echo ERROR: CMake configuration failed
    cd ..
    exit /b 1
)

echo.
echo Building examples...
echo.

cmake --build . --config Release

if errorlevel 1 (
    echo ERROR: Build failed
    cd ..
    exit /b 1
)

cd ..

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo Executables are in the 'build' or 'build\Release' directory
echo.
echo To run the web server:
echo   cd build
echo   example_web.exe (or Release\example_web.exe)
echo.

endlocal
