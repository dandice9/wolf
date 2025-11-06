@echo off
REM Wolf Tests Build Script for Windows (Batch)
REM Requires: Clang++, Boost, Catch2

setlocal enabledelayedexpansion

echo.
echo ========================================
echo Wolf Tests Build Script (Windows)
echo ========================================
echo.

REM Check for BOOST_DIR
if "%BOOST_DIR%"=="" (
    echo [ERROR] BOOST_DIR environment variable is not set
    echo Please set BOOST_DIR to your Boost installation directory
    echo Example: set BOOST_DIR=c:\libraries\boost
    exit /b 1
)

if not exist "%BOOST_DIR%" (
    echo [ERROR] Boost directory not found: %BOOST_DIR%
    exit /b 1
)

set BOOST_INCLUDE=%BOOST_DIR%\include
set BOOST_LIB=%BOOST_DIR%\lib

if not exist "%BOOST_INCLUDE%" (
    echo [ERROR] Boost include directory not found: %BOOST_INCLUDE%
    exit /b 1
)

if not exist "%BOOST_LIB%" (
    echo [ERROR] Boost lib directory not found: %BOOST_LIB%
    exit /b 1
)

echo [INFO] BOOST_DIR: %BOOST_DIR%
echo [INFO] Boost headers: %BOOST_INCLUDE%
echo [INFO] Boost libraries: %BOOST_LIB%

REM Check for CATCH2_DIR
if "%CATCH2_DIR%"=="" (
    echo [ERROR] CATCH2_DIR environment variable is not set
    echo Please set CATCH2_DIR to your Catch2 installation directory
    echo Example: set CATCH2_DIR=c:\libraries\Catch2
    exit /b 1
)

if not exist "%CATCH2_DIR%" (
    echo [ERROR] Catch2 directory not found: %CATCH2_DIR%
    exit /b 1
)

set CATCH2_INCLUDE=%CATCH2_DIR%\include
set CATCH2_LIB=%CATCH2_DIR%\lib

if not exist "%CATCH2_INCLUDE%" (
    echo [ERROR] Catch2 include directory not found: %CATCH2_INCLUDE%
    exit /b 1
)

if not exist "%CATCH2_LIB%" (
    echo [ERROR] Catch2 lib directory not found: %CATCH2_LIB%
    exit /b 1
)

echo [INFO] CATCH2_DIR: %CATCH2_DIR%
echo [INFO] Catch2 headers: %CATCH2_INCLUDE%
echo [INFO] Catch2 libraries: %CATCH2_LIB%

REM Check for clang++
where clang++ >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] clang++ not found in PATH
    echo Please install LLVM: winget install LLVM.LLVM
    exit /b 1
)

echo [INFO] Compiler: clang++
clang++ --version | findstr /C:"clang version"

REM Create build directory
if not exist "build" mkdir build

echo.
echo [INFO] Building tests...
echo.

REM Note: You need to manually set the library names based on your Boost/Catch2 build
REM Example: libboost_json-clang21-mt-s-x64-1_89.lib
REM 
REM If Catch2 was compiled with Clang, it produces .a files instead of .lib files
REM In that case, use the .a filename WITHOUT the .a extension and WITHOUT the "lib" prefix
REM Example: libCatch2Main.a -> Catch2Main
REM         libCatch2Maind.a -> Catch2Maind
REM
REM Update these lines with your actual library names:

set BOOST_JSON_LIB=libboost_json-clang21-mt-s-x64-1_89
set BOOST_URL_LIB=libboost_url-clang21-mt-s-x64-1_89
set CATCH2_MAIN_LIB=Catch2Maind

REM If you have .a files, remove "lib" prefix:
REM For libCatch2Main.a use: set CATCH2_MAIN_LIB=Catch2Main
REM For libCatch2Maind.a use: set CATCH2_MAIN_LIB=Catch2Maind

echo [INFO] Using Boost.JSON: %BOOST_JSON_LIB%.lib
echo [INFO] Using Boost.URL: %BOOST_URL_LIB%.lib
echo [INFO] Using Catch2: %CATCH2_MAIN_LIB%.lib

REM Build router_test
echo.
echo [1/2] Building router_test...
clang++ -std=c++23 ^
    -I"%BOOST_INCLUDE%" ^
    -I"%CATCH2_INCLUDE%" ^
    -I.. ^
    router_test.cpp -o build\router_test.exe ^
    -L"%BOOST_LIB%" ^
    -L"%CATCH2_LIB%" ^
    -l%BOOST_JSON_LIB% ^
    -l%BOOST_URL_LIB% ^
    -l%CATCH2_MAIN_LIB% ^
    -lws2_32 -lwsock32

if %ERRORLEVEL% equ 0 (
    echo [SUCCESS] router_test.exe - OK
) else (
    echo [ERROR] Failed to build router_test.exe
    exit /b 1
)

REM Build web_server_test
echo.
echo [2/2] Building web_server_test...
clang++ -std=c++23 ^
    -I"%BOOST_INCLUDE%" ^
    -I"%CATCH2_INCLUDE%" ^
    -I.. ^
    web_server_test.cpp -o build\web_server_test.exe ^
    -L"%BOOST_LIB%" ^
    -L"%CATCH2_LIB%" ^
    -l%BOOST_JSON_LIB% ^
    -l%BOOST_URL_LIB% ^
    -l%CATCH2_MAIN_LIB% ^
    -lws2_32 -lwsock32

if %ERRORLEVEL% equ 0 (
    echo [SUCCESS] web_server_test.exe - OK
) else (
    echo [ERROR] Failed to build web_server_test.exe
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo Test executables:
echo   build\router_test.exe
echo   build\web_server_test.exe
echo.
echo Run tests:
echo   cd build
echo   router_test.exe
echo   web_server_test.exe
echo.

endlocal
