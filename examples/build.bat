@echo off
REM Build script for Wolf examples on Windows using clang++
REM Requires: BOOST_DIR environment variable to be set to Boost directory
REM Directory structure: BOOST_DIR\include\boost\... and BOOST_DIR\lib\...
REM Requires: clang++ to be in PATH

setlocal EnableDelayedExpansion

echo ========================================
echo Wolf Examples Build Script (Windows)
echo ========================================
echo.

REM Check if BOOST_DIR is set
if not defined BOOST_DIR (
    echo ERROR: BOOST_DIR environment variable is not set
    echo Please set BOOST_DIR to your Boost installation directory
    echo Example: set BOOST_DIR=c:\libraries\boost
    echo.
    echo Expected structure:
    echo   BOOST_DIR\include\boost\...  (headers)
    echo   BOOST_DIR\lib\...            (libraries)
    exit /b 1
)

echo BOOST_DIR: %BOOST_DIR%

REM Verify Boost directory structure
if not exist "%BOOST_DIR%\include\boost" (
    echo ERROR: Boost headers not found at %BOOST_DIR%\include\boost
    echo Please ensure Boost is properly installed with headers in include\boost\
    exit /b 1
)

if not exist "%BOOST_DIR%\lib" (
    echo ERROR: Boost libraries not found at %BOOST_DIR%\lib
    echo Please ensure Boost libraries are built and placed in lib\
    exit /b 1
)

echo Boost headers: %BOOST_DIR%\include
echo Boost libraries: %BOOST_DIR%\lib
echo.

REM Check if clang++ is available
where clang++ >nul 2>&1
if errorlevel 1 (
    echo ERROR: clang++ not found in PATH
    echo Please install LLVM/Clang and add it to your PATH
    echo Download from: https://github.com/llvm/llvm-project/releases
    exit /b 1
)

echo Compiler: clang++
clang++ --version | findstr "clang version"
echo.

REM Create build directory if it doesn't exist
if not exist "build" mkdir build
cd build

echo Building examples...
echo.

REM Common compiler flags
set CXX_FLAGS=-std=c++23 -Wall -Wextra -Wpedantic -O2
set INCLUDE_FLAGS=-I"%BOOST_DIR%\include" -I..\..
set LINK_FLAGS=-L"%BOOST_DIR%\lib" -lws2_32 -lwsock32

REM Auto-detect Boost library naming convention
REM Try common naming patterns: libboost_json*.lib or boost_json*.lib
set BOOST_JSON_LIB=
set BOOST_URL_LIB=

for %%f in ("%BOOST_DIR%\lib\*boost_json*.lib") do (
    set "BOOST_JSON_LIB=%%~nf"
    goto :found_json
)
:found_json

for %%f in ("%BOOST_DIR%\lib\*boost_url*.lib") do (
    set "BOOST_URL_LIB=%%~nf"
    goto :found_url
)
:found_url

if defined BOOST_JSON_LIB (
    echo Found: %BOOST_JSON_LIB%.lib
    set LINK_FLAGS=%LINK_FLAGS% -l%BOOST_JSON_LIB%
) else (
    echo WARNING: boost_json library not found, trying default name
    set LINK_FLAGS=%LINK_FLAGS% -lboost_json
)

if defined BOOST_URL_LIB (
    echo Found: %BOOST_URL_LIB%.lib
    set LINK_FLAGS=%LINK_FLAGS% -l%BOOST_URL_LIB%
) else (
    echo WARNING: boost_url library not found, trying default name
    set LINK_FLAGS=%LINK_FLAGS% -lboost_url
)
echo.

REM Build example_router
echo [1/3] Building example_router...
clang++ %CXX_FLAGS% %INCLUDE_FLAGS% ..\example_router.cpp -o example_router.exe %LINK_FLAGS%
if errorlevel 1 (
    echo ERROR: Failed to build example_router
    echo.
    echo Troubleshooting:
    echo 1. Verify BOOST_DIR structure: %BOOST_DIR%\include\boost and %BOOST_DIR%\lib
    echo 2. Check that Boost libraries are built for your compiler
    echo 3. Ensure clang++ can find ws2_32 and wsock32 (Windows SDK)
    cd ..
    exit /b 1
)
echo [1/3] example_router.exe - OK
echo.

REM Build example_web
echo [2/3] Building example_web...
clang++ %CXX_FLAGS% %INCLUDE_FLAGS% ..\example_web.cpp -o example_web.exe %LINK_FLAGS%
if errorlevel 1 (
    echo ERROR: Failed to build example_web
    cd ..
    exit /b 1
)
echo [2/3] example_web.exe - OK
echo.

REM Build test_client
echo [3/3] Building test_client...
clang++ %CXX_FLAGS% %INCLUDE_FLAGS% ..\test_client.cpp -o test_client.exe %LINK_FLAGS%
if errorlevel 1 (
    echo ERROR: Failed to build test_client
    cd ..
    exit /b 1
)
echo [3/3] test_client.exe - OK
echo.

cd ..

echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo Executables are in the 'build' directory:
echo   - build\example_router.exe
echo   - build\example_web.exe
echo   - build\test_client.exe
echo.
echo To run the web server:
echo   cd build
echo   example_web.exe
echo.

endlocal